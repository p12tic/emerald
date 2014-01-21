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

#include <libengine/engine.h>
#include <libengine/filesystem.h>
#include <libengine/format.h>
#include <signal.h>
#include <boost/algorithm/string.hpp>
#include <list>
#include <cstring>
#include <memory>

namespace fs = boost::filesystem;

EngineColumns g_engine_columns;
std::list<SettingItem> g_setting_list;
std::list<EngineData> g_engine_list;
Gtk::ComboBox* engine_combo_;
Glib::RefPtr<Gtk::ListStore> engine_model_;
Gtk::Alignment* engine_container_;
bool apply = false;
bool changed = false;

std::shared_ptr<KeyFile> global_theme_file;
std::shared_ptr<KeyFile> global_settings_file;

#ifdef USE_DBUS
DBusConnection* dbcon;
#endif
std::string active_engine;

std::string display_part(std::string name)
{
    auto colon_pos = name.find_last_of(':');
    if (colon_pos != std::string::npos) {
        name = name.substr(colon_pos+1);
    }
    auto dot_pos = name.find_first_of('.');
    name = name.substr(0, dot_pos);

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

void add_color_alpha_value(const std::string& caption, const std::string& basekey,
                           const std::string& sect, bool active)
{
    std::string colorkey = active ? "active_" : "inactive_";
    colorkey += basekey;
    std::string alphakey = active ? "active_" : "inactive_";
    alphakey += basekey + "_alpha";

    table_append(*Gtk::manage(new Gtk::Label(caption)), false);

    auto* color_btn = Gtk::manage(new Gtk::ColorButton());
    table_append(*color_btn, false);
    SettingItem::create(*color_btn, sect, colorkey);

    auto* scaler = scaler_new(0.0, 1.0, 0.01);
    table_append(*scaler, true);
    SettingItem::create(*scaler, sect, alphakey);
    //we don't g_free because they are registered with SettingItem::create
}

void make_labels(const std::string& header)
{
    table_append(*Gtk::manage(new Gtk::Label(header)), false);
    table_append(*Gtk::manage(new Gtk::Label("Color")), false);
    table_append(*Gtk::manage(new Gtk::Label("Opacity")), false);
}

Gtk::Box* build_frame(Gtk::Box& vbox, const std::string& title, bool is_hbox)
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

    std::string atom_name = std::string{"_COMPIZ_DM_S"} + display_part(getenv("DISPLAY"));

    if (dpy) {
        wmAtom = XInternAtom(dpy, atom_name.c_str(), 0);
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
        XSendEvent(dpy, w, False, NoEventMask, &clientEvent);
        XSync(dpy, False);
    } else {
        std::vector<std::string> args =
            {"killall", "-u", g_get_user_name(), "-SIGUSR1", "emerald", NULL};
        int ret;
        Glib::spawn_sync(".", args, Glib::SPAWN_STDERR_TO_DEV_NULL | Glib::SPAWN_SEARCH_PATH,
                         sigc::slot<void>{}, nullptr, nullptr, &ret);
        if (ret != 0) {
            g_warning("Couldn't find running emerald, no reload signal sent.");
        }
    }
#endif
}

void apply_settings()
{
    fs::path homedir = g_get_home_dir();
    fs::path path = homedir / ".emerald/theme";
    fs::path file = path / "theme.ini";

    for (auto& item : g_setting_list) {
        item.write_setting(*global_theme_file);
    }

    global_theme_file->set_string("theme", "version", VERSION);
    fs::create_directories(path);
    Glib::file_set_contents(file.native(), global_theme_file->to_data());
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
    item->write_setting(*global_theme_file);
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

void do_engine(const std::string& name)
{
    if (active_engine != name) {
        return;
    }

    active_engine = name;
    engine_container_->remove();

    for (auto& item : g_engine_list) {
        if (name == item.canname) {
            engine_container_->add(*(item.vbox));
            engine_container_->show_all();
        }
    }
}

bool get_engine_meta_info(const std::string& engine, EngineMetaInfo* inf)
{
    for (auto& item : g_engine_list) {
        if (item.canname == engine) {
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
    std::string homedir = g_get_home_dir();
    std::string file;

    file = homedir + "/.emerald/theme/theme.ini";
    global_theme_file.reset(new KeyFile);
    global_theme_file->load_from_file(file, Glib::KEY_FILE_KEEP_COMMENTS);

    file = homedir + "/.emerald/settings.ini";
    global_settings_file.reset(new KeyFile);
    global_settings_file->load_from_file(file, Glib::KEY_FILE_KEEP_COMMENTS);

    for (auto& item : g_setting_list) {
        item.read_setting(*global_theme_file);
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
    static_cast<Gtk::FileChooserDialog*>(item->widget_)->unselect_all();
    item->write_setting(*global_theme_file);
    if (apply) {
        apply_settings();
    }
}

void init_key_files()
{
    global_theme_file.reset(new KeyFile{});
    global_settings_file.reset(new KeyFile{});
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

std::string canonize_name(const std::string& dlname)
{
    auto res = boost::algorithm::find_last(dlname, "/lib");
    if (res.begin() == res.end()) {
        return "";
    }
    auto res2 = boost::algorithm::find_last(dlname, ".so");
    if (res2.end() == dlname.end()) {
        return std::string{res.end(), res2.begin()};
    } else {
        return std::string{res.end(), dlname.end()};
    }
}

bool engine_is_unique(const std::string& canname)
{
    for (auto& item : g_engine_list) {
        if (item.canname == canname) {
            return false;
        }
    }
    return true;
}

void append_engine(const std::string& dlname)
{
    (void) dlerror();
    void* hand = dlopen(dlname.c_str(), RTLD_NOW);
    const char* err = dlerror();
    if (!hand || err) {
        g_warning("%s", err);
        if (hand) {
            dlclose(hand);
        }
        return;
    }
    std::string can = canonize_name(dlname);
    if (engine_is_unique(can)) {
        layout_settings_proc lay = reinterpret_cast<layout_settings_proc>(dlsym(hand, "layout_engine_settings"));
        if ((err = dlerror())) {
            g_warning("%s", err);
        }
        if (lay) {
            get_meta_info_proc meta;
            g_engine_list.push_back(EngineData());
            EngineData* d = &g_engine_list.back();
            meta = reinterpret_cast<get_meta_info_proc>(dlsym(hand, "get_meta_info"));
            if ((err = dlerror())) {
                g_warning("%s", err);
            }
            d->meta.description = "No Description";
            d->meta.version = "0.0";
            d->meta.last_compat = "0.0";
            auto icon_theme = Gtk::IconTheme::get_default();
            d->meta.icon = icon_theme->load_icon(GTK_STOCK_MISSING_IMAGE, Gtk::ICON_SIZE_LARGE_TOOLBAR);

            if (meta) {
                meta(&(d->meta));
            } else {
                g_warning("Engine %s has no meta info, please update it, using defaults.", dlname.c_str());
            }

            d->dlname = dlname;
            d->canname = can;
            d->vbox = Gtk::manage(new Gtk::VBox{false, 2});
            lay(*(d->vbox));

            std::string markup = format("<b>%s</b> (%s)\n<i><small>%s</small></i>",
                                        markup_escape(d->canname),
                                        markup_escape(d->meta.version),
                                        markup_escape(d->meta.description));

            Gtk::TreeModel::Row row = *(engine_model_->append());
            row[g_engine_columns.dlname] = d->dlname;
            row[g_engine_columns.name] = d->canname;
            row[g_engine_columns.version] = d->meta.version;
            row[g_engine_columns.last_compat] = d->meta.last_compat;
            row[g_engine_columns.icon] = d->meta.icon;
            row[g_engine_columns.markup] = markup;

            //engine_combo_->prepend_text(d->canname);
        }
    }
    dlclose(hand);
}

void engine_scan_dir(const std::string& dir)
{
    if (!fs::is_directory(dir)) {
        return;
    }

    Glib::PatternSpec ps("lib*.so");
    for (auto& entry : fs::directory_iterator(dir)) {
        if (ps.match(entry.path().filename().native())) {
            append_engine(entry.path().native());
        }
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

Gtk::Box* build_notebook_page(const std::string& title, Gtk::Notebook& notebook)
{
    auto* vbox = Gtk::manage(new Gtk::VBox(false, 2));
    vbox->set_border_width(8);
    notebook.append_page(*vbox, title, "");
    return vbox;
}

