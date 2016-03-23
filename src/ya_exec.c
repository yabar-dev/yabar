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

static void ya_exec_redir_once(ya_block_t *blk) {
	int opipe[2];
	pipe(opipe);
	if (fork() == 0) {
		dup2(opipe[1], STDOUT_FILENO);
		close(opipe[1]);
		setvbuf(stdout,NULL,_IONBF,0);

		execl(yashell, yashell, "-c", blk->cmd, (char *) NULL);
		_exit(EXIT_SUCCESS);
	}
	else {
		wait(NULL);
		if (read(opipe[0], blk->buf, BUFSIZE) != 0) {
			ya_draw_pango_text(blk);
		}
	}
}


static void ya_exec_redir_period(ya_block_t *blk) {
	int opipe[2];
	pipe(opipe);
	while (1) {
		int pid = fork();
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
		if (read(opipe[0], blk->buf, BUFSIZE) != 0) {
			ya_draw_pango_text(blk);
			memset(blk->buf, '\0', BUFSIZE);
		}
		sleep(blk->sleep);
	}
}

static void ya_exec_redir_persist(ya_block_t *blk) {
	int opipe[2];
	pipe(opipe);
	int pid = fork();
	if (pid == 0) {
		dup2(opipe[1], STDOUT_FILENO);
		close(opipe[1]);
		setvbuf(stdout,NULL,_IONBF,0);

		execl(yashell, yashell, "-c", blk->cmd, (char *) NULL);
		_exit(EXIT_SUCCESS);
	}
	blk->pid = pid;
	close(opipe[1]);
	while (read(opipe[0], blk->buf, BUFSIZE) != 0) {
		ya_draw_pango_text(blk);
		memset(blk->buf, '\0', BUFSIZE);
	}
}

void * ya_exec_l (void * _blk) {
	ya_block_t *blk = (ya_block_t *) _blk;
	if (blk->type & BLKA_EXTERNAL) {
		if (blk->type & BLKA_PERIODIC) {
			ya_exec_redir_period(blk);
		}
		else if (blk->type & BLKA_PERSIST) {
			ya_exec_redir_persist(blk);
		}
		else if (blk->type & BLKA_ONCE) {
			ya_exec_redir_once(blk);
		}
		/*Shouldn't get here*/
		else {
		}
	}
	else if (blk->type & BLKA_INTERNAL) {
		/*TODO*/
	}
	/*Shouldn't get here*/
	else {
	}
	return NULL;
}


void ya_sighandler(int signum) {
	printf("\n\nExiting...\n");
	ya_cleanup_x();
	ya_cleanup_blocks();
	exit(EXIT_SUCCESS);
}

void ya_cleanup_x() {
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
	xcb_flush(ya.c);
	xcb_disconnect(ya.c);
}

// Terminates all persistent scripts attached to blocks
void ya_cleanup_blocks() {
	ya_bar_t * curbar = ya.curbar;
	ya_block_t *curblk;
	for(;curbar; curbar = curbar->next_bar) {
		for (int align = 0; align < 3; align++) {
			for (curblk = curbar->curblk[align]; curblk; curblk = curblk->next_blk) {
				if(curblk->pid > 0 && ((curblk->type & BLKA_EXTERNAL))  && ((curblk->type & BLKA_PERSIST))) {
					kill(curblk->pid, SIGTERM);
				}
			}
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
#ifdef YABAR_RANDR
	const xcb_query_extension_reply_t  *ya_reply;
	ya_reply = xcb_get_extension_data(ya.c, &xcb_randr_id);
	if (ya_reply->present) {
		ya.gen_flag |= GEN_RANDR;
		ya_init_randr();
	}
#endif //YABAR_RANDR
	ya_config_parse();
}
void ya_execute() {
	ya_bar_t *curbar;
	ya_block_t *curblk;
	curbar = ya.curbar;
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
					pthread_create(&curblk->thread, NULL, ya_exec_l, (void *) curblk);
				}
			}
		}
	}
}

inline void ya_exec_cmd(ya_block_t * blk, xcb_button_press_event_t *eb) {
	if (fork() == 0) {
		char blkx[6], blky[6], blkw[6];
		snprintf(blkx, 6, "%d", eb->root_x - eb->event_x + blk->xpos);
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
		execl(yashell, yashell, "-c", blk->button_cmd[eb->detail-1], (char *) NULL);
		_exit(EXIT_SUCCESS);
	}
	else
		wait(NULL);
}


void ya_process_path(char *cpath) {
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
				printf ("Yabar version %s\n", VERSION);
				exit(EXIT_SUCCESS);
			default:
				break;
		}
	}
}

#ifdef YABAR_RANDR
int ya_init_randr() {
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

ya_monitor_t * ya_get_monitor_from_name(const char *name) {
	ya_monitor_t *mon = ya.curmon;
	if (mon == NULL)
		return NULL;
	for(mon=ya.curmon; mon->prev_mon; mon = mon->prev_mon);
	for(; mon; mon = mon->next_mon){
		if (strcmp(mon->name, name)==0)
			return mon;
	}
	return NULL;
}

#endif //YABAR_RANDR
