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

#include <libengine/emerald.h>
#include <libengine/libengine.h>
#include <libengine/titlebar.h>
#include "cairo_utils.h"
#include "utils.h"

void gdk_cairo_set_source_color_alpha(cairo_t* cr, GdkColor* color, double alpha)
{
    cairo_set_source_rgba(cr,
                          color->red / 65535.0,
                          color->green / 65535.0,
                          color->blue / 65535.0, alpha);
}

void draw_shadow_background(decor_t* d, cairo_t* cr)
{
    cairo_matrix_t matrix;
    double w, h, x2, y2;
    int width, height;
    int left, right, top, bottom;
    window_settings* ws = d->fs->ws;

    if (!ws->large_shadow_pixmap) {
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
        cairo_paint(cr);

        return;
    }

    gdk_drawable_get_size(ws->large_shadow_pixmap, &width, &height);

    left = ws->left_space + ws->left_corner_space;
    right = ws->right_space + ws->right_corner_space;
    top = ws->top_space + ws->top_corner_space;
    bottom = ws->bottom_space + ws->bottom_corner_space;

    if (d->width - left - right < 0) {
        left = d->width / 2;
        right = d->width - left;
    }

    if (d->height - top - bottom < 0) {
        top = d->height / 2;
        bottom = d->height - top;
    }

    w = d->width - left - right;
    h = d->height - top - bottom;

    x2 = d->width - right;
    y2 = d->height - bottom;

    /* top left */
    cairo_matrix_init_identity(&matrix);
    cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
    cairo_set_source(cr, ws->shadow_pattern);
    cairo_rectangle(cr, 0.0, 0.0, left, top);
    cairo_fill(cr);

    /* top */
    if (w > 0) {
        cairo_matrix_init_translate(&matrix, left, 0.0);
        cairo_matrix_scale(&matrix, 1.0 / w, 1.0);
        cairo_matrix_translate(&matrix, -left, 0.0);
        cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
        cairo_set_source(cr, ws->shadow_pattern);
        cairo_rectangle(cr, left, 0.0, w, top);
        cairo_fill(cr);
    }

    /* top right */
    cairo_matrix_init_translate(&matrix, width - right - x2, 0.0);
    cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
    cairo_set_source(cr, ws->shadow_pattern);
    cairo_rectangle(cr, x2, 0.0, right, top);
    cairo_fill(cr);

    /* left */
    if (h > 0) {
        cairo_matrix_init_translate(&matrix, 0.0, top);
        cairo_matrix_scale(&matrix, 1.0, 1.0 / h);
        cairo_matrix_translate(&matrix, 0.0, -top);
        cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
        cairo_set_source(cr, ws->shadow_pattern);
        cairo_rectangle(cr, 0.0, top, left, h);
        cairo_fill(cr);
    }

    /* right */
    if (h > 0) {
        cairo_matrix_init_translate(&matrix, width - right - x2, top);
        cairo_matrix_scale(&matrix, 1.0, 1.0 / h);
        cairo_matrix_translate(&matrix, 0.0, -top);
        cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
        cairo_set_source(cr, ws->shadow_pattern);
        cairo_rectangle(cr, x2, top, right, h);
        cairo_fill(cr);
    }

    /* bottom left */
    cairo_matrix_init_translate(&matrix, 0.0, height - bottom - y2);
    cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
    cairo_set_source(cr, ws->shadow_pattern);
    cairo_rectangle(cr, 0.0, y2, left, bottom);
    cairo_fill(cr);

    /* bottom */
    if (w > 0) {
        cairo_matrix_init_translate(&matrix, left, height - bottom - y2);
        cairo_matrix_scale(&matrix, 1.0 / w, 1.0);
        cairo_matrix_translate(&matrix, -left, 0.0);
        cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
        cairo_set_source(cr, ws->shadow_pattern);
        cairo_rectangle(cr, left, y2, w, bottom);
        cairo_fill(cr);
    }

    /* bottom right */
    cairo_matrix_init_translate(&matrix, width - right - x2,
                                height - bottom - y2);
    cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
    cairo_set_source(cr, ws->shadow_pattern);
    cairo_rectangle(cr, x2, y2, right, bottom);
    cairo_fill(cr);
}

void draw_help_button(cairo_t* cr)
{
    cairo_rel_move_to(cr, 0.0, 6.0);
    cairo_rel_line_to(cr, 0.0, 3.0);
    cairo_rel_line_to(cr, 4.5, 0.0);
    cairo_rel_line_to(cr, 0.0, 4.5);
    cairo_rel_line_to(cr, 3.0, 0.0);
    cairo_rel_line_to(cr, 0.0, -4.5);

    cairo_rel_line_to(cr, 4.5, 0.0);

    cairo_rel_line_to(cr, 0.0, -3.0);
    cairo_rel_line_to(cr, -4.5, 0.0);
    cairo_rel_line_to(cr, 0.0, -4.5);
    cairo_rel_line_to(cr, -3.0, 0.0);
    cairo_rel_line_to(cr, 0.0, 4.5);

    cairo_close_path(cr);
}

void draw_close_button(cairo_t* cr, double s)
{
    cairo_rel_move_to(cr, 0.0, s);

    cairo_rel_line_to(cr, s, -s);
    cairo_rel_line_to(cr, s, s);
    cairo_rel_line_to(cr, s, -s);
    cairo_rel_line_to(cr, s, s);

    cairo_rel_line_to(cr, -s, s);
    cairo_rel_line_to(cr, s, s);
    cairo_rel_line_to(cr, -s, s);
    cairo_rel_line_to(cr, -s, -s);

    cairo_rel_line_to(cr, -s, s);
    cairo_rel_line_to(cr, -s, -s);
    cairo_rel_line_to(cr, s, -s);

    cairo_close_path(cr);
}

void draw_max_button(cairo_t* cr, double s)
{
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

    cairo_rel_line_to(cr, 12.0, 0.0);
    cairo_rel_line_to(cr, 0.0, 12.0);
    cairo_rel_line_to(cr, -12.0, 0.0);

    cairo_close_path(cr);

    cairo_rel_move_to(cr, 2.0, s);

    cairo_rel_line_to(cr, 8.0, 0.0);
    cairo_rel_line_to(cr, 0.0, 10.0 - s);
    cairo_rel_line_to(cr, -8.0, 0.0);

    cairo_close_path(cr);
}

void draw_unmax_button(cairo_t* cr, double s)
{
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

    cairo_rel_move_to(cr, 1.0, 1.0);

    cairo_rel_line_to(cr, 10.0, 0.0);
    cairo_rel_line_to(cr, 0.0, 10.0);
    cairo_rel_line_to(cr, -10.0, 0.0);

    cairo_close_path(cr);

    cairo_rel_move_to(cr, 2.0, s);

    cairo_rel_line_to(cr, 6.0, 0.0);
    cairo_rel_line_to(cr, 0.0, 8.0 - s);
    cairo_rel_line_to(cr, -6.0, 0.0);

    cairo_close_path(cr);
}

void draw_min_button(cairo_t* cr, double s)
{
    cairo_rel_move_to(cr, 0.0, 8.0);

    cairo_rel_line_to(cr, 12.0, 0.0);
    cairo_rel_line_to(cr, 0.0, s);
    cairo_rel_line_to(cr, -12.0, 0.0);

    cairo_close_path(cr);
}

void get_button_pos(window_settings* ws, int b_t,
                    decor_t* d, double y1, double* rx, double* ry)
{
    //y1 - 4.0 + ws->titlebar_height / 2,
    *ry = y1 + ws->button_offset;
    *rx = get_real_pos(ws, b_t, d);
}

void button_state_paint(cairo_t* cr,
                        alpha_color* color, alpha_color* color_2, unsigned state)
{
    double alpha;

    if (state & IN_EVENT_WINDOW) {
        alpha = 1.0;
    } else {
        alpha = color->alpha;
    }

    if ((state & (PRESSED_EVENT_WINDOW | IN_EVENT_WINDOW))
            == (PRESSED_EVENT_WINDOW | IN_EVENT_WINDOW)) {
        cairo_set_source_rgba(cr, color->color.r, color->color.g,
                              color->color.b, alpha);

        cairo_fill_preserve(cr);

        cairo_set_source_alpha_color(cr, color_2);

        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
        cairo_set_line_width(cr, 2.0);
    } else {
        cairo_set_source_alpha_color(cr, color_2);
        cairo_stroke_preserve(cr);

        cairo_set_source_rgba(cr, color->color.r, color->color.g,
                              color->color.b, alpha);

        cairo_fill(cr);
    }
}

int get_b_state(decor_t* d, int button)
{
    int ret = d->active ? 0 : 3;

    if (d->button_states[button] & IN_EVENT_WINDOW) {
        ret++;
        if (d->button_states[button] & PRESSED_EVENT_WINDOW) {
            ret++;
        }
    }
    return ret;
}

void draw_pixbuf(GdkPixbuf* pixbuf, cairo_t* cr,
                 double x, double y, double x2, double y2, double alpha)
{
    cairo_save(cr);
    cairo_rectangle(cr, x, y, x2 - x, y2 - y);
    cairo_clip(cr);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, x, y);
    cairo_paint_with_alpha(cr, alpha);
    cairo_restore(cr);
}

void draw_button_with_glow_alpha_bstate(int b_t, decor_t* d, cairo_t* cr,
                                        int y1, double button_alpha,
                                        double glow_alpha, int b_state)
{
    (void) y1;
    int b = b_t;
    double x, y;
    double x2, y2;
    double glow_x, glow_y;                // glow top left coordinates
    double glow_x2, glow_y2;        // glow bottom right coordinates
    window_settings* ws = d->fs->ws;

    if (b_state < 0) {
        b_state = get_b_state(d, b_t);
    }

    b = get_b_t_offset(b_t);

    if (btbistate[b_t])
        if (d->state & btstateflag[b_t]) {
            b++;
        }

    if (BUTTON_NOT_VISIBLE(d, b_t)) {
        return;
    }
    button_region_t* button_region =
        (d->active ? &d->button_region[b_t] : &d->
         button_region_inact[b_t]);
    x = button_region->base_x1;
    y = button_region->base_y1;

    if (ws->use_pixmap_buttons) {
        x2 = button_region->base_x2;
        y2 = button_region->base_y2;
        draw_pixbuf(ws->ButtonPix[b_state + b * S_COUNT], cr, x, y, x2, y2,
                    button_alpha);

        if (glow_alpha > 1e-5) {      // i.e. glow is on
            glow_x = button_region->glow_x1;
            glow_y = button_region->glow_y1;
            glow_x2 = button_region->glow_x2;
            glow_y2 = button_region->glow_y2;
            if (d->active) {
                // Draw glow
                draw_pixbuf(ws->ButtonGlowPix[b], cr, glow_x, glow_y, glow_x2,
                            glow_y2, glow_alpha);
            } else {                            // assume this function won't be called with glow_alpha>0
                // if ws->use_inactive_glow is false
                // Draw inactive glow
                draw_pixbuf(ws->ButtonInactiveGlowPix[b], cr, glow_x, glow_y,
                            glow_x2, glow_y2, glow_alpha);
            }
        }
    } else {
        y += 3;
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, x, y);
        switch (b) {
        case B_CLOSE:
            draw_close_button(cr, 3.1);
            break;
        case B_MAXIMIZE:
            draw_max_button(cr, 4.0);
            break;
        case B_RESTORE:
            draw_unmax_button(cr, 4.0);
            break;
        case B_MINIMIZE:
            draw_min_button(cr, 4.0);
            break;
        case B_HELP:
            cairo_move_to(cr, x, y);
            draw_help_button(cr); // was (cr, 3.1)
            break;
        default:
            //FIXME - do something here
            break;
        }
        button_state_paint(cr, &d->fs->button, &d->fs->button_halo,
                           d->button_states[b_t]);
    }
}

void draw_button_with_glow(int b_t, decor_t* d, cairo_t* cr, int y1,
                           bool with_glow)
{
    draw_button_with_glow_alpha_bstate(b_t, d, cr, y1, 1.0,
                                       (with_glow ? 1.0 : 0.0), -1);
}

void draw_button(int b_t, decor_t* d, cairo_t* cr, int y1)
{
    draw_button_with_glow_alpha_bstate(b_t, d, cr, y1, 1.0, 0.0, -1);
}
