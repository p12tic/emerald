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

#include <engine.h>
#include <signal.h>
#include <list>
#include <cstring>

EngineColumns g_engine_columns;
std::list<SettingItem> g_setting_list;
std::list<EngineData> g_engine_list;
Gtk::ComboBox* engine_combo_;
Glib::RefPtr<Gtk::ListStore> engine_model_;
Gtk::Alignment* engine_container_;
//GtkWidget * PreviewImage[BX_COUNT];
//GtkWidget * ButtonImage[BX_COUNT];
bool apply = false;
bool changed = false;
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

std::list<SettingItem>& get_setting_list()
{
    return g_setting_list;
}

Gtk::Scale* scaler_new(double low, double high, double prec)
{
    Gtk::Scale* w = Gtk::manage(new Gtk::HScale(low, high, prec));
    w->set_value_pos(Gtk::POS_RIGHT);
    w->set_update_policy(Gtk::UPDATE_DISCONTINUOUS);
    w->set_size_request(100, -1);
    return w;
}

void add_color_alpha_value(const char* caption, const char* basekey,
                           const char* sect, bool active)
{
    char* colorkey;
    char* alphakey;
    colorkey = g_strdup_printf(active ? "active_%s" : "inactive_%s", basekey);
    alphakey = g_strdup_printf(active ? "active_%s_alpha" : "inactive_%s_alpha",
                               basekey);

    table_append(*Gtk::manage(new Gtk::Label(caption)), false);

    auto* color_btn = Gtk::manage(new Gtk::ColorButton());
    table_append(*color_btn, false);
    SettingItem::create(*color_btn, sect, colorkey);

    auto* scaler = scaler_new(0.0, 1.0, 0.01);
    table_append(*scaler, true);
    SettingItem::create(*scaler, sect, alphakey);
    //we don't g_free because they are registered with SettingItem::create
}

void make_labels(const char* header)
{
    table_append(*Gtk::manage(new Gtk::Label(header)), false);
    table_append(*Gtk::manage(new Gtk::Label("Color")), false);
    table_append(*Gtk::manage(new Gtk::Label("Opacity")), false);
}

Gtk::Box* build_frame(Gtk::Box& vbox, const char* title, bool is_hbox)
{
    Gtk::Frame* frame = Gtk::manage(new Gtk::Frame(title));
    vbox.pack_start(*frame, Gtk::PACK_EXPAND_WIDGET);
    Gtk::Box* box;
    if (is_hbox) {
        box = Gtk::manage(new Gtk::HBox(false, 2));
    } else {
        box = Gtk::manage(new Gtk::VBox(false, 2));
    }
    box->set_border_width(8);
    frame->add(*box);
    return box;
}

static int current_table_width;
static Gtk::Table* current_table_;
static int current_table_col;
static int current_table_row;

void table_new(int width, bool same, bool labels)
{
    //WARNING - clobbers all the current_table_ vars.
    current_table_ = Gtk::manage(new Gtk::Table());
    current_table_->set_homogeneous(same);
    current_table_->resize(1, width);
    current_table_->set_row_spacings(8);
    current_table_->set_col_spacings(8);
    current_table_col = labels ? 1 : 0;
    current_table_row = 0;
    current_table_width = width;
}

void table_append(Gtk::Widget& child, bool stretch)
{
    current_table_->attach(child, current_table_col, current_table_col+1,
                           current_table_row, current_table_row+1,
                           (stretch ? Gtk::EXPAND : Gtk::SHRINK) | Gtk::FILL,
                           (stretch ? Gtk::EXPAND : Gtk::SHRINK) | Gtk::FILL);
    current_table_col++;
    if (current_table_col == current_table_width) {
        current_table_col = 0;
        current_table_row++;
    //  current_table_.resize(current_table_row+1, current_table_width);
    }
}

void table_append_separator()
{
    current_table_col = 0;
    current_table_row++;
//  current_table_->resize(current_table_row+1, current_table_width);
    current_table_->attach(*Gtk::manage(new Gtk::HSeparator()),
                           0, current_table_width,
                           current_table_row, current_table_row+1);
    current_table_row++;
//  current_table_.resize(current_table_row+1, current_table_width);
}

Gtk::Table& get_current_table()
{
    return *current_table_;
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
        const char* args[] =
        {"killall", "-u", (char*)g_get_user_name(), "-SIGUSR1", "emerald", NULL};
        const char* ret = NULL;
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
    for (auto& item : g_setting_list) {
        item.write_setting(global_theme_file);
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
void cb_apply_setting(SettingItem* item)
{
    if (item->type_ == ST_IMG_FILE) {
        std::string s = ((Gtk::FileChooser*) item->widget_)->get_filename();
        if (s.empty()) {
            return;    // for now just ignore setting it to an invalid name
        }
        if (s == item->fvalue_) {
            return;
        }
        item->fvalue_ = s;
        item->check_file(s.c_str());
    }
    item->write_setting((void*) global_theme_file);
    if (apply) {
        apply_settings();
    } else {
        changed = true;
    }
}
#ifdef USE_DBUS
void setup_dbus()
{
    dbcon = dbus_bus_get(DBUS_BUS_SESSION, NULL);
    dbus_connection_setup_with_g_main(dbcon, NULL);
}
#endif

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
    engine_container_->remove();
    for (auto& item : g_engine_list) {
        if (strcmp(nam, item.canname) == 0) {
            engine_container_->add(*(item.vbox));
            engine_container_->show_all();
        }
    }

}

bool get_engine_meta_info(const char* engine, EngineMetaInfo* inf)
{
    for (auto& item : g_engine_list) {
        if (!strcmp(item.canname, engine) == 0) {
            *inf = item.meta;
            return true;
        }
    }
    return false;
}
void update_preview(Gtk::FileChooser* chooser, const std::string& filename,
                    Gtk::Image* img)
{
    auto pixbuf = Gdk::Pixbuf::create_from_file(filename);
    bool have_preview = bool(pixbuf);
    img->set(pixbuf);
    chooser->set_preview_widget_active(have_preview);
}

void update_preview_cb(Gtk::FileChooser* chooser, Gtk::Image* img)
{
    std::string fn = std::string(chooser->get_preview_filename());
    update_preview(chooser, fn, img);
}

void init_settings()
{
    char* file = g_strjoin("/", g_get_home_dir(), ".emerald/theme/theme.ini", NULL);
    g_key_file_load_from_file(global_theme_file, file, G_KEY_FILE_KEEP_COMMENTS, NULL);
    g_free(file);
    file = g_strjoin("/", g_get_home_dir(), ".emerald/settings.ini", NULL);
    g_key_file_load_from_file(global_settings_file, file, G_KEY_FILE_KEEP_COMMENTS, NULL);
    g_free(file);
    for (auto& item : g_setting_list) {
        item.read_setting((void*)global_theme_file);
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
void cb_clear_file(SettingItem* item)
{
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
void layout_engine_list(Gtk::Box& vbox)
{
    engine_combo_ = Gtk::manage(new Gtk::ComboBox{});
    auto hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, Gtk::PACK_SHRINK);
    vbox.pack_start(*Gtk::manage(new Gtk::Label(_("Select\nEngine"))), Gtk::PACK_SHRINK);
    vbox.pack_start(*engine_combo_, Gtk::PACK_SHRINK);
    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()));
    // really only needed for the bin-ness
    engine_container_ = Gtk::manage(new Gtk::Alignment(0.0, 0.0, 1.0, 1.0));
    vbox.pack_start(*engine_container_, Gtk::PACK_EXPAND_WIDGET);
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

static bool engine_is_unique(const char* canname)
{
    for (auto& item : g_engine_list) {
        if (strcmp(item.canname, canname) == 0) {
            return false;
        }
    }
    return true;
}

static void append_engine(const char* dlname)
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
        layout_settings_proc lay = dlsym(hand, "layout_engine_settings");
        if ((err = dlerror())) {
            g_warning("%s", err);
        }
        if (lay) {
            get_meta_info_proc meta;
            g_engine_list.push_back(EngineData());
            EngineData* d = &g_engine_list.back();
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
            auto icon_theme = Gtk::IconTheme::get_default();
            d->meta.icon = icon_theme->load_icon(GTK_STOCK_MISSING_IMAGE, Gtk::ICON_SIZE_LARGE_TOOLBAR);

            if (meta) {
                meta(&(d->meta));
            } else {
                g_warning("Engine %s has no meta info, please update it, using defaults.", dlname);
            }

            d->dlname = dlname;
            d->canname = can;
            d->vbox = Gtk::manage(new Gtk::VBox{false, 2});
            lay(*(d->vbox));

            Gtk::TreeModel::Row row = *(engine_model_->append());
            row[g_engine_columns.dlname] = d->dlname;
            row[g_engine_columns.name] = d->canname;
            row[g_engine_columns.version] = d->meta.version;
            row[g_engine_columns.last_compat] = d->meta.last_compat;
            row[g_engine_columns.icon] = d->meta.icon;
            row[g_engine_columns.markup] = g_markup_printf_escaped(format, d->canname, d->meta.version, d->meta.description); // FIXME: LEAK possible

            //engine_combo_->prepend_text(d->canname);
        }
    }
    dlclose(hand);
}

static void engine_scan_dir(const char* dir)
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
    Gtk::CellRenderer* r;

    std::string local_engine_dir{g_get_home_dir()};
    local_engine_dir += "/.emerald/engines";

    engine_model_ = Gtk::ListStore::create(g_engine_columns);
    engine_combo_->set_model(engine_model_);
    r = Gtk::manage(new Gtk::CellRendererPixbuf());
    engine_combo_->pack_start(*r, false);
    engine_combo_->add_attribute(*r, "pixbuf", g_engine_columns.icon.index());
    r = Gtk::manage(new Gtk::CellRendererText());
    engine_combo_->pack_start(*r, true);
    engine_combo_->add_attribute(*r, "markup", g_engine_columns.markup.index());
    engine_scan_dir(local_engine_dir.c_str());
    engine_scan_dir(ENGINE_DIR);
    SettingItem::create_engine(*engine_combo_, "engine", "engine");
}

Gtk::Box* build_notebook_page(const char* title, Gtk::Notebook& notebook)
{
    auto* vbox = Gtk::manage(new Gtk::VBox(false, 2));
    vbox->set_border_width(8);
    notebook.append_page(*vbox, title, "");
    return vbox;
}

