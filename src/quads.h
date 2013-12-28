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

#ifndef EMERALD_QUADS_H
#define EMERALD_QUADS_H

#include <libengine/emerald.h>
#include <libengine/libengine.h>

void update_window_extents(window_settings* ws);
int my_add_quad_row(decor_quad_t* q,
                    int width, int left,
                    int right, int ypush, int vgrav, int x0, int y0);
int my_add_quad_col(decor_quad_t* q,
                    int height, int xpush, int hgrav, int x0, int y0);
int my_set_window_quads(decor_quad_t* q, int width, int height,
                        window_settings* ws, bool max_horz, bool max_vert);
int set_switcher_quads(decor_quad_t* q, int width, int height, window_settings* ws);
int set_shadow_quads(decor_quad_t* q, int width, int height, window_settings* ws);

#endif
