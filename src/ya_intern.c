/*
 * Yabar - A modern and lightweight status bar for X window managers.
 *
 * Copyright (c) 2016, George Badawi
 * See LICENSE for more information.
 *
 */

#include "yabar.h"

#define YA_INTERNAL
void ya_int_time(ya_block_t * blk);
void ya_int_uptime(ya_block_t * blk);
struct reserved_blk ya_reserved_blks[YA_INTERNAL_LEN] = { 
	{"YA_INT_TIME", ya_int_time},
	{"YA_INT_UPTIME", ya_int_uptime}
}; 

//#ifdef YA_INTERNAL

#include <time.h>
void ya_int_time(ya_block_t * blk) {
	time_t rawtime;
	struct tm * ya_tm;
	while(1) {
		time(&rawtime);
		ya_tm = localtime(&rawtime);
		strftime(blk->buf, 512, blk->internal->option[0], ya_tm);
		ya_draw_pango_text(blk);
		sleep(blk->sleep);
	}
}

#include <sys/sysinfo.h>

void ya_int_uptime(ya_block_t *blk) {
	struct sysinfo ya_sysinfo;
	long hr, tmp;
	uint8_t min;
	while(1) {
		sysinfo(&ya_sysinfo);
		tmp = ya_sysinfo.uptime;
		hr = tmp/3600;
		tmp %= 3600;
		min = tmp/60;
		tmp %= 60;
		snprintf(blk->buf, 512, "%ld:%02d:%02ld", hr, min, tmp);
		ya_draw_pango_text(blk);
		sleep(blk->sleep);
	}
}

//#endif //YA_INTERNAL
