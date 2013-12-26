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

////////////////////////////////////////////////////
//themer stuff
#include <engine.h>
#include <signal.h>

GSList* SettingList = NULL;
GSList* EngineList = NULL;
GtkWidget* EngineCombo;
GtkListStore* EngineModel;
GtkWidget* EngineContainer;
//GtkWidget * PreviewImage[BX_COUNT];
//GtkWidget * ButtonImage[BX_COUNT];
bool apply = FALSE;
bool changed = FALSE;
GKeyFile* global_theme_file;
GKeyFile* global_settings_file;
#ifdef USE_DBUS
DBusConnection* dbcon;
#endif
char* active_engine = NULL;

static char* display_part(const char* p)
{
    char* name = g_strdup(p);
    char* tmp;

    if ((tmp = g_strrstr(name, ":"))) {
        *tmp++ = 0;
        tmp = g_strdup(tmp);
        g_free(name);
        name = tmp;
    }

    if ((tmp = g_strrstr(name, "."))) {
        *tmp = 0;
    }

    return name;
}

GSList* get_setting_list()
{
    return SettingList;
}
GtkWidget* scaler_new(double low, double high, double prec)
{
    GtkWidget* w;
    w = gtk_hscale_new_with_range(low, high, prec);
    gtk_scale_set_value_pos(GTK_SCALE(w), GTK_POS_RIGHT);
    gtk_range_set_update_policy(GTK_RANGE(w), GTK_UPDATE_DISCONTINUOUS);
    gtk_widget_set_size_request(w, 100, -1);
    return w;
}
void add_color_alpha_value(const char* caption, const char* basekey,
                           const char* sect, bool active)
{
    GtkWidget* w;
    char* colorkey;
    char* alphakey;
    colorkey = g_strdup_printf(active ? "active_%s" : "inactive_%s", basekey);
    alphakey = g_strdup_printf(active ? "active_%s_alpha" : "inactive_%s_alpha",
                               basekey);

    w = gtk_label_new(caption);
    table_append(w, FALSE);

    w = gtk_color_button_new();
    table_append(w, FALSE);
    SettingItem::register_setting(w, ST_COLOR, sect, colorkey);

    w = scaler_new(0.0, 1.0, 0.01);
    table_append(w, TRUE);
    SettingItem::register_setting(w, ST_FLOAT, sect, alphakey);
    //we don't g_free because they are registered with SettingItem::register_setting
}
void make_labels(const char* header)
{
    table_append(gtk_label_new(header), FALSE);
    table_append(gtk_label_new("Color"), FALSE);
    table_append(gtk_label_new("Opacity"), FALSE);
}
GtkWidget* build_frame(GtkWidget* vbox, const char* title, bool is_hbox)
{
    GtkWidget* frame;
    GtkWidget* box;
    frame = gtk_frame_new(title);
    gtk_box_pack_startC(vbox, frame, TRUE, TRUE, 0);
    box = is_hbox ? gtk_hbox_new(FALSE, 2) : gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_widthC(box, 8);
    gtk_container_addC(frame, box);
    return box;
}

static int current_table_width;
static GtkTable* current_table;
static int current_table_col;
static int current_table_row;

void table_new(int width, bool same, bool labels)
{
    //WARNING - clobbers all the current_table_ vars.
    current_table = GTK_TABLE(gtk_table_new(width, 1, same));
    gtk_table_set_row_spacings(current_table, 8);
    gtk_table_set_col_spacings(current_table, 8);
    current_table_col = labels ? 1 : 0;
    current_table_row = 0;
    current_table_width = width;
}
void table_append(GtkWidget* child, bool stretch)
{
    gtk_table_attach(current_table, child, current_table_col, current_table_col + 1,
                     current_table_row, current_table_row + 1,
                     (stretch ? GTK_EXPAND : GTK_SHRINK) | GTK_FILL,
                     (stretch ? GTK_EXPAND : GTK_SHRINK) | GTK_FILL,
                     0, 0);
    current_table_col++;
    if (current_table_col == current_table_width) {
        current_table_col = 0;
        current_table_row++;
//        gtk_table_resize(current_table,current_table_width,current_table_row+1);
    }
}
void table_append_separator()
{
    current_table_col = 0;
    current_table_row++;
//    gtk_table_resize(current_table,current_table_width,current_table_row+1);
    gtk_table_attach_defaults(current_table,
                              gtk_hseparator_new(),
                              0, current_table_width,
                              current_table_row,
                              current_table_row + 1);
    current_table_row++;
//    gtk_table_resize(current_table,current_table_width,current_table_row+1);
}
GtkTable* get_current_table()
{
    return current_table;
}
void send_reload_signal()
{
#ifdef USE_DBUS
    DBusMessage* message;
    message = dbus_message_new_signal("/", "org.metascape.emerald.dbus.Signal", "Reload");
    dbus_connection_send(dbcon, message, NULL);
    dbus_message_unref(message);
#else
    Atom wmAtom = 0;
    Display* dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    char buffer[128];
    char* part = display_part(getenv("DISPLAY"));

    sprintf(buffer, "_COMPIZ_DM_S%s", part);
    free(part);

    if (dpy) {
        wmAtom = XInternAtom(dpy, buffer, 0);
    }

    if (wmAtom) {
        XEvent clientEvent;
        Window w = XGetSelectionOwner(dpy, wmAtom);
        Atom ReloadIt = XInternAtom(dpy, "emerald-sigusr1", 0);
        clientEvent.xclient.type = ClientMessage;
        clientEvent.xclient.window = w;
        clientEvent.xclient.message_type = ReloadIt;
        clientEvent.xclient.format = 32;
        clientEvent.xclient.display = dpy;
        clientEvent.xclient.data.l[0]    = 0;
        clientEvent.xclient.data.l[1]    = 0;
        clientEvent.xclient.data.l[2]    = 0;
        clientEvent.xclient.data.l[3]    = 0;
        clientEvent.xclient.data.l[4]    = 0;
        Status missed = XSendEvent(dpy, w,
                                   False,
                                   NoEventMask,
                                   &clientEvent);
        XSync(dpy, False);
    } else {
        /* The old way */
        char* args[] =
        {"killall", "-u", (char*)g_get_user_name(), "-SIGUSR1", "emerald", NULL};
        char* ret = NULL;
        if (!g_spawn_sync(NULL, args, NULL, G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_SEARCH_PATH,
                          NULL, NULL, &ret, NULL, NULL, NULL) || !ret) {
            g_warning("Couldn't find running emerald, no reload signal sent.");
        }
    }
#endif
}
void apply_settings()
{
    char* file = g_strjoin("/", g_get_home_dir(), ".emerald/theme/theme.ini", NULL);
    char* path = g_strjoin("/", g_get_home_dir(), ".emerald/theme/", NULL);
    char* at;
    for (auto list = SettingList; list; list = list->next) {
        SettingItem* st = list->data;
        st->write_setting(global_theme_file);
    }
    g_key_file_set_string(global_theme_file, "theme", "version", VERSION);
    g_mkdir_with_parents(path, 00755);
    at = g_key_file_to_data(global_theme_file, NULL, NULL);
    if (at) {
        g_file_set_contents(file, at, -1, NULL);
        g_free(at);
    }
    g_free(file);
    g_free(path);
    send_reload_signal();
}
void cb_apply_setting(GtkWidget* w, void* p)
{
    SettingItem* item = p;
    if (item->type_ == ST_IMG_FILE) {
        char* s;
        if (!(s = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(item->widget_)))) {
            return;    // for now just ignore setting it to an invalid name
        }
        if (!strcmp(s, item->fvalue_)) {
            g_free(s);
            return;
        }
        g_free(item->fvalue_);
        item->fvalue_ = s;
        item->check_file(s);
    }
    item->write_setting((void*) global_theme_file);
    if (apply) {
        apply_settings();
    } else {
        changed = TRUE;
    }
}
#ifdef USE_DBUS
void setup_dbus()
{
    dbcon = dbus_bus_get(DBUS_BUS_SESSION, NULL);
    dbus_connection_setup_with_g_main(dbcon, NULL);
}
#endif

void show_engine_named(EngineData* d, void* p)
{
    char* nam = p;
    if (!strcmp(nam, d->canname)) {
        gtk_container_add(GTK_CONTAINER(EngineContainer), d->vbox);
        gtk_widget_show_all(EngineContainer);
    }
}
void do_engine(const char* nam)
{
    GtkWidget* w;
    if (active_engine && !strcmp(active_engine, nam)) {
        return;
    }
    if (active_engine) {
        g_free(active_engine);
    }
    active_engine = g_strdup(nam);
    if ((w = gtk_bin_get_child(GTK_BIN(EngineContainer)))) {
        gtk_container_remove(GTK_CONTAINER(EngineContainer), w);
    }
    g_slist_foreach(EngineList, (GFunc) show_engine_named, (void*) nam);

}
void search_engine(EngineData* d, void* p)
{
    FindEngine* fe = p;
    if (!fe->found) {
        if (!strcmp(d->canname, fe->canname)) {
            fe->d = d;
            fe->found = TRUE;
        } else {
            fe->i++;
        }
    }
}
bool get_engine_meta_info(const char* engine, EngineMetaInfo* inf)
{
    FindEngine fe;
    fe.canname = engine;
    fe.found = FALSE;
    fe.i = 0;
    fe.d = NULL;
    g_slist_foreach(EngineList, (GFunc) search_engine, &fe);
    if (fe.found) {
        memcpy(inf, &(fe.d->meta), sizeof(EngineMetaInfo));
    }
    return fe.found;
}
void update_preview(GtkFileChooser* fc, char* filename, GtkImage* img)
{
    GdkPixbuf* pixbuf;
    bool have_preview;
    pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
    have_preview = (pixbuf != NULL);
    gtk_image_set_from_pixbuf(GTK_IMAGE(img), pixbuf);
    if (pixbuf) {
        g_object_unref(pixbuf);
    }
    gtk_file_chooser_set_preview_widget_active(fc, have_preview);
}
void update_preview_cb(GtkFileChooser* file_chooser, void* data)
{
    char* filename;
    filename = gtk_file_chooser_get_preview_filename(file_chooser);
    update_preview(file_chooser, filename, GTK_IMAGE(data));
    g_free(filename);
}
void init_settings()
{
    char* file = g_strjoin("/", g_get_home_dir(), ".emerald/theme/theme.ini", NULL);
    g_key_file_load_from_file(global_theme_file, file, G_KEY_FILE_KEEP_COMMENTS, NULL);
    g_free(file);
    file = g_strjoin("/", g_get_home_dir(), ".emerald/settings.ini", NULL);
    g_key_file_load_from_file(global_settings_file, file, G_KEY_FILE_KEEP_COMMENTS, NULL);
    g_free(file);
    for (auto list = SettingList; list; list = list->next) {
        SettingItem* st = list->data;
        st->read_setting((void*)global_theme_file);
    }
}

void set_changed(bool schanged)
{
    changed = schanged;
}
void set_apply(bool sapply)
{
    apply = sapply;
}
void cb_clear_file(GtkWidget* button, void* p)
{
    SettingItem* item = p;
    item->check_file("");
    item->fvalue_ = "";
    gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(item->widget_));
    item->write_setting(global_theme_file);
    if (apply) {
        apply_settings();
    }
}
void init_key_files()
{
    global_theme_file = g_key_file_new();
    global_settings_file = g_key_file_new();
}
void layout_engine_list(GtkWidget* vbox)
{
    GtkWidget* hbox;
    EngineCombo = gtk_combo_box_new();
    hbox = gtk_hbox_new(FALSE, 2);
    gtk_box_pack_startC(vbox, hbox, FALSE, FALSE, 0);
    gtk_box_pack_startC(hbox, gtk_label_new(_("Select\nEngine")), FALSE, FALSE, 0);
    gtk_box_pack_startC(hbox, EngineCombo, FALSE, FALSE, 0);
    gtk_box_pack_startC(vbox, gtk_hseparator_new(), FALSE, FALSE, 0);
    EngineContainer = gtk_alignment_new(0, 0, 1, 1); // really only needed for the bin-ness
    gtk_box_pack_startC(vbox, EngineContainer, TRUE, TRUE, 0);
}
static char* canonize_name(char* dlname)
{
    char* end;
    char* begin = g_strrstr(dlname, "/lib");
    if (!begin) {
        return g_strdup("");
    }
    begin += 4;
    begin = g_strdup(begin);
    end = g_strrstr(begin, ".so");
    end[0] = '\0';
    return begin;
}
static void engine_comp(EngineData* d, void* p)
{
    FindEngine* e = p;
    if (!strcmp(e->canname, d->canname)) {
        e->found = TRUE;
    }
}
static bool engine_is_unique(char* canname)
{
    FindEngine e;
    e.canname = canname;
    e.found = FALSE;
    g_slist_foreach(EngineList, (GFunc)engine_comp, &e);
    return !e.found;
}
static void append_engine(char* dlname)
{
    char* can;
    char* err;
    (void) dlerror();
    void* hand = dlopen(dlname, RTLD_NOW);
    err = dlerror();
    if (!hand || err) {
        g_warning("%s", err);
        if (hand) {
            dlclose(hand);
        }
        return;
    }
    can = canonize_name(dlname);
    if (engine_is_unique(can)) {
        layout_settings_proc lay;
        lay = dlsym(hand, "layout_engine_settings");
        if ((err = dlerror())) {
            g_warning("%s", err);
        }
        if (lay) {
            get_meta_info_proc meta;
            EngineData* d = malloc(sizeof(EngineData));
            GtkTreeIter i;
            const char* format =
                "<b>%s</b> (%s)\n"
                "<i><small>%s</small></i>";
            meta = dlsym(hand, "get_meta_info");
            if ((err = dlerror())) {
                g_warning("%s", err);
            }
            d->meta.description = g_strdup("No Description");
            d->meta.version = g_strdup("0.0");
            d->meta.last_compat = g_strdup("0.0");
            d->meta.icon = gtk_widget_render_icon(EngineCombo, GTK_STOCK_MISSING_IMAGE,
                                                  GTK_ICON_SIZE_LARGE_TOOLBAR, "themeengine");
            if (meta) {
                meta(&(d->meta));
            } else {
                g_warning("Engine %s has no meta info, please update it, using defaults.", dlname);
            }

            d->dlname = dlname;
            d->canname = can;
            d->vbox = gtk_vbox_new(FALSE, 2);
            g_object_ref(d->vbox);
            lay(d->vbox);
            EngineList = g_slist_append(EngineList, d);
            gtk_list_store_append(EngineModel, &i);

            gtk_list_store_set(EngineModel, &i, ENGINE_COL_DLNAME, d->dlname, ENGINE_COL_NAME, d->canname,
                               ENGINE_COL_VER, d->meta.version, ENGINE_COL_LAST_COMPAT, d->meta.last_compat,
                               ENGINE_COL_ICON, d->meta.icon, ENGINE_COL_MARKUP,
                               g_markup_printf_escaped(format, d->canname, d->meta.version, d->meta.description),
                               -1);
            //gtk_combo_box_prepend_text(GTK_COMBO_BOX(EngineCombo),d->canname);
        }
    }
    dlclose(hand);
}
static void engine_scan_dir(char* dir)
{
    GDir* d;
    d = g_dir_open(dir, 0, NULL);
    if (d) {
        char* n;
        GPatternSpec* ps;
        ps = g_pattern_spec_new("lib*.so");
        while ((n = (char*) g_dir_read_name(d))) {
            if (g_pattern_match_string(ps, n)) {
                char* dln = g_strjoin("/", dir, n, NULL);
                append_engine(dln);
            }
        }
        g_pattern_spec_free(ps);
        g_dir_close(d);
    }
}
void init_engine_list()
{
    //presumes the container & combo are created
    //presumes the combo is NOT registered
    GtkCellRenderer* r;

    EngineModel = gtk_list_store_new(ENGINE_COL_COUNT, G_TYPE_STRING,
                                     G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, GDK_TYPE_PIXBUF);
    char* local_engine_dir = g_strjoin("/", g_get_home_dir(), ".emerald/engines", NULL);
    gtk_combo_box_set_model(GTK_COMBO_BOX(EngineCombo), GTK_TREE_MODEL(EngineModel));
    r = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(EngineCombo), r, FALSE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(EngineCombo), r, "pixbuf", ENGINE_COL_ICON);
    r = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(EngineCombo), r, TRUE);
    gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(EngineCombo), r, "markup", ENGINE_COL_MARKUP);
    engine_scan_dir(local_engine_dir);
    g_free(local_engine_dir);
    engine_scan_dir(ENGINE_DIR);

    SettingItem::register_setting(EngineCombo, ST_ENGINE_COMBO, "engine", "engine");
}
GtkWidget* build_notebook_page(char* title, GtkWidget* notebook)
{
    GtkWidget* vbox;
    vbox = gtk_vbox_new(FALSE, 2);
    gtk_container_set_border_widthC(vbox, 8);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox,
                             gtk_label_new(title));
    return vbox;
}

