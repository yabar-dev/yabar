/*
 * Yabar - A modern and lightweight status bar for X window managers.
 *
 * Copyright (c) 2016, George Badawi
 * See LICENSE for more information.
 *
 */

#include "yabar.h"

void ya_int_date(ya_block_t * blk);
void ya_int_uptime(ya_block_t * blk);
void ya_int_memory(ya_block_t * blk);
void ya_int_thermal(ya_block_t *blk);
void ya_int_brightness(ya_block_t *blk);
void ya_int_bandwidth(ya_block_t *blk);
void ya_int_cpu(ya_block_t *blk);
void ya_int_loadavg(ya_block_t *blk);
void ya_int_diskio(ya_block_t *blk);
void ya_int_network(ya_block_t *blk);
void ya_int_battery(ya_block_t *blk);

struct reserved_blk ya_reserved_blks[YA_INTERNAL_LEN] = {
	{"YABAR_DATE", ya_int_date},
	{"YABAR_UPTIME", ya_int_uptime},
	{"YABAR_THERMAL", ya_int_thermal},
	{"YABAR_BRIGHTNESS", ya_int_brightness},
	{"YABAR_BANDWIDTH", ya_int_bandwidth},
	{"YABAR_MEMORY", ya_int_memory},
	{"YABAR_CPU", ya_int_cpu},
	{"YABAR_LOADAVG", ya_int_loadavg},
	{"YABAR_DISKIO", ya_int_diskio},
	{"YABAR_NETWORK", ya_int_network},
	{"YABAR_BATTERY", ya_int_battery},
#ifdef YA_INTERNAL_EWMH
	{"YABAR_TITLE", NULL},
	{"YABAR_WORKSPACE", NULL}
#endif
};

//#define YA_INTERNAL

#ifdef YA_INTERNAL

inline void ya_setup_prefix_suffix(ya_block_t *blk, size_t * prflen, size_t *suflen, char **startstr) {
	if(blk->internal->prefix) {
		*prflen = strlen(blk->internal->prefix);
		if(*prflen) {
			strcpy(blk->buf, blk->internal->prefix);
			*startstr += *prflen;
		}
	}
	if(blk->internal->suffix) {
		*suflen = strlen(blk->internal->suffix);
	}
}


#include <time.h>
void ya_int_date(ya_block_t * blk) {
	time_t rawtime;
	struct tm * ya_tm;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	if(blk->internal->option[0]==NULL) {
		blk->internal->option[0] = "%c";
	}
	while(1) {
		time(&rawtime);
		ya_tm = localtime(&rawtime);
		strftime(startstr, 100, blk->internal->option[0], ya_tm);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		sleep(blk->sleep);
	}
}

#include <sys/sysinfo.h>

void ya_int_uptime(ya_block_t *blk) {
	struct sysinfo ya_sysinfo;
	long hr, tmp;
	uint8_t min;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	while(1) {
		sysinfo(&ya_sysinfo);
		tmp = ya_sysinfo.uptime;
		hr = tmp/3600;
		tmp %= 3600;
		min = tmp/60;
		//tmp %= 60;
		sprintf(startstr, "%ld:%02d", hr, min);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		sleep(blk->sleep);
	}
}


void ya_int_thermal(ya_block_t *blk) {
	FILE *tfile;
	int temp, wrntemp, crttemp, space;
	uint32_t wrnbg, wrnfg; //warning colors
	uint32_t crtbg, crtfg; //critical colors
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	if(blk->internal->spacing)
		space = 3;
	else
		space = 0;
	char *fpath = blk->internal->option[0];

	if((blk->internal->option[1]==NULL) ||
			(sscanf(blk->internal->option[1], "%d; %x; %x", &crttemp, &crtfg, &crtbg)!=3)) {
		crttemp = 70;
		crtbg = 0xFFED303C;
		crtfg = blk->fgcolor;
	}
	if((blk->internal->option[2]==NULL) ||
			(sscanf(blk->internal->option[2], "%d; %x; %x", &wrntemp, &wrnfg, &wrnbg)!=3)) {
		wrntemp = 58;
		wrnbg = 0xFFF4A345;
		wrnfg = blk->fgcolor;
	}
	tfile = fopen(fpath, "r");
	if (tfile == NULL) {
		fprintf(stderr, "Error opening file %s\n", fpath);
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	fclose(tfile);
	while (1) {
		tfile = fopen(fpath, "r");
		if(fscanf(tfile, "%d", &temp) != 1) {
			fprintf(stderr, "Error getting values from file (%s)\n", fpath);
		}
		temp/=1000;

#ifdef YA_DYN_COL
		if(temp > crttemp) {
			blk->bgcolor = crtbg;
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){crtbg});
			blk->attr |= BLKA_DIRTY_COL;
			blk->fgcolor = crtfg;
		}
		else if (temp > wrntemp) {
			blk->bgcolor = wrnbg;
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){wrnbg});
			blk->attr |= BLKA_DIRTY_COL;
			blk->fgcolor = wrnfg;
		}
		else {
			blk->bgcolor = blk->bgcolor_old;
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){blk->bgcolor});
			blk->attr &= ~BLKA_DIRTY_COL;
			blk->fgcolor = blk->fgcolor_old;
		}
#endif //YA_DYN_COL

		sprintf(startstr, "%*d", space, temp);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}

}

void ya_int_brightness(ya_block_t *blk) {
	int bright, space;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	FILE *tfile;
	if(blk->internal->spacing)
		space = 3;
	else
		space = 0;
	char *fpath = blk->internal->option[0];
	tfile = fopen(fpath, "r");
	if (tfile == NULL) {
		fprintf(stderr, "Error opening file %s\n", fpath);
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	while(1) {
		tfile = fopen(fpath, "r");
		if(fscanf(tfile, "%d", &bright) != 1)
			fprintf(stderr, "Error getting values from file (%s)\n", fpath);
		sprintf(startstr, "%*d", space, bright);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}
}


void ya_str_get_default_net_interface(char* if_name, size_t len) {
        FILE *route_h = NULL;
        char buffer[255] = { 0 };

        route_h = fopen("/proc/net/route", "r");
        if(route_h) {
                //Ignore first line as it is headers
                fgets(buffer, sizeof(buffer), route_h);
                fgets(buffer, sizeof(buffer), route_h);
                char *it = (char*) memchr(buffer, '\t', strlen(buffer));
                *it = '\0';
        }

        // Fallback to loopback interface if nothing
        // has been found yet
        if (strcmp(buffer, "Iface") == 0 || buffer[0] == '\0') {
                strcpy(buffer, "lo");
        }

        strncpy(if_name, buffer, len);
        if_name[len-1] = '\0';
        fclose(route_h);

}

void ya_int_bandwidth(ya_block_t * blk) {
	int space;
	unsigned long rx, tx, orx, otx;
        rx = tx = orx = otx = 0;
	unsigned int rxrate, txrate;
	FILE *rxfile, *txfile;
	char rxpath[128];
	char txpath[128];
	char if_buffer[25];
	char rxc, txc;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	char dnstr[20], upstr[20];
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
        space = (blk->internal->spacing) ? 4 : 0;
        char *if_name = blk->internal->option[0];

	if(blk->internal->option[1]) {
		sscanf(blk->internal->option[1], "%[^;]; %[^;]", dnstr, upstr);
	}

	for(;;) {
                if(if_name == NULL || if_name[0] == '\0') {
                        ya_str_get_default_net_interface(if_buffer, sizeof(if_buffer));
                        if_name = if_buffer;
                }
                snprintf(rxpath, sizeof(rxpath), "/sys/class/net/%s/statistics/rx_bytes", if_name);
                snprintf(txpath, sizeof(txpath), "/sys/class/net/%s/statistics/tx_bytes", if_name);

                rxfile = fopen(rxpath, "r");
                txfile = fopen(txpath, "r");
                if(rxfile == NULL || txfile == NULL) break;

		if(fscanf(rxfile, "%lu", &rx)!=1) fprintf(stderr, "Error getting values from file (%s)\n", rxpath);
		if(fscanf(txfile, "%lu", &tx)!=1) fprintf(stderr, "Error getting values from file (%s)\n", txpath);

		rxrate = (rx - orx) / (blk->sleep * 1024);
		txrate = (tx - otx) / (blk->sleep * 1024);

		txc = rxc = 'K';
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

		sprintf(startstr, "%s%*u%c %s%*u%c", dnstr, space, rxrate, rxc, upstr, space, txrate, txc);
		if(suflen) strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);

		if(rxfile) fclose(rxfile);
		if(txfile) fclose(txfile);
		sleep(blk->sleep);
	}

        if(rxfile) fclose(rxfile);
        if(txfile) fclose(txfile);

        strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
        ya_draw_pango_text(blk);
        pthread_detach(blk->thread);
        pthread_exit(NULL);
        return;
}

void ya_int_memory(ya_block_t *blk) {
	int space;
	unsigned long total, free, cached, buffered;
	float used;
	FILE *tfile;
	char tline[50];
	char unit;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	if(blk->internal->spacing)
		space = 6;
	else
		space = 0;
	tfile = fopen("/proc/meminfo", "r");
	if (tfile == NULL) {
		fprintf(stderr, "Error opening file %s\n", "/proc/meminfo");
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_exit(NULL);
	}
	fclose(tfile);
	while(1) {
		tfile = fopen("/proc/meminfo", "r");
		while(fgets(tline, 50, tfile) != NULL) {
			sscanf(tline, "MemTotal: %lu kB\n", &total);
			sscanf(tline, "MemFree: %lu kB\n", &free);
			sscanf(tline, "Buffers: %lu kB\n", &buffered);
			sscanf(tline, "Cached: %lu kB\n", &cached);
		}
		used = (float)(total - free - buffered - cached)/1024.0;
		unit = 'M';
		if(((int)used)>1024) {
			used = used/1024.0;
			unit = 'G';
		}
		sprintf(startstr, "%*.1f%c", space, used, unit);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}
}


void ya_int_cpu(ya_block_t *blk) {
	int space;
	char fpath[] = "/proc/stat";
	FILE *tfile;
	long double old[4], cur[4], ya_avg=0.0;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	char cpustr[20];
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	if(blk->internal->spacing)
		space = 5;
	else
		space = 0;
	tfile = fopen(fpath, "r");
	if (tfile == NULL) {
		fprintf(stderr, "Error opening file (%s)\n", fpath);
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_exit(NULL);
	}
	else {
		if(fscanf(tfile,"%s %Lf %Lf %Lf %Lf",cpustr, &old[0],&old[1],&old[2],&old[3])!=5)
			fprintf(stderr, "Error getting values from file (%s)\n", fpath);

	}
	fclose(tfile);
	while(1) {
		tfile = fopen(fpath, "r");
		if(fscanf(tfile,"%s %Lf %Lf %Lf %Lf", cpustr, &cur[0],&cur[1],&cur[2],&cur[3])!=5)
			fprintf(stderr, "Error getting values from file (%s)\n", fpath);
		ya_avg = ((cur[0]+cur[1]+cur[2]) - (old[0]+old[1]+old[2])) / ((cur[0]+cur[1]+cur[2]+cur[3]) - (old[0]+old[1]+old[2]+old[3]));
		for(int i=0; i<4;i++)
			old[i]=cur[i];
		ya_avg *= 100.0;
		sprintf(startstr, "%*.1Lf", space, ya_avg);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}
}

void ya_int_loadavg(ya_block_t *blk) {
	int space, avg = 0;
	char fpath[] = "/proc/loadavg";
	FILE *tfile;
	long double cur[4], ya_avg = 0.0;
	char *startstr = blk->buf;
	size_t prflen = 0,suflen = 0;
	char avgstr[20];
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	if(blk->internal->spacing)
		space = 3;
	else
		space = 0;
#ifdef YA_DYN_COL
	long double crttemp;
	uint32_t crtbg, crtfg;
	if(blk->internal->option[0]) {
		avg = atoi(blk->internal->option[0]);
		switch (avg) {
			case 15:
				avg = 2;
				break;
			case 5:
				avg = 1;
				break;
			default:
				avg = 0;
				break;
		}
	}
	if((blk->internal->option[1]==NULL) ||
			(sscanf(blk->internal->option[1], "%Lf %x %x", &crttemp, &crtfg, &crtbg)!=3)) {
		crttemp = 1.0;
		crtbg = 0xFFED303C;
		crtfg = blk->fgcolor;
	}
#endif
	while(1) {
		tfile = fopen(fpath, "r");
		if(fscanf(tfile,"%Lf %Lf %Lf %s %Lf", &cur[0],&cur[1],&cur[2],avgstr,&cur[3])!=5)
			fprintf(stderr, "Error getting values from file (%s)\n", fpath);
		ya_avg = cur[avg];
#ifdef YA_DYN_COL
		if(ya_avg > crttemp) {
			blk->bgcolor = crtbg;
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){crtbg});
			blk->attr |= BLKA_DIRTY_COL;
			blk->fgcolor = crtfg;
		}
		else {
			blk->bgcolor = blk->bgcolor_old;
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){blk->bgcolor});
			blk->attr &= ~BLKA_DIRTY_COL;
			blk->fgcolor = blk->fgcolor_old;
		}
#endif
		sprintf(startstr, "%*.2Lf", space, ya_avg);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}
}


void ya_int_diskio(ya_block_t *blk) {
	int space;
	unsigned long tdo[11], tdc[11];
	unsigned long drd=0, dwr=0;
	char crd, cwr;
	FILE *tfile;
	char tpath[100];
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	char dnstr[20], upstr[20];
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	if(blk->internal->spacing)
		space = 4;
	else
		space = 0;
	if(blk->internal->option[1]) {
		sscanf(blk->internal->option[1], "%[^;]; %[^;]", dnstr, upstr);
	}
	snprintf(tpath, 100, "/sys/class/block/%s/stat", blk->internal->option[0]);
	tfile = fopen(tpath, "r");
	if (tfile == NULL) {
		fprintf(stderr, "Error opening file %s\n", tpath);
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	else {
		if(fscanf(tfile,"%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &tdo[0], &tdo[1], &tdo[2], &tdo[3], &tdo[4], &tdo[5], &tdo[6], &tdo[7], &tdo[8], &tdo[9], &tdo[10])!=11)
			fprintf(stderr, "Error getting values from file (%s)\n", tpath);
	}
	fclose(tfile);
	while(1) {
		tfile = fopen(tpath, "r");
		if(fscanf(tfile,"%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &tdc[0], &tdc[1], &tdc[2], &tdc[3], &tdc[4], &tdc[5], &tdc[6], &tdc[7], &tdc[8], &tdc[9], &tdc[10])!=11)
			fprintf(stderr, "Error getting values from file (%s)\n", tpath);
		drd = (unsigned long)(((float)(tdc[2] - tdo[2])*0.5)/((float)(blk->sleep)));
		dwr = (unsigned long)(((float)(tdc[6] - tdo[6])*0.5)/((float)(blk->sleep)));
		crd = cwr = 'K';
		if(drd >1024) {
			drd /= 1024;
			crd = 'M';
		}
		if(dwr >1024) {
			dwr /= 1024;
			cwr = 'M';
		}
		sprintf(startstr, "%s%*lu%c %s%*lu%c", dnstr, space, drd, crd, upstr, space, dwr, cwr);
		for(int i=0; i<11;i++)
			tdo[i] = tdc[i];
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);

		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}

}

void ya_int_battery(ya_block_t *blk) {
	int bat, space;
	char stat;
	char bat_25str[20], bat_50str[20], bat_75str[20], bat_100str[20], bat_chargestr[20];
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	FILE *cfile, *sfile;
	if(blk->internal->spacing)
		space = 3;
	else
		space = 0;
	char cpath[128], spath[128];
	snprintf(cpath, 128, "/sys/class/power_supply/%s/capacity", blk->internal->option[0]);
	snprintf(spath, 128, "/sys/class/power_supply/%s/status", blk->internal->option[0]);
	if(blk->internal->option[1]) {
		sscanf(blk->internal->option[1], "%[^;]; %[^;]; %[^;]; %[^;]; %[^;]", bat_25str, bat_50str, bat_75str, bat_100str, bat_chargestr);
	}
	cfile = fopen(cpath, "r");
	sfile = fopen(spath, "r");
	if (cfile == NULL || sfile == NULL) {
		fprintf(stderr, "Error opening file %s or %s\n", cpath, spath);
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		//Close if one of the files is opened
		if(cfile)
			fclose(cfile);
		if(sfile)
			fclose(sfile);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	fclose(cfile);
	fclose(sfile);
	while(1) {
		cfile = fopen(cpath, "r");
		sfile = fopen(spath, "r");
		if(fscanf(cfile, "%d", &bat) != 1)
			fprintf(stderr, "Error getting values from file (%s)\n", cpath);
		if(fscanf(sfile, "%c", &stat) != 1)
			fprintf(stderr, "Error getting values from file (%s)\n", spath);
		if(bat <= 25)
			strcpy(startstr, bat_25str);
		else if(bat <= 50)
			strcpy(startstr, bat_50str);
		else if(bat <= 75)
			strcpy(startstr, bat_75str);
		else
			strcpy(startstr, bat_100str);
		if((stat == 'C' || stat == 'U') && bat != 100)
			strcat(startstr, bat_chargestr);
		sprintf(startstr+strlen(startstr), "%*d", space, bat);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(cfile);
		fclose(sfile);
		sleep(blk->sleep);
	}
}

#define _GNU_SOURCE
#include <sys/socket.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <arpa/inet.h>
#include <netdb.h>

void ya_int_network(ya_block_t *blk) {
	pthread_detach(blk->thread);
	pthread_exit(NULL);
	/*
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char host[1025];
	if(getifaddrs(&ifaddr) == -1) {
		fprintf(stderr, "error in getifaddrs\n");
		pthread_exit(NULL);
	}
	for(ifa = ifaddr; ifa; ifa = ifa->ifa_next, n++) {
		if(ifa == NULL)
			continue;
		family = ifa->ifa_addr->sa_family;
		//printf("%s\n", ifa->ifa_name);
		if (family == AF_INET || family == AF_INET6) {
			s = getnameinfo(ifa->ifa_addr,
					(family == AF_INET) ? sizeof(struct sockaddr_in) :
					sizeof(struct sockaddr_in6), host, 1025,
					NULL, 0, NI_NUMERICHOST);
			printf("%s %s\n", ifa->ifa_name, host);
		}
	}
	while(1) {

		ya_draw_pango_text(blk);
		sleep(blk->sleep);
		memset(blk->buf, '\0', 12);
	}
	*/
}
#endif //YA_INTERNAL
