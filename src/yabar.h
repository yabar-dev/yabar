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
#include <errno.h>
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

//just to suppress gcc syntastic warnings in vim
//VERSION is obtained from Makefile
#ifndef VERSION
#define VERSION ""
#endif


extern char *strdup(const char *s); //to suppress implicit decleration warning for strdup

#define BUFSIZE_EXT 512 //buffer size for external blocks
#define BUFSIZE_EXT_PANGO 1024 //buffer size for extern blocks with pango markup
#define BUFSIZE_INT 256 //initial value of buffer size for internal blocks, can be overriden in runtime if needed

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

//Flags of block attributes; stored in blk->attr
enum {
	BLKA_PERIODIC		= 1<<0,
	BLKA_PERSIST		= 1<<1,
	BLKA_ONCE			= 1<<2,
	BLKA_INTERNAL		= 1<<3,
	BLKA_EXTERNAL		= 1<<4,
	BLKA_MARKUP_PANGO 	= 1<<5,
	BLKA_FIXED_WIDTH 	= 1<<6,
	BLKA_ALIGN_LEFT 	= 1<<7,
	BLKA_ALIGN_CENTER 	= 1<<8,
	BLKA_ALIGN_RIGHT 	= 1<<9,
	BLKA_BGCOLOR 		= 1<<10,
	BLKA_FGCOLOR		= 1<<11,
	BLKA_UNDERLINE 		= 1<<12,
	BLKA_OVERLINE 		= 1<<13,
	BLKA_INHERIT		= 1<<14
};


enum {
	BARA_INHERIT = 1<<0,
	BARA_INHERIT_ALL = 1<<1
};

#define NOT_INHERIT_BAR(bar) (((bar)->attr & BARA_INHERIT)==0)
#define NOT_INHERIT_BLK(blk) (((blk)->attr & BLKA_INHERIT)==0)

#define CMONLEN 16
#define YA_INTERNAL_LEN 4

typedef struct ya_monitor ya_monitor_t;
typedef struct ya_bar ya_bar_t;
typedef struct ya_block ya_block_t;
typedef struct blk_intern blk_intern_t;
typedef void(*funcp)(ya_block_t *);

struct reserved_blk {
	char *name;
	funcp function;
};

struct ya_monitor {
	char name[CMONLEN];
	xcb_rectangle_t pos;
	struct ya_monitor *prev_mon;
	struct ya_monitor *next_mon;
};

struct ya_block {
	char *name;
	char *buf;
	size_t bufsize;
	char *cmd;
	char *button_cmd[5]; 

	uint32_t sleep;
	uint32_t attr; //block atributes
	uint8_t align;
	uint8_t justify; //justify text within block
	uint16_t shift;
	uint16_t width;

	xcb_pixmap_t pixmap;
	xcb_gcontext_t gc;

	struct ya_block *prev_blk;
	struct ya_block *next_blk;

	ya_bar_t *bar;
	pthread_t thread;
	pid_t pid;

	uint32_t bgcolor; //background color
	uint32_t fgcolor; //foreground color
	uint32_t ulcolor; //underline color
	uint32_t olcolor; //overline color

	blk_intern_t *internal;
};


struct blk_intern {
	char *prefix;
	char *suffix;
	char *option[3];
	uint8_t index;
};


struct ya_bar {
	char *name;
	uint16_t occupied_width[3]; // occupied for each alignment (left, center and right)

	uint16_t hgap; //horizontal gap
	uint16_t vgap; //vertical gap

	uint32_t bgcolor; //background color
	uint16_t width;
	uint16_t height;

	xcb_window_t win;
	uint8_t position; //top, bottom, left or right.

	PangoFontDescription *desc;

	ya_block_t *curblk[3];
	/* curblk[i] should point to the last added block for each alignment. then
	 * should point to the first block for each alignment after invoking ya_execute()
	 */

	ya_bar_t *prev_bar;
	ya_bar_t *next_bar;

	uint8_t ulsize; //underline size
	uint8_t olsize; //overline size
	uint8_t slack; //slack size

	uint32_t brcolor; //border color
	uint8_t brsize; //border size
	uint8_t attr; //bar attributes

	ya_monitor_t *mon;
};


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
	ya_monitor_t *curmon;
};
typedef struct yabar_gen_info yabar_info_t;

extern yabar_info_t ya;
extern char conf_file[CFILELEN]; 
extern struct reserved_blk ya_reserved_blks[YA_INTERNAL_LEN]; 

void ya_init();
void ya_execute();
void ya_process_opt(int argc, char *argv[]);
xcb_visualtype_t * ya_get_visualtype();
void ya_config_parse();

void ya_create_bar(ya_bar_t * bar);
void ya_create_block(ya_block_t *blk);

void ya_draw_pango_text(struct ya_block *blk);
void ya_exec_button(ya_block_t * blk, xcb_button_press_event_t *eb);

ya_block_t * ya_get_blk_from_event( xcb_button_press_event_t *eb);

#endif /*YABAR_H*/
