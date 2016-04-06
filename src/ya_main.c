/*
 * Yabar - A modern and lightweight status bar for X window managers.
 *
 * Copyright (c) 2016, George Badawi
 * See LICENSE for more information.
 *
 */

#include "yabar.h"


yabar_info_t ya;

int main (int argc, char * argv[]) {
	ya_process_opt(argc, argv);
	ya_init();
	ya_execute();


	xcb_generic_event_t *ev;
	while((ev = xcb_wait_for_event(ya.c))) {
		switch(ev->response_type & ~0x80) {
#ifdef YA_INTERNAL_EWMH
			case XCB_PROPERTY_NOTIFY: {
				ya_handle_prop_notify((xcb_property_notify_event_t *) ev);
				break;
			}
#endif //YA_INTERNAL_EWMH
			case XCB_BUTTON_PRESS: {
				ya_handle_button((xcb_button_press_event_t *) ev);
				break;
			}
		}
		free(ev);
	}
	//shouldn't get here
	xcb_disconnect(ya.c);
}

