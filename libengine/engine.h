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

#ifndef EMERALD_LIBENGINE_ENGINE_H
#define EMERALD_LIBENGINE_ENGINE_H

#include <libengine/emerald.h>
#include <libengine/libengine.h>

typedef void (*init_engine_proc)(window_settings*);         // init_engine
typedef void (*load_settings_proc)(const KeyFile& f, window_settings* ws);             // load_engine_settings
typedef void (*fini_engine_proc)(window_settings*);                      // fini_engine
typedef void (*draw_frame_proc)(decor_t* d, cairo_t* cr);   // engine_draw_frame
typedef void (*layout_settings_proc)(Gtk::Box& vbox);  // layout_engine_settings
typedef void (*get_meta_info_proc)(EngineMetaInfo* d);  // get meta info

struct EnginePlugin {
    void* enginedl;
    init_engine_proc fun_init_engine;
    fini_engine_proc fun_fini_engine;
    load_settings_proc fun_load_settings;
    draw_frame_proc fun_draw_frame;
    // the rest are used by the themer only

    void clear()
    {
        enginedl = nullptr;
        fun_init_engine = nullptr;
        fun_fini_engine = nullptr;
        fun_load_settings = nullptr;
        fun_draw_frame = nullptr;
    }
};

#endif
