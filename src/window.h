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

#ifndef EMERALD_WINDOW_UTILS_H
#define EMERALD_WINDOW_UTILS_H

#include <emerald.h>

GdkPixmap* create_pixmap(int w, int h);
GdkPixmap* pixmap_new_from_pixbuf(GdkPixbuf* pixbuf);
void position_title_object(char obj, WnckWindow* win, window_settings* ws,
                           int x, int s);
void layout_title_objects(WnckWindow* win);
void update_event_windows(WnckWindow* win);
int max_window_name_width(WnckWindow* win);
void update_window_decoration_name(WnckWindow* win);
void update_window_decoration_icon(WnckWindow* win);
void update_window_decoration_state(WnckWindow* win);
void update_window_decoration_actions(WnckWindow* win);
bool update_window_decoration_size(WnckWindow* win);
void add_frame_window(WnckWindow* win, Window frame);
bool update_switcher_window(WnckWindow* win, Window selected);
void remove_frame_window(WnckWindow* win);
void window_name_changed(WnckWindow* win);
void window_geometry_changed(WnckWindow* win);
void window_icon_changed(WnckWindow* win);
void window_state_changed(WnckWindow* win);
void window_actions_changed(WnckWindow* win);
void connect_window(WnckWindow* win);
void active_window_changed(WnckScreen* screen);
void window_opened(WnckScreen* screen, WnckWindow* win);
void window_closed(WnckScreen* screen, WnckWindow* win);
void connect_screen(WnckScreen* screen);
void move_resize_window(WnckWindow* win, int direction, XEvent* xevent);
void restack_window(WnckWindow* win, int stack_mode);

#endif
