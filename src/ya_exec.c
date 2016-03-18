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
		exit(EXIT_SUCCESS);
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
		if (fork() == 0) {
			dup2(opipe[1], STDOUT_FILENO);
			close(opipe[1]);
			setvbuf(stdout,NULL,_IONBF,0);

			execl(yashell, yashell, "-c", blk->cmd, (char *) NULL);
			exit(EXIT_SUCCESS);
		}
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
	if (fork() == 0) {
		dup2(opipe[1], STDOUT_FILENO);
		close(opipe[1]);
		setvbuf(stdout,NULL,_IONBF,0);

		execl(yashell, yashell, "-c", blk->cmd, (char *) NULL);
		_exit(EXIT_SUCCESS);
	}
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

void ya_init() {
	signal(SIGTERM, ya_sighandler);
	signal(SIGINT, ya_sighandler);
	signal(SIGKILL, ya_sighandler);
	signal(SIGHUP, ya_sighandler);
	ya.c 	= xcb_connect(NULL, NULL);
	ya.scr 	= xcb_setup_roots_iterator(xcb_get_setup(ya.c)).data;
	ya.visualtype = ya_get_visualtype();
	ya.depth = 32;
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


void ya_setup_bar(config_setting_t * set) {
	int retcnf, retint;
	const char *retstr;
	ya_bar_t *bar = calloc(1, sizeof(ya_bar_t));
	if (ya.curbar) {
		bar->prev_bar = ya.curbar;
		ya.curbar->next_bar = bar;
	}
	ya.curbar = bar;
	retcnf = config_setting_lookup_string(set, "font", &retstr);
	if(retcnf == CONFIG_FALSE) {
		bar->desc = pango_font_description_from_string(YA_DEF_FONT);
	}
	else {
		bar->desc = pango_font_description_from_string(retstr);
	}

	retcnf = config_setting_lookup_string(set, "position", &retstr);
	if(retcnf == CONFIG_FALSE) {
		bar->position = YA_TOP;
	}
	else {
		if(strcmp(retstr, "top")==0) {
			bar->position = YA_TOP;
		}
		else if(strcmp(retstr, "bottom")==0) {
			bar->position = YA_BOTTOM;
		}
		else if(strcmp(retstr, "left")==0) {
			bar->position = YA_LEFT;
		}
		else if(strcmp(retstr, "right")==0) {
			bar->position = YA_RIGHT;
		}
		else{
			bar->position = YA_TOP;
		}
	}
#ifdef YABAR_RANDR
	retcnf = config_setting_lookup_string(set, "monitor", &retstr);
	if(retcnf == CONFIG_FALSE) {
		//If not explicitly specified, fall back to the first active monitor.
		if((ya.gen_flag & GEN_RANDR)) {
			for(bar->mon= ya.curmon; bar->mon->prev_mon; bar->mon = bar->mon->prev_mon);
		}
	}
	else {
		if((ya.gen_flag & GEN_RANDR)) {
			bar->mon = ya_get_monitor_from_name(retstr);
			//If null, fall back to the first active monitor
			if(bar->mon == NULL)
				for(bar->mon= ya.curmon; bar->mon->prev_mon; bar->mon = bar->mon->prev_mon);
		}
	}
#endif //YABAR_RANDR

	retcnf = config_setting_lookup_int(set, "gap-horizontal", &retint);
	if(retcnf == CONFIG_FALSE) {
		bar->hgap = 0;
	}
	else {
		bar->hgap = retint;
	}

	retcnf = config_setting_lookup_int(set, "gap-vertical", &retint);
	if(retcnf == CONFIG_FALSE) {
		bar->vgap = 0;
	}
	else {
		bar->vgap = retint;
	}
	retcnf = config_setting_lookup_int(set, "height", &retint);
	if(retcnf == CONFIG_FALSE) {
		bar->height = 20;
	}
	else {
		bar->height = retint;
	}
	retcnf = config_setting_lookup_int(set, "width", &retint);
	if(retcnf == CONFIG_FALSE) {
#ifdef YABAR_RANDR
		if(bar->mon) {
			bar->width = bar->mon->pos.width - 2*(bar->hgap);
		}
		else {
			bar->width = ya.scr->width_in_pixels - 2*(bar->hgap);
		}
#else
		bar->width = ya.scr->width_in_pixels - 2*(bar->hgap);
#endif //YABAR_RANDR
	}
	else {
		bar->width = retint;
	}
	retcnf = config_setting_lookup_int(set, "underline-size", &retint);
	if(retcnf == CONFIG_TRUE) {
		bar->ulsize = retint;
	}
	retcnf = config_setting_lookup_int(set, "overline-size", &retint);
	if(retcnf == CONFIG_TRUE) {
		bar->olsize = retint;
	}
	retcnf = config_setting_lookup_int(set, "background-color-argb", &retint);
	if(retcnf == CONFIG_FALSE) {
		bar->bgcolor = 0xff1d1d1d;
	}
	else {
		bar->bgcolor = retint;
	}
	retcnf = config_setting_lookup_int(set, "background-color-rgb", &retint);
	if(retcnf == CONFIG_TRUE) {
		bar->bgcolor = retint | 0xff000000;
	}
	retcnf = config_setting_lookup_int(set, "slack-size", &retint);
	if(retcnf == CONFIG_TRUE) {
		bar->slack = retint;
	}
	ya_create_bar(bar);
}

#define YA_RESERVED_NUM 1
char *ya_reserved_blocks[YA_RESERVED_NUM]={"ya_time"};

void ya_setup_block(config_setting_t * set, uint32_t type_init) {
	struct ya_block * blk = calloc(1,sizeof(ya_block_t));
	int retcnf, retint;
	const char *retstr;
	blk->type = type_init;

	retcnf = config_setting_lookup_string(set, "type", &retstr);
	if(retcnf == CONFIG_FALSE) {
		fprintf(stderr, "No type is defined for block: %s. Skipping this block...\n", config_setting_name(set));
		free(blk);
		return;
	}
	else {
		if(strcmp(retstr, "persist")==0) {
			blk->type |= BLKA_PERSIST;
		}
		else if(strcmp(retstr, "once")==0) {
			blk->type |= BLKA_ONCE;
		}
		else if(strcmp(retstr, "periodic")==0) {
			blk->type |= BLKA_PERIODIC;
			retcnf = config_setting_lookup_int(set, "interval", &retint);
			if(retcnf == CONFIG_FALSE) {
				blk->sleep = 5;
			}
			else {
				blk->sleep = retint;
			}

		}
	}


	retcnf = config_setting_lookup_string(set, "exec", &retstr);
	if(retcnf == CONFIG_FALSE) {
		fprintf(stderr, "No exec is defined for block: %s. Skipping this block...\n", config_setting_name(set));
		free(blk);
		return;
	}
	else {
		int len = strlen(retstr);
		if (len) {
			blk->cmd = malloc(len);
			strcpy(blk->cmd, retstr);
		}
		else {
			free(blk);
			return;
		}
	}
	retcnf = config_setting_lookup_string(set, "command-button1", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->button_cmd[0] = malloc(strlen(retstr));
		strcpy(blk->button_cmd[0], retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button2", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->button_cmd[1] = malloc(strlen(retstr));
		strcpy(blk->button_cmd[1], retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button3", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->button_cmd[2] = malloc(strlen(retstr));
		strcpy(blk->button_cmd[2], retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button4", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->button_cmd[3] = malloc(strlen(retstr));
		strcpy(blk->button_cmd[3], retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button5", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->button_cmd[4] = malloc(strlen(retstr));
		strcpy(blk->button_cmd[4], retstr);
	}
	retcnf = config_setting_lookup_string(set, "align", &retstr);
	if(retcnf == CONFIG_FALSE) {
		blk->align = A_CENTER;
	}
	else {
		if(strcmp(retstr, "left")==0) {
			blk->align =	A_LEFT;
		}
		else if(strcmp(retstr, "center")==0) {
			blk->align =	A_CENTER;
		}
		else if(strcmp(retstr, "right")==0) {
			blk->align =	A_RIGHT;
		}
		else {
			blk->align = A_CENTER;
		}
	}
	retcnf = config_setting_lookup_int(set, "fixed-size", &retint);
	if(retcnf == CONFIG_FALSE) {
		blk->width = 80;
	}
	else {
		blk->width = retint;
	}
	retcnf = config_setting_lookup_bool(set, "pango-markup", &retint);
	if((retcnf == CONFIG_TRUE) && retint) {
		blk->type |= BLKA_MARKUP_PANGO;
	}

	retcnf = config_setting_lookup_int(set, "background-color-argb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->bgcolor = (uint32_t) retint;
		blk->type |= BLKA_BGCOLOR;
	}
	retcnf = config_setting_lookup_int(set, "background-color-rgb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->bgcolor = retint | 0xff000000;
		blk->type |= BLKA_BGCOLOR;
	}
	retcnf = config_setting_lookup_int(set, "foreground-color-argb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->fgcolor = retint;
		blk->type |= BLKA_FGCOLOR;
	}
	retcnf = config_setting_lookup_int(set, "foreground-color-rgb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->fgcolor = retint | 0xff000000;
		blk->type |= BLKA_FGCOLOR;
	}
	else {
		blk->fgcolor = 0xffffffff;
	}
	retcnf = config_setting_lookup_int(set, "underline-color-argb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->ulcolor = retint;
		blk->type |= BLKA_UNDERLINE;
	}
	retcnf = config_setting_lookup_int(set, "underline-color-rgb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->ulcolor = retint | 0xff000000;
		blk->type |= BLKA_UNDERLINE;
	}
	retcnf = config_setting_lookup_int(set, "overline-color-argb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->olcolor = retint;
		blk->type |= BLKA_OVERLINE;
	}
	retcnf = config_setting_lookup_int(set, "overline-color-rgb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->olcolor = retint | 0xff000000;
		blk->type |= BLKA_OVERLINE;
	}

	ya_create_block(blk);
}

void ya_config_parse() {
	int ret;
	const char * const envhome = getenv("HOME");
	if ((ya.gen_flag & GEN_EXT_CONF) == 0)
	    snprintf(conf_file, 128, "%s/.config/yabar/yabar.conf", envhome);
	config_t ya_conf;
	config_init(&ya_conf);
	config_set_auto_convert(&ya_conf, CONFIG_TRUE);
	ret=config_read_file(&ya_conf, conf_file);
	if (ret == CONFIG_FALSE) {
		printf("Error in the config file at line %d : %s\nExiting...", config_error_line(&ya_conf), config_error_text(&ya_conf));
		config_destroy(&ya_conf);
		exit(EXIT_SUCCESS);
	}
	char *barstr, *blkstr;
	config_setting_t *barlist_set, *blklist_set;
	config_setting_t *curbar_set, *curblk_set;
	barlist_set = config_lookup(&ya_conf, "bar-list");
	ya.barnum = config_setting_length(barlist_set);
	int blknum;
	for(int i=0; i < ya.barnum; i++) {
		barstr = (char *)config_setting_get_string_elem(barlist_set, i);
		curbar_set = config_lookup(&ya_conf, barstr);
		ya_setup_bar(curbar_set);

		blklist_set = config_setting_lookup(curbar_set, "block-list");
		blknum = config_setting_length(blklist_set);

		for (int i=0; i < blknum; i++) {
			uint32_t type_init;
			blkstr = (char *)config_setting_get_string_elem(blklist_set, i);
			curblk_set = config_setting_lookup(curbar_set, blkstr);
			for(int i=0; i < YA_RESERVED_NUM; i++) {
				if(strcmp(blkstr, ya_reserved_blocks[i])==0) {
					type_init = BLKA_INTERNAL;
					break;
				}
			}
			if(type_init == 0)
				type_init = BLKA_EXTERNAL;
			ya_setup_block(curblk_set, type_init);
		}
	}
	config_destroy(&ya_conf);
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
		ya.gen_flag |= GEN_EXT_CONF;
	}
	else {
		printf("Invalid config file path.\nExiting...\n");
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
		strncpy(tmpmon->name, tname, CMONLEN);
		if (ya.curmon) {
			ya.curmon->next_mon = tmpmon;
			tmpmon->prev_mon = ya.curmon;
		}
		ya.curmon = tmpmon;
		//printf("%s %d %d %d %d\n", tmpmon->name, tmpmon->pos.x,
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
