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

#ifndef EMERALD_CAIRO_UTILS_H
#define EMERALD_CAIRO_UTILS_H

#include <libengine/emerald.h>

void gdk_cairo_set_source_color_alpha(Cairo::RefPtr<Cairo::Context>& cr,
                                      const Gdk::Color& color, double alpha);
void draw_shadow_background(decor_t* d, Cairo::RefPtr<Cairo::Context>& cr);
void draw_help_button(Cairo::RefPtr<Cairo::Context>& cr);
void draw_close_button(Cairo::RefPtr<Cairo::Context>& cr, double s);
void draw_max_button(Cairo::RefPtr<Cairo::Context>& cr, double s);
void draw_unmax_button(Cairo::RefPtr<Cairo::Context>& cr, double s);
void draw_min_button(Cairo::RefPtr<Cairo::Context>& cr, double s);
void get_button_pos(window_settings* ws, int b_t,
                    decor_t* d, double y1, double* rx, double* ry);
void button_state_paint(Cairo::RefPtr<Cairo::Context>& cr,
                        alpha_color* color, alpha_color* color_2, unsigned state);
int get_b_state(decor_t* d, int button);
void draw_pixbuf(GdkPixbuf* pixbuf, Cairo::RefPtr<Cairo::Context>& cr,
                 double x, double y, double x2, double y2, double alpha);
void draw_button_with_glow_alpha_bstate(int b_t, decor_t* d, Cairo::RefPtr<Cairo::Context>& cr,
                                        int y1, double button_alpha,
                                        double glow_alpha, int b_state);
void draw_button_with_glow(int b_t, decor_t* d, Cairo::RefPtr<Cairo::Context>& cr, int y1,
                           bool with_glow);
void draw_button(int b_t, decor_t* d, Cairo::RefPtr<Cairo::Context>& cr, int y1);











#endif
