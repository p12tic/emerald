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
#include <libengine/emerald.h>
#include <libengine/engine.h>
#include <libengine/filesystem.h>

#define LOCAL_ENGINE_DIR ".emerald/engines"

static EnginePlugin g_engine_plugin;

bool load_engine(const std::string& engine_name, window_settings* ws)
{
    EnginePlugin& plugin = g_engine_plugin;

    void* newengine;
    ws->stretch_sides = true;

    std::string engine_ldname = std::string{"lib"} + engine_name + ".so";
    if (plugin.enginedl) {
        if (plugin.fun_fini_engine) {
            plugin.fun_fini_engine(ws);
        }
        dlclose(plugin.enginedl);
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
            plugin.clear();
            return false;
        }
    }
    plugin.clear();
    plugin.enginedl = newengine;
    if (plugin.enginedl) {
        //lookup our procs
        plugin.fun_init_engine = reinterpret_cast<init_engine_proc>(dlsym(plugin.enginedl, "init_engine"));
        plugin.fun_fini_engine = reinterpret_cast<fini_engine_proc>(dlsym(plugin.enginedl, "fini_engine"));
        plugin.fun_load_settings = reinterpret_cast<load_settings_proc>(dlsym(plugin.enginedl, "load_engine_settings"));
        plugin.fun_draw_frame = reinterpret_cast<draw_frame_proc>(dlsym(plugin.enginedl, "engine_draw_frame"));
    }
    if (plugin.fun_init_engine) {
        plugin.fun_init_engine(ws);
    }
    return plugin.enginedl ? true : false;
}

void load_engine_settings(const KeyFile& f, window_settings* ws)
{
    if (g_engine_plugin.enginedl && g_engine_plugin.fun_load_settings) {
        g_engine_plugin.fun_load_settings(f, ws);
    }
}
void engine_draw_frame(decor_t* d, cairo_t* cr)
{
    if (g_engine_plugin.enginedl && g_engine_plugin.fun_draw_frame) {
        g_engine_plugin.fun_draw_frame(d, cr);
    }
}

