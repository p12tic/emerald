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

#ifndef EMERALD_UTILS_H
#define EMERALD_UTILS_H

#include <libengine/emerald.h>

int get_b_offset(int b);
int get_b_t_offset(int b_t);
int get_real_pos(window_settings* ws, int tobj, decor_t* d);
int get_title_object_type(char obj);
int get_title_object_width(char obj, window_settings* ws, decor_t* d);

unsigned int get_mwm_prop(Window xwindow);
bool get_window_prop(Window xwindow, Atom atom, Window* val);

#define BUTTON_NOT_VISIBLE(ddd, xxx) \
    ((ddd)->tobj_item_state[(xxx)] == 3 || !((ddd)->actions & button_actions[(xxx)]))

#include <X11/cursorfont.h>

struct _cursor {
    Cursor cursor;
    unsigned int shape;
};

void engine_draw_frame(decor_t* d, Cairo::RefPtr<Cairo::Context>& cr);
bool load_engine(const std::string& engine_name, window_settings* ws);
void load_engine_settings(const KeyFile& f, window_settings* ws);

extern GHashTable* frame_table;

extern _cursor cursor[3][3];
extern _cursor button_cursor;
extern window_settings* global_ws;

extern bool action_menu_mapped;
extern Wnck::ActionMenu* action_menu;
extern std::string program_name;

extern std::vector<decor_t*> g_draw_list;
extern GtkWidget* style_window;

extern Atom frame_window_atom;
extern Atom win_decor_atom;
extern Atom win_blur_decor_atom;
extern Atom wm_move_resize_atom;
extern Atom restack_window_atom;
extern Atom select_window_atom;
extern Atom net_wm_context_help_atom;
extern Atom wm_protocols_atom;
extern Atom mwm_hints_atom;
extern Atom switcher_fg_atom;

extern Atom toolkit_action_atom;
extern Atom toolkit_action_window_menu_atom;
extern Atom toolkit_action_force_quit_dialog_atom;

#endif
