#include "e.h"
#include "e_mod_gad_gsm.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define FALSE 0
#define TRUE 1

static E_DBus_Connection *conn = NULL;
static E_DBus_Connection *conn_system = NULL;
static E_DBus_Signal_Handler *changed_h = NULL;
static E_DBus_Signal_Handler *changed_fso_h = NULL;
static E_DBus_Signal_Handler *operatorch_h = NULL;
static E_DBus_Signal_Handler *operatorch_fso_h = NULL;
static E_DBus_Signal_Handler *namech_h = NULL;
static E_DBus_Signal_Handler *namech_system_h = NULL;
static E_DBus_Signal_Handler *device_status_changed_fso_h = NULL;

static Ecore_Timer *try_again_timer = NULL;

typedef enum _Phone_Sys
{
   PH_SYS_UNKNOWN,
   PH_SYS_QTOPIA,
   PH_SYS_FSO
} Phone_Sys;

static Phone_Sys detected_system = PH_SYS_UNKNOWN;

typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object *obj;
   int strength;
   char *oper;
   int on;
   int registered;
   int init;
};

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "illume-gsm",
     {
	_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
	e_gadcon_site_is_not_toolbar
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};
static E_Module *mod = NULL;

static void gsmModemState_callback(void *data, void *ret, DBusError *err);
static void update_signal(int sig, void *data);
static void* gsmModemState_unmarhsall(DBusMessage *msg, DBusError *err);
static void gsmModemState_free(void *data);
static void get_gsmModemState(void *data);
static void fso_resource_changed(void* data, DBusMessage* msg);
static void updateStatus(Instance* inst);
static void setLabel(Evas_Object* obj, char* label);
static void setLevel(Evas_Object* obj, int strength);
static void setState(Evas_Object* obj, int state);
static int try_again(void *data);
static void *signal_unmarhsall(DBusMessage *msg, DBusError *err);
static void *operator_unmarhsall(DBusMessage *msg, DBusError *err);
static void signal_callback_qtopia(void *data, void *ret, DBusError *err);
static void signal_callback_fso(void *data, void *ret, DBusError *err);
static void operator_callback_qtopia(void *data, void *ret, DBusError *err);
static void operator_callback_fso(void *data, void *ret, DBusError *err);
static void signal_result_free(void *data);
static void operator_result_free(void *data);
static void get_signal(void *data);
static void get_operator(void *data);
static void signal_changed(void *data, DBusMessage *msg);
static void operator_changed(void *data, DBusMessage *msg);
static void fso_operator_changed(void *data, DBusMessage *msg);
static void name_changed(void *data, DBusMessage *msg);

static int
try_again(void *data)
{
   E_MOD_GAD_GSM_DEBUG_PRINTF("Try again called");
   get_gsmModemState(data);
   get_signal(data);
   get_operator(data);
   try_again_timer = NULL;
   return 0;
}

/* called from the module core */
void
_e_mod_gad_gsm_init(E_Module *m)
{
   mod = m;
   e_gadcon_provider_register(&_gadcon_class);
}

void
_e_mod_gad_gsm_shutdown(void)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   mod = NULL;
}

/* internal calls */
static Evas_Object *
_theme_obj_new(Evas *e, const char *custom_dir, const char *group)
{
   Evas_Object *o;

   o = edje_object_add(e);
   if (!e_theme_edje_object_set(o, "base/theme/modules/illume", group))
     {
	if (custom_dir)
	  {
	     char buf[PATH_MAX];

	     snprintf(buf, sizeof(buf), "%s/illume.edj", custom_dir);
	     if (edje_object_file_set(o, buf, group))
	       {
		  printf("OK FALLBACK %s\n", buf);
	       }
	  }
     }
   return o;
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;

   inst = E_NEW(Instance, 1);
   o = _theme_obj_new(gc->evas, e_module_dir_get(mod),
		      "e/modules/illume/gadget/gsm");
   evas_object_show(o);
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   inst->gcc = gcc;
   inst->obj = o;
   e_gadcon_client_util_menu_attach(gcc);

   inst->init = TRUE;
   inst->on = FALSE;
   inst->registered = FALSE;
   inst->strength = -1;
   inst->oper = NULL;

   conn = e_dbus_bus_get(DBUS_BUS_SESSION);
   conn_system = e_dbus_bus_get(DBUS_BUS_SYSTEM);

   E_MOD_GAD_GSM_DEBUG_PRINTF("before updateState");
   updateStatus(inst);
   E_MOD_GAD_GSM_DEBUG_PRINTF("after updateState");

   if (conn)
     {
	namech_h = e_dbus_signal_handler_add(conn,
					     "org.freedesktop.DBus",
					     "/org/freedesktop/DBus",
					     "org.freedesktop.DBus",
					     "NameOwnerChanged",
					     name_changed, inst);
	changed_h = e_dbus_signal_handler_add(conn,
					      "org.openmoko.qtopia.Phonestatus",
					      "/Status",
					      "org.openmoko.qtopia.Phonestatus",
					      "signalStrengthChanged",
					      signal_changed, inst);
	operatorch_h = e_dbus_signal_handler_add(conn,
						 "org.openmoko.qtopia.Phonestatus",
						 "/Status",
						 "org.openmoko.qtopia.Phonestatus",
						 "networkOperatorChanged",
						 operator_changed, inst);
     }
   if (conn_system)
     {
	namech_system_h = e_dbus_signal_handler_add(conn_system,
						    "org.freedesktop.DBus",
						    "/org/freedesktop/DBus",
						    "org.freedesktop.DBus",
						    "NameOwnerChanged",
						    name_changed, inst);
	device_status_changed_fso_h = e_dbus_signal_handler_add(conn_system,
                                                "org.freesmartphone.ousaged",
                                                "/org/freesmartphone/Usage",
                                                "org.freesmartphone.Usage",
                                                "ResourceChanged",
                                                fso_resource_changed, inst);
	changed_fso_h = e_dbus_signal_handler_add(conn_system,
						  "org.freesmartphone.ogsmd",
						  "/org/freesmartphone/GSM/Device",
						  "org.freesmartphone.GSM.Network",
						  "SignalStrength",
						  signal_changed, inst);
	operatorch_fso_h = e_dbus_signal_handler_add(conn_system,
						     "org.freesmartphone.ogsmd",
						     "/org/freesmartphone/GSM/Device",
						     "org.freesmartphone.GSM.Network",
						     "Status",
						     fso_operator_changed, inst);
     }
   get_gsmModemState(inst);
   get_signal(inst);
   get_operator(inst);

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   if (conn) e_dbus_connection_close(conn);
   if (conn_system) e_dbus_connection_close(conn_system);

   inst = gcc->data;
   evas_object_del(inst->obj);
   if (inst->oper) free(inst->oper);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;
   Evas_Coord mw, mh, mxw, mxh;

   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->obj, &mw, &mh);
   edje_object_size_max_get(inst->obj, &mxw, &mxh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->obj, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   if ((mxw > 0) && (mxh > 0))
     e_gadcon_client_aspect_set(gcc, mxw, mxh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class)
{
   return "GSM (Illume)";
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas)
{
/* FIXME: need icon
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-clock.edj",
	    e_module_dir_get(clock_module));
   edje_object_file_set(o, buf, "icon");
   return o;
 */
   return NULL;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class)
{
   return _gadcon_class.name;
}

static void
update_operator(Instance *newData, void *data)
{
   Instance *inst = data;
   char *poper;
   char *op;

   poper = inst->oper;
   op = newData->oper;
   if (op)
   {
      inst->oper = strdup(op);
      newData->oper = NULL;
   }
   else
   {
      inst->oper = NULL;
   }
   inst->registered = newData->registered;
   updateStatus(inst);
   if (poper)
      free(poper);

   E_MOD_GAD_GSM_DEBUG_PRINTF("update_operator finished"); 
}

static void
update_signal(int sig, void *data)
{
   Instance *inst = data;

   inst->strength = sig;
   updateStatus(inst);
}


static void *
signal_unmarhsall(DBusMessage *msg, DBusError *err)
{
   dbus_int32_t val = -1;

   if (dbus_message_get_args(msg, NULL, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID))
     {
	int *val_ret;

	val_ret = malloc(sizeof(int));
	if (val_ret)
	  {
	     *val_ret = val;
	     return val_ret;
	  }
     }
   return NULL;
}

static void *
operator_unmarhsall(DBusMessage *msg, DBusError *err)
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

static void *
_fso_operator_unmarhsall(DBusMessage *msg)
{
   /* We care only about the provider name right now. All the other status
    * informations get ingnored for the gadget for now */
   const char *provider = 0 , *name = 0, *reg_stat = 0;
   const char *display = 0;
   dbus_int32_t strength;
   DBusMessageIter iter, a_iter, s_iter, v_iter;
   Instance* newData = E_NEW(Instance, 1);
   newData->oper = NULL;


   if (!dbus_message_has_signature(msg, "a{sv}")) return newData;

   dbus_message_iter_init(msg, &iter);
   dbus_message_iter_recurse(&iter, &a_iter);
   while (dbus_message_iter_get_arg_type(&a_iter) != DBUS_TYPE_INVALID)
     {
	dbus_message_iter_recurse(&a_iter, &s_iter);
	dbus_message_iter_get_basic(&s_iter, &name);

	if (strcmp(name, "registration") == 0)
	  {
	     dbus_message_iter_next(&s_iter);
	     dbus_message_iter_recurse(&s_iter, &v_iter);
	     dbus_message_iter_get_basic(&v_iter, &reg_stat);
	  }
	if (strcmp(name, "provider") == 0)
	  {
	     dbus_message_iter_next(&s_iter);
	     dbus_message_iter_recurse(&s_iter, &v_iter);
	     dbus_message_iter_get_basic(&v_iter, &provider);
	  }
	if (strcmp(name, "display") == 0)
	  {
	     dbus_message_iter_next(&s_iter);
	     dbus_message_iter_recurse(&s_iter, &v_iter);
	     dbus_message_iter_get_basic(&v_iter, &display);
	  }
	if (strcmp(name, "strength") == 0)
	  {
	     dbus_message_iter_next(&s_iter);
	     dbus_message_iter_recurse(&s_iter, &v_iter);
	     dbus_message_iter_get_basic(&v_iter, &strength);
	     newData->strength = strength;
	  }
	dbus_message_iter_next(&a_iter);
     }

   if (!reg_stat) return newData;
   E_MOD_GAD_GSM_DEBUG_PRINTF("registration status:");
   E_MOD_GAD_GSM_DEBUG_PRINTF(reg_stat);
   newData->registered = TRUE;
   if (strcmp(reg_stat, "unregistered") == 0)
   {
       newData->registered = FALSE;
       provider = "No Service";
   }
   else if (strcmp(reg_stat, "busy") == 0) provider = "Searching...";
   else if (strcmp(reg_stat, "denied") == 0) provider = "SOS only";

   if (display && *display) newData->oper = strdup(display);
   else if (provider && *provider) newData->oper = strdup(provider);

   return newData;
}

static void *
fso_operator_unmarhsall(DBusMessage *msg, DBusError *err)
{
   return _fso_operator_unmarhsall(msg);
}

static void
signal_callback_qtopia(void *data, void *ret, DBusError *err)
{
   E_MOD_GAD_GSM_DEBUG_PRINTF("Qtopia signal callback called");
   if (ret)
     {
	int *val_ret;

	if ((detected_system == PH_SYS_UNKNOWN) && (changed_h) && (conn))
	  {
	     e_dbus_signal_handler_del(conn, changed_h);
	     changed_h = e_dbus_signal_handler_add(conn,
						   "org.openmoko.qtopia.Phonestatus",
						   "/Status",
						   "org.openmoko.qtopia.Phonestatus",
						   "signalStrengthChanged",
						   signal_changed, data);
	     detected_system = PH_SYS_QTOPIA;
	  }
	val_ret = ret;
	update_signal(*val_ret, data);
     }
   else
     {
	E_MOD_GAD_GSM_DEBUG_PRINTF("Qtopia signal callback  else part called");
	detected_system = PH_SYS_UNKNOWN;
	if (try_again_timer) ecore_timer_del(try_again_timer);
	try_again_timer = ecore_timer_add(5.0, try_again, data);
     }
}

static void
signal_callback_fso(void *data, void *ret, DBusError *err)
{
   E_MOD_GAD_GSM_DEBUG_PRINTF("FSO signal callback called");
   if (ret)
     {
	int *val_ret;

	if ((detected_system == PH_SYS_UNKNOWN) && (changed_fso_h) && (conn_system))
	  {
	     e_dbus_signal_handler_del(conn_system, changed_fso_h);
	     changed_fso_h = e_dbus_signal_handler_add(conn_system,
						       "org.freesmartphone.ogsmd",
						       "/org/freesmartphone/GSM/Device",
						       "org.freesmartphone.GSM.Network",
						       "SignalStrength",
						       signal_changed, data);
	     detected_system = PH_SYS_FSO;
	  }
	val_ret = ret;
	update_signal(*val_ret, data);
     }
   else
     {
	E_MOD_GAD_GSM_DEBUG_PRINTF("FSO signal callback else part called");
	detected_system = PH_SYS_UNKNOWN;
	if (try_again_timer) ecore_timer_del(try_again_timer);
	try_again_timer = ecore_timer_add(5.0, try_again, data);
     }
}

static void
operator_callback_qtopia(void *data, void *ret, DBusError *err)
{
   E_MOD_GAD_GSM_DEBUG_PRINTF("Qtopia operator callback called");
   if (ret)
     {
	if ((detected_system == PH_SYS_UNKNOWN) && (operatorch_h) && (conn))
	  {
	     e_dbus_signal_handler_del(conn, operatorch_h);
	     operatorch_h = e_dbus_signal_handler_add(conn,
						      "org.openmoko.qtopia.Phonestatus",
						      "/Status",
						      "org.openmoko.qtopia.Phonestatus",
						      "networkOperatorChanged",
						      operator_changed, data);
	     detected_system = PH_SYS_QTOPIA;
	  }
	Instance* newData = E_NEW(Instance, 1);
	newData->oper = ret;
	newData->registered = TRUE;
	update_operator(newData, data);
	E_FREE(newData);
     }
   else
     {
	E_MOD_GAD_GSM_DEBUG_PRINTF("Qtopia operator callback else part called");
	detected_system = PH_SYS_UNKNOWN;
	if (try_again_timer) ecore_timer_del(try_again_timer);
	try_again_timer = ecore_timer_add(5.0, try_again, data);
     }
}

static void
operator_callback_fso(void *data, void *ret, DBusError *err)
{
   E_MOD_GAD_GSM_DEBUG_PRINTF("FSO operator callback called");
   if (ret)
     {
	if ((detected_system == PH_SYS_UNKNOWN) && (operatorch_fso_h) && (conn_system))
	  {
	     e_dbus_signal_handler_del(conn_system, operatorch_fso_h);
	     operatorch_fso_h = e_dbus_signal_handler_add(conn_system,
							  "org.freesmartphone.ogsmd",
							  "/org/freesmartphone/GSM/Device",
							  "org.freesmartphone.GSM.Network",
							  "Status",
							  fso_operator_changed, data);
	     detected_system = PH_SYS_FSO;
	  }
	update_operator(ret, data);
     }
   else
     {
	E_MOD_GAD_GSM_DEBUG_PRINTF("FSO operator callback else part called");
	detected_system = PH_SYS_UNKNOWN;
	if (try_again_timer) ecore_timer_del(try_again_timer);
	try_again_timer = ecore_timer_add(5.0, try_again, data);
     }
}

static void
signal_result_free(void *data)
{
   free(data);
}

static void
operator_result_free(void *data)
{
   free(data);
}

static void gsmModemState_callback(void *data, void *ret, DBusError *err)
{
   E_MOD_GAD_GSM_DEBUG_PRINTF("gsmModemState_callback started");
   if (ret)
   {
      if ((detected_system == PH_SYS_UNKNOWN) && (device_status_changed_fso_h) && (conn_system))
      {
         e_dbus_signal_handler_del(conn_system, device_status_changed_fso_h);
         device_status_changed_fso_h = e_dbus_signal_handler_add(conn_system,
                                                "org.freesmartphone.ousaged",
                                                "/org/freesmartphone/Usage",
                                                "org.freesmartphone.Usage",
                                                "ResourceChanged",
                                                fso_resource_changed, data);
         detected_system = PH_SYS_FSO;
      }
      Instance* inst = data;
      int *val_ret;

      val_ret = ret;
      inst->init = FALSE;
      if (*val_ret <= 0)
      {
         inst->on = FALSE;
         inst->registered = FALSE;
      }
      else
      {
         inst->on = TRUE;
      }
      updateStatus(inst);
   }
   else
   {
      E_MOD_GAD_GSM_DEBUG_PRINTF("gsmModemState_callback else part called");
      detected_system = PH_SYS_UNKNOWN;
      if (try_again_timer) ecore_timer_del(try_again_timer);
      try_again_timer = ecore_timer_add(5.0, try_again, data);
   }
   E_MOD_GAD_GSM_DEBUG_PRINTF("gsmModemState_callback finished");
}

static void* gsmModemState_unmarhsall(DBusMessage *msg, DBusError *err)
{
   E_MOD_GAD_GSM_DEBUG_PRINTF("gsmModemState_unmarhsall started");
   unsigned int *val = NULL;
   
   if (dbus_message_get_args(msg, NULL, DBUS_TYPE_BOOLEAN, &val, DBUS_TYPE_INVALID))
   {
      int *val_ret;
       
       val_ret = malloc(sizeof(int));
       if (val_ret)
         {
            *val_ret = val;
            E_MOD_GAD_GSM_DEBUG_PRINTF("gsmModemState_unmarhsall finished");
            return val_ret;
         }
     }
   E_MOD_GAD_GSM_DEBUG_PRINTF("gsmModemState_unmarhsall finished (NULL)");
   return NULL;
}

static void gsmModemState_free(void *data)
{
    E_MOD_GAD_GSM_DEBUG_PRINTF("gsmModemState started");
   free(data);
    E_MOD_GAD_GSM_DEBUG_PRINTF("gsmModemState finished");
}

static void get_gsmModemState(void *data)
{
    DBusMessage *msg;
    
    E_MOD_GAD_GSM_DEBUG_PRINTF("Get gsm modem state called");
    if (((detected_system == PH_SYS_UNKNOWN) || (detected_system == PH_SYS_FSO)) && (conn_system))
    {
        msg = dbus_message_new_method_call("org.freesmartphone.ousaged",
                                          "/org/freesmartphone/Usage",
                                          "org.freesmartphone.Usage",
                                          "GetResourceState");
        if (msg)
        {

            DBusMessageIter iter;
            const char *str;
            dbus_message_iter_init_append(msg, &iter);
            str = "GSM";
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &str);

            e_dbus_method_call_send(conn_system, msg,gsmModemState_unmarhsall,gsmModemState_callback,gsmModemState_free, -1, data);
            dbus_message_unref(msg);
        }
    }
    E_MOD_GAD_GSM_DEBUG_PRINTF("Get gsm modem state call finished");
}


static void
get_signal(void *data)
{
   DBusMessage *msg;

   E_MOD_GAD_GSM_DEBUG_PRINTF("Get signal called");
   if (((detected_system == PH_SYS_UNKNOWN) || (detected_system == PH_SYS_QTOPIA)) && (conn))
     {
	msg = dbus_message_new_method_call("org.openmoko.qtopia.Phonestatus",
					   "/Status",
					   "org.openmoko.qtopia.Phonestatus",
					   "signalStrength");
	if (msg)
	  {
	     e_dbus_method_call_send(conn, msg,
				     signal_unmarhsall,
				     signal_callback_qtopia,
				     signal_result_free, -1, data);
	     dbus_message_unref(msg);
	  }
     }
   if (((detected_system == PH_SYS_UNKNOWN) || (detected_system == PH_SYS_FSO)) && (conn_system))
     {
	msg = dbus_message_new_method_call("org.freesmartphone.ogsmd",
					   "/org/freesmartphone/GSM/Device",
					   "org.freesmartphone.GSM.Network",
					   "GetSignalStrength");
	if (msg)
	  {
	     e_dbus_method_call_send(conn_system, msg,
				     signal_unmarhsall,
				     signal_callback_fso,
				     signal_result_free, -1, data);
	     dbus_message_unref(msg);
	  }
     }
}

static void
get_operator(void *data)
{
   DBusMessage *msg;
   E_MOD_GAD_GSM_DEBUG_PRINTF("Get operator called");
   if (((detected_system == PH_SYS_UNKNOWN) || (detected_system == PH_SYS_QTOPIA)) && (conn))
     {
	msg = dbus_message_new_method_call("org.openmoko.qtopia.Phonestatus",
					   "/Status",
					   "org.openmoko.qtopia.Phonestatus",
					   "networkOperator");
	if (msg)
	  {
	     e_dbus_method_call_send(conn, msg,
				     operator_unmarhsall,
				     operator_callback_qtopia,
				     operator_result_free, -1, data);
	     dbus_message_unref(msg);
	  }
     }
   if (((detected_system == PH_SYS_UNKNOWN) || (detected_system == PH_SYS_FSO)) && (conn_system))
     {
	msg = dbus_message_new_method_call("org.freesmartphone.ogsmd",
					   "/org/freesmartphone/GSM/Device",
					   "org.freesmartphone.GSM.Network",
					   "GetStatus");
	if (msg)
	  {
	     e_dbus_method_call_send(conn_system, msg,
				     fso_operator_unmarhsall,
				     operator_callback_fso,
				     operator_result_free, -1, data);
	     dbus_message_unref(msg);
	  }
     }
}

static void
signal_changed(void *data, DBusMessage *msg)
{
   DBusError err;
   dbus_int32_t val = -1;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID))
     return;
   update_signal(val, data);
}

static void
operator_changed(void *data, DBusMessage *msg)
{
   DBusError err;
   char *str = NULL;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID))
     return;

   Instance* newData = E_NEW(Instance, 1);
   newData->oper = str;
   newData->registered = TRUE;
   update_operator(newData, data);
   E_FREE(newData);

}

static void
fso_operator_changed(void *data, DBusMessage *msg)
{
   E_MOD_GAD_GSM_DEBUG_PRINTF("fso_operator_changed started");
   Instance* newData;
   newData = _fso_operator_unmarhsall(msg);
   update_operator(newData, data);
   E_FREE(newData);
   E_MOD_GAD_GSM_DEBUG_PRINTF("fso_operator_changed finished");
}

static void fso_resource_changed(void* data, DBusMessage* msg)
{
    E_MOD_GAD_GSM_DEBUG_PRINTF("fso_resource_changed started");
    Instance* inst = data;
    DBusError err;
    DBusMessageIter args;

    char *resource = NULL;
    bool state = NULL;
   
    dbus_error_init(&err);
    if (dbus_message_has_signature(msg, "sba{sv}"))
    {
       E_MOD_GAD_GSM_DEBUG_PRINTF("fso_resource_changed extract 1.parameter");
       dbus_message_iter_init(msg, &args);
       dbus_message_iter_get_basic(&args, &resource);
       E_MOD_GAD_GSM_DEBUG_PRINTF("fso_resource_changed extract 2.parameter");
       dbus_message_iter_next(&args);
       dbus_message_iter_get_basic(&args, &state);
       E_MOD_GAD_GSM_DEBUG_PRINTF("fso_resource_changed resource");
        if (strcmp("GSM",resource) == 0)
        {
            if (state == FALSE)
            {
                inst->on = FALSE;
                inst->registered = FALSE;
            }
            else
            {
                inst->on = TRUE;
            }
            updateStatus(inst);
        }
    }
    else
    {
        E_MOD_GAD_GSM_DEBUG_PRINTF("fso_resource_changed wrong message format");
    }
    E_MOD_GAD_GSM_DEBUG_PRINTF("fso_resource_changed finished");

}

static void
name_changed(void *data, DBusMessage *msg)
{
   DBusError err;
   const char *s1, *s2, *s3;

   dbus_error_init(&err);
   if (!dbus_message_get_args(msg, &err,
			      DBUS_TYPE_STRING, &s1,
			      DBUS_TYPE_STRING, &s2,
			      DBUS_TYPE_STRING, &s3,
			      DBUS_TYPE_INVALID))
     return;
   if ((!strcmp(s1, "org.openmoko.qtopia.Phonestatus")) && (conn))
     {
	E_MOD_GAD_GSM_DEBUG_PRINTF("Qtopia name owner changed");
	if (changed_h)
	  {
	     e_dbus_signal_handler_del(conn, changed_h);
	     changed_h = e_dbus_signal_handler_add(conn,
						   "org.openmoko.qtopia.Phonestatus",
						   "/Status",
						   "org.openmoko.qtopia.Phonestatus",
						   "signalStrengthChanged",
						   signal_changed, data);
	     get_signal(data);
	  }
	if (operatorch_h)
	  {
	     e_dbus_signal_handler_del(conn, operatorch_h);
	     operatorch_h = e_dbus_signal_handler_add(conn,
						      "org.openmoko.qtopia.Phonestatus",
						      "/Status",
						      "org.openmoko.qtopia.Phonestatus",
						      "networkOperatorChanged",
						      operator_changed, data);
	     get_operator(data);
	  }
     }
   else if ((!strcmp(s1, "org.freesmartphone.ogsmd")) && (conn_system))
     {
	E_MOD_GAD_GSM_DEBUG_PRINTF("FSO name owner changed");
      if (device_status_changed_fso_h)
      {
         e_dbus_signal_handler_del(conn_system, device_status_changed_fso_h);
         device_status_changed_fso_h = e_dbus_signal_handler_add(conn_system,
                                                "org.freesmartphone.ousaged",
                                                "/org/freesmartphone/Usage",
                                                "org.freesmartphone.Usage",
                                                "ResourceChanged",
                                                fso_resource_changed, data);
      }
	if (changed_fso_h) 
	  {
	     e_dbus_signal_handler_del(conn_system, changed_fso_h);
	     changed_fso_h = e_dbus_signal_handler_add(conn_system,
						       "org.freesmartphone.ogsmd",
						       "/org/freesmartphone/GSM/Device",
						       "org.freesmartphone.GSM.Network",
						       "SignalStrength",
						       signal_changed, data);
	     get_signal(data);
	  }
	if (operatorch_fso_h)
	  {
	     e_dbus_signal_handler_del(conn_system, operatorch_fso_h);
	     operatorch_fso_h = e_dbus_signal_handler_add(conn_system,
							  "org.freesmartphone.ogsmd",
							  "/org/freesmartphone/GSM/Device",
							  "org.freesmartphone.GSM.Network",
							  "Status",
							  fso_operator_changed, data);
	     get_operator(data);
	  }
     }
   return;
}

static void updateStatus(Instance* inst)
{
    E_MOD_GAD_GSM_DEBUG_PRINTF("updateStatus started");
    if (inst->init)
    {
        E_MOD_GAD_GSM_DEBUG_PRINTF("updateStatus init = TRUE");
        setLabel(inst->obj, "Init...");
        setState(inst->obj, FALSE);
    }
    else
    {
        E_MOD_GAD_GSM_DEBUG_PRINTF("updateStatus init = FALSE");
        if (inst->on)
        {
            E_MOD_GAD_GSM_DEBUG_PRINTF("updateStatus on = TRUE");
            setState(inst->obj, TRUE);
            if (inst->registered)
            {
                E_MOD_GAD_GSM_DEBUG_PRINTF("updateStatus registered = TRUE");
                setLabel(inst->obj, inst->oper);
                setLevel(inst->obj, inst->strength);
            }
            else
            {
                E_MOD_GAD_GSM_DEBUG_PRINTF("updateStatus registered = FALSE");
                setLabel(inst->obj, "None");
                setLevel(inst->obj, 0);
            }
        }
        else
        {
            E_MOD_GAD_GSM_DEBUG_PRINTF("updateStatus on = FALSE");
            setLabel(inst->obj, "Off");
            setState(inst->obj, FALSE);
        }
    }
    E_MOD_GAD_GSM_DEBUG_PRINTF("updateStatus finished");
}

static void setLabel(Evas_Object* obj, char* label)
{
    Edje_Message_String msg;
    msg.str = label;
    edje_object_message_send(obj, EDJE_MESSAGE_STRING, 1, &msg);
}

static void setLevel(Evas_Object* obj, int strength)
{
    Edje_Message_Float msg;
    double level;
    if (strength > 80)
    {
        level = 1.0;
    }
    else if (strength > 60)
    {
        level = 0.8;
    }
    else if (strength > 40)
    {
        level = 0.6;
    }
    else if (strength > 20)
    {
        level = 0.4;
    }
    else if (strength > 0)
    {
        level = 0.2;
    }
    else
    {
        level = 0.0;
    }
    E_MOD_GAD_GSM_DEBUG_PRINTF("setlevel")
    msg.val = level;
    edje_object_message_send(obj, EDJE_MESSAGE_FLOAT, 1, &msg);
}

static void setState(Evas_Object* obj, int state)
{
    if (state)
    {
        edje_object_signal_emit(obj, "e,state,active", "e");
    }
    else
    {
        edje_object_signal_emit(obj, "e,state,passive", "e");
    }
}
