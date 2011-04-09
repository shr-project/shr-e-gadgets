#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Battery Battery;

struct _Battery
{
   const char *udi;
   Eina_Bool present:1;
   Eina_Bool charging:1;
   int percent;
   int current_charge;
   int design_charge;
   int last_full_charge;
   int charge_rate;
   int time_full;
   int time_left;
   const char *type;
   const char *charge_units;
   const char *technology;
   const char *model;
   const char *vendor;
   Eina_Bool got_prop:1;
};

Battery *_battery_battery_find(const char *udi);
void _battery_device_update(void);
/* in e_mod_fso.c */
int  _battery_fso_start(void);
void _battery_fso_stop(void);

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

#endif
