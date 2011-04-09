#include "e.h"
#include "e_mod_main.h"

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);

Eina_List *device_batteries;
double init_time;

/* local variables */
static Eina_List *instances = NULL;
static const char *mod_dir = NULL;

/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "battery-fso",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* actual module specifics */
typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_battery;
   Evas_Object     *popup_battery;
   E_Gadcon_Popup  *warning;
};

static void _battery_update(int full, int time_left, int time_full, Eina_Bool have_battery, Eina_Bool have_power);
static void _battery_face_level_set(Evas_Object *battery, double level);
static void _battery_face_time_set(Evas_Object *battery, int time);

static Eina_Bool  _battery_cb_warning_popup_timeout(void *data);
static void _battery_cb_warning_popup_hide(void *data, Evas *e, Evas_Object *obj, void *event);
static void _battery_warning_popup_destroy(Instance *inst);
static void _battery_warning_popup(Instance *inst, int time, double percent);

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;

   inst = E_NEW(Instance, 1);

   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/battery", "e/modules/battery/main");

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_battery = o;
   inst->warning = NULL;
   inst->popup_battery = NULL;
   instances = eina_list_append(instances, inst);

   e_dbus_init();
   _battery_fso_start();

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   e_dbus_shutdown();

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);
   evas_object_del(inst->o_battery);
   if (inst->warning)
     {
        e_object_del(E_OBJECT(inst->warning));
        inst->popup_battery = NULL;
     }
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   Instance *inst;
   Evas_Coord mw, mh, mxw, mxh;

   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->o_battery, &mw, &mh);
   edje_object_size_max_get(inst->o_battery, &mxw, &mxh);
   if ((mw < 1) || (mh < 1)) edje_object_size_min_calc(inst->o_battery, &mw, &mh); 
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   if ((mxw > 0) && (mxh > 0)) e_gadcon_client_aspect_set(gcc, mxw, mxh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return ("Battery-FSO");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/e-module-battery-fso.edj", mod_dir);
   o = edje_object_add(evas);
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _gadcon_class.name;
}

static void
_battery_face_level_set(Evas_Object *battery, double level)
{
   Edje_Message_Float msg;
   char buf[256];

   snprintf(buf, sizeof(buf), "%i%%", (int)(level*100.0));
   edje_object_part_text_set(battery, "e.text.reading", buf);

   if (level < 0.0) level = 0.0;
   else if (level > 1.0) level = 1.0;
   msg.val = level;
   edje_object_message_send(battery, EDJE_MESSAGE_FLOAT, 1, &msg);
}

static void 
_battery_face_time_set(Evas_Object *battery, int time)
{
   char buf[256];
   int hrs, mins;

   if (time < 0) return;

   hrs = (time / 3600);
   mins = ((time) / 60 - (hrs * 60));
   if (hrs < 0) hrs = 0;
   if (mins < 0) mins = 0;
   snprintf(buf, sizeof(buf), "%i:%02i", hrs, mins);
   edje_object_part_text_set(battery, "e.text.time", buf);
}


Battery *
_battery_battery_find(const char *udi)
{
   Eina_List *l;
   Battery *bat;
   EINA_LIST_FOREACH(device_batteries, l, bat)
     {/* these are always stringshared */
        if (udi == bat->udi) return bat;
     }

   return NULL;
}

void
_battery_device_update(void)
{
   Eina_List *l;
   Battery *bat;
   int full = -1;
   int time_left = -1;
   int time_full = -1;
   int have_battery = 0;
   int have_power = 0;
   int charging = 0;

   int batnum = 0;

   EINA_LIST_FOREACH(device_batteries, l, bat)
     {
        if (!bat->got_prop)
          continue;
        have_battery = 1;
        batnum++;
        if (bat->charging == 1) have_power = 1;
        if (full == -1) full = 0;
        if (bat->percent >= 0)
          full += bat->percent;
        else if (bat->last_full_charge > 0)
          full += (bat->current_charge * 100) / bat->last_full_charge;
        else if (bat->design_charge > 0)
          full += (bat->current_charge * 100) / bat->design_charge;
        if (bat->time_left > 0)
          {
             if (time_left < 0) time_left = bat->time_left;
             else time_left += bat->time_left;
          }
        if (bat->time_full > 0)
          {
             if (time_full < 0) time_full = bat->time_full;
             else time_full += bat->time_full;
          }
        charging += bat->charging;
     }

   if ((device_batteries) && (batnum == 0))
     return; /* not ready yet, no properties received for any battery */

   if (batnum > 0) full /= batnum;
   if ((full == 100) && have_power)
     {
        time_left = -1;
        time_full = -1;
     }
   if (time_left < 1) time_left = -1;
   if (time_full < 1) time_full = -1;
   
   _battery_update(full, time_left, time_full, have_battery, have_power);
   if (batnum == 0)
     e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
}

static Eina_Bool
_battery_cb_warning_popup_timeout(void *data)
{
   Instance *inst;

   inst = data;
   e_gadcon_popup_hide(inst->warning);
   return ECORE_CALLBACK_CANCEL;
}

static void
_battery_cb_warning_popup_hide(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   Instance *inst = NULL;

   inst = (Instance *)data;
   if ((!inst) || (!inst->warning)) return;
   e_gadcon_popup_hide(inst->warning);
}

static void
_battery_warning_popup_destroy(Instance *inst)
{

   if ((!inst) || (!inst->warning)) return;
   e_object_del(E_OBJECT(inst->warning));
   inst->warning = NULL;
   inst->popup_battery = NULL;
}

static void
_battery_warning_popup(Instance *inst, int time, double percent)
{
   Evas *e = NULL;
   Evas_Object *rect = NULL, *popup_bg = NULL;
   int x,y,w,h;

   if ((!inst) || (inst->warning)) return;

   inst->warning = e_gadcon_popup_new(inst->gcc);
   if (!inst->warning) return;

   e = inst->warning->win->evas;

   popup_bg = edje_object_add(e);
   inst->popup_battery = edje_object_add(e);
     
   if ((!popup_bg) || (!inst->popup_battery))
     {
        e_object_free(E_OBJECT(inst->warning));
        inst->warning = NULL;
        return;
     }

   e_theme_edje_object_set(popup_bg, "base/theme/modules/battery/popup", "e/modules/battery/popup");
   e_theme_edje_object_set(inst->popup_battery, "base/theme/modules/battery", "e/modules/battery/main");
   edje_object_part_swallow(popup_bg, "battery", inst->popup_battery);

   edje_object_part_text_set(popup_bg, "e.text.title", ("Your battery is low!"));
   edje_object_part_text_set(popup_bg, "e.text.label", ("AC power is recommended."));

   e_gadcon_popup_content_set(inst->warning, popup_bg);
   e_gadcon_popup_show(inst->warning);

   evas_object_geometry_get(inst->warning->o_bg, &x, &y, &w, &h);

   rect = evas_object_rectangle_add(e);
   if (rect)
     {
        evas_object_move(rect, x, y);
        evas_object_resize(rect, w, h);
        evas_object_color_set(rect, 255, 255, 255, 0);
        evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_DOWN, _battery_cb_warning_popup_hide, inst);
        evas_object_repeat_events_set(rect, 1);
        evas_object_show(rect);
     }

   _battery_face_time_set(inst->popup_battery, time);
   _battery_face_level_set(inst->popup_battery, percent);
   edje_object_signal_emit(inst->popup_battery, "e,state,discharging", "e");

}

static void
_battery_update(int full, int time_left, int time_full, Eina_Bool have_battery, Eina_Bool have_power)
{
   Eina_List *l;
   Instance *inst;
   static double debounce_time = 0.0;
   
   EINA_LIST_FOREACH(instances, l, inst)
     {
        if (have_power && (full < 100))
          edje_object_signal_emit(inst->o_battery, "e,state,charging", "e");
        else
          {
             edje_object_signal_emit(inst->o_battery, "e,state,discharging", "e");
             if (inst->popup_battery)
               edje_object_signal_emit(inst->popup_battery, "e,state,discharging", "e");
          }
        if (have_battery)
          {
             double val;

             if (full >= 100) val = 1.0;
             else val = (double)full / 100.0;
             _battery_face_level_set(inst->o_battery, val);
             if (inst->popup_battery)
               _battery_face_level_set(inst->popup_battery, val);
          }
        else
          {
             _battery_face_level_set(inst->o_battery, 0.0);
             edje_object_part_text_set(inst->o_battery, "e.text.reading", ("N/A"));
          }
        
        if (time_full < 0)
          {
             _battery_face_time_set(inst->o_battery, time_left);
             if (inst->popup_battery)
               _battery_face_time_set(inst->popup_battery, 
                                      time_left);
          }
        else if (time_left < 0)
          {
             _battery_face_time_set(inst->o_battery, time_full);
             if (inst->popup_battery)
               _battery_face_time_set(inst->popup_battery, 
                                      time_full);
          }
     }
   if (!have_battery)
     e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
   else
     {
        if ((have_power) || (full > 95))
          e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
        else if (full > 30)
          e_powersave_mode_set(E_POWERSAVE_MODE_HIGH);
        else
          e_powersave_mode_set(E_POWERSAVE_MODE_EXTREME);
     }
}

/* module setup */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Battery-FSO" };

EAPI void *
e_modapi_init(E_Module *m)
{
   mod_dir = eina_stringshare_add(m->dir);
   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   if (mod_dir) eina_stringshare_del(mod_dir);
   mod_dir = NULL;

   _battery_fso_stop();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
