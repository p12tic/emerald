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
