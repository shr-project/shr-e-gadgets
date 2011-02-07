#include "elm_softkey.h"

#ifndef ELM_LIB_QUICKLAUNCH

/* local function prototypes */
static void _cb_win_del(void *data, Evas_Object *obj, void *event);
static void _cb_btn_close_clicked(void *data, Evas_Object *obj, void *event);
static void _cb_btn_back_clicked(void *data, Evas_Object *obj, void *event);
static void _cb_btn_forward_clicked(void *data, Evas_Object *obj, void *event);

/* local variables */

EAPI int 
elm_main(int argc, char **argv) 
{
	Ecore_X_Window root, *zones = NULL, xwin;
	Ecore_X_Window_State states[2];
	Evas_Object *win, *bg, *box, *btn, *icon;
	char buff[PATH_MAX];
	int zx, zy, zw, zh, finger = elm_finger_size_get();
	int count = 0;
	unsigned int *zoneid;

	root = ecore_x_window_root_first_get();
	if (!root) return EXIT_FAILURE;

	count = ecore_x_window_prop_window_list_get(root, ECORE_X_ATOM_E_ILLUME_ZONE_LIST, &zones);

	if (!zones) return EXIT_FAILURE;

	zoneid = calloc(1, sizeof(unsigned int));
	*zoneid = zones[0];

	/* create new window */
	win = elm_win_add(NULL, "Illume-Softkey", ELM_WIN_BASIC);
	elm_win_title_set(win, "Illume Softkey");
	elm_win_quickpanel_set(win, 1);
	elm_win_quickpanel_priority_major_set(win, 5);
	elm_win_quickpanel_priority_minor_set(win, 5);
	evas_object_smart_callback_add(win, "delete-request", _cb_win_del, NULL);
	evas_object_data_set(win, "zone", (const void *)zoneid);

	xwin = elm_win_xwindow_get(win);
	ecore_x_icccm_hints_set(xwin, 0, 0, 0, 0, 0, 0, 0);
	states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
	states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
	ecore_x_netwm_window_state_set(xwin, states, 2);

	bg = elm_bg_add(win);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, bg);
	evas_object_show(bg);

	box = elm_box_add(win);
	elm_box_horizontal_set(box, EINA_TRUE);
	elm_box_homogenous_set(box, EINA_TRUE);
	elm_box_padding_set(box, finger/2, finger/4);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, box);
	evas_object_show(box);

	icon = elm_icon_add(win);
	snprintf(buff, sizeof(buff), "%s/images/back.png", PACKAGE_DATA_DIR);
	elm_icon_file_set(icon, buff, NULL);
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

	btn = elm_button_add(win);
	elm_button_icon_set(btn, icon);
	evas_object_smart_callback_add(btn, "clicked", _cb_btn_back_clicked, win);
	evas_object_size_hint_align_set(btn, 0.5, 0.5);
	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_box_pack_end(box, btn);
	evas_object_show(btn);
	evas_object_show(icon);

	icon = elm_icon_add(win);
	snprintf(buff, sizeof(buff), "%s/images/close.png", PACKAGE_DATA_DIR);
	elm_icon_file_set(icon, buff, NULL);
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

	btn = elm_button_add(win);
	elm_button_icon_set(btn, icon);
	evas_object_smart_callback_add(btn, "clicked", _cb_btn_close_clicked, win);
	evas_object_size_hint_align_set(btn, 0.5, 0.5);
	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_box_pack_end(box, btn);
	evas_object_show(btn);
	evas_object_show(icon);

	icon = elm_icon_add(win);
	snprintf(buff, sizeof(buff), "%s/images/forward.png", PACKAGE_DATA_DIR);
	elm_icon_file_set(icon, buff, NULL);
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

	btn = elm_button_add(win);
	elm_button_icon_set(btn, icon);
	evas_object_smart_callback_add(btn, "clicked", _cb_btn_forward_clicked, win);
	evas_object_size_hint_align_set(btn, 0.5, 0.5);
	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_box_pack_end(box, btn);
	evas_object_show(btn);
	evas_object_show(icon);

	ecore_x_window_geometry_get(zones[0], &zx, &zy, &zw, &zh);
	ecore_x_e_illume_softkey_geometry_set(zones[0], zx, (zy + zh - finger), zw, finger);

	evas_object_move(win, zx, (zy + zh - finger));
	evas_object_resize(win, zw, finger);
	evas_object_show(win);

	free(zones);

	elm_run();

	elm_shutdown();
	return EXIT_SUCCESS;
}

static void 
_cb_win_del(void *data, Evas_Object *obj, void *event) 
{
	Ecore_X_Window *zone;

	zone = (Ecore_X_Window*)evas_object_data_get(obj, "zone");
	ecore_x_e_illume_softkey_geometry_set(*zone, 0, 0, 0, 0);

	elm_exit();
}

static void 
_cb_btn_close_clicked(void *data, Evas_Object *obj, void *event) 
{
	Evas_Object *win;
	Ecore_X_Window *zone;

	if (!(win = data)) return;
	zone = (Ecore_X_Window*)evas_object_data_get(win, "zone");
	ecore_x_e_illume_close_send(*zone);
	ecore_x_e_illume_quickpanel_state_send(*zone, ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
}

static void 
_cb_btn_back_clicked(void *data, Evas_Object *obj, void *event) 
{
	Evas_Object *win;
	Ecore_X_Window *zone;

	if (!(win = data)) return;
	zone = (Ecore_X_Window*)evas_object_data_get(win, "zone");
	ecore_x_e_illume_focus_back_send(*zone);
}

static void 
_cb_btn_forward_clicked(void *data, Evas_Object *obj, void *event) 
{
	Evas_Object *win;
	Ecore_X_Window *zone;

	if (!(win = data)) return;
	zone = (Ecore_X_Window*)evas_object_data_get(win, "zone");
	ecore_x_e_illume_focus_forward_send(*zone);
}
#endif
ELM_MAIN();
