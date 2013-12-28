/*  This file is part of emerald

    Copyright (C) 2006  Novell, Inc.
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

#ifndef EMERALD_LIBENGINE_TITLEBAR_H
#define EMERALD_LIBENGINE_TITLEBAR_H

#define IN_EVENT_WINDOW      (1 << 0)
#define PRESSED_EVENT_WINDOW (1 << 1)
enum buttons {
    B_CLOSE,
    B_MAXIMIZE,
    B_RESTORE,
    B_MINIMIZE,
    B_HELP,
    B_MENU,
    B_SHADE,
    B_UNSHADE,
    B_ABOVE,
    B_UNABOVE,
    B_STICK,
    B_UNSTICK,
    B_COUNT
};
enum states {
    S_ACTIVE,
    S_ACTIVE_HOVER,
    S_ACTIVE_PRESS,
    S_INACTIVE,
    S_INACTIVE_HOVER,
    S_INACTIVE_PRESS,
    S_COUNT
};
enum btypes {
    B_T_CLOSE,
    B_T_MAXIMIZE,
    B_T_MINIMIZE,
    B_T_HELP,
    B_T_MENU,
    B_T_SHADE,
    B_T_ABOVE,
    B_T_STICKY,
    B_T_COUNT
};
static const bool btbistate[B_T_COUNT] = {
    false,
    true,
    false,
    false,
    false,
    true,
    true,
    true,
};
static const int btstateflag[B_T_COUNT] = {
    0,
    Wnck::WINDOW_STATE_MAXIMIZED_HORIZONTALLY | Wnck::WINDOW_STATE_MAXIMIZED_VERTICALLY,
    0,
    0,
    0,
    Wnck::WINDOW_STATE_SHADED,
    Wnck::WINDOW_STATE_ABOVE,
    Wnck::WINDOW_STATE_STICKY,
};

enum tbtypes {
    TBT_CLOSE = B_T_CLOSE,
    TBT_MAXIMIZE = B_T_MAXIMIZE,
    TBT_MINIMIZE = B_T_MINIMIZE,
    TBT_HELP = B_T_HELP,
    TBT_MENU = B_T_MENU,
    TBT_SHADE = B_T_SHADE,
    TBT_ONTOP = B_T_ABOVE,
    TBT_STICKY = B_T_STICKY,
    TBT_TITLE = B_T_COUNT,
    TBT_ICON,
    TBT_ONBOTTOM,
    TBT_COUNT,
};
#define BX_COUNT B_COUNT+2
static unsigned button_actions[B_T_COUNT] = {
    Wnck::WINDOW_ACTION_CLOSE,
    Wnck::WINDOW_ACTION_MAXIMIZE,
    Wnck::WINDOW_ACTION_MINIMIZE,
    FAKE_WINDOW_ACTION_HELP,
    0xFFFFFFFF, // any action should match
    Wnck::WINDOW_ACTION_SHADE,
    0xFFFFFFFF,//Wnck::WINDOW_ACTION_ABOVE,
    Wnck::WINDOW_ACTION_STICK,
};
static const char* b_types[] = {
    "close",
    "max",
    "restore",
    "min",
    "help",
    "menu",
    "shade",
    "unshade",
    "above",
    "unabove",
    "sticky",
    "unsticky",
    "glow",
    "inactive_glow"
};
static const char* b_names[] = {
    "Close",
    "Maximize",
    "Restore",
    "Minimize",
    "Help",
    "Menu",
    "Shade",
    "Un-Shade",
    "Set Above",
    "Un-Set Above",
    "Stick",
    "Un-Stick",
    "Glow(Halo)",
    "Inactive Glow"
};
enum {
    DOUBLE_CLICK_SHADE = 0,
    DOUBLE_CLICK_MAXIMIZE,
    DOUBLE_CLICK_MINIMIZE,
    TITLEBAR_ACTION_COUNT
};
static const char* titlebar_action_name[TITLEBAR_ACTION_COUNT] = {
    "Shade",
    "Maximize/Restore",
    "Minimize",
};

#endif
