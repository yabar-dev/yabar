/*
 * Yabar - A modern and lightweight status bar for X window managers.
 *
 * Copyright (c) 2016, George Badawi
 * See LICENSE for more information.
 *
 */

#ifndef YABAR_H
#define YABAR_H

#include <stdio.h>
#define __USE_XOPEN2K //for setenv implicit function decleration warning
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <xcb/xcb.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <cairo/cairo-xcb.h>
#include <cairo/cairo.h>
#include <signal.h>
#include <getopt.h>
#include <libconfig.h>

#include <xcb/randr.h>

#ifndef VERSION
#define VERSION ""
#endif



#define BUFSIZE 512
#define CFILELEN 256
#define YA_DEF_FONT "sans bold 9"

#define PT1 printf("%d:%s:%s\n", __LINE__, __FUNCTION__, __FILE__)

#define GET_ALPHA(c)	((double)(((c)>>24) & 0xff)/255.0)
#define GET_RED(c)		((double)(((c)>>16) & 0xff)/255.0)
#define GET_GREEN(c)	((double)(((c))     & 0xff)/255.0)
#define GET_BLUE(c)		((double)(((c)>>8)  & 0xff)/255.0)



enum {
	A_LEFT =0,
	A_CENTER=1,
	A_RIGHT=2
};

enum {
	YA_TOP = 0,
	YA_BOTTOM,
	YA_LEFT,
	YA_RIGHT
};

enum {
	BLKA_PERIODIC 	= 1<<0,
	BLKA_PERSIST 	= 1<<1,
	BLKA_ONCE 		= 1<<2,
	BLKA_INTERNAL 	= 1<<3,
	BLKA_EXTERNAL 	= 1<<4,
	BLKA_MARKUP_PANGO 	= 1<<5,
	BLKA_FIXED_WIDTH 	= 1<<6,
	BLKA_ALIGN_LEFT 	= 1<<7,
	BLKA_ALIGN_CENTER 	= 1<<8,
	BLKA_ALIGN_RIGHT 	= 1<<9,
	BLKA_BGCOLOR 		= 1<<10,
	BLKA_FGCOLOR 		= 1<<11,
	BLKA_UNDERLINE 		= 1<<12,
	BLKA_OVERLINE 		= 1<<13
};

#ifdef YABAR_RANDR

#define CMONLEN 16

typedef struct ya_monitor ya_monitor_t;
struct ya_monitor {
	char name[CMONLEN];
	xcb_rectangle_t pos;
	struct ya_monitor *prev_mon;
	struct ya_monitor *next_mon;
};
#endif // YABAR_RANDR

typedef struct ya_bar ya_bar_t;
struct ya_block {
	char buf [BUFSIZE];
	char *cmd;
	char *button_cmd[5];

	uint32_t sleep;
	uint32_t type;
	uint8_t align;
	uint16_t xpos;
	uint16_t width;

	xcb_pixmap_t pixmap;
	xcb_gcontext_t gc;

	struct ya_block *prev_blk;
	struct ya_block *next_blk;

	ya_bar_t *bar;
	pthread_t thread;
	pid_t pid;

	uint32_t bgcolor;
	uint32_t fgcolor;
	uint32_t ulcolor;
	uint32_t olcolor;
};

typedef struct ya_block ya_block_t;

struct ya_bar {
	uint16_t occupied_width[3];

	uint16_t hgap;
	uint16_t vgap;

	uint32_t bgcolor;
	uint16_t width;
	uint16_t height;

	xcb_window_t win;
	uint8_t position;

	PangoFontDescription *desc;

	ya_block_t *curblk[3];

	ya_bar_t *prev_bar;
	ya_bar_t *next_bar;

	uint8_t ulsize;
	uint8_t olsize;
	uint8_t slack;

	uint32_t brcolor;
	uint8_t brsize;

#ifdef YABAR_RANDR
	ya_monitor_t *mon;
#endif //YABAR_RANDR
};

//typedef struct ya_bar ya_bar_t;

enum {
	GEN_EXT_CONF	= 1 << 0,
	GEN_RANDR		= 1 << 1
};

struct yabar_gen_info {
	xcb_connection_t *c;
	xcb_screen_t *scr;
	xcb_visualtype_t *visualtype;
	xcb_colormap_t colormap;
	uint16_t barnum;
	ya_bar_t *curbar;
	uint8_t depth;
	uint8_t gen_flag;
#ifdef YABAR_RANDR
	ya_monitor_t *curmon;
#endif //YABAR_RANDR
};
typedef struct yabar_gen_info yabar_info_t;

extern yabar_info_t ya;
extern char conf_file[CFILELEN]; 
void * ya_exec(void * _blk);
xcb_visualtype_t * ya_get_visual();
void ya_sighandler(int signum);
void ya_init();
xcb_visualtype_t * 	ya_get_visualtype();
void ya_draw_pango_text(struct ya_block *blk);
void ya_setup_ewmh (ya_bar_t *bar);
void ya_config_parse();
int ya_create_bar(ya_bar_t * bar);
void ya_execute();
//void ya_exec_cmd(char * cmd);
void ya_exec_cmd(ya_block_t * blk, xcb_button_press_event_t *eb);
void ya_cleanup_x();
void ya_cleanup_blocks();
void ya_create_block(ya_block_t *blk);
ya_block_t * ya_get_blk_from_event( xcb_button_press_event_t *eb);
void ya_process_opt(int argc, char *argv[]);
int ya_init_randr();
#ifdef YABAR_RANDR
ya_monitor_t * ya_get_monitor_from_name(const char *name);
#endif //YABAR_RANDR
#endif /*YABAR_H*/
