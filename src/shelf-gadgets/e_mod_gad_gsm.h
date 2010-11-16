#ifndef E_MOD_GAD_GSM_H
#define E_MOD_GAD_GSM_H

#define E_MOD_GAD_GSM_DEBUG 0

#if E_MOD_GAD_GSM_DEBUG
#define E_MOD_GAD_GSM_DEBUG_PRINTF(msg) fprintf(stderr, "%s: %s\n", "GSM-gadget", msg);
#else
#define E_MOD_GAD_GSM_DEBUG_PRINTF(...) /* */
#endif

void _e_mod_gad_gsm_init(E_Module *m);
void _e_mod_gad_gsm_shutdown(void);

#endif
