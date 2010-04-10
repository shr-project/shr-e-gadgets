#include "e.h"
#include "e_mod_main.h"

/* local structures */
enum Mode{
	MODE_LEFT,
	MODE_RIGHT,
	MODE_1RIGHT /*Putting it here makes it useless, this is on purpose*/

};
typedef struct _Instance Instance;
struct _Instance 
{
   E_Gadcon_Client *gcc;
   Evas_Object *o_btn;
   enum Mode mode;
};

/* local function prototypes */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *cc);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *cc);
static void _cb_btn_click(void *data, void *data2);
static void _set_icon(Instance *inst);

/* local variables */
static Eina_List *instances = NULL;
static const char *mod_dir = NULL;

static const E_Gadcon_Client_Class _gc_class = 
{
   GADCON_CLIENT_CLASS_VERSION, "illume-rightclick-toggle", 
     { _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, 
          e_gadcon_site_is_not_toolbar
     }, E_GADCON_CLIENT_STYLE_PLAIN
};

/* public functions */
EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume Rightclick Toggle" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   mod_dir = eina_stringshare_add(m->dir);
   e_gadcon_provider_register(&_gc_class);
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   e_gadcon_provider_unregister(&_gc_class);
   if (mod_dir) eina_stringshare_del(mod_dir);
   mod_dir = NULL;
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return 1;
}

/* local functions */
static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style) 
{
   Instance *inst;

   inst = E_NEW(Instance, 1);

   inst->o_btn = e_widget_button_add(gc->evas, NULL, NULL, 
                                     _cb_btn_click, inst, NULL);
   inst->gcc = e_gadcon_client_new(gc, name, id, style, inst->o_btn);
   inst->gcc->data = inst;

   _set_icon(inst);

   instances = eina_list_append(instances, inst);
   return inst->gcc;
}

static void 
_gc_shutdown(E_Gadcon_Client *gcc) 
{
   Instance *inst;

   if (!(inst = gcc->data)) return;
   instances = eina_list_remove(instances, inst);
   if (inst->o_btn) evas_object_del(inst->o_btn);
   E_FREE(inst);
}

static void 
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient) 
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *cc) 
{
   return "Illume-Rightclick-Toggle";
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *cc, Evas *evas) 
{
   Evas_Object *o;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-rightclick-toggle.edj", mod_dir);
   o = edje_object_add(evas);
   edje_object_file_set(o, buff, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *cc) 
{
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s.%d", _gc_class.name, 
            eina_list_count(instances));
   return strdup(buff);
}

static void
_set_touchscreen_mode(enum Mode mode)
{
   switch (mode) {
      case MODE_LEFT:
         system("xinput set-button-map Touchscreen 1 2 3 4 5");
         break;
      case MODE_1RIGHT: /*FIXME: make 1RIGHT usable */
      case MODE_RIGHT:
         system("xinput set-button-map Touchscreen 3 2 1 4 5");
         break;
   }
}
static void 
_cb_btn_click(void *data, void *data2) 
{
   Instance *inst;

   if (!(inst = data)) return;

   inst->mode += 1;
   if (inst->mode > MODE_RIGHT)
     inst->mode = MODE_LEFT;
   _set_touchscreen_mode(inst->mode);
   _set_icon(inst);
}


static void 
_set_icon(Instance *inst) 
{
   Evas_Object *icon;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), "%s/e-module-illume-rightclick-toggle.edj", mod_dir);
   icon = e_icon_add(evas_object_evas_get(inst->o_btn));

   switch (inst->mode) {
      case MODE_LEFT:
         e_icon_file_edje_set(icon, buff, "left");
         break;
      case MODE_1RIGHT:
         e_icon_file_edje_set(icon, buff, "1right");
         break;
      case MODE_RIGHT:
         e_icon_file_edje_set(icon, buff, "right");
         break;
   }

   e_widget_button_icon_set(inst->o_btn, icon);
}
