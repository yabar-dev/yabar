/*
 * Yabar - A modern and lightweight status bar for X window managers.
 *
 * Copyright (c) 2016, George Badawi
 * See LICENSE for more information.
 *
 */

#include "yabar.h"

void ya_int_time(ya_block_t * blk);
void ya_int_uptime(ya_block_t * blk);
void ya_int_memory(ya_block_t * blk);
void ya_int_thermal(ya_block_t *blk);
void ya_int_brightness(ya_block_t *blk);
void ya_int_bandwidth(ya_block_t *blk);

struct reserved_blk ya_reserved_blks[YA_INTERNAL_LEN] = { 
	{"YA_INT_TIME", ya_int_time},
	{"YA_INT_UPTIME", ya_int_uptime},
	{"YA_INT_THERMAL", ya_int_thermal},
	{"YA_INT_BRIGHTNESS", ya_int_brightness},
	{"YA_INT_BANDWIDTH", ya_int_bandwidth}
	//{"YA_INT_MEMORY", ya_int_memory}
}; 

//#define YA_INTERNAL

#ifdef YA_INTERNAL

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


void ya_int_thermal(ya_block_t *blk) {
	FILE *tfile;
	int temp, wrntemp, crttemp;
	uint32_t oldbg, oldfg;
	uint32_t wrnbg, wrnfg; //warning colors
	uint32_t crtbg, crtfg; //critical colors
	if(blk->attr & BLKA_BGCOLOR)
		oldbg = blk->bgcolor;
	else
		oldbg = blk->bar->bgcolor;
	oldfg = blk->fgcolor;
	char fpath[128];
	snprintf(fpath, 128, "/sys/class/thermal/%s/temp", blk->internal->option[0]);

	if((blk->internal->option[1]==NULL) ||
			(sscanf(blk->internal->option[1], "%d %u %u", &crttemp, &crtfg, &crtbg)!=3)) {
		crttemp = 70;
		crtbg = 0xFFED303C;
		crtfg = blk->fgcolor;
	}
	if((blk->internal->option[2]==NULL) ||
			(sscanf(blk->internal->option[2], "%d %u %u", &wrntemp, &wrnfg, &wrnbg)!=3)) {
		wrntemp = 58;
		wrnbg = 0xFFF4A345;
		wrnfg = blk->fgcolor;
	}
	tfile = fopen(fpath, "r");
	if (tfile == NULL) {
		//TODO handle and exit thread
		fprintf(stderr, "Error opening file %s\n", fpath);
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_exit(NULL);
	}
	fclose(tfile);
	while (1) {
		tfile = fopen(fpath, "r");
		fscanf(tfile, "%d", &temp);
		temp/=1000;

		if(temp > crttemp) {
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){crtbg});
			blk->fgcolor = crtfg;
		}
		else if (temp > wrntemp) {
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){wrnbg});
			blk->fgcolor = wrnfg;
		}
		else {
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){oldbg});
			blk->fgcolor = oldfg;
		}

		snprintf(blk->buf, 4, "%d", temp);
		ya_draw_pango_text(blk);
		sleep(blk->sleep);
		memset(blk->buf, '\0', 4);
		fclose(tfile);
	}

}

void ya_int_brightness(ya_block_t *blk) {
	int bright;
	FILE *tfile;
	char fpath[128];
	snprintf(fpath, 128, "/sys/class/backlight/%s/brightness", blk->internal->option[0]);
	tfile = fopen(fpath, "r");
	if (tfile == NULL) {
		//TODO handle and exit thread
		fprintf(stderr, "Error opening file %s\n", fpath);
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_exit(NULL);
	}
	while(1) {
		tfile = fopen(fpath, "r");
		fscanf(tfile, "%d", &bright);
		snprintf(blk->buf, 12, "%d", bright);
		ya_draw_pango_text(blk);
		sleep(blk->sleep);
		memset(blk->buf, '\0', 12);
		fclose(tfile);
	}
}

void ya_int_bandwidth(ya_block_t * blk) {
	unsigned long rx, tx, orx, otx;
	unsigned int rxrate, txrate; 
	FILE *rxfile, *txfile;
	char rxpath[128];
	char txpath[128];
	char rxc, txc;
	snprintf(rxpath, 128, "/sys/class/net/%s/statistics/rx_bytes", blk->internal->option[0]);
	snprintf(txpath, 128, "/sys/class/net/%s/statistics/tx_bytes", blk->internal->option[0]);
	rxfile = fopen(rxpath, "r");
	txfile = fopen(txpath, "r");
	if (rxfile == NULL || txfile == NULL) {
		//TODO handle and exit thread
		fprintf(stderr, "Error opening file %s or %s\n", rxpath, txpath);
		strncpy(blk->buf, "BLOCK \nERROR!", strlen("BLOCK \nERROR!"));
		ya_draw_pango_text(blk);
		pthread_exit(NULL);
	}
	else {
		fscanf(rxfile, "%lu", &orx);
		fscanf(txfile, "%lu", &otx);
	}
	fclose(rxfile);
	fclose(txfile);
	while(1) {
		txc = rxc = 'K';
		rxfile = fopen(rxpath, "r");
		txfile = fopen(txpath, "r");

		fscanf(rxfile, "%lu", &rx);
		fscanf(txfile, "%lu", &tx);

		rxrate = (rx - orx)/((blk->sleep)*1024);
		txrate = (tx - otx)/((blk->sleep)*1024);

		if(rxrate > 1024) {
			rxrate/=1024;
			rxc = 'M';
		}
		if(txrate > 1024) {
			txrate/=1024;
			txc = 'M';
		}


		orx = rx;
		otx = tx;

		snprintf(blk->buf, 40, "%4u%c %4u%c", rxrate, rxc, txrate, txc);
		ya_draw_pango_text(blk);
		fclose(rxfile);
		fclose(txfile);
		memset(blk->buf, '\0', 40);
		sleep(blk->sleep);
	}
	
}
#endif //YA_INTERNAL
