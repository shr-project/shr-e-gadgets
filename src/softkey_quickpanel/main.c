#include "elm_softkey.h"

#ifndef ELM_LIB_QUICKLAUNCH

/* local function prototypes */
static void _cb_win_del(void *data, Evas_Object *obj, void *event);
static void _cb_btn_close_clicked(void *data, Evas_Object *obj, void *event);
static void _cb_btn_back_clicked(void *data, Evas_Object *obj, void *event);
static void _cb_btn_forward_clicked(void *data, Evas_Object *obj, void *event);
static void _cb_btn_down_clicked(void *data, Evas_Object *obj, void *event);
static void show_tasklist(void);

/* local types */

typedef struct _swkey_Window
{
	Ecore_X_Window xwin;
	char *name;
	char *title;
} SwKeyWindow;

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

	icon = elm_icon_add(win);
	elm_icon_file_set(icon, THEME, "down");
	evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);

	btn = elm_button_add(win);
	elm_button_icon_set(btn, icon);
	evas_object_smart_callback_add(btn, "clicked", _cb_btn_down_clicked, win);
	evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_table_pack(table, btn, 1, 1, 1, 1);
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

static void 
_cb_btn_down_clicked(void *data, Evas_Object *obj, void *event) 
{
	Evas_Object *quickpanel = (Evas_Object*)data;
	Ecore_X_Window *zone;

	/* hide the quickpanel as it's no longer necessary */
	if (!quickpanel) return;
	zone = (Ecore_X_Window*)evas_object_data_get(quickpanel, "zone");
	ecore_x_e_illume_quickpanel_state_send(*zone, ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
	show_tasklist();
}

static void 
on_win_select(void *data, Evas_Object *obj, void *event) 
{
	Evas_Object *win = elm_object_parent_widget_get(obj);
	Ecore_X_Window_State tasklist = elm_win_xwindow_get(win);
	SwKeyWindow *window = (SwKeyWindow*)data;

	if(!window) return;

	printf("Selected window: %s / %s / %d\n", window->title, window->name, window->xwin);
	ecore_x_netwm_client_active_request(root, window->xwin, 0, tasklist);
}

static void show_tasklist(void)
{
	Evas_Object *win, *bg, *box, *list;
	Ecore_X_Window_State *state=NULL;
	Ecore_X_Window *windows = NULL;
	int num, i, snum, j, include;
	SwKeyWindow *window;
	char *tmp=NULL;

	num = ecore_x_window_prop_window_list_get(root, ECORE_X_ATOM_NET_CLIENT_LIST, &windows);

	if( !(num && windows)) return;

	win = elm_win_add(NULL, "Illume-Softkey-Tasklist", ELM_WIN_BASIC);
	elm_win_title_set(win, "Running Tasks");
	elm_win_autodel_set(win, EINA_TRUE);

	bg = elm_bg_add(win);
	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, bg);
	evas_object_show(bg);

	box = elm_box_add(win);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_win_resize_object_add(win, box);

	list = elm_list_add(win);
	evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	for (i = 0; i < num; i++)
	{
		include = 1;

		ecore_x_netwm_window_state_get(windows[i], &state, &snum);
		for (j = 0; j < snum; j++, state++)
			if ( (*state == ECORE_X_WINDOW_STATE_SKIP_TASKBAR) || (*state == ECORE_X_WINDOW_STATE_SKIP_PAGER) )
				include = 0;
		if(include)
		{
			window = calloc(1, sizeof(SwKeyWindow));
			window->xwin = windows[i];
			ecore_x_icccm_name_class_get(windows[i], &window->name, NULL);
			window->title = ecore_x_icccm_title_get(windows[i]);
			if(window->title)
				elm_list_item_append(list, window->title, NULL, NULL, on_win_select, window);
			else if(window->name)
				elm_list_item_append(list, window->name, NULL, NULL, on_win_select, window);
			else 
				elm_list_item_append(list, "Untitled", NULL, NULL, on_win_select, window);
		}
	}

	elm_list_go(list);
	evas_object_show(list);
	elm_box_pack_end(box, list);

	evas_object_show(box);
	evas_object_show(win);
}

#endif
ELM_MAIN();
