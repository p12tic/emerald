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

#include <libengine/emerald.h>

GdkPixmap* create_pixmap(int w, int h);
GdkPixmap* pixmap_new_from_pixbuf(GdkPixbuf* pixbuf);
void position_title_object(char obj, Wnck::Window* win, window_settings* ws,
                           int x, int s);
void layout_title_objects(Wnck::Window* win);
void update_event_windows(Wnck::Window* win);
int max_window_name_width(Wnck::Window* win);
void update_window_decoration_name(Wnck::Window* win);
void update_window_decoration_icon(Wnck::Window* win);
void update_window_decoration_state(Wnck::Window* win);
void update_window_decoration_actions(Wnck::Window* win);
bool update_window_decoration_size(Wnck::Window* win);
void add_frame_window(Wnck::Window* win, Window frame);
bool update_switcher_window(Wnck::Window* win, Window selected);
void remove_frame_window(Wnck::Window* win);
void window_name_changed(Wnck::Window* win);
void window_geometry_changed(Wnck::Window* win);
void window_icon_changed(Wnck::Window* win);
void window_state_changed(Wnck::WindowState, Wnck::WindowState,
                          Wnck::Window* win);
void window_actions_changed(Wnck::WindowActions changed_mask,
                            Wnck::WindowActions new_state, Wnck::Window* win);
void connect_window(Wnck::Window* win);
void active_window_changed(Wnck::Window* previously_active_win, Wnck::Screen* screen);
void window_opened(Wnck::Window* win);
void window_closed(Wnck::Window* win);
void connect_screen(Wnck::Screen* screen);
void move_resize_window(Wnck::Window* win, int direction, XEvent* xevent);
void restack_window(Wnck::Window* win, int stack_mode);

#endif
