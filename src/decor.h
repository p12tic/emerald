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

#ifndef EMERALD_DECOR_H
#define EMERALD_DECOR_H

#include <emerald.h>
#include <vector>

void reset_buttons_bg_and_fade(decor_t* d);
void stop_button_fade(decor_t* d);
void draw_button_backgrounds(decor_t* d, int* necessary_update_type);
void update_button_regions(decor_t* d);
void draw_window_decoration(decor_t* d);
void draw_shadow_window(decor_t* d);
void decor_update_switcher_property(decor_t* d);
void draw_switcher_background(decor_t* d);
void draw_switcher_foreground(decor_t* d);
void draw_switcher_decoration(decor_t* d);
void queue_decor_draw_for_buttons(decor_t* d, bool for_buttons);
void queue_decor_draw(decor_t* d);
int draw_buttons_timer_func(void* data);
bool draw_decor_list(void* data);
void draw_buttons_with_fade(decor_t* d, cairo_t* cr, double y1);
void draw_buttons_without_fade(decor_t* d, cairo_t* cr, double y1);
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
                           int     right_offset);
void decor_update_window_property(decor_t* d);

#endif
