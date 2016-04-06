/*
 * Yabar - A modern and lightweight status bar for X window managers.
 *
 * Copyright (c) 2016, George Badawi
 * See LICENSE for more information.
 *
 */

#include "yabar.h"

char conf_file[CFILELEN]; 
static const char * const yashell = "/bin/sh";

#ifdef YA_INTERNAL_EWMH
inline static void ya_exec_intern_ewmh_blk(ya_block_t *blk) {
	switch(blk->internal->index) {
		case YA_INT_TITLE: {
			ya_get_cur_window_title(blk);
			ya_draw_pango_text(blk);
			break;
		}
		case YA_INT_WORKSPACE: {
			uint32_t current_desktop;
			xcb_get_property_cookie_t ck = xcb_ewmh_get_current_desktop(ya.ewmh, 0);
			xcb_ewmh_get_current_desktop_reply(ya.ewmh, ck, &current_desktop, NULL);
			sprintf(blk->buf, "%u", current_desktop);
			ya_draw_pango_text(blk);
			break;
		}
	}

}
#endif //YA_INTERNAL_EWMH

static void ya_exec_redir_once(ya_block_t *blk) {
	int opipe[2];
	if(pipe(opipe)==-1) {
		fprintf(stderr, "Error opening pipe for block %s.%s. Terminating block's thread...\n", blk->bar->name, blk->name);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	if (fork() == 0) {
		dup2(opipe[1], STDOUT_FILENO);
		close(opipe[1]);
		setvbuf(stdout,NULL,_IONBF,0);

		execl(yashell, yashell, "-c", blk->cmd, (char *) NULL);
		_exit(EXIT_SUCCESS);
	}
	
	ssize_t read_ret = read(opipe[0], blk->buf, blk->bufsize);
	if (read_ret < 0) {
		fprintf(stderr, "Error with block %s: %s\n", blk->name, strerror(errno));
	} else if (read_ret > 0) {
#ifdef YA_DYN_COL
		ya_buf_color_parse(blk);
#endif
		ya_draw_pango_text(blk);
	}
}


static void ya_exec_redir_period(ya_block_t *blk) {
	int opipe[2];
	if(pipe(opipe)==-1) {
		fprintf(stderr, "Error opening pipe for block %s.%s. Terminating block's thread...\n", blk->bar->name, blk->name);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	while (1) {
		pid_t pid = fork();
		if (pid == 0) {
			dup2(opipe[1], STDOUT_FILENO);
			close(opipe[1]);
			setvbuf(stdout,NULL,_IONBF,0);

			execl(yashell, yashell, "-c", blk->cmd, (char *) NULL);
			_exit(EXIT_SUCCESS);
		}
		blk->pid = pid;
		//close(opipe[1]);
		wait(NULL);
		ssize_t read_ret = read(opipe[0], blk->buf, blk->bufsize);
		if (read_ret < 0) {
			fprintf(stderr, "Error with block %s: %s\n", blk->name, strerror(errno));
		} else if (read_ret > 0) {
			blk->buf[read_ret] = '\0';
#ifdef YA_DYN_COL
			ya_buf_color_parse(blk);
#endif
			ya_draw_pango_text(blk);
		}
		sleep(blk->sleep);
	}
}

static void ya_exec_redir_persist(ya_block_t *blk) {
	int opipe[2];
	if(pipe(opipe)==-1) {
		fprintf(stderr, "Error opening pipe for block %s.%s. Terminating block's thread...\n", blk->bar->name, blk->name);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	pid_t pid = fork();
	if (pid == 0) {
		dup2(opipe[1], STDOUT_FILENO);
		close(opipe[1]);
		setvbuf(stdout,NULL,_IONBF,0);

		execl(yashell, yashell, "-c", blk->cmd, (char *) NULL);
		_exit(EXIT_SUCCESS);
	}
	blk->pid = pid;
	close(opipe[1]);

	ssize_t read_ret;
	while (1) {
		read_ret = read(opipe[0], blk->buf, blk->bufsize);
		if(read_ret == 0) {
			break;
		} else if (read_ret < 0) {
			fprintf(stderr, "Error with block %s: %s\n", blk->name, strerror(errno));
			continue;
		} else {
			blk->buf[read_ret] = '\0';
#ifdef YA_DYN_COL
			ya_buf_color_parse(blk);
#endif
			ya_draw_pango_text(blk);
		}
	}
}

static void * ya_exec(void * _blk) {
	ya_block_t *blk = (ya_block_t *) _blk;
	if (blk->attr & BLKA_EXTERNAL) {
		if (blk->attr & BLKA_PERIODIC) {
			ya_exec_redir_period(blk);
		}
		else if (blk->attr & BLKA_PERSIST) {
			ya_exec_redir_persist(blk);
		}
		else if (blk->attr & BLKA_ONCE) {
			ya_exec_redir_once(blk);
		}
		/*Shouldn't get here*/
		else {
		}
	}
	else if (blk->attr & BLKA_INTERNAL) {
		ya_reserved_blks[blk->internal->index].function(blk);
	}
	/*Shouldn't get here*/
	else {
	}
	return NULL;
}

static void ya_cleanup_x() {
	ya_bar_t * curbar = ya.curbar;
	ya_block_t *curblk;
	for(;curbar; curbar = curbar->next_bar) {
		for (int align = 0; align < 3; align++) {
			for (curblk = curbar->curblk[align]; curblk; curblk = curblk->next_blk) {
				xcb_free_gc(ya.c, curblk->gc);
				xcb_free_pixmap(ya.c, curblk->pixmap);
			}
		}
		xcb_destroy_window(ya.c, curbar->win);
	}
#ifdef YA_INTERNAL_EWMH
	xcb_ewmh_connection_wipe(ya.ewmh);
#endif //YA_INTERNAL_EWMH
	xcb_flush(ya.c);
	xcb_disconnect(ya.c);
}

// Terminates all persistent scripts attached to blocks
static void ya_cleanup_blocks() {
	ya_bar_t * curbar = ya.curbar;
	ya_block_t *curblk;
	for(;curbar; curbar = curbar->next_bar) {
		for (int align = 0; align < 3; align++) {
			for (curblk = curbar->curblk[align]; curblk; curblk = curblk->next_blk) {
				if(curblk->pid > 0 && ((curblk->attr & BLKA_EXTERNAL))  && ((curblk->attr & BLKA_PERSIST))) {
					kill(curblk->pid, SIGTERM);
				}
			}
		}
	}
}

static void ya_sighandler(int signum) {
	ya_cleanup_x();
	ya_cleanup_blocks();
	exit(EXIT_SUCCESS);
}

static void ya_process_path(char *cpath) {
	struct stat st;
	if (stat(cpath, &st)==0) {
		strncpy(conf_file, cpath, CFILELEN);
		conf_file[CFILELEN-1]='\0';
		ya.gen_flag |= GEN_EXT_CONF;
	}
	else {
		printf("Invalid config file path. Exiting...\n");
		exit(EXIT_SUCCESS);
	}
}

static int ya_init_randr() {
	xcb_randr_get_screen_resources_current_reply_t *res_reply;
	res_reply = xcb_randr_get_screen_resources_current_reply(ya.c,
			xcb_randr_get_screen_resources_current(ya.c, ya.scr->root), NULL); 
	if (!res_reply) {
		return -1; //just report error
	}
	int mon_num = xcb_randr_get_screen_resources_current_outputs_length(res_reply);
	xcb_randr_output_t *ops = xcb_randr_get_screen_resources_current_outputs(res_reply);

	xcb_randr_get_output_info_reply_t *op_reply;
	xcb_randr_get_crtc_info_reply_t *crtc_reply;

	ya_monitor_t *tmpmon;
	char *tname;
	int tname_len;

	for (int i=0; i < mon_num; i++) {
		op_reply = xcb_randr_get_output_info_reply(ya.c,
				xcb_randr_get_output_info(ya.c, ops[i], XCB_CURRENT_TIME), NULL);
		if (op_reply->crtc == XCB_NONE)
			continue;
		crtc_reply = xcb_randr_get_crtc_info_reply(ya.c,
				xcb_randr_get_crtc_info(ya.c, op_reply->crtc, XCB_CURRENT_TIME), NULL);
		if(!crtc_reply)
			continue;
		tmpmon = calloc(1, sizeof(ya_monitor_t));
		tmpmon->pos = (xcb_rectangle_t){crtc_reply->x, 
			crtc_reply->y, crtc_reply->width, crtc_reply->height};
		tname = (char *)xcb_randr_get_output_info_name(op_reply);
		tname_len = xcb_randr_get_output_info_name_length(op_reply);
		strncpy(tmpmon->name, tname, tname_len);
		tmpmon->name[CMONLEN-1] = '\0';
		if (ya.curmon) {
			ya.curmon->next_mon = tmpmon;
			tmpmon->prev_mon = ya.curmon;
		}
		ya.curmon = tmpmon;
		//printf("%s %d %d %d %d %d\n", tmpmon->name, tname_len, tmpmon->pos.x,
		//		tmpmon->pos.y, tmpmon->pos.width, tmpmon->pos.height);
	}
	return 0;
}

void ya_process_opt(int argc, char *argv[]) {
	char opt;

	while ((opt = getopt(argc, argv, "cvh")) != (char)-1) {
		switch (opt) {
			case 'c':
				ya_process_path(*(argv+2));
				break;
			case 'h':
				printf("Usage: yabar [-c CONFIG_FILE] [-h] [-v]\n");
				exit(EXIT_SUCCESS);
			case 'v':
				printf ("yabar v%s\n", VERSION);
				exit(EXIT_SUCCESS);
			default:
				break;
		}
	}
}

void ya_init() {
	signal(SIGTERM, ya_sighandler);
	signal(SIGINT, ya_sighandler);
	signal(SIGKILL, ya_sighandler);
	signal(SIGHUP, ya_sighandler);
	ya.depth = 32;
	ya.c 	= xcb_connect(NULL, NULL);
	ya.scr 	= xcb_setup_roots_iterator(xcb_get_setup(ya.c)).data;
	ya.visualtype = ya_get_visualtype();
	if (ya.visualtype == NULL) {
		// if depth=32 not found, fallback to depth=24
		ya.depth = 24;
		ya.visualtype = ya_get_visualtype();
	}
	ya.colormap = xcb_generate_id(ya.c);
	xcb_create_colormap(ya.c, XCB_COLORMAP_ALLOC_NONE, ya.colormap, ya.scr->root, ya.visualtype->visual_id);
	const xcb_query_extension_reply_t  *ya_reply;
	ya_reply = xcb_get_extension_data(ya.c, &xcb_randr_id);
	if (ya_reply->present) {
		ya.gen_flag |= GEN_RANDR;
		ya_init_randr();
	}

#ifdef YA_INTERNAL_EWMH
	ya.ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	if (xcb_ewmh_init_atoms_replies(ya.ewmh, xcb_ewmh_init_atoms(ya.c, ya.ewmh), NULL)==0) {
		fprintf(stderr, "Cannot use EWMH\n");
		//Should exit program or not?
		//To be decided.
	}

	ya.lstwin = XCB_NONE;
	uint32_t evm = XCB_EVENT_MASK_PROPERTY_CHANGE;
	xcb_get_property_cookie_t prop_ck = xcb_ewmh_get_active_window(ya.ewmh, 0);
	xcb_ewmh_get_active_window_reply(ya.ewmh, prop_ck, &ya.curwin, NULL);
	xcb_change_window_attributes(ya.c, ya.curwin, XCB_CW_EVENT_MASK, &evm);
	xcb_change_window_attributes(ya.c, ya.scr->root, XCB_CW_EVENT_MASK, &evm);
#endif //YA_INTERNAL_EWMH

	ya_config_parse();
}

void ya_execute() {
	ya_bar_t *curbar;
	ya_block_t *curblk;
	curbar = ya.curbar;
#ifdef YA_INTERNAL_EWMH
	if(ya.ewmh_blk) {
		for(;ya.ewmh_blk->prev_ewblk; ya.ewmh_blk = ya.ewmh_blk->prev_ewblk);
		//ya_ewmh_blk *ewmh_blk = ya.ewmh_blk;
		//for(;ewmh_blk; ewmh_blk = ewmh_blk->next_ewblk)
		//	ya_exec_intern_ewmh_blk(ewmh_blk->blk);
	}
#endif //YA_INTERNAL_EWMH
	for(; curbar->prev_bar; curbar = curbar->prev_bar);
	for(; curbar; curbar = curbar->next_bar)
		xcb_map_window(ya.c, curbar->win);
	xcb_flush(ya.c);
	for(curbar = ya.curbar; curbar->prev_bar; curbar = curbar->prev_bar);
	ya.curbar = curbar;
	for(; curbar; curbar = curbar->next_bar) {
		for(int align =0; align < 3; align++){
			if ((curblk = curbar->curblk[align])) {
				for(; curblk->prev_blk; curblk = curblk->prev_blk);	
				curbar->curblk[align] = curblk;
				for(;curblk; curblk = curblk->next_blk) {
#ifdef YA_INTERNAL_EWMH
					if(!(curblk->attr & BLKA_INTERN_X_EV))
						pthread_create(&curblk->thread, NULL, ya_exec, (void *) curblk);
#else
					pthread_create(&curblk->thread, NULL, ya_exec, (void *) curblk);
#endif 
				}
			}
		}
	}
}

inline void ya_exec_button(ya_block_t * blk, xcb_button_press_event_t *eb) {
	if (fork() == 0) {
#ifdef YA_ENV_VARS
		char blkx[6], blky[6], blkw[6];
		snprintf(blkx, 6, "%d", eb->root_x - eb->event_x + blk->shift);
		if (blk->bar->position == YA_TOP)
			snprintf(blky, 6, "%d", blk->bar->height + blk->bar->vgap);
		else if (blk->bar->position == YA_BOTTOM)
			snprintf(blky, 6, "%d", ya.scr->height_in_pixels - (blk->bar->height + blk->bar->vgap));
		else {
			//TODO for right and left
		}
		snprintf(blkw, 6, "%d", blk->width);
		setenv("YABAR_BLOCK_X", blkx, 1);
		setenv("YABAR_BLOCK_Y", blky, 1);
		setenv("YABAR_BLOCK_WIDTH", blkw, 1);
#endif
		execl(yashell, yashell, "-c", blk->button_cmd[eb->detail-1], (char *) NULL);
		_exit(EXIT_SUCCESS);
	}
	else
		wait(NULL);
}

#ifdef YA_INTERNAL_EWMH
void ya_handle_prop_notify(xcb_property_notify_event_t *ep) {
	uint32_t no_ev_val = XCB_EVENT_MASK_NO_EVENT;
	uint32_t pr_ev_val = XCB_EVENT_MASK_PROPERTY_CHANGE;
	ya_ewmh_blk *ewblk;
	if(ep->atom == ya.ewmh->_NET_ACTIVE_WINDOW) {
		xcb_get_property_cookie_t prop_ck = xcb_ewmh_get_active_window(ya.ewmh, 0);
		xcb_ewmh_get_active_window_reply(ya.ewmh, prop_ck, &ya.curwin, NULL);
		if (ya.curwin != ya.lstwin) {
			xcb_change_window_attributes(ya.c, ya.lstwin, XCB_CW_EVENT_MASK, &no_ev_val);
			xcb_change_window_attributes(ya.c, ya.curwin, XCB_CW_EVENT_MASK, &pr_ev_val);
		}
		else if(ya.curwin==XCB_NONE && ya.lstwin==XCB_NONE) {
			//Don't exit, used when switch between two empty workspaces
		}
		else {
			return;
		}
	}
	else if ((ep->atom == ya.ewmh->_NET_WM_NAME) || (ep->atom == ya.ewmh->_NET_WM_VISIBLE_NAME)) {
	}
	else {
		return;
	}
	for(ewblk = ya.ewmh_blk; ewblk; ewblk=ewblk->next_ewblk) {
		ya_exec_intern_ewmh_blk(ewblk->blk);
	}
	ya.lstwin = ya.curwin;
}
#endif //YA_INTERNAL_EWMH
