/*
 * Yabar - A modern and lightweight status bar for X window managers.
 *
 * Copyright (c) 2016, George Badawi
 * See LICENSE for more information.
 *
 */

#include "yabar.h"


static ya_monitor_t * ya_get_monitor_from_name(const char *name) {
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


static ya_bar_t * ya_get_bar_from_name(const char *name) {
	ya_bar_t *curbar;
	for(curbar = ya.curbar; curbar->prev_bar; curbar = curbar->prev_bar);
	for(;curbar; curbar = curbar->next_bar) {
		if(strcmp(curbar->name, name)==0)
			return curbar;
	}
	return NULL;
}

static ya_block_t * ya_get_blk_from_name (const char *name, ya_bar_t * curbar) {
	ya_block_t *curblk;
	for(int align =0; align < 3; align++){
		if ((curblk = curbar->curblk[align])) {
			for(; curblk->prev_blk; curblk = curblk->prev_blk);
			for(;curblk; curblk = curblk->next_blk) {
				if(strcmp(curblk->name, name)==0)
					return curblk;
			}
		}
	}
	return NULL;
}


static void ya_copy_bar_members(ya_bar_t *dstb, ya_bar_t *srcb) {
	for(int i=0; i<5; i++)
		dstb->button_cmd[i] = srcb->button_cmd[i];
	dstb->hgap = srcb->hgap;
	dstb->vgap = srcb->vgap;
	dstb->bgcolor = srcb->bgcolor;
	dstb->width = srcb->width;
	dstb->height = srcb->height;
	dstb->position = srcb->position;
	dstb->desc = srcb->desc;
	dstb->ulsize = srcb->ulsize;
	dstb->olsize = srcb->olsize;
	dstb->slack = srcb->slack;
	dstb->brcolor = srcb->brcolor;
	dstb->brsize = srcb->brsize;
	dstb->mon = srcb->mon;
}

static void ya_copy_blk_members(ya_block_t *dstb, ya_block_t *srcb) {
	dstb->cmd = srcb->cmd;
	for(int i=0; i<5; i++)
		dstb->button_cmd[i] = srcb->button_cmd[i];
	dstb->sleep = srcb->sleep;
	dstb->attr = srcb->attr;
	dstb->align = srcb->align;
	dstb->justify = srcb->justify;
	dstb->width = srcb->width;
	dstb->bgcolor = srcb->bgcolor;
	dstb->fgcolor = srcb->fgcolor;
	dstb->ulcolor = srcb->ulcolor;
	dstb->olcolor = srcb->olcolor;
	dstb->bufsize = srcb->bufsize;
	if(srcb->attr & BLKA_INTERNAL) {
		dstb->internal = calloc(1, sizeof(blk_intern_t));
		dstb->internal->prefix = srcb->internal->prefix;
		dstb->internal->suffix = srcb->internal->suffix;
		for(int i=0; i < 3; i++)
			dstb->internal->option[i] = srcb->internal->option[i];
		dstb->internal->index = srcb->internal->index;
	}
}

static int ya_inherit_bar(ya_bar_t *dstb, const char *srcname, bool inherit_all, ya_bar_t **inherit_bar) {
	ya_bar_t *srcb = ya_get_bar_from_name(srcname);
	if(srcb == NULL) {
		fprintf(stderr, "The bar (%s) cannot find a bar named (%s) to inherit.\n", dstb->name, srcname);
		return -1;
	}
	ya_copy_bar_members(dstb, srcb);
	if(inherit_all) {
		dstb->attr = BARA_INHERIT_ALL;
		*inherit_bar = srcb;
	}
	dstb->attr |= BARA_INHERIT;
	return 0;
}

static int ya_inherit_blk(ya_block_t *dstb, const char *name) {
	char *per = "Please enter a valid string.\n";
	char *barname, *blkname;
	int barnamelen=0, blknamelen=0;
	ya_bar_t *src_bar;
	ya_block_t *srcb;
	int nlen = strlen(name);
	if(nlen < 1) {
		fprintf(stderr, "No inherit entry. ");
		fprintf(stderr, per);
		return -1;
	}
	for(int i = 0; i < nlen; i++) {
		if((name[i]=='.') && (i > 0) && (i < (nlen-1))) {
			barnamelen = i+1;
			break;
		}
	}
	if(barnamelen == 0) {
		fprintf(stderr, per);
		return -1;
	}

	blknamelen = nlen - barnamelen + 1;

	barname = calloc(1, barnamelen);
	blkname = calloc(1, blknamelen);

	strncpy(barname, name, barnamelen-1);
	name += barnamelen;
	strncpy(blkname, name, blknamelen);

	src_bar = ya_get_bar_from_name(barname);
	if(src_bar == NULL) {
		fprintf(stderr, "Block (%s.%s) cannot process inheritance. No bar has the name (%s).\n", dstb->bar->name, dstb->name, barname);
		free(barname);
		free(blkname);
		return -1;
	}
	srcb = ya_get_blk_from_name(blkname, src_bar);
	if(srcb == NULL) {
		fprintf(stderr, "Block (%s.%s) cannot process inheritance. No block has the name (%s).\n", dstb->bar->name, dstb->name, blkname);
		free(barname);
		free(blkname);
		return -1;
	}

	ya_copy_blk_members(dstb, srcb);
	dstb->attr |= BLKA_INHERIT;
	free(barname);
	free(blkname);
	return 0;
}

static void ya_setup_bar(config_setting_t * set) {
	int retcnf, retint;
	const char *retstr;
	ya_bar_t *inherit_bar;
	ya_bar_t *bar = calloc(1, sizeof(ya_bar_t));
	if (ya.curbar) {
		bar->prev_bar = ya.curbar;
		ya.curbar->next_bar = bar;
	}
	ya.curbar = bar;
	bar->name = strdup(config_setting_name(set));
	retcnf = config_setting_lookup_string(set, "inherit", &retstr);
	if(retcnf == CONFIG_TRUE) {
		if (ya_inherit_bar(bar, retstr, false, NULL) == -1) {
			fprintf(stderr, "Skipping bar (%s).\n", bar->name);
			free(bar->name);
			free(bar);
			return;
		}
	}
	retcnf = config_setting_lookup_string(set, "inherit-all", &retstr);
	if(retcnf == CONFIG_TRUE) {
		if (ya_inherit_bar(bar, retstr, true, &inherit_bar) == -1) {
			fprintf(stderr, "Skipping bar (%s).\n", bar->name);
			free(bar->name);
			free(bar);
			return;
		}
	}
	retcnf = config_setting_lookup_string(set, "font", &retstr);
	if(retcnf == CONFIG_FALSE) {
		if(NOT_INHERIT_BAR(bar))
			bar->desc = pango_font_description_from_string(YA_DEF_FONT);
	}
	else {
		bar->desc = pango_font_description_from_string(retstr);
	}

	retcnf = config_setting_lookup_string(set, "position", &retstr);
	if(retcnf == CONFIG_FALSE) {
		if(NOT_INHERIT_BAR(bar))
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
	retcnf = config_setting_lookup_string(set, "monitor", &retstr);
	if(retcnf == CONFIG_FALSE) {
		//If not explicitly specified, fall back to the first active monitor.
		if((ya.gen_flag & GEN_RANDR) && NOT_INHERIT_BAR(bar)) {
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

	bool is_gap_horizontal_defined = false;
	retcnf = config_setting_lookup_int(set, "gap-horizontal", &retint);
	if(retcnf == CONFIG_TRUE) {
		bar->hgap = retint;
		is_gap_horizontal_defined = true;
	}

	retcnf = config_setting_lookup_int(set, "gap-vertical", &retint);
	if(retcnf == CONFIG_TRUE) {
		bar->vgap = retint;
	}
	retcnf = config_setting_lookup_int(set, "height", &retint);
	if(retcnf == CONFIG_FALSE) {
		if (NOT_INHERIT_BAR(bar))
			bar->height = 20;
	}
	else {
		bar->height = retint;
	}
	retcnf = config_setting_lookup_int(set, "width", &retint);
	if(retcnf == CONFIG_FALSE) {
		if(NOT_INHERIT_BAR(bar)) {
			if(bar->mon) {
				bar->width = bar->mon->pos.width - 2*(bar->hgap);
			}
			else {
				bar->width = ya.scr->width_in_pixels - 2*(bar->hgap);
			}
		}
	}
	else {
		bar->width = retint;
		if(!is_gap_horizontal_defined) {
			if((ya.gen_flag & GEN_RANDR)) {
				bar->hgap = (bar->mon->pos.width - bar->width) /2;
			}
			else {
				bar->hgap = (ya.scr->width_in_pixels - bar->width) /2;
			}
		}
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
		if(NOT_INHERIT_BAR(bar))
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
	retcnf = config_setting_lookup_int(set, "border-size", &retint);
	if(retcnf == CONFIG_TRUE) {
		bar->brsize = retint;
		retcnf = config_setting_lookup_int(set, "border-color-rgb", &retint);
		if(retcnf == CONFIG_TRUE) {
			bar->brcolor = retint;
		}
	}

	retcnf = config_setting_lookup_string(set, "command-button1", &retstr);
	if(retcnf == CONFIG_TRUE) {
		bar->button_cmd[0] = strdup(retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button2", &retstr);
	if(retcnf == CONFIG_TRUE) {
		bar->button_cmd[1] = strdup(retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button3", &retstr);
	if(retcnf == CONFIG_TRUE) {
		bar->button_cmd[2] = strdup(retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button4", &retstr);
	if(retcnf == CONFIG_TRUE) {
		bar->button_cmd[3] = strdup(retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button5", &retstr);
	if(retcnf == CONFIG_TRUE) {
		bar->button_cmd[4] = strdup(retstr);
	}

	ya_create_bar(bar);
	if(bar->attr & BARA_INHERIT_ALL) {
		ya_block_t * dstblk, *srcblk;
		for(int i=0; i <3; i++) {
			if((srcblk = inherit_bar->curblk[i])) {
				for(;srcblk->prev_blk; srcblk = srcblk->prev_blk);
				for(;srcblk; srcblk = srcblk->next_blk) {
					dstblk = calloc(1, sizeof(ya_block_t));
					ya_copy_blk_members(dstblk, srcblk);
					dstblk->buf = calloc(1, dstblk->bufsize);
					ya_create_block(dstblk);
				}
			}
		}
	}
}

#ifdef YA_INTERNAL_EWMH
inline static void ya_setup_ewmh_intern_blk(ya_block_t *blk) {
	ya_ewmh_blk *cur=NULL;
	switch(blk->internal->index) {
		case YA_INT_WORKSPACE:
		case YA_INT_TITLE:
			blk->attr |= BLKA_INTERN_X_EV;
			break;
	}
	cur = calloc(1, sizeof(ya_ewmh_blk));
	cur->blk = blk;
	if (ya.ewmh_blk) {
		ya.ewmh_blk->next_ewblk = cur;
		cur->prev_ewblk = ya.ewmh_blk;
	}
	ya.ewmh_blk = cur;
}
#endif //YA_INTERNAL_EWMH

inline static void ya_check_blk_internal(ya_block_t *blk, config_setting_t *set, const char *strexec) {
	int retint;
	const char *retstr;
	for (int i=0; i < YA_INTERNAL_LEN; i++) {
		if(strcmp(strexec, ya_reserved_blks[i].name)==0) {
			blk->internal = calloc(1, sizeof(blk_intern_t));
			blk->attr |= BLKA_INTERNAL;
			blk->internal->index = i;
#ifdef YA_INTERNAL_EWMH
			ya_setup_ewmh_intern_blk(blk);
#endif //YA_INTERNAL_EWMH
			if(config_setting_lookup_string(set, "internal-prefix", &retstr) == CONFIG_TRUE)
				blk->internal->prefix = strdup(retstr);
			if(config_setting_lookup_string(set, "internal-suffix", &retstr) == CONFIG_TRUE)
				blk->internal->suffix = strdup(retstr);
			if(config_setting_lookup_string(set, "internal-option1", &retstr) == CONFIG_TRUE)
				blk->internal->option[0] = strdup(retstr);
			if(config_setting_lookup_string(set, "internal-option2", &retstr) == CONFIG_TRUE)
				blk->internal->option[1] = strdup(retstr);
			if(config_setting_lookup_string(set, "internal-option3", &retstr) == CONFIG_TRUE)
				blk->internal->option[2] = strdup(retstr);
			if(config_setting_lookup_bool(set, "internal-spacing", &retint) == CONFIG_TRUE)
				blk->internal->spacing = retint;
			else
				blk->internal->spacing = false;
		}
	}
	//check if the for-loop never found a matching internal block
	if ((blk->attr & BLKA_INTERNAL)==0)
		blk->attr |= BLKA_EXTERNAL;
}

static void ya_setup_block(config_setting_t * set) {
	struct ya_block * blk = calloc(1,sizeof(ya_block_t));
	int retcnf, retint;
	const char *retstr;

	blk->pid = -1;
	blk->name = strdup(config_setting_name(set));

	retcnf = config_setting_lookup_string(set, "inherit", &retstr);
	if(retcnf == CONFIG_TRUE) {
		if (ya_inherit_blk(blk, retstr) == -1) {
			fprintf(stderr, "Skipping block (%s.%s)\n", blk->bar->name, blk->name);
			free(blk->name);
			free(blk);
			return;
		}
	}

	retcnf = config_setting_lookup_string(set, "exec", &retstr);
	if(retcnf == CONFIG_FALSE) {
		if(NOT_INHERIT_BLK(blk)) {
			fprintf(stderr, "No exec is defined for block: %s.%s. Skipping this block...\n", blk->bar->name, blk->name);
			free(blk->name);
			free(blk);
			return;
		}
	}
	else if(strlen(retstr) < 1) {
		fprintf(stderr, "exec entry is empty for block: %s.%s. Skipping this block...\n", blk->bar->name, blk->name);
		free(blk->name);
		free(blk);
		return;

	}
	else {
#ifdef YA_INTERNAL
		//check if internal found, otherwise set external
		//Fast check: if first char is not Y, don't even call ya_check_blk_internal()
		if(retstr[0] != 'Y')
			blk->attr |= BLKA_EXTERNAL;
		else
			ya_check_blk_internal(blk, set, retstr);
#else
		//just set it external
		blk->attr |= BLKA_EXTERNAL;
#endif //YA_INTERNAL
		if (blk->attr & BLKA_EXTERNAL) {
				blk->cmd = strdup(retstr);
			}
	}
	if (blk->attr & BLKA_INTERNAL)
		goto skip_type;
	retcnf = config_setting_lookup_string(set, "type", &retstr);
	if(retcnf == CONFIG_FALSE) {
		if(NOT_INHERIT_BLK(blk)) {
			fprintf(stderr, "No type is defined for block: %s. Skipping this block...\n", config_setting_name(set));
			free(blk->name);
			free(blk);
			return;
		}
	}
	else {
		if(strcmp(retstr, "persist")==0) {
			blk->attr |= BLKA_PERSIST;
		}
		else if(strcmp(retstr, "once")==0) {
			blk->attr |= BLKA_ONCE;
		}
		else if(strcmp(retstr, "periodic")==0) {
			blk->attr |= BLKA_PERIODIC;
		}
		else {
			//TODO handle
		}
	}
skip_type:
	retcnf = config_setting_lookup_int(set, "interval", &retint);
	if(retcnf == CONFIG_FALSE) {
		if(NOT_INHERIT_BLK(blk))
			blk->sleep = 5;
	}
	else {
		blk->sleep = retint;
	}

	retcnf = config_setting_lookup_string(set, "command-button1", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->button_cmd[0] = strdup(retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button2", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->button_cmd[1] = strdup(retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button3", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->button_cmd[2] = strdup(retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button4", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->button_cmd[3] = strdup(retstr);
	}
	retcnf = config_setting_lookup_string(set, "command-button5", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->button_cmd[4] = strdup(retstr);
	}
#ifdef YA_ICON
	retcnf = config_setting_lookup_string(set, "image", &retstr);
	if(retcnf == CONFIG_TRUE) {
		blk->icon_path = strdup(retstr);
		blk->attr |= BLKA_ICON;
		double retflt;
		if(config_setting_lookup_float(set, "image-scale-width", &retflt)==CONFIG_TRUE) {
			blk->ic_scale_w = retflt;
		}
		else {
			blk->ic_scale_w = 1.0;
		}
		if(config_setting_lookup_float(set, "image-scale-height", &retflt)==CONFIG_TRUE) {
			blk->ic_scale_h = retflt;
		}
		else {
			blk->ic_scale_h = 1.0;
		}
		if(config_setting_lookup_int(set, "image-shift-x", &retint)==CONFIG_TRUE) {
			blk->ic_x = retint;
		}
		if(config_setting_lookup_int(set, "image-shift-y", &retint)==CONFIG_TRUE) {
			blk->ic_y = retint;
		}
	}
#endif //YA_ICON
	retcnf = config_setting_lookup_string(set, "align", &retstr);
	if(retcnf == CONFIG_FALSE) {
		if(NOT_INHERIT_BLK(blk))
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
		if(NOT_INHERIT_BLK(blk))
			blk->width = 80;
	}
	else {
		blk->width = retint;
	}
	retcnf = config_setting_lookup_bool(set, "pango-markup", &retint);
	if((retcnf == CONFIG_TRUE) && retint) {
		blk->attr |= BLKA_MARKUP_PANGO;
	}

	retcnf = config_setting_lookup_int(set, "background-color-argb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->bgcolor = (uint32_t) retint;
		blk->attr |= BLKA_BGCOLOR;
	}
	retcnf = config_setting_lookup_int(set, "background-color-rgb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->bgcolor = retint | 0xff000000;
		blk->attr |= BLKA_BGCOLOR;
	}
	retcnf = config_setting_lookup_int(set, "foreground-color-argb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->fgcolor = retint;
		blk->attr |= BLKA_FGCOLOR;
	}
	retcnf = config_setting_lookup_int(set, "foreground-color-rgb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->fgcolor = retint | 0xff000000;
		blk->attr |= BLKA_FGCOLOR;
	}
	else {
		blk->fgcolor = 0xffffffff;
	}
	retcnf = config_setting_lookup_int(set, "underline-color-argb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->ulcolor = retint;
		blk->attr |= BLKA_UNDERLINE;
	}
	retcnf = config_setting_lookup_int(set, "underline-color-rgb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->ulcolor = retint | 0xff000000;
		blk->attr |= BLKA_UNDERLINE;
	}
	retcnf = config_setting_lookup_int(set, "overline-color-argb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->olcolor = retint;
		blk->attr |= BLKA_OVERLINE;
	}
	retcnf = config_setting_lookup_int(set, "overline-color-rgb", &retint);
	if(retcnf == CONFIG_TRUE) {
		blk->olcolor = retint | 0xff000000;
		blk->attr |= BLKA_OVERLINE;
	}
	retcnf = config_setting_lookup_string(set, "justify", &retstr);
	if(retcnf == CONFIG_TRUE) {
		if(strcmp(retstr, "left")==0) {
			blk->justify =	PANGO_ALIGN_LEFT;
		}
		else if(strcmp(retstr, "center")==0) {
			blk->justify =	PANGO_ALIGN_CENTER;
		}
		else if(strcmp(retstr, "right")==0) {
			blk->justify =	PANGO_ALIGN_RIGHT;
		}
		else {
			blk->justify = PANGO_ALIGN_CENTER;
		}
	}
	else {
		if(NOT_INHERIT_BLK(blk))
			blk->justify = PANGO_ALIGN_CENTER;
	}


	if(blk->attr & BLKA_EXTERNAL) {
		if(blk->attr & BLKA_MARKUP_PANGO)
			blk->bufsize = BUFSIZE_EXT_PANGO;
		else
			blk->bufsize = BUFSIZE_EXT;
	}
	else {
		blk->bufsize = BUFSIZE_INT;
	}

	blk->buf = calloc(1, blk->bufsize);

#ifdef YA_DYN_COL
	blk->bgcolor_old = blk->bgcolor;
	blk->fgcolor_old = blk->fgcolor;
	blk->ulcolor_old = blk->ulcolor;
	blk->olcolor_old = blk->olcolor;
	blk->strbuf = blk->buf;
#endif

	ya_create_block(blk);
}

void ya_config_parse() {
	int ret;
	const char * const envhome = getenv("HOME");
	if ((ya.gen_flag & GEN_EXT_CONF) == 0)
	    snprintf(conf_file, CFILELEN, "%s/.config/yabar/yabar.conf", envhome);
	struct stat st;
	if(stat(conf_file, &st) == -1) {
		fprintf(stderr, "Error opening file (%s). Please make sure that this file really exists or use the command `yabar -c [CONFIG_FILE_PATH]`. Exiting...\n", conf_file);
#ifdef YA_INTERNAL_EWMH
		xcb_ewmh_connection_wipe(ya.ewmh);
#endif //YA_INTERNAL_EWMH
		xcb_flush(ya.c);
		xcb_disconnect(ya.c);
		exit(EXIT_FAILURE);
	}

	config_t ya_conf;
	config_init(&ya_conf);
	config_set_auto_convert(&ya_conf, CONFIG_TRUE);
	ret=config_read_file(&ya_conf, conf_file);
	if (ret == CONFIG_FALSE) {
		fprintf(stderr, "Error in the config file at line %d : %s\nExiting...\n", config_error_line(&ya_conf), config_error_text(&ya_conf));
		config_destroy(&ya_conf);
#ifdef YA_INTERNAL_EWMH
		xcb_ewmh_connection_wipe(ya.ewmh);
#endif //YA_INTERNAL_EWMH
		xcb_flush(ya.c);
		xcb_disconnect(ya.c);
		exit(EXIT_FAILURE);
	}
	char *barstr, *blkstr;
	config_setting_t *barlist_set, *blklist_set;
	config_setting_t *curbar_set, *curblk_set;
	barlist_set = config_lookup(&ya_conf, "bar-list");
	if(barlist_set == NULL) {
		fprintf(stderr, "Please add a `bar-list` entry with at least one bar.\nExiting...\n");
		config_destroy(&ya_conf);
		exit(EXIT_SUCCESS);
	}
	ya.barnum = config_setting_length(barlist_set);
	if(ya.barnum < 1) {
		fprintf(stderr, "Please add at least one bar in the `bar-list` entry.\nExiting...\n");
		config_destroy(&ya_conf);
		exit(EXIT_SUCCESS);
	}
	int blknum;
	for(int i=0; i < ya.barnum; i++) {
		barstr = (char *)config_setting_get_string_elem(barlist_set, i);
		curbar_set = config_lookup(&ya_conf, barstr);
		if(curbar_set == NULL) {
			fprintf(stderr, "No valid bar with the name (%s), skipping this invalid bar.\n", barstr);
			//If there is only one bar, continue won't work and we get a segfault.
			//We should exit anyway because we have one && invalid bar name.
			if(ya.barnum == 1)
				exit(EXIT_SUCCESS);
			continue;
		}
		ya_setup_bar(curbar_set);

#ifdef OLD_LIBCONFIG
		blklist_set = config_lookup_from(curbar_set, "block-list");
#else
		blklist_set = config_setting_lookup(curbar_set, "block-list");
#endif
		if(blklist_set == NULL) {
			fprintf(stderr, "No `block-list` entry found for the bar(%s).\n", barstr);
			continue;
		}

		blknum = config_setting_length(blklist_set);
		if(blknum < 1) {
			fprintf(stderr, "No blocks in `block-list` entry for the bar(%s).\n", barstr);
			continue;
		}


		for (int i=0; i < blknum; i++) {
			blkstr = (char *)config_setting_get_string_elem(blklist_set, i);
#ifdef OLD_LIBCONFIG
			curblk_set = config_lookup_from(curbar_set, blkstr);
#else
			curblk_set = config_setting_lookup(curbar_set, blkstr);
#endif
			if(curblk_set == NULL) {
				fprintf(stderr, "No valid block with the name (%s) in the bar (%s), skipping this block\n", blkstr, barstr);
				continue;
			}
			ya_setup_block(curblk_set);
		}
	}
	config_destroy(&ya_conf);
}

