/*  This file is part of emerald

    Copyright (C) 2006  Novell Inc.
    Copyright (C) 2013  Povilas Kanapickas <povilas@radix.lt>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "decor.h"
#include "cairo_utils.h"
#include "quads.h"
#include "utils.h"
#include "window.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xregion.h>

#include <libengine/emerald.h>
#include <libengine/libengine.h>

static unsigned draw_idle_id = 0;

void reset_buttons_bg_and_fade(decor_t* d)
{
    d->draw_only_buttons_region = false;
    d->button_fade_info.cr = NULL;
    d->button_fade_info.timer = -1;
    int b_t;

    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        d->button_fade_info.counters[b_t] = 0;
        d->button_fade_info.pulsating[b_t] = 0;
        d->button_region[b_t].base_x1 = -100;
        d->button_region[b_t].glow_x1 = -100;
        if (d->button_region[b_t].bg_pixmap) {
            g_object_unref(G_OBJECT(d->button_region[b_t].bg_pixmap));
        }
        d->button_region[b_t].bg_pixmap = NULL;
        d->button_region_inact[b_t].base_x1 = -100;
        d->button_region_inact[b_t].glow_x1 = -100;
        if (d->button_region_inact[b_t].bg_pixmap) {
            g_object_unref(G_OBJECT(d->button_region_inact[b_t].bg_pixmap));
        }
        d->button_region_inact[b_t].bg_pixmap = NULL;
        d->button_last_drawn_state[b_t] = 0;
    }
}

void stop_button_fade(decor_t* d)
{
    int j;

    if (d->button_fade_info.cr) {
        cairo_destroy(d->button_fade_info.cr);
        d->button_fade_info.cr = NULL;
    }
    if (d->button_fade_info.timer >= 0) {
        g_source_remove(d->button_fade_info.timer);
        d->button_fade_info.timer = -1;
    }
    for (j = 0; j < B_T_COUNT; j++) {
        d->button_fade_info.counters[j] = 0;
    }
}

void draw_button_backgrounds(decor_t* d, int* necessary_update_type)
{
    int b_t;
    window_settings* ws = d->fs->ws;

    // Draw button backgrounds
    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        if (BUTTON_NOT_VISIBLE(d, b_t)) {
            continue;
        }
        button_region_t* button_region = (d->active ? &d->button_region[b_t] :
                                          &d->button_region_inact[b_t]);
        int src_x = 0, src_y = 0, w = 0, h = 0, dest_x = 0, dest_y = 0;

        if (necessary_update_type[b_t] == 1) {
            w = button_region->base_x2 - button_region->base_x1;
            h = button_region->base_y2 - button_region->base_y1;
            if (ws->use_pixmap_buttons) {
                dest_x = button_region->base_x1;
                dest_y = button_region->base_y1;
                if ((ws->use_button_glow && d->active) ||
                        (ws->use_button_inactive_glow && !d->active)) {
                    src_x = button_region->base_x1 - button_region->glow_x1;
                    src_y = button_region->base_y1 - button_region->glow_y1;
                }
            } else {
                dest_x = button_region->base_x1 - 2;
                dest_y = button_region->base_y1 + 1;
            }
        } else if (necessary_update_type[b_t] == 2) {
            dest_x = button_region->glow_x1;
            dest_y = button_region->glow_y1;
            w = button_region->glow_x2 - button_region->glow_x1;
            h = button_region->glow_y2 - button_region->glow_y1;
        } else {
            return;
        }
        if (button_region->bg_pixmap)
            gdk_draw_drawable(IS_VALID(d->buffer_pixmap) ? d->buffer_pixmap :
                              d->pixmap,
                              d->gc, button_region->bg_pixmap, src_x, src_y,
                              dest_x, dest_y, w, h);
        d->min_drawn_buttons_region.x1 =
            MIN(d->min_drawn_buttons_region.x1, dest_x);
        d->min_drawn_buttons_region.y1 =
            MIN(d->min_drawn_buttons_region.y1, dest_y);
        d->min_drawn_buttons_region.x2 =
            MAX(d->min_drawn_buttons_region.x2, dest_x + w);
        d->min_drawn_buttons_region.y2 =
            MAX(d->min_drawn_buttons_region.y2, dest_y + h);
    }
}

void update_button_regions(decor_t* d)
{
    window_settings* ws = d->fs->ws;
    int y1 = ws->top_space - ws->win_extents.top;

    int b_t, b_t2;
    double x, y;
    double glow_x, glow_y;                // glow top left coordinates

    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        if (BUTTON_NOT_VISIBLE(d, b_t)) {
            continue;
        }
        button_region_t* button_region = &(d->button_region[b_t]);

        if (button_region->bg_pixmap) {
            g_object_unref(G_OBJECT(button_region->bg_pixmap));
            button_region->bg_pixmap = NULL;
        }
        if (d->button_region_inact[b_t].bg_pixmap) {
            g_object_unref(G_OBJECT(d->button_region_inact[b_t].bg_pixmap));
            d->button_region_inact[b_t].bg_pixmap = NULL;
        }
        // Reset overlaps
        for (b_t2 = 0; b_t2 < b_t; b_t2++)
            if (!BUTTON_NOT_VISIBLE(d, b_t2)) {
                d->button_region[b_t].overlap_buttons[b_t2] = false;
            }
        for (b_t2 = 0; b_t2 < b_t; b_t2++)
            if (!BUTTON_NOT_VISIBLE(d, b_t2)) {
                d->button_region_inact[b_t].overlap_buttons[b_t2] = false;
            }
    }
    d->button_fade_info.first_draw = true;

    if (ws->use_pixmap_buttons) {
        if ((d->active && ws->use_button_glow) ||
                (!d->active && ws->use_button_inactive_glow)) {
            for (b_t = 0; b_t < B_T_COUNT; b_t++) {
                if (BUTTON_NOT_VISIBLE(d, b_t)) {
                    continue;
                }
                get_button_pos(ws, b_t, d, y1, &x, &y);
                button_region_t* button_region = &(d->button_region[b_t]);

                glow_x = x - (ws->c_glow_size.w - ws->c_icon_size[b_t].w) / 2;
                glow_y = y - (ws->c_glow_size.h - ws->c_icon_size[b_t].h) / 2;

                button_region->base_x1 = x;
                button_region->base_y1 = y;
                button_region->base_x2 = x + ws->c_icon_size[b_t].w;
                button_region->base_y2 = MIN(y + ws->c_icon_size[b_t].h,
                                             ws->top_space +
                                             ws->titlebar_height);

                button_region->glow_x1 = glow_x;
                button_region->glow_y1 = glow_y;
                button_region->glow_x2 = glow_x + ws->c_glow_size.w;
                button_region->glow_y2 = MIN(glow_y + ws->c_glow_size.h,
                                             ws->top_space +
                                             ws->titlebar_height);

                // Update glow overlaps of each pair

                for (b_t2 = 0; b_t2 < b_t; b_t2++) {
                    // coordinates for these b_t2's will be ready for this b_t here
                    if (BUTTON_NOT_VISIBLE(d, b_t2)) {
                        continue;
                    }
                    if ((button_region->base_x1 > d->button_region[b_t2].base_x1 &&        //right of b_t2
                            button_region->glow_x1 <= d->button_region[b_t2].base_x2) || (button_region->base_x1 < d->button_region[b_t2].base_x1 &&        //left of b_t2
                                    button_region->
                                    glow_x2
                                    >=
                                    d->
                                    button_region
                                    [b_t2].
                                    base_x1)) {
                        button_region->overlap_buttons[b_t2] = true;
                    } else {
                        button_region->overlap_buttons[b_t2] = false;
                    }

                    // buttons' protruding glow length might be asymmetric
                    if ((d->button_region[b_t2].base_x1 > button_region->base_x1 &&        //left of b_t2
                            d->button_region[b_t2].glow_x1 <= button_region->base_x2) || (d->button_region[b_t2].base_x1 < button_region->base_x1 &&        //right of b_t2
                                    d->
                                    button_region
                                    [b_t2].
                                    glow_x2
                                    >=
                                    button_region->
                                    base_x1)) {
                        d->button_region[b_t2].overlap_buttons[b_t] = true;
                    } else {
                        d->button_region[b_t2].overlap_buttons[b_t] = false;
                    }
                }
            }
        } else {
            for (b_t = 0; b_t < B_T_COUNT; b_t++) {
                if (BUTTON_NOT_VISIBLE(d, b_t)) {
                    continue;
                }
                get_button_pos(ws, b_t, d, y1, &x, &y);
                button_region_t* button_region = &(d->button_region[b_t]);

                button_region->base_x1 = x;
                button_region->base_y1 = y;
                button_region->base_x2 = x + ws->c_icon_size[b_t].w;
                button_region->base_y2 = MIN(y + ws->c_icon_size[b_t].h,
                                             ws->top_space +
                                             ws->titlebar_height);
            }
        }
    } else {
        for (b_t = 0; b_t < B_T_COUNT; b_t++) {
            if (BUTTON_NOT_VISIBLE(d, b_t)) {
                continue;
            }
            get_button_pos(ws, b_t, d, y1, &x, &y);
            button_region_t* button_region = &(d->button_region[b_t]);

            button_region->base_x1 = x;
            button_region->base_y1 = y;
            button_region->base_x2 = x + 16;
            button_region->base_y2 = y + 16;
        }
    }
    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        button_region_t* button_region = &(d->button_region[b_t]);
        button_region_t* button_region_inact = &(d->button_region_inact[b_t]);

        memcpy(button_region_inact, button_region, sizeof(button_region_t));
    }
}
static void draw_window_decoration_real(decor_t* d, bool shadow_time)
{
    cairo_t* cr;
    frame_settings* fs = d->fs;
    window_settings* ws = fs->ws;

    if (!d->pixmap) {
        return;
    }

    double y1 = ws->top_space - ws->win_extents.top;

    if (!d->draw_only_buttons_region) {      // if not only drawing buttons
        cr = gdk_cairo_create(GDK_DRAWABLE
                              (IS_VALID(d->buffer_pixmap) ? d->buffer_pixmap :
                               d->pixmap));
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_line_width(cr, 1.0);
        cairo_save(cr);
        draw_shadow_background(d, cr);
        engine_draw_frame(d, cr);
        cairo_restore(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_set_line_width(cr, 2.0);

        /*color.r = 1;
          color.g = 1;
          color.b = 1; */

        // the buttons were previously drawn here, so we need to save the cairo state here
        cairo_save(cr);

        if (d->layout && d->tobj_item_state[TBT_TITLE] != 3) {
            pango_layout_set_alignment(d->layout, ws->title_text_align);
            cairo_move_to(cr,
                          get_real_pos(ws, TBT_TITLE, d),
                          y1 + 2.0 + (ws->titlebar_height -
                                      ws->text_height) / 2.0);

            /* ===================active text colors */
            cairo_set_source_alpha_color(cr, &fs->text_halo);
            pango_cairo_layout_path(cr, d->layout);
            cairo_stroke(cr);

            cairo_set_source_alpha_color(cr, &fs->text);

            cairo_move_to(cr,
                          get_real_pos(ws, TBT_TITLE, d),
                          y1 + 2.0 + (ws->titlebar_height -
                                      ws->text_height) / 2.0);

            pango_cairo_show_layout(cr, d->layout);
        }
        if (d->icon && d->tobj_item_state[TBT_ICON] != 3) {
            cairo_translate(cr, get_real_pos(ws, TBT_ICON, d),
                            y1 - 5.0 + ws->titlebar_height / 2);

            cairo_set_source(cr, d->icon);
            cairo_rectangle(cr, 0.0, 0.0, 16.0, 16.0);
            cairo_clip(cr);
            cairo_paint(cr);
        }
        // Copy button region backgrounds to buffers
        // for fast drawing of buttons from now on
        // when drawing is done for buttons
        bool bg_pixmaps_update_needed = false;
        int b_t;

        for (b_t = 0; b_t < B_T_COUNT; b_t++) {
            button_region_t* button_region =
                (d->active ? &d->button_region[b_t] : &d->
                 button_region_inact[b_t]);
            if (BUTTON_NOT_VISIBLE(d, b_t)) {
                continue;
            }
            if (!button_region->bg_pixmap && button_region->base_x1 >= 0) {      // if region is valid
                bg_pixmaps_update_needed = true;
                break;
            }
        }
        if (bg_pixmaps_update_needed && !shadow_time) {
            for (b_t = 0; b_t < B_T_COUNT; b_t++) {
                if (BUTTON_NOT_VISIBLE(d, b_t)) {
                    continue;
                }

                button_region_t* button_region =
                    (d->active ? &d->button_region[b_t] : &d->
                     button_region_inact[b_t]);
                int rx, ry, rw, rh;

                if (ws->use_pixmap_buttons &&
                        ((ws->use_button_glow && d->active) ||
                         (ws->use_button_inactive_glow && !d->active))) {
                    if (button_region->glow_x1 == -100) {      // skip uninitialized regions
                        continue;
                    }
                    rx = button_region->glow_x1;
                    ry = button_region->glow_y1;
                    rw = button_region->glow_x2 - button_region->glow_x1;
                    rh = button_region->glow_y2 - button_region->glow_y1;
                } else {
                    if (button_region->base_x1 == -100) {      // skip uninitialized regions
                        continue;
                    }
                    rx = button_region->base_x1;
                    ry = button_region->base_y1;
                    if (!ws->use_pixmap_buttons) {      // offset: (-2,1)
                        rx -= 2;
                        ry++;
                    }
                    rw = button_region->base_x2 - button_region->base_x1;
                    rh = button_region->base_y2 - button_region->base_y1;
                }
                if (!button_region->bg_pixmap) {
                    button_region->bg_pixmap = create_pixmap(rw, rh);
                }
                if (!button_region->bg_pixmap) {
                    std::cerr << program_name << ": Error allocating buffer.\n";
                } else {
                    gdk_draw_drawable(button_region->bg_pixmap, d->gc,
                                      IS_VALID(d->buffer_pixmap) ?
                                      d->buffer_pixmap : d->pixmap,
                                      rx, ry, 0, 0,
                                      rw, rh);
                }
            }
        }
        cairo_restore(cr);                // and restore the state for button drawing
        /*if (!shadow_time)
          {
        //workaround for slowness, will grab and rotate the two side-pieces
        int w, h;
        cairo_surface_t * csur;
        cairo_pattern_t * sr;
        cairo_matrix_t cm;
        cairo_destroy(cr);
        int topspace = ws->top_space + ws->titlebar_height;
        cr = gdk_cairo_create (GDK_DRAWABLE (d->buffer_pixmap ? d->buffer_pixmap : d->pixmap));
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

        gdk_drawable_get_size(pbuff,&w,&h);
        csur = cairo_xlib_surface_create(
        GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
        GDK_PIXMAP_XID(pbuff),
        GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(pbuff)),
        w,h);

        cairo_set_source_surface(cr, csur, 0, 0);
        sr = cairo_get_source(cr);
        cairo_pattern_get_matrix(sr, &cm);

        //draw all four quads from the old one to the new one
        //first top quad
        cairo_save(cr);
        cairo_rectangle(cr, 0, 0, d->width, topspace);
        cairo_clip(cr);
        cairo_pattern_set_matrix(sr, &cm);
        cairo_paint(cr);
        cairo_restore(cr);

        //then bottom, easiest this way
        cairo_save(cr);
        cairo_rectangle(cr, 0, topspace, d->width, ws->bottom_space);
        cairo_clip(cr);
        cm.y0 = d->height - (top_space + ws->bottom_space);
        cm.x0 = 0;
        cairo_pattern_set_matrix(sr,&cm);
        cairo_paint(cr);
        cairo_restore(cr);

        //now left
        cairo_save(cr);
        cairo_rectangle(cr, 0, topspace + ws->bottom_space,
        d->height-(topspace + ws->bottom_space), ws->left_space);
        cairo_clip(cr);
        cm.xx=0;
        cm.xy=1;
        cm.yx=1;
        cm.yy=0;
        cm.x0 = - topspace - ws->bottom_space;
        cm.y0 = topspace;
        cairo_pattern_set_matrix(sr,&cm);
        cairo_paint(cr);
        cairo_restore(cr);

        //now right
        cairo_save(cr);
        cairo_rectangle(cr, 0, topspace + ws->bottom_space + ws->left_space,
        d->height-(topspace + ws->bottom_space), ws->right_space);
        cairo_clip(cr);
        cm.y0 = topspace;
        cm.x0 = d->width-
        (topspace + ws->bottom_space + ws->left_space + ws->right_space);
        cairo_pattern_set_matrix(sr,&cm);
        cairo_paint(cr);
        cairo_restore(cr);


        cairo_destroy(cr);
        g_object_unref (G_OBJECT (pbuff));
        cairo_surface_destroy(csur);
        }
        */
    }
    // Draw buttons

    cr = gdk_cairo_create(GDK_DRAWABLE(IS_VALID(d->buffer_pixmap) ?
                                       d->buffer_pixmap : d->pixmap));

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    if (ws->use_button_fade && ws->use_pixmap_buttons) {
        draw_buttons_with_fade(d, cr, y1);
    } else {
        draw_buttons_without_fade(d, cr, y1);
    }

    cairo_destroy(cr);

    if (IS_VALID(d->buffer_pixmap)) {
        /*if (d->draw_only_buttons_region && d->min_drawn_buttons_region.x1 < 10000)        // if region is updated at least once
          {
          gdk_draw_drawable(d->pixmap,
          d->gc,
          d->buffer_pixmap,
          d->min_drawn_buttons_region.x1,
          d->min_drawn_buttons_region.y1,
          d->min_drawn_buttons_region.x1,
          d->min_drawn_buttons_region.y1,
          d->min_drawn_buttons_region.x2 -
          d->min_drawn_buttons_region.x1,
          d->min_drawn_buttons_region.y2 -
          d->min_drawn_buttons_region.y1);
          }
          else*/
        {
            gdk_draw_drawable(d->pixmap,
                              d->gc,
                              d->buffer_pixmap,
                              0,
                              0,
                              0,
                              0,
                              d->width,
                              d->height);
            //ws->top_space + ws->bottom_space +
            //ws->titlebar_height + 2);
        }
    }
}

void draw_window_decoration(decor_t* d)
{
    if (d->active) {
        d->pixmap = d->p_active;
        d->buffer_pixmap = d->p_active_buffer;
    } else {
        d->pixmap = d->p_inactive;
        d->buffer_pixmap = d->p_inactive_buffer;
    }
    if (d->draw_only_buttons_region) {
        draw_window_decoration_real(d, false);
    }
    if (!d->only_change_active) {
        bool save = d->active;
        frame_settings* fs = d->fs;

        d->active = true;
        d->fs = d->fs->ws->fs_act;
        d->pixmap = d->p_active;
        d->buffer_pixmap = d->p_active_buffer;
        draw_window_decoration_real(d, false);
        d->active = false;
        d->fs = d->fs->ws->fs_inact;
        d->pixmap = d->p_inactive;
        d->buffer_pixmap = d->p_inactive_buffer;
        draw_window_decoration_real(d, false);
        d->active = save;
        d->fs = fs;
    } else {
        d->only_change_active = false;
    }
    if (d->active) {
        d->pixmap = d->p_active;
        d->buffer_pixmap = d->p_active_buffer;
    } else {
        d->pixmap = d->p_inactive;
        d->buffer_pixmap = d->p_inactive_buffer;
    }
    if (d->prop_xid) {
        decor_update_window_property(d);
        d->prop_xid = 0;
    }
    d->draw_only_buttons_region = false;
}
void draw_shadow_window(decor_t* d)
{
    draw_window_decoration_real(d, true);
}

#define SWITCHER_ALPHA 0xa0a0

void decor_update_switcher_property(decor_t* d)
{
    long* data = NULL;
    Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    unsigned int nQuad;
    decor_quad_t quads[N_QUADS_MAX];
    window_settings* ws = d->fs->ws;
    decor_extents_t extents = ws->switcher_extents;
    GtkStyle* style;
    long fgColor[4];

    nQuad = set_switcher_quads(quads, d->width, d->height, ws);

    data = decor_alloc_property(1, WINDOW_DECORATION_TYPE_PIXMAP);

    decor_quads_to_property(data, 0, GDK_PIXMAP_XID(d->pixmap),
                            &extents, &extents, &extents, &extents,
                            0, 0, quads, nQuad, 0xffffff, 0, 0);

    style = gtk_widget_get_style(style_window);

    fgColor[0] = style->fg[GTK_STATE_NORMAL].red;
    fgColor[1] = style->fg[GTK_STATE_NORMAL].green;
    fgColor[2] = style->fg[GTK_STATE_NORMAL].blue;
    fgColor[3] = SWITCHER_ALPHA;

    gdk_error_trap_push();
    XChangeProperty(xdisplay, d->prop_xid,
                    win_decor_atom,
                    XA_INTEGER,
                    32, PropModeReplace, (guchar*) data,
                    PROP_HEADER_SIZE + BASE_PROP_SIZE + QUAD_PROP_SIZE * N_QUADS_MAX);
    XChangeProperty(xdisplay, d->prop_xid, switcher_fg_atom,
                    XA_INTEGER, 32, PropModeReplace, (guchar*) fgColor, 4);
    XSync(xdisplay, false);
    gdk_error_trap_pop();
    if (data) {
        free(data);
    }
}

void draw_switcher_background(decor_t* d)
{
    Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    cairo_t* cr;
    GtkStyle* style;
    decor_color_t color;
    alpha_color acolor;
    alpha_color acolor2;
    double alpha = SWITCHER_ALPHA / 65535.0;
    double x1, y1, x2, y2, h;
    int top;
    unsigned long pixel;
    ushort a = SWITCHER_ALPHA;
    window_settings* ws = d->fs->ws;

    if (!IS_VALID(d->buffer_pixmap)) {
        return;
    }

    style = gtk_widget_get_style(style_window);

    color.r = style->bg[GTK_STATE_NORMAL].red / 65535.0;
    color.g = style->bg[GTK_STATE_NORMAL].green / 65535.0;
    color.b = style->bg[GTK_STATE_NORMAL].blue / 65535.0;
    acolor.color = acolor2.color = color;
    acolor.alpha = alpha;
    acolor2.alpha = alpha * 0.75;

    cr = gdk_cairo_create(GDK_DRAWABLE(d->buffer_pixmap));

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    top = ws->win_extents.bottom;

    x1 = ws->left_space - ws->win_extents.left;
    y1 = ws->top_space - ws->win_extents.top;
    x2 = d->width - ws->right_space + ws->win_extents.right;
    y2 = d->height - ws->bottom_space + ws->win_extents.bottom;

    h = y2 - y1 - ws->win_extents.bottom - ws->win_extents.bottom;

    cairo_set_line_width(cr, 1.0);

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    if (d->prop_xid || !IS_VALID(d->buffer_pixmap)) {
        draw_shadow_background(d, cr);
    }

    fill_rounded_rectangle(cr,
                           x1 + 0.5,
                           y1 + 0.5,
                           ws->win_extents.left - 0.5,
                           top - 0.5,
                           CORNER_TOPLEFT,
                           &acolor, &acolor2,
                           SHADE_TOP | SHADE_LEFT, ws, 5.0);

    fill_rounded_rectangle(cr,
                           x1 + ws->win_extents.left,
                           y1 + 0.5,
                           x2 - x1 - ws->win_extents.left -
                           ws->win_extents.right,
                           top - 0.5,
                           0, &acolor, &acolor2, SHADE_TOP, ws, 5.0);

    fill_rounded_rectangle(cr,
                           x2 - ws->win_extents.right,
                           y1 + 0.5,
                           ws->win_extents.right - 0.5,
                           top - 0.5,
                           CORNER_TOPRIGHT,
                           &acolor, &acolor2,
                           SHADE_TOP | SHADE_RIGHT, ws, 5.0);

    fill_rounded_rectangle(cr,
                           x1 + 0.5,
                           y1 + top,
                           ws->win_extents.left - 0.5,
                           h, 0, &acolor, &acolor2, SHADE_LEFT, ws, 5.0);

    fill_rounded_rectangle(cr,
                           x2 - ws->win_extents.right,
                           y1 + top,
                           ws->win_extents.right - 0.5,
                           h, 0, &acolor, &acolor2, SHADE_RIGHT, ws, 5.0);

    fill_rounded_rectangle(cr,
                           x1 + 0.5,
                           y2 - ws->win_extents.bottom,
                           ws->win_extents.left - 0.5,
                           ws->win_extents.bottom - 0.5,
                           CORNER_BOTTOMLEFT,
                           &acolor, &acolor2,
                           SHADE_BOTTOM | SHADE_LEFT, ws, 5.0);

    fill_rounded_rectangle(cr,
                           x1 + ws->win_extents.left,
                           y2 - ws->win_extents.bottom,
                           x2 - x1 - ws->win_extents.left -
                           ws->win_extents.right,
                           ws->win_extents.bottom - 0.5,
                           0, &acolor, &acolor2, SHADE_BOTTOM, ws, 5.0);

    fill_rounded_rectangle(cr,
                           x2 - ws->win_extents.right,
                           y2 - ws->win_extents.bottom,
                           ws->win_extents.right - 0.5,
                           ws->win_extents.bottom - 0.5,
                           CORNER_BOTTOMRIGHT,
                           &acolor, &acolor2,
                           SHADE_BOTTOM | SHADE_RIGHT, ws, 5.0);

    cairo_rectangle(cr, x1 + ws->win_extents.left,
                    y1 + top,
                    x2 - x1 - ws->win_extents.left - ws->win_extents.right,
                    h);
    gdk_cairo_set_source_color_alpha(cr, &style->bg[GTK_STATE_NORMAL], alpha);
    cairo_fill(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_identity_matrix(cr);
    cairo_rectangle(cr, x1 + 1, y1 + 1,
                    x2 - x1 - 1.0,
                    y2 - y1 - 1.0);



    cairo_clip(cr);

    cairo_translate(cr, 1.0, 1.0);

    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                      CORNER_BOTTOMRIGHT, ws, 5.0);

    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
    cairo_stroke(cr);

    cairo_translate(cr, -2.0, -2.0);

    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                      CORNER_BOTTOMRIGHT, ws, 5.0);

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.1);
    cairo_stroke(cr);

    cairo_translate(cr, 1.0, 1.0);

    cairo_reset_clip(cr);

    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                      CORNER_BOTTOMRIGHT, ws, 5.0);

    gdk_cairo_set_source_color_alpha(cr, &style->fg[GTK_STATE_NORMAL], alpha);

    cairo_stroke(cr);

    cairo_destroy(cr);

    gdk_draw_drawable(d->pixmap,
                      d->gc,
                      d->buffer_pixmap, 0, 0, 0, 0, d->width, d->height);

    pixel = ((((a * style->bg[GTK_STATE_NORMAL].red) >> 24) & 0x0000ff) |
             (((a * style->bg[GTK_STATE_NORMAL].green) >> 16) & 0x00ff00) |
             (((a * style->bg[GTK_STATE_NORMAL].blue) >> 8) & 0xff0000) |
             (((a & 0xff00) << 16)));

    decor_update_switcher_property(d);

    gdk_error_trap_push();
    XSetWindowBackground(xdisplay, d->prop_xid, pixel);
    XClearWindow(xdisplay, d->prop_xid);
    XSync(xdisplay, false);
    gdk_error_trap_pop();

    d->prop_xid = 0;
}

static void draw_switcher_foreground(decor_t* d)
{
    if (!IS_VALID(d->pixmap) || !IS_VALID(d->buffer_pixmap)) {
        return;
    }

    double alpha = SWITCHER_ALPHA / 65535.0;
    window_settings* ws = d->fs->ws;

    GtkStyle* style = gtk_widget_get_style(style_window);

    //decor_color_t color;

    //color.r = style->bg[GTK_STATE_NORMAL].red / 65535.0;
    //color.g = style->bg[GTK_STATE_NORMAL].green / 65535.0;
    //color.b = style->bg[GTK_STATE_NORMAL].blue / 65535.0;

    int top = ws->win_extents.bottom;

    double x1 = ws->left_space - ws->win_extents.left;
    double y1 = ws->top_space - ws->win_extents.top;
    double x2 = d->width - ws->right_space + ws->win_extents.right;

    cairo_t* cr = gdk_cairo_create(GDK_DRAWABLE(d->buffer_pixmap));

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    cairo_rectangle(cr, x1 + ws->win_extents.left,
                    y1 + top + ws->switcher_top_corner_space,
                    x2 - x1 - ws->win_extents.left - ws->win_extents.right,
                    SWITCHER_SPACE);

    gdk_cairo_set_source_color_alpha(cr, &style->bg[GTK_STATE_NORMAL], alpha);
    cairo_fill(cr);

    if (d->layout) {
        int w;
        int text_width;
        int text_len = 0;
        const char* text = NULL;
        const int SWITCHER_SUBST_FONT_SIZE = 10;

        pango_layout_get_pixel_size(d->layout, &text_width, NULL);
        text = pango_layout_get_text(d->layout);
        if (text != NULL) {
            text_len = strlen(text);
        } else {
            return;
        }

        // some themes ("frame" e.g.) set the title text font to 0.0pt, try to fix it
        if (text_width == 0) {
            if (text_len) {
                // if the font size is set to 0 indeed, set it to 10, otherwise don't draw anything

                if (!pango_layout_get_font_description(d->layout)) {
                    PangoContext* context =
                        pango_layout_get_context(d->layout);
                    PangoFontDescription* font =
                        pango_context_get_font_description(context);
                    if (font == NULL) {
                        return;
                    }

                    int font_size = pango_font_description_get_size(font);

                    if (font_size == 0) {
                        pango_font_description_set_size(font,
                                                        SWITCHER_SUBST_FONT_SIZE
                                                        * PANGO_SCALE);
                        pango_layout_get_pixel_size(d->layout, &text_width,
                                                    NULL);
                    } else {
                        return;
                    }
                }
            } else {
                return;
            }
        }

        // fix too long title text in switcher
        // using ellipsize instead of cutting off text
        pango_layout_set_ellipsize(d->layout, PANGO_ELLIPSIZE_END);
        pango_layout_set_width(d->layout, (x2 - x1) * PANGO_SCALE);

        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

        gdk_cairo_set_source_color_alpha(cr,
                                         &style->fg[GTK_STATE_NORMAL], 1.0);

        pango_layout_get_pixel_size(d->layout, &w, NULL);

        cairo_move_to(cr, d->width / 2 - w / 2,
                      y1 + top + ws->switcher_top_corner_space +
                      SWITCHER_SPACE / 2 - ws->text_height / 2);

        pango_cairo_show_layout(cr, d->layout);
    }

    cairo_destroy(cr);

    gdk_draw_drawable(d->pixmap,
                      d->gc,
                      d->buffer_pixmap, 0, 0, 0, 0, d->width, d->height);
}

void draw_switcher_decoration(decor_t* d)
{
    if (d->prop_xid) {
        draw_switcher_background(d);
    }

    draw_switcher_foreground(d);
}

void queue_decor_draw_for_buttons(decor_t* d, bool for_buttons)
{
    auto it = std::find(g_draw_list.begin(), g_draw_list.end(), d);
    if (it != g_draw_list.end()) {
        // handle possible previously queued drawing
        if (d->draw_only_buttons_region) {
            // the old drawing request is only for buttons, so override it
            d->draw_only_buttons_region = for_buttons;
        }
        return;
    }

    d->draw_only_buttons_region = for_buttons;

    g_draw_list.push_back(d);

    if (!draw_idle_id) {
        draw_idle_id = g_idle_add(draw_decor_list, NULL);
    }
}

void queue_decor_draw(decor_t* d)
{
    d->only_change_active = false;
    queue_decor_draw_for_buttons(d, false);
}

bool draw_decor_list(void* data)
{
    draw_idle_id = 0;

    for (decor_t* d : g_draw_list) {
        (*d->draw)(d);
    }

    g_draw_list.clear();
    return false;
}


void draw_buttons_with_fade(decor_t* d, cairo_t* cr, double y1)
{
    window_settings* ws = d->fs->ws;
    int b_t;

    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        if (BUTTON_NOT_VISIBLE(d, b_t)) {
            continue;
        }
        if (!(d->active ? d->button_region[b_t] : d->button_region_inact[b_t]).bg_pixmap) {      // don't draw if bg_pixmaps are not valid
            return;
        }
    }
    button_fade_info_t* fade_info = &(d->button_fade_info);
    bool button_pressed = false;

    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        if (BUTTON_NOT_VISIBLE(d, b_t)) {
            continue;
        }
        int b_state = get_b_state(d, b_t);

        if (fade_info->counters[b_t] != 0 &&
                (b_state == S_ACTIVE_PRESS || b_state == S_INACTIVE_PRESS)) {
            // Button pressed, stop fade
            fade_info->counters[b_t] = 0;
            button_pressed = true;
        } else if (fade_info->counters[b_t] > 0 && (b_state == S_ACTIVE || b_state == S_INACTIVE)) {    // moved out
            // Change fade in -> out and proceed 1 step
            fade_info->counters[b_t] =
                1 - MIN(fade_info->counters[b_t],
                        ws->button_fade_num_steps + 1);
        } else if (fade_info->counters[b_t] < 0 &&
                   (b_state == S_ACTIVE_HOVER || b_state == S_INACTIVE_HOVER)) {
            // Change fade out -> in and proceed 1 step
            fade_info->counters[b_t] = 1 - fade_info->counters[b_t];
        } else if (fade_info->counters[b_t] == 0 &&
                   (b_state == S_ACTIVE_HOVER || b_state == S_INACTIVE_HOVER)) {
            // Start fade in
            fade_info->counters[b_t] = 1;
        }
        if (fade_info->pulsating[b_t] &&
                b_state != S_ACTIVE_HOVER && b_state != S_INACTIVE_HOVER) {
            // Stop pulse
            fade_info->pulsating[b_t] = false;
        }
    }

    if (fade_info->timer == -1 || button_pressed)
        // button_pressed is needed because sometimes after a button is pressed,
        // this function is called twice, first with S_(IN)ACTIVE, then with S_(IN)ACTIVE_PRESS
        // where it should have been only once with S_(IN)ACTIVE_PRESS
    {
        fade_info->d = (void*) d;
        fade_info->y1 = y1;
        if (draw_buttons_timer_func((void*) fade_info) == true) {      // call once now
            // and start a new timer for the next step
            fade_info->timer =
                g_timeout_add(ws->button_fade_step_duration,
                              draw_buttons_timer_func,
                              (void*) fade_info);
        }
    }
}

int draw_buttons_timer_func(void* data)
{
    button_fade_info_t* fade_info = (button_fade_info_t*) data;
    decor_t* d = (decor_t*)(fade_info->d);
    window_settings* ws = d->fs->ws;
    int num_steps = ws->button_fade_num_steps;

    /* decorations no longer available? */
    if (!d->buffer_pixmap && !d->pixmap) {
        stop_button_fade(d);
        return false;
    }

    d->min_drawn_buttons_region.x1 = 10000;
    d->min_drawn_buttons_region.y1 = 10000;
    d->min_drawn_buttons_region.x2 = -100;
    d->min_drawn_buttons_region.y2 = -100;

    if (!fade_info->cr) {
        fade_info->cr =
            gdk_cairo_create(GDK_DRAWABLE
                             (IS_VALID(d->buffer_pixmap) ? d->buffer_pixmap :
                              d->pixmap));
        cairo_set_operator(fade_info->cr, CAIRO_OPERATOR_OVER);
    }

    // Determine necessary updates
    int b_t;
    int necessary_update_type[B_T_COUNT];        // 0: none, 1: only base, 2: base+glow

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
        necessary_update_type[b_t] = (ws->use_button_glow && d->active) ||
                                     (ws->use_button_inactive_glow && !d->active) ? 2 : 1;
    draw_button_backgrounds(d, necessary_update_type);

    // Draw the buttons that are in "non-hovered" or pressed state
    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        if (BUTTON_NOT_VISIBLE(d, b_t) || fade_info->counters[b_t] ||
                necessary_update_type[b_t] == 0) {
            continue;
        }
        int b_state = get_b_state(d, b_t);
        int toBeDrawnState =
            (d->
             active ? (b_state == S_ACTIVE_PRESS ? 2 : 0) : (b_state ==
                         S_INACTIVE_PRESS
                         ? 5 : 3));
        draw_button_with_glow_alpha_bstate(b_t, d, fade_info->cr, fade_info->y1, 1.0, 0.0, toBeDrawnState);        // no glow here
    }

    // Draw the buttons that are in "hovered" state (fading in/out or at max fade)
    double button_alphas[B_T_COUNT];

    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        button_alphas[b_t] = 0;
        if (BUTTON_NOT_VISIBLE(d, b_t) ||
                (!fade_info->pulsating[b_t] && !fade_info->counters[b_t])) {
            continue;
        }

        if (ws->button_fade_pulse_len_steps > 0 && fade_info->counters[b_t] &&
                fade_info->pulsating[b_t]) {
            // If it is time, reverse the fade
            if (fade_info->counters[b_t] ==
                    -num_steps + ws->button_fade_pulse_len_steps) {
                fade_info->counters[b_t] = 1 - fade_info->counters[b_t];
            }
            if (fade_info->counters[b_t] ==
                    num_steps + 1 + ws->button_fade_pulse_wait_steps)
                fade_info->counters[b_t] =
                    1 - MIN(fade_info->counters[b_t], num_steps + 1);
        }
        if (ws->button_fade_pulse_len_steps > 0 &&
                fade_info->counters[b_t] == num_steps) {
            fade_info->pulsating[b_t] = true;    // start pulse
        }

        if (fade_info->counters[b_t] != num_steps + 1 ||        // unless fade is at max
                (ws->button_fade_pulse_len_steps > 0 &&        // or at pulse max
                 fade_info->counters[b_t] !=
                 num_steps + 1 + ws->button_fade_pulse_wait_steps)) {
            fade_info->counters[b_t]++;        // increment fade counter
        }
        d->button_last_drawn_state[b_t] = fade_info->counters[b_t];

        double alpha;

        if (fade_info->counters[b_t] > 0)
            alpha = (MIN(fade_info->counters[b_t], num_steps + 1) -
                     1) / (double) num_steps;
        else {
            alpha = -fade_info->counters[b_t] / (double) num_steps;
        }

        if (fade_info->counters[b_t] < num_steps + 1) {      // not at max fade
            // Draw button's non-hovered version (with 1-alpha)
            draw_button_with_glow_alpha_bstate(b_t, d, fade_info->cr,
                                               fade_info->y1,
                                               pow(1 - alpha, 0.4), 0.0,
                                               d->active ? 0 : 3);
        }
        button_alphas[b_t] = alpha;
    }
    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        if (button_alphas[b_t] > 1e-4) {
            double glow_alpha = 0.0;

            if ((ws->use_button_glow && d->active) ||
                    (ws->use_button_inactive_glow && !d->active)) {
                glow_alpha = button_alphas[b_t];
            }

            // Draw button's hovered version (with alpha)
            draw_button_with_glow_alpha_bstate(b_t, d, fade_info->cr,
                                               fade_info->y1,
                                               button_alphas[b_t], glow_alpha,
                                               d->active ? 1 : 4);
        }
    }

    // Check if the fade has come to an end
    bool any_active_buttons = false;

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
        if (!BUTTON_NOT_VISIBLE(d, b_t) &&
                ((fade_info->counters[b_t] &&
                  fade_info->counters[b_t] < num_steps + 1) ||
                 fade_info->pulsating[b_t])) {
            any_active_buttons = true;
            break;
        }

    if (IS_VALID(d->buffer_pixmap) && !d->button_fade_info.first_draw &&
            d->min_drawn_buttons_region.x1 < 10000) {
        // if region is updated at least once
        gdk_draw_drawable(d->pixmap,
                          d->gc,
                          d->buffer_pixmap,
                          d->min_drawn_buttons_region.x1,
                          d->min_drawn_buttons_region.y1,
                          d->min_drawn_buttons_region.x1,
                          d->min_drawn_buttons_region.y1,
                          d->min_drawn_buttons_region.x2 -
                          d->min_drawn_buttons_region.x1,
                          d->min_drawn_buttons_region.y2 -
                          d->min_drawn_buttons_region.y1);
    }
    fade_info->first_draw = false;
    if (!any_active_buttons) {
        cairo_destroy(fade_info->cr);
        fade_info->cr = NULL;
        if (fade_info->timer >= 0) {
            g_source_remove(fade_info->timer);
            fade_info->timer = -1;
        }
        return false;
    }
    return true;
}

void draw_buttons_without_fade(decor_t* d, cairo_t* cr, double y1)
{
    window_settings* ws = d->fs->ws;

    d->min_drawn_buttons_region.x1 = 10000;
    d->min_drawn_buttons_region.y1 = 10000;
    d->min_drawn_buttons_region.x2 = -100;
    d->min_drawn_buttons_region.y2 = -100;

    int b_t;
    int necessary_update_type[B_T_COUNT];        // 0: none, 1: only base, 2: base+glow

    for (b_t = 0; b_t < B_T_COUNT; b_t++)
        necessary_update_type[b_t] = (ws->use_button_glow && d->active) ||
                                     (ws->use_button_inactive_glow && !d->active) ? 2 : 1;
    //necessary_update_type[b_t] = 2;

    draw_button_backgrounds(d, necessary_update_type);

    // Draw buttons
    int button_hovered_on = -1;

    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        if (necessary_update_type[b_t] == 0) {
            continue;
        }
        int b_state = get_b_state(d, b_t);

        if (ws->use_pixmap_buttons &&
                ((ws->use_button_glow && b_state == S_ACTIVE_HOVER) ||
                 (ws->use_button_inactive_glow && b_state == S_INACTIVE_HOVER))) {
            // skip the one being hovered on, if any
            button_hovered_on = b_t;
        } else {
            draw_button(b_t, d, cr, y1);
        }
    }
    if (button_hovered_on >= 0) {
        // Draw the button and the glow for the button hovered on
        draw_button_with_glow(button_hovered_on, d, cr, y1, true);
    }
}


void
decor_update_blur_property(decor_t* d,
                           int     width,
                           int     height,
                           Region  top_region,
                           int     top_offset,
                           Region  bottom_region,
                           int     bottom_offset,
                           Region  left_region,
                           int     left_offset,
                           Region  right_region,
                           int     right_offset)
{
    Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    long*    data = NULL;
    int     size = 0;
    window_settings* ws = d->fs->ws;

    if (ws->blur_type != BLUR_TYPE_ALL) {
        bottom_region = NULL;
        left_region   = NULL;
        right_region  = NULL;

        if (ws->blur_type != BLUR_TYPE_TITLEBAR) {
            top_region = NULL;
        }
    }

    if (top_region) {
        size += top_region->numRects;
    }
    if (bottom_region) {
        size += bottom_region->numRects;
    }
    if (left_region) {
        size += left_region->numRects;
    }
    if (right_region) {
        size += right_region->numRects;
    }

    if (size) {
        data = malloc(sizeof(long) * (2 + size * 6));
    }

    if (data) {
        decor_region_to_blur_property(data, 4, 0, width, height,
                                      top_region, top_offset,
                                      bottom_region, bottom_offset,
                                      left_region, left_offset,
                                      right_region, right_offset);

        gdk_error_trap_push();
        XChangeProperty(xdisplay, d->prop_xid,
                        win_blur_decor_atom,
                        XA_INTEGER,
                        32, PropModeReplace, (guchar*) data,
                        2 + size * 6);
        XSync(xdisplay, false);
        gdk_error_trap_pop();

        free(data);
    } else {
        gdk_error_trap_push();
        XDeleteProperty(xdisplay, d->prop_xid, win_blur_decor_atom);
        XSync(xdisplay, false);
        gdk_error_trap_pop();
    }
}

void decor_update_window_property(decor_t* d)
{
    long* data = NULL;
    Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    window_settings* ws = d->fs->ws;
    decor_extents_t maxextents;
    decor_extents_t extents = ws->win_extents;
    unsigned int nQuad;
    decor_quad_t quads[N_QUADS_MAX];
    int                    w, h;
    int            stretch_offset;
    REGION            top, bottom, left, right;

    w = d->width;
    h = d->height;

    stretch_offset = 60;

    nQuad = my_set_window_quads(quads, d->width, d->height, ws,
                                d->state & Wnck::WINDOW_STATE_MAXIMIZED_HORIZONTALLY,
                                d->state & Wnck::WINDOW_STATE_MAXIMIZED_VERTICALLY);

    extents.top += ws->titlebar_height;

    if (ws->use_decoration_cropping) {
        maxextents.left = 0;
        maxextents.right = 0;
        maxextents.top = ws->titlebar_height + ws->win_extents.top;
        maxextents.bottom = 0;
    } else {
        maxextents = extents;
    }

    data = decor_alloc_property(1, WINDOW_DECORATION_TYPE_PIXMAP);

    decor_quads_to_property(data, 0, GDK_PIXMAP_XID(d->pixmap),
                            &extents, &maxextents, &maxextents, &maxextents,
                            0, 0, quads, nQuad, 0xffffff, 0, 0);

    gdk_error_trap_push();
    XChangeProperty(xdisplay, d->prop_xid,
                    win_decor_atom,
                    XA_INTEGER,
                    32, PropModeReplace, (guchar*) data,
                    PROP_HEADER_SIZE + BASE_PROP_SIZE + QUAD_PROP_SIZE * N_QUADS_MAX);
    XSync(xdisplay, false);
    gdk_error_trap_pop();

    top.rects = &top.extents;
    top.numRects = top.size = 1;

    top.extents.x1 = -extents.left;
    top.extents.y1 = -extents.top;
    top.extents.x2 = w + extents.right;
    top.extents.y2 = 0;

    bottom.rects = &bottom.extents;
    bottom.numRects = bottom.size = 1;

    bottom.extents.x1 = -extents.left;
    bottom.extents.y1 = 0;
    bottom.extents.x2 = w + extents.right;
    bottom.extents.y2 = extents.bottom;

    left.rects = &left.extents;
    left.numRects = left.size = 1;

    left.extents.x1 = -extents.left;
    left.extents.y1 = 0;
    left.extents.x2 = 0;
    left.extents.y2 = h;

    right.rects = &right.extents;
    right.numRects = right.size = 1;

    right.extents.x1 = 0;
    right.extents.y1 = 0;
    right.extents.x2 = extents.right;
    right.extents.y2 = h;

    decor_update_blur_property(d,
                               w, h,
                               &top, stretch_offset,
                               &bottom, w / 2,
                               &left, h / 2,
                               &right, h / 2);
    if (data) {
        free(data);
    }
}
