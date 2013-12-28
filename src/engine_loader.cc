/*
 * Copyright © 2006 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

//engine loader
#include <emerald.h>
#include <engine.h>
#include <filesystem.h>

#define LOCAL_ENGINE_DIR ".emerald/engines"

static void* engine = NULL;
init_engine_proc e_init = NULL;
fini_engine_proc e_fini = NULL;
load_settings_proc e_load = NULL;
draw_frame_proc e_draw = NULL;

bool load_engine(const std::string& engine_name, window_settings* ws)
{
    void* newengine;
    ws->stretch_sides = true;

    std::string engine_ldname = std::string{"lib"} + engine_name + ".so";
    if (engine) {
        if (e_fini) {
            e_fini(ws);
        }
        dlclose(engine);
        engine = NULL;
    }
    dlerror(); // clear errors

    fs::path homedir = g_get_home_dir();
    fs::path local_path = homedir / LOCAL_ENGINE_DIR / engine_ldname;
    fs::path global_path = fs::path{ENGINE_DIR} / engine_ldname;

    newengine = dlopen(local_path.native().c_str(), RTLD_NOW);
    if (!newengine) {
        newengine = dlopen(global_path.native().c_str(), RTLD_NOW);
        if (!newengine) {
            g_warning("%s", dlerror());
            engine = nullptr;
            return false;
        }
    }
    engine = newengine;
    if (engine) {
        //lookup our procs
        e_init = dlsym(engine, "init_engine");
        e_fini = dlsym(engine, "fini_engine");
        e_load = dlsym(engine, "load_engine_settings");
        e_draw = dlsym(engine, "engine_draw_frame");
    } else {
        e_init = NULL;
        e_fini = NULL;
        e_load = NULL;
        e_draw = NULL;
    }
    if (e_init) {
        e_init(ws);
    }
    return engine ? true : false;
}

void load_engine_settings(const KeyFile& f, window_settings* ws)
{
    if (e_load && engine) {
        e_load(f, ws);
    }
}
void engine_draw_frame(decor_t* d, cairo_t* cr)
{
    if (e_draw && engine) {
        e_draw(d, cr);
    }
}

