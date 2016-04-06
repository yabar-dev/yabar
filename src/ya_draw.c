/*
 * Yabar - A modern and lightweight status bar for X window managers.
 *
 * Copyright (c) 2016, George Badawi
 * See LICENSE for more information.
 *
 */

#include "yabar.h"

enum {
	NET_WM_WINDOW_TYPE,
	NET_WM_WINDOW_TYPE_DOCK,
	NET_WM_DESKTOP,
	NET_WM_STRUT_PARTIAL,
	NET_WM_STRUT,
	NET_WM_STATE,
	NET_WM_STATE_STICKY,
	NET_WM_STATE_ABOVE,
};

static void ya_setup_ewmh(ya_bar_t *bar) {
	// I really hope I understood this correctly from lemonbar code :| 
	const char *atom_names[] = {
		"_NET_WM_WINDOW_TYPE",
		"_NET_WM_WINDOW_TYPE_DOCK",
		"_NET_WM_DESKTOP",
		"_NET_WM_STRUT_PARTIAL",
		"_NET_WM_STRUT",
		"_NET_WM_STATE",
		"_NET_WM_STATE_STICKY",
		"_NET_WM_STATE_ABOVE",
	};
	const int atoms = sizeof(atom_names)/sizeof(char *);
	xcb_intern_atom_cookie_t atom_cookie[atoms];
	xcb_atom_t atom_list[atoms];
	xcb_intern_atom_reply_t *atom_reply;
	for (int i = 0; i < atoms; i++)
		atom_cookie[i] = xcb_intern_atom(ya.c, 0, strlen(atom_names[i]), atom_names[i]);

	for (int i = 0; i < atoms; i++) {
		atom_reply = xcb_intern_atom_reply(ya.c, atom_cookie[i], NULL);
		if (!atom_reply)
			return;
		atom_list[i] = atom_reply->atom;
		free(atom_reply);
	}

	int strut[12];

	if (bar->position == YA_TOP) {
		strut[2] = bar->height;
		strut[8] = bar->hgap;
		strut[9] = bar->hgap + bar->width;
	}
	else if (bar->position == YA_BOTTOM) {
		strut[3] = bar->height;
		strut[10] = bar->hgap;
		strut[11] = bar->hgap + bar->width;
	}
	else {
	//TODO right and left bars if implemented.
	}

	xcb_change_property(ya.c, XCB_PROP_MODE_REPLACE, bar->win, atom_list[NET_WM_WINDOW_TYPE], XCB_ATOM_ATOM, 32, 1, &atom_list[NET_WM_WINDOW_TYPE_DOCK]);
	xcb_change_property(ya.c, XCB_PROP_MODE_APPEND,  bar->win, atom_list[NET_WM_STATE], XCB_ATOM_ATOM, 32, 2, &atom_list[NET_WM_STATE_STICKY]);
	xcb_change_property(ya.c, XCB_PROP_MODE_REPLACE, bar->win, atom_list[NET_WM_DESKTOP], XCB_ATOM_CARDINAL, 32, 1, (const uint32_t []){ -1 } );
	xcb_change_property(ya.c, XCB_PROP_MODE_REPLACE, bar->win, atom_list[NET_WM_STRUT_PARTIAL], XCB_ATOM_CARDINAL, 32, 12, strut);
	xcb_change_property(ya.c, XCB_PROP_MODE_REPLACE, bar->win, atom_list[NET_WM_STRUT], XCB_ATOM_CARDINAL, 32, 4, strut);
	xcb_change_property(ya.c, XCB_PROP_MODE_REPLACE, bar->win, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen("yabar"), "yabar");
}

void ya_create_block(ya_block_t *blk) {
	uint32_t gc_col;
	ya_block_t *tmpblk;
	blk->bar = ya.curbar;
	if (blk->bar->curblk[blk->align]) {
		blk->bar->curblk[blk->align]->next_blk = blk;	
		blk->prev_blk = blk->bar->curblk[blk->align];
	}
	switch (blk->align) {
		case A_LEFT:
			blk->shift = blk->bar->occupied_width[A_LEFT];
			blk->bar->occupied_width[A_LEFT] += blk->width + blk->bar->slack;
			break;
		case A_CENTER:
			blk->bar->occupied_width[A_CENTER] += blk->width + blk->bar->slack;
			tmpblk = blk->bar->curblk[A_CENTER];
			if(tmpblk) {
				for(;tmpblk->prev_blk; tmpblk = tmpblk->prev_blk);
				tmpblk->shift = (blk->bar->width - blk->bar->occupied_width[A_CENTER])/2;
				for(tmpblk = tmpblk->next_blk; tmpblk; tmpblk = tmpblk->next_blk) {
					tmpblk->shift = tmpblk->prev_blk->shift + tmpblk->prev_blk->width + blk->bar->slack;	
				}
			}
			else {
				blk->shift = (blk->bar->width - blk->bar->occupied_width[A_CENTER])/2;
			}
			break;
		case A_RIGHT:
			tmpblk = blk->bar->curblk[A_RIGHT];
			blk->bar->occupied_width[A_RIGHT] += (blk->width + blk->bar->slack);
			blk->shift = blk->bar->width - blk->width;
			if(tmpblk) {
				for(; tmpblk; tmpblk = tmpblk->prev_blk) {
					tmpblk->shift -= (blk->width + blk->bar->slack);
				}
		
			}
			break;
	}
	blk->bar->curblk[blk->align] = blk;
	blk->pixmap = xcb_generate_id(ya.c);
	xcb_create_pixmap(ya.c,
			ya.depth,
			blk->pixmap,
			blk->bar->win, blk->width, blk->bar->height);
	blk->gc = xcb_generate_id(ya.c);
	if (blk->attr & BLKA_BGCOLOR)
		gc_col = blk->bgcolor;
	else
		gc_col = blk->bar->bgcolor;
	xcb_create_gc(ya.c, blk->gc, blk->pixmap, XCB_GC_FOREGROUND, (const uint32_t[]){gc_col});
}

void ya_create_bar(ya_bar_t * bar) {
	bar->win = xcb_generate_id(ya.c);
	int x=0, y=0;
	if ((ya.gen_flag & GEN_RANDR))
		x = bar->hgap + bar->mon->pos.x - bar->brsize;
	else 
		x = bar->hgap - bar->brsize;
	switch(bar->position){
		case YA_TOP:{
			if ((ya.gen_flag & GEN_RANDR))
				y = bar->vgap + bar->mon->pos.y;
			else
				y = bar->vgap;
			break;
		} 
		case YA_BOTTOM: {
			if ((ya.gen_flag & GEN_RANDR))
				y = bar->mon->pos.height - bar->vgap - bar->height - 2*bar->brsize;
			else
				y = ya.scr->height_in_pixels - bar->vgap - bar->height - 2*bar->brsize;
			break;
		}
	}
	uint32_t w_mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
	uint32_t w_val[] = {bar->bgcolor, bar->brcolor, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_BUTTON_PRESS, ya.colormap};
	xcb_create_window(ya.c,
			ya.depth,
			bar->win,
			ya.scr->root,
			x,y,
			bar->width, bar->height,
			bar->brsize,
			XCB_WINDOW_CLASS_INPUT_OUTPUT,
			ya.visualtype->visual_id,
			w_mask,
			w_val);
	ya_setup_ewmh(bar);
}

xcb_visualtype_t * ya_get_visualtype() {
	xcb_depth_iterator_t depth_iter;
	depth_iter = xcb_screen_allowed_depths_iterator (ya.scr);
	xcb_visualtype_iterator_t visual_iter;
	for (; depth_iter.rem; xcb_depth_next (&depth_iter)) {
		if(depth_iter.data->depth == ya.depth) {
			visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
			return visual_iter.data;
		}
	}
	return NULL;
}

void ya_draw_pango_text(struct ya_block *blk) {
	xcb_poly_fill_rectangle(ya.c, blk->pixmap, blk->gc, 1, (const xcb_rectangle_t[]) { {0,0,blk->width, blk->bar->height} });
	cairo_surface_t *surface = cairo_xcb_surface_create(ya.c, blk->pixmap, ya.visualtype, blk->width, blk->bar->height);
	cairo_t *cr = cairo_create(surface);
	PangoContext *context = pango_cairo_create_context(cr);
	PangoLayout *layout = pango_layout_new(context);
	pango_layout_set_font_description(layout, blk->bar->desc);

	cairo_set_source_rgba(cr, 
			GET_RED(blk->fgcolor), 
			GET_BLUE(blk->fgcolor),
			GET_GREEN(blk->fgcolor),
			GET_ALPHA(blk->fgcolor));

#ifdef YA_DYN_COL
	if (!(blk->attr & BLKA_MARKUP_PANGO))
		pango_layout_set_text(layout, blk->strbuf, strlen(blk->strbuf));
	else
		pango_layout_set_markup(layout, blk->strbuf, strlen(blk->strbuf));
#else
	if (!(blk->attr & BLKA_MARKUP_PANGO))
		pango_layout_set_text(layout, blk->buf, strlen(blk->buf));
	else
		pango_layout_set_markup(layout, blk->buf, strlen(blk->buf));
#endif
	pango_layout_set_alignment(layout, blk->justify);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	pango_layout_set_width(layout, blk->width * PANGO_SCALE);
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
	pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_auto_dir(layout, false);

	int wd, ht;
	pango_layout_set_height(layout, blk->bar->height);
	pango_layout_get_pixel_size(layout, &wd, &ht);

	int offset = (blk->bar->height - ht)/2;
	cairo_move_to(cr, 0, offset);
	pango_cairo_show_layout(cr, layout);
	cairo_move_to(cr, 0, offset);

	if(blk->attr & BLKA_OVERLINE) {
		cairo_set_source_rgba(cr, 
			GET_RED(blk->olcolor), 
			GET_BLUE(blk->olcolor),
			GET_GREEN(blk->olcolor),
			GET_ALPHA(blk->olcolor));
		cairo_rectangle(cr, 0, 0, blk->width, blk->bar->olsize);
		cairo_fill(cr);
	}
	if(blk->attr & BLKA_UNDERLINE) {
		cairo_set_source_rgba(cr, 
			GET_RED(blk->ulcolor), 
			GET_BLUE(blk->ulcolor),
			GET_GREEN(blk->ulcolor),
			GET_ALPHA(blk->ulcolor));
		cairo_rectangle(cr, 0, blk->bar->height - blk->bar->ulsize, blk->width, blk->bar->ulsize);
		cairo_fill(cr);
	}

	cairo_surface_flush(surface);
	xcb_copy_area(ya.c, blk->pixmap, blk->bar->win, blk->gc, 0,0,blk->shift, 0, blk->width, blk->bar->height);
	xcb_flush(ya.c);
	
	g_object_unref(layout);
	g_object_unref(context);
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}


void ya_handle_button( xcb_button_press_event_t *eb) {
	ya_bar_t *curbar = ya.curbar;
	ya_block_t *curblk;
	for (;curbar; curbar = curbar->next_bar) {
		if ( curbar->win == eb->event) {
			for(int align = 0; align < 3; align++) {
				for(curblk = curbar->curblk[align]; curblk; curblk = curblk->next_blk) {
					if ((curblk->shift <= eb->event_x) && ((curblk->shift + curblk->width) >= eb->event_x)) {
						if(curblk->button_cmd[eb->detail-1]) {
							ya_exec_button(curblk, eb);
							return;
						}
					}
				}
			}	
			//If block button not found or not defined, execute button for bar(if defined).
			//We reach this path only if function has not yet returned because no blk has been found.
			if(curbar->button_cmd[eb->detail-1]) {
				if(fork()==0) {
					execl("/bin/sh", "/bin/sh", "-c", curbar->button_cmd[eb->detail-1], (char *) NULL);
					_exit(EXIT_SUCCESS);
				}
				else {
					wait(NULL);
				}
			}
			return;
		}
	}
}

#ifdef YA_DYN_COL
void ya_buf_color_parse(ya_block_t *blk) {
	char *cur = blk->buf;
	char *end;
	int offset = 0;
	if(cur[0] != '!' && cur[1] != 'Y') {
		blk->strbuf = cur;
		blk->bgcolor = blk->bgcolor_old;
		xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){blk->bgcolor});
		blk->fgcolor = blk->fgcolor_old;
		blk->ulcolor = blk->ulcolor_old;
		blk->olcolor = blk->olcolor_old;
		return;
	}
	cur+=2;
	offset+=2;
	for(; *cur != 'Y'; cur++) {
		if((cur[0]=='B' && cur[1]=='G') || (cur[0]=='b' && cur[1]=='g')) {
			cur+=2;
			offset+=2;
			blk->bgcolor = strtoul(cur, &end, 16);
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){blk->bgcolor});
			offset+=(end-cur);
		}
		if((cur[0]=='F' && cur[1]=='G') || (cur[0]=='f' && cur[1]=='g')) {
			cur+=2;
			offset+=2;
			blk->fgcolor = strtoul(cur, &end, 16);
			offset+=(end-cur);
		}
		else if ((cur[0]=='U') || (cur[0]=='u')) {
			cur++;
			offset++;
			blk->ulcolor = strtoul(cur, &end, 16);
			offset+=(end-cur);
		}
		else if ((cur[0]=='O') || (cur[0]=='o')) {
			cur++;
			offset++;
			blk->olcolor = strtoul(cur, &end, 16);
			offset+=(end-cur);
		}
		else if (*cur == ' '){
			offset++;
		}
	}
	if(cur[0]=='Y' && cur[1]=='!') {
		blk->strbuf =  blk->buf+offset+2;
	}
}
#endif //YA_DYN_COL

#ifdef YA_INTERNAL_EWMH
inline void ya_get_cur_window_title(ya_block_t * blk) {
	xcb_ewmh_get_utf8_strings_reply_t reply;
	if(ya.curwin == XCB_NONE)
		blk->buf[0] = '\0';
	else {
		xcb_get_property_cookie_t wm_ck, wmvis_ck;
		wm_ck = xcb_ewmh_get_wm_name(ya.ewmh, ya.curwin);
		wmvis_ck = xcb_ewmh_get_wm_visible_name(ya.ewmh, ya.curwin);
		if (xcb_ewmh_get_wm_name_reply(ya.ewmh, wm_ck, &reply, NULL) == 1 || xcb_ewmh_get_wm_visible_name_reply(ya.ewmh, wmvis_ck, &reply, NULL) == 1) {
			strncpy(blk->buf, reply.strings, reply.strings_len);
			blk->buf[reply.strings_len]='\0';
		}
		xcb_ewmh_get_utf8_strings_reply_wipe(&reply);
	}

}
#endif //YA_INTERNAL_EWMH
