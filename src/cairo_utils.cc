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

void gdk_cairo_set_source_color_alpha(Cairo::RefPtr<Cairo::Context>& cr, GdkColor* color, double alpha)
{
    cr->set_source_rgba(color->red / 65535.0,
                        color->green / 65535.0,
                        color->blue / 65535.0, alpha);
}

void draw_shadow_background(decor_t* d, Cairo::RefPtr<Cairo::Context>& cr)
{
    double w, h, x2, y2;
    int width, height;
    int left, right, top, bottom;
    window_settings* ws = d->fs->ws;

    if (!ws->large_shadow_pixmap) {
        cr->set_source_rgba(0.0, 0.0, 0.0, 0.0);
        cr->paint();

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
    ws->shadow_pattern->set_matrix(Cairo::identity_matrix());
    cr->set_source(ws->shadow_pattern);
    cr->rectangle(0.0, 0.0, left, top);
    cr->fill();

    /* top */
    if (w > 0) {
        auto mat = Cairo::translation_matrix(left, 0.0);
        mat.scale(1.0 / w, 1.0);
        mat.translate(-left, 0.0);
        ws->shadow_pattern->set_matrix(mat);
        cr->set_source(ws->shadow_pattern);
        cr->rectangle(left, 0.0, w, top);
        cr->fill();
    }

    /* top right */
    ws->shadow_pattern->set_matrix(
                Cairo::translation_matrix(width - right - x2, 0.0));
    cr->set_source(ws->shadow_pattern);
    cr->rectangle(x2, 0.0, right, top);
    cr->fill();

    /* left */
    if (h > 0) {
        auto mat = Cairo::translation_matrix(0.0, top);
        mat.scale(1.0, 1.0 / h);
        mat.translate(0.0, -top);
        ws->shadow_pattern->set_matrix(mat);
        cr->set_source(ws->shadow_pattern);
        cr->rectangle(0.0, top, left, h);
        cr->fill();
    }

    /* right */
    if (h > 0) {
        auto mat = Cairo::translation_matrix(width - right - x2, top);
        mat.scale(1.0, 1.0 / h);
        mat.translate(0.0, -top);
        ws->shadow_pattern->set_matrix(mat);
        cr->set_source(ws->shadow_pattern);
        cr->rectangle(x2, top, right, h);
        cr->fill();
    }

    /* bottom left */
    ws->shadow_pattern->set_matrix(
                Cairo::translation_matrix(0.0, height - bottom - y2));
    cr->set_source(ws->shadow_pattern);
    cr->rectangle(0.0, y2, left, bottom);
    cr->fill();

    /* bottom */
    if (w > 0) {
        auto mat = Cairo::translation_matrix(left, height - bottom - y2);
        mat.scale(1.0 / w, 1.0);
        mat.translate(-left, 0.0);
        ws->shadow_pattern->set_matrix(mat);
        cr->set_source(ws->shadow_pattern);
        cr->rectangle(left, y2, w, bottom);
        cr->fill();
    }

    /* bottom right */
    ws->shadow_pattern->set_matrix(
                Cairo::translation_matrix(width - right - x2, height - bottom - y2));
    cr->set_source(ws->shadow_pattern);
    cr->rectangle(x2, y2, right, bottom);
    cr->fill();
}

void draw_help_button(Cairo::RefPtr<Cairo::Context>& cr)
{
    cr->rel_move_to(0.0, 6.0);
    cr->rel_line_to(0.0, 3.0);
    cr->rel_line_to(4.5, 0.0);
    cr->rel_line_to(0.0, 4.5);
    cr->rel_line_to(3.0, 0.0);
    cr->rel_line_to(0.0, -4.5);

    cr->rel_line_to(4.5, 0.0);

    cr->rel_line_to(0.0, -3.0);
    cr->rel_line_to(-4.5, 0.0);
    cr->rel_line_to(0.0, -4.5);
    cr->rel_line_to(-3.0, 0.0);
    cr->rel_line_to(0.0, 4.5);

    cr->close_path();
}

void draw_close_button(Cairo::RefPtr<Cairo::Context>& cr, double s)
{
    cr->rel_move_to(0.0, s);

    cr->rel_line_to(s, -s);
    cr->rel_line_to(s, s);
    cr->rel_line_to(s, -s);
    cr->rel_line_to(s, s);

    cr->rel_line_to(-s, s);
    cr->rel_line_to(s, s);
    cr->rel_line_to(-s, s);
    cr->rel_line_to(-s, -s);

    cr->rel_line_to(-s, s);
    cr->rel_line_to(-s, -s);
    cr->rel_line_to(s, -s);

    cr->close_path();
}

void draw_max_button(Cairo::RefPtr<Cairo::Context>& cr, double s)
{
    cr->set_fill_rule(Cairo::FILL_RULE_EVEN_ODD);

    cr->rel_line_to(12.0, 0.0);
    cr->rel_line_to(0.0, 12.0);
    cr->rel_line_to(-12.0, 0.0);

    cr->close_path();

    cr->rel_move_to(2.0, s);

    cr->rel_line_to(8.0, 0.0);
    cr->rel_line_to(0.0, 10.0 - s);
    cr->rel_line_to(-8.0, 0.0);

    cr->close_path();
}

void draw_unmax_button(Cairo::RefPtr<Cairo::Context>& cr, double s)
{
    cr->set_fill_rule(Cairo::FILL_RULE_EVEN_ODD);

    cr->rel_move_to(1.0, 1.0);

    cr->rel_line_to(10.0, 0.0);
    cr->rel_line_to(0.0, 10.0);
    cr->rel_line_to(-10.0, 0.0);

    cr->close_path();

    cr->rel_move_to(2.0, s);

    cr->rel_line_to(6.0, 0.0);
    cr->rel_line_to(0.0, 8.0 - s);
    cr->rel_line_to(-6.0, 0.0);

    cr->close_path();
}

void draw_min_button(Cairo::RefPtr<Cairo::Context>& cr, double s)
{
    cr->rel_move_to(0.0, 8.0);

    cr->rel_line_to(12.0, 0.0);
    cr->rel_line_to(0.0, s);
    cr->rel_line_to(-12.0, 0.0);

    cr->close_path();
}

void get_button_pos(window_settings* ws, int b_t,
                    decor_t* d, double y1, double* rx, double* ry)
{
    //y1 - 4.0 + ws->titlebar_height / 2,
    *ry = y1 + ws->button_offset;
    *rx = get_real_pos(ws, b_t, d);
}

void button_state_paint(Cairo::RefPtr<Cairo::Context>& cr,
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
        cr->set_source_rgba(color->color.r, color->color.g,
                            color->color.b, alpha);

        cr->fill_preserve();

        cairo_set_source_alpha_color(cr, color_2);

        cr->set_line_width(1.0);
        cr->stroke();
        cr->set_line_width(2.0);
    } else {
        cairo_set_source_alpha_color(cr, color_2);
        cr->stroke_preserve();

        cr->set_source_rgba(color->color.r, color->color.g,
                            color->color.b, alpha);

        cr->fill();
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

void draw_pixbuf(GdkPixbuf* pixbuf, Cairo::RefPtr<Cairo::Context>& cr,
                 double x, double y, double x2, double y2, double alpha)
{
    cr->save();
    cr->rectangle(x, y, x2 - x, y2 - y);
    cr->clip();
    gdk_cairo_set_source_pixbuf(cr->cobj(), pixbuf, x, y);
    cr->paint_with_alpha(alpha);
    cr->restore();
}

void draw_button_with_glow_alpha_bstate(int b_t, decor_t* d, Cairo::RefPtr<Cairo::Context>& cr,
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
        cr->set_line_width(2.0);
        cr->move_to(x, y);
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
            cr->move_to(x, y);
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

void draw_button_with_glow(int b_t, decor_t* d, Cairo::RefPtr<Cairo::Context>& cr, int y1,
                           bool with_glow)
{
    draw_button_with_glow_alpha_bstate(b_t, d, cr, y1, 1.0,
                                       (with_glow ? 1.0 : 0.0), -1);
}

void draw_button(int b_t, decor_t* d, Cairo::RefPtr<Cairo::Context>& cr, int y1)
{
    draw_button_with_glow_alpha_bstate(b_t, d, cr, y1, 1.0, 0.0, -1);
}
