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
	xcb_button_press_event_t *eb;
	ya_block_t *blkev;
	while((ev = xcb_wait_for_event(ya.c))) {
		switch(ev->response_type & ~0x80) {
			case XCB_EXPOSE: {
				break;
			}
			case XCB_BUTTON_PRESS: {
				eb = (xcb_button_press_event_t *) ev;
				if ((blkev= ya_get_blk_from_event(eb)) && (blkev->button_cmd[eb->detail-1])) {
					ya_exec_button(blkev, eb);
				}
				break;
			}
		}
		free(ev);
	}
	//shouldn't get here
	xcb_disconnect(ya.c);
}

