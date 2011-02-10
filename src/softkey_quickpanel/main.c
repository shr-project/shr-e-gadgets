#include "elm_softkey.h"

#ifndef ELM_LIB_QUICKLAUNCH

/* local function prototypes */
static void _cb_win_del(void *data, Evas_Object *obj, void *event);
static void _cb_btn_close_clicked(void *data, Evas_Object *obj, void *event);
static void _cb_btn_back_clicked(void *data, Evas_Object *obj, void *event);
static void _cb_btn_forward_clicked(void *data, Evas_Object *obj, void *event);

/* local variables */

Eina_Bool visible=0;
Ecore_X_Window root;

#define THEME PACKAGE_DATA_DIR "/themes/default.edj"

EAPI int 
elm_main(int argc, char **argv) 
{
	Ecore_X_Window *zones = NULL, xwin;
	Ecore_X_Window_State states[2];
	Evas_Object *win, *bg, *layout, *table, *btn, *icon;
	char buff[PATH_MAX];
	int zx, zy, zw, zh, finger = elm_finger_size_get();
	int count = 0;
	unsigned int *zoneid;

	root = ecore_x_window_root_first_get();
	if (!root) return EXIT_FAILURE;

	count = ecore_x_window_prop_window_list_get(root, ECORE_X_ATOM_E_ILLUME_ZONE_LIST, &zones);

	if (!zones) return EXIT_FAILURE;

	elm_theme_extension_add(NULL, THEME);

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

	layout = elm_layout_add(win);
	elm_layout_file_set(layout, THEME, "shr_elm_sk/layout");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, layout);
	evas_object_show(layout);

	table = elm_table_add(win);
	elm_table_homogenous_set(table, EINA_TRUE);
	evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, table);

	icon = elm_icon_add(win);
	elm_icon_file_set(icon, THEME, "back");
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

	btn = elm_button_add(win);
	elm_button_icon_set(btn, icon);
	evas_object_smart_callback_add(btn, "clicked", _cb_btn_back_clicked, win);
	evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_table_pack(table, btn, 0, 0, 1, 1);
	evas_object_show(icon);
	evas_object_show(btn);

	icon = elm_icon_add(win);
	elm_icon_file_set(icon, THEME, "close");
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

	btn = elm_button_add(win);
	elm_button_icon_set(btn, icon);
	evas_object_smart_callback_add(btn, "clicked", _cb_btn_close_clicked, win);
	evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_table_pack(table, btn, 1, 0, 1, 1);
	evas_object_show(icon);
	evas_object_show(btn);

	icon = elm_icon_add(win);
	elm_icon_file_set(icon, THEME, "forward");
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

	btn = elm_button_add(win);
	elm_button_icon_set(btn, icon);
	evas_object_smart_callback_add(btn, "clicked", _cb_btn_forward_clicked, win);
	evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_table_pack(table, btn, 2, 0, 1, 1);
	evas_object_show(icon);
	evas_object_show(btn);

	evas_object_show(table);

	ecore_x_window_geometry_get(zones[0], &zx, &zy, &zw, &zh);
	ecore_x_e_illume_softkey_geometry_set(zones[0], zx, (zy + zh - finger), zw, finger);

	evas_object_move(win, zx, (zy + zh - finger));
	evas_object_resize(win, zw, finger);

	elm_layout_content_set(layout, "buttons", table);

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
