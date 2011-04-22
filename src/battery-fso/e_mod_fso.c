#include "e.h"
#include "e_mod_main.h"

#define FALSE   0
#define TRUE    1

int _battery_fso_start(void);
void _battery_fso_stop(void);

static void _battery_fso_get_capacity(void *data);
static void *_battery_fso_capacity_unmarshall(DBusMessage *msg, DBusError *err);
static void _battery_fso_capacity_callback(void *data, void *ret, DBusError *err);
static void _battery_fso_on_capacity_change(void *data, DBusMessage *msg);
static void _battery_fso_capacity_update(void *data, int percent);

static void _battery_fso_get_powerstatus(void *data);
static void *_battery_fso_powerstatus_unmarshall(DBusMessage *msg, DBusError *err);
static void _battery_fso_powerstatus_callback(void *data, void *ret, DBusError *err);
static void _battery_fso_on_powerstatus_change(void *data, DBusMessage *msg);
static void _battery_fso_powerstatus_update(void *data, char *charging);

static void _battery_fso_on_nameowner_change(void *data, DBusMessage *msg);
static void _battery_fso_result_free(void *data);

static E_DBus_Signal_Handler *nameowner_handler = NULL;
static E_DBus_Signal_Handler *capacity_handler = NULL;
static E_DBus_Signal_Handler *powerstatus_handler = NULL;

extern Eina_List *device_batteries;
extern double init_time;

static E_DBus_Connection *conn = NULL;
static Battery *battery = NULL;

int
_battery_fso_start(void)
{
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return 0;

   /* Initiate the battery gadget */
   Battery *bat;
   const char *udi = "FSO";
   bat = _battery_battery_find( udi );
   if (!bat)
     {
        bat = E_NEW(Battery, 1);
        if (!bat) return 0;
        bat->udi = eina_stringshare_add(udi);
        device_batteries = eina_list_append(device_batteries, bat);
     }
   battery = bat;

   /* Get initial status */
   _battery_fso_get_capacity( bat );
   _battery_fso_get_powerstatus( bat );

/* Add signal listeners for PowerSupply.Capacity and PowerSupply.PowerStatus */
if( conn ) {
   nameowner_handler = e_dbus_signal_handler_add(conn,
         "org.freedesktop.DBus",
         "/org/freedesktop/DBus",
         "org.freedesktop.DBus",
         "NameOwnerChanged",
         _battery_fso_on_nameowner_change, bat);

   capacity_handler = e_dbus_signal_handler_add( conn,
         "org.freesmartphone.odeviced",
         "/org/freesmartphone/Device/PowerSupply",
         "org.freesmartphone.Device.PowerSupply",
         "Capacity",
         _battery_fso_on_capacity_change, bat );

   powerstatus_handler = e_dbus_signal_handler_add( conn,
         "org.freesmartphone.odeviced",
         "/org/freesmartphone/Device/PowerSupply",
         "org.freesmartphone.Device.PowerSupply",
         "PowerStatus",
         _battery_fso_on_powerstatus_change, bat );
   }

   init_time = ecore_time_get();
   return 1;
}

static void
_battery_fso_get_capacity(void *data)
{
   DBusMessage *msg;

   if( conn )
     {
     msg = dbus_message_new_method_call( "org.freesmartphone.odeviced",
         "/org/freesmartphone/Device/PowerSupply",
         "org.freesmartphone.Device.PowerSupply",
         "GetCapacity" );
   if( msg )
     {
     e_dbus_method_call_send( conn, msg,
            _battery_fso_capacity_unmarshall,
            _battery_fso_capacity_callback,
            _battery_fso_result_free, -1, data);
     dbus_message_unref(msg);
     }
   }
}

static void *
_battery_fso_capacity_unmarshall(DBusMessage *msg, DBusError *err)
{
   dbus_int32_t val = 0;

   if (dbus_message_get_args(msg, NULL, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID))
     {
     int *val_ret;

     val_ret = malloc(sizeof(int));
     if( val_ret )
       {
       *val_ret = val;
       return val_ret;
       }
     }
   return NULL;
}

static void
_battery_fso_capacity_callback(void *data, void *ret, DBusError *err)
{
   if (ret)
     {
     int *val_ret;
     val_ret = ret;
     _battery_fso_capacity_update(data, *val_ret);
     }
}

static void
_battery_fso_on_capacity_change(void *data, DBusMessage *msg)
{
   DBusError err;
   dbus_int32_t val = 0;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID))
     return;
   _battery_fso_capacity_update( data, val );
}

static void
_battery_fso_capacity_update(void *data, int percent)
{
   Battery *bat = data;

   /* capacity in percent */
   bat->percent = percent;
   bat->got_prop = 1;

   /* Update the Battery */
   _battery_device_update();
}

static void
_battery_fso_get_powerstatus(void *data)
{
   DBusMessage *msg;

   if( conn )
     {
     msg = dbus_message_new_method_call( "org.freesmartphone.odeviced",
         "/org/freesmartphone/Device/PowerSupply",
         "org.freesmartphone.Device.PowerSupply",
         "GetPowerStatus" );
   if( msg )
     {
     e_dbus_method_call_send( conn, msg,
            _battery_fso_powerstatus_unmarshall,
            _battery_fso_powerstatus_callback,
            _battery_fso_result_free, -1, data);
     dbus_message_unref(msg);
     }
   }
}

static void *
_battery_fso_powerstatus_unmarshall(DBusMessage *msg, DBusError *err)
{
   const char *str;

   if (dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID))
     {
     char *str_ret;

     str_ret = malloc(strlen(str)+1);
     if (str_ret)
       {
       strcpy(str_ret, str);
       return str_ret;
       }
     }
   return NULL;
}

static void
_battery_fso_powerstatus_callback(void *data, void *ret, DBusError *err)
{
   if (ret)
     {
     _battery_fso_powerstatus_update(data, ret);
     }
}

static void
_battery_fso_on_powerstatus_change(void *data, DBusMessage *msg)
{
   DBusError err;
   char *str = NULL;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID))
     return;
   _battery_fso_powerstatus_update( data, str );
}

static void
_battery_fso_powerstatus_update(void *data, char *powerstatus)
{
   Battery *bat = data;

     const char *my_str = "charging";

     if( strcmp( my_str, powerstatus ) == 0 ) {
       bat->charging = TRUE;
       }
     else {
       bat->charging = FALSE;
       }
   bat->got_prop = 1;

   /* Update the Battery */
   _battery_device_update();
}

static void
_battery_fso_on_nameowner_change(void *data, DBusMessage *msg)
{
   DBusError err;
   const char *s1, *s2, *s3;

   dbus_error_init(&err);
   if(!dbus_message_get_args(msg, &err,
         DBUS_TYPE_STRING, &s1,
         DBUS_TYPE_STRING, &s2,
         DBUS_TYPE_STRING, &s3,
         DBUS_TYPE_INVALID))
     return;

   if ((!strcmp(s1, "org.freesmartphone.odeviced")) && (conn))
     {
     if(!strcmp(s3, "")) {
        printf("\n\nERROR: No Name Owner!\nSetting Capacity to 0.\n\n\n");
        if(battery) {
        battery->charging = FALSE;
        battery->percent = 0;
        battery->got_prop = 1;
        _battery_device_update();
        }
     }
     else {
     if (capacity_handler)
       {
       e_dbus_signal_handler_del(conn, capacity_handler);
       capacity_handler = e_dbus_signal_handler_add(conn,
               "org.freesmartphone.odeviced",
               "/org/freesmartphone/Device/PowerSupply",
               "org.freesmartphone.Device.PowerSupply",
               "Capacity",
               _battery_fso_on_capacity_change, data);
       _battery_fso_get_capacity( data );
       }
     if (powerstatus_handler)
       {
       e_dbus_signal_handler_del(conn, powerstatus_handler);
       powerstatus_handler = e_dbus_signal_handler_add(conn,
               "org.freesmartphone.odeviced",
               "/org/freesmartphone/Device/PowerSupply",
               "org.freesmartphone.Device.PowerSupply",
               "PowerStatus",
               _battery_fso_on_powerstatus_change, data);
       _battery_fso_get_powerstatus( data );
       }
      }
     }
   return;
}

static void
_battery_fso_result_free(void *data)
{
   free(data);
}

void
_battery_fso_stop(void)
{
   Battery *bat;

   EINA_LIST_FREE(device_batteries, bat)
     {
        eina_stringshare_del(bat->udi);
        free(bat);
     }

   if (!conn) return;
   if (capacity_handler) e_dbus_signal_handler_del(conn, capacity_handler);
   if (powerstatus_handler) e_dbus_signal_handler_del(conn, powerstatus_handler);
   if (nameowner_handler) e_dbus_signal_handler_del(conn, nameowner_handler);
   e_dbus_connection_close(conn);
   conn = NULL;
}
