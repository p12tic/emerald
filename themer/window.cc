/*  This file is part of emerald

    Copyright (C) 2006  Novell Inc.
    Copyright (C) 2013-2014  Povilas Kanapickas <povilas@radix.lt>

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

#include "window.h"
#include "compat.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <libengine/emerald.h>
#include <libengine/engine.h>
#include <libengine/keyfile.h>
#include <libengine/filesystem.h>
#include <libengine/format.h>
#include <gtkmm.h>
#include <boost/algorithm/string.hpp>
#include <functional>

namespace fs = boost::filesystem;

ThemerWindow::ThemerWindow(int install_file, const std::string& input_file)
{
    main_window_ = new Gtk::Window();

    if (install_file == 1) {
        create_filechooserdialog1(input_file);
    }

    main_window_->set_title("Emerald Themer " VERSION);
    main_window_->set_resizable(true);
    main_window_->set_default_size(700, 500);
    main_window_->signal_delete_event().connect([=](GdkEventAny* ev){ return cb_main_destroy(ev); });
    main_window_->set_default_icon_from_file(PIXMAPS_DIR "/emerald-theme-manager-icon.png");

    main_window_->set_border_width(5);

    layout_main_window();
    main_window_->show_all();

    refresh_theme_list("");
    copy_from_defaults_if_needed();
    init_settings();
    set_changed(false);
    set_apply(true);
}

struct FetcherInfo {
    Gtk::Dialog* dialog;
    Gtk::ProgressBar* progbar;
    GPid        pd;
};

void ThemerWindow::theme_list_append(const std::string& value,
                                     const std::string& dir,
                                     const std::string& fil)
{
    std::string path = dir + "/" + fil + "/theme.ini";
    std::string imgpath = dir + "/" + fil + "/theme.screenshot.png";

    EngineMetaInfo metainfo;

    KeyFile f;
    if (!f.load_from_file(path)) {
        return;
    }

    ThemeList::ThemeDesc desc;
    desc.name = value;

    auto engine_o = f.get_string_opt("engine", "engine");
    desc.engine = engine_o ? *engine_o : "";

    if (engine_o) {
        get_engine_meta_info(desc.engine, &metainfo);
        auto ver_o = f.get_string_opt("engine_version", *engine_o);
        std::string ver = ver_o ? *ver_o : "0.0.0"; //val2

        auto th_ver_o = f.get_string_opt("theme", "version");
        std::string th_ver = th_ver_o ? *th_ver_o : "0.0.0"; // tver

        std::string last_compat = metainfo.last_compat;
        if (last_compat.empty()) {
            last_compat = "0.0.0";
        }

        bool is_en_uptodate = strverscmp(ver.c_str(), last_compat.c_str()) >= 0;
        bool is_th_uptodate = strverscmp(th_ver.c_str(), LAST_COMPAT_VER) >= 0;

        desc.engine_version = std::string("Engine: ") + (is_en_uptodate ? "YES" : "NO") +
                " (" + ver + ")\n" + "Emerald: " + (is_th_uptodate ? "YES" : "NO") +
                " (" + th_ver + ")\n";

    } else {
        auto th_ver_o = f.get_string_opt("theme", "version");
        std::string th_ver = th_ver_o ? *th_ver_o : "0.0.0";

        bool is_th_uptodate = strverscmp(th_ver.c_str(), LAST_COMPAT_VER) >= 0;

        desc.engine_version = std::string("No Engine\n") +
                "Emerald: " + (is_th_uptodate ? "YES" : "NO") + " (" + th_ver + ")\n";
    }

    std::string s_creator, s_desc, s_th_version, s_sugg;
    auto opt = f.get_string_opt("theme", "creator");
    desc.creator = s_creator = opt ? *opt : "";

    opt = f.get_string_opt("theme", "description");
    desc.description = s_desc = opt ? *opt : "";

    opt = f.get_string_opt("theme", "theme_version");
    desc.theme_version = s_th_version = opt ? *opt : "";

    opt = f.get_string_opt("theme", "suggested");
    desc.suggested = s_sugg = opt ? *opt : "";

    {
        if (s_creator == "") {
            s_creator = _("Unknown");
        }
        if (s_th_version == "") {
            s_th_version = _("Unknown");
        }
        if (s_sugg == "") {
            s_sugg = _("(no hint)");
        }
        if (s_desc == "") {
            s_desc = _("No Description");
        }

        //create the Theme column data
        const char* fmt =
            _("<b><big>%s</big></b>\n"
              "<i>%s</i>\n"
              "<small>"
              "<b>Version</b> %s\n"
              "<b>Created By</b> %s\n"
              "<b>Use With</b> %s\n"
              "</small>");
        std::string name = value;
        if (value[0] == '*') {
            name = value.substr(2);
            fmt =
                _("<b><big>%s</big></b> (System Theme)\n"
                  "<i>%s</i>\n"
                  "<small>"
                  "<b>Version</b> %s\n"
                  "<b>Created By</b> %s\n"
                  "<b>Use With</b> %s\n"
                  "</small>");
        }

        desc.markup = format(fmt, markup_escape(name),
                             markup_escape(s_desc),
                             markup_escape(s_th_version),
                             markup_escape(s_creator),
                             markup_escape(s_sugg));
    }

    auto pix = Gdk::Pixbuf::create_from_file(imgpath, -1, 100);
    if (pix) {
        desc.pixbuf = pix;
    } else {
        auto icon_theme = Gtk::IconTheme::create();
        pix = icon_theme->load_icon(GTK_STOCK_MISSING_IMAGE, Gtk::ICON_SIZE_LARGE_TOOLBAR);
        desc.pixbuf = pix;
    }
    theme_list_.add_theme(desc);
}

void ThemerWindow::theme_scan_dir(const std::string& dir, bool writable)
{
    if (!fs::exists(dir)) {
        return;
    }
    for (auto& entry : fs::directory_iterator{dir}) {
        fs::path fn = entry.path() / "theme.ini";
        std::string name = entry.path().filename().native();
        if (fs::is_regular_file(fn)) {
            std::string th_name = writable ? name : std::string{"* "} + name;
            theme_list_append(th_name, dir, name);
        }
    }
}

void ThemerWindow::scroll_to_theme(const std::string& theme)
{
    theme_list_.scroll_to_theme(theme);
}

void ThemerWindow::refresh_theme_list(const std::string& theme)
{
    theme_list_.clear();
    theme_scan_dir(DATA_DIR "/emerald/themes/", false);
    auto path = std::string{g_get_home_dir()} + "/.emerald/themes/";
    theme_scan_dir(path, true);
    if (!theme.empty()) {
        scroll_to_theme(theme);
    }
}

bool ThemerWindow::confirm_dialog(const std::string& fmt, const std::string& val)
{
    Gtk::MessageDialog dlg(*main_window_, std::string(format(fmt, val)), false,
                           Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
    return (dlg.run() == Gtk::RESPONSE_YES);
}

void ThemerWindow::info_dialog(const std::string& val)
{
    Gtk::MessageDialog dlg(*main_window_, val, false, Gtk::MESSAGE_INFO,
                           Gtk::BUTTONS_CLOSE, true);
    dlg.run();
}

void ThemerWindow::error_dialog(const std::string& val)
{
    Gtk::MessageDialog dlg(*main_window_, val, false, Gtk::MESSAGE_ERROR,
                           Gtk::BUTTONS_CLOSE, true);
    dlg.run();
}

void ThemerWindow::cb_load()
{
    //first normalize the name
    auto sel_opt = theme_list_.get_selected_theme_name();
    if (!sel_opt) {
        return;
    }
    std::string at = *sel_opt;
    std::string mt = at;
    bool dist = false;
    if (at.length() >= 1 && at[0] == '*') {
        if (at.length() >= 2) {
            mt = at.substr(2);
            dist = true;
        } else {
            error_dialog(_("Invalid Theme Name"));
            return;
        }
    }
    if (mt.length() == 0) {
        error_dialog(_("Invalid Theme Name"));
        return;
    }
    delete_button_->set_sensitive(!dist);

    std::string xt = std::string{g_get_home_dir()} + "/.emerald/theme/";
    for (auto& entry : fs::directory_iterator{xt}) {
        fs::remove(entry.path());
    }

    std::string fn;
    if (dist) {
        fn = std::string{DATA_DIR} + "/emerald/themes/" + mt + "/theme.ini";
        at = std::string{DATA_DIR} + "/emerald/themes/" + mt + "/";
    } else {
        fn = std::string{g_get_home_dir()} + "/.emerald/themes/" + mt + "/theme.ini";
        at = std::string{g_get_home_dir()} + "/.emerald/themes/" + mt + "/";
    }

    for (auto& entry : fs::directory_iterator{at}) {
        fs::path dest = fs::path{xt} / entry.path().filename();
        fs::copy_file(entry.path(), dest);
    }

    KeyFile f;
    if (!f.load_from_file(fn)) {
        error_dialog("Invalid Theme File / Name");
        return;
    }
    entry_box_->set_text(mt);
    set_changed(true);
    set_apply(false);
    for (auto& item : get_setting_list()) {
        item.read_setting(f);
    }
    {
        auto opt = f.get_string_opt("theme", "version");
        if (opt) {
            version_entry_->set_text(*opt);
        } else {
            version_entry_->set_text("Pre-0.8");
        }
    }
    set_apply(true);
    send_reload_signal();
}

std::string ThemerWindow::import_theme(const std::string& file_s)
{
    fs::path homedir = g_get_home_dir();
    fs::path file = file_s;

    if (file.extension() != ".emerald") {
        error_dialog(_("Invalid theme file.  Does not end in .emerald"));
        return NULL;
    }

    std::string name = file.stem().native();
    fs::path theme_dir = homedir / ".emerald/themes" / name;
    fs::create_directories(theme_dir);

    std::string quoted_dir = Glib::shell_quote(theme_dir.native());
    std::string quoted_name = Glib::shell_quote(file.native());

    std::string command = std::string{"tar -xzf "} + quoted_name + " -C " + quoted_dir;
    int ret;
    Glib::spawn_command_line_sync(command, nullptr, nullptr, &ret);
    if (WIFEXITED(ret) && WEXITSTATUS(ret)) {
        error_dialog("Error calling tar.");
        return nullptr;
    }
//   info_dialog(_("Theme Imported"));
    return name;
}

void ThemerWindow::export_theme(const std::string& file_s)
{
    fs::path homedir = g_get_home_dir();

    std::string themename = entry_box_->get_text();
    if (themename.empty() || themename[0] == '*'
            || themename.find_first_of('/') != std::string::npos) {
        error_dialog(_("Invalid Theme Name\nCould Not Export"));
        //these conditions should already have been handled but another check is ok
        return;
    }

    fs::path file = file_s;
    if (file.extension() != ".emerald") {
        error_dialog(_("Invalid File Name\nMust End in .emerald"));
        return;
    }

    fs::path theme_dir = homedir / ".emerald/theme";

    std::string quoted_dir = Glib::shell_quote(theme_dir.native());
    std::string quoted_file = Glib::shell_quote(file.native());

    std::string command = std::string{"tar -czf "} + quoted_file + " -C "
            + quoted_dir + " ./ --exclude=*~";

    int ret;
    Glib::spawn_command_line_sync(command, nullptr, nullptr, &ret);
    if (WIFEXITED(ret) && WEXITSTATUS(ret)) {
        error_dialog(_("Error calling tar."));
        return;
    }
    info_dialog(_("Theme Exported"));
}

void ThemerWindow::cb_save()
{
    //first normalize the name
    std::string at = entry_box_->get_text();
    if (at.empty()) {
        return;
    }
    if (at.length() >= 1 && at[0] == '*') {
        error_dialog(_("Can't save over system themes\n(or you forgot to enter a name)"));
        return;
    }
    if (at.length() || at.find_first_of('/') != std::string::npos) {
        error_dialog(_("Invalid Theme Name"));
        return;
    }

    fs::path homedir = g_get_home_dir();
    fs::path theme_dir = homedir / ".emerald/themes" / at;
    fs::create_directories(theme_dir);
    fs::path fn = theme_dir / "theme.ini";

    KeyFile f;
    f.load_from_file(fn.native());
    for (auto& item : get_setting_list()) {
        item.write_setting(f);
    }
    f.set_string("theme", "version", VERSION);
    if (fs::is_regular_file(fn)) {
        if (!confirm_dialog(_("Overwrite Theme %s?"), at)) {
            return;
        }
    }
    std::string mt = at;

    std::string dt = f.to_data();
    //little fix since we're now copying from ~/.emerald/theme/*

    fs::path def_fn = homedir / ".emerald/theme/theme.ini";

    bool success = false;
    try {
        Glib::file_set_contents(def_fn.native(), dt);
        success = true;
    } catch (...) { // TODO: handle elsewhere
        error_dialog(_("Couldn't Write Theme"));
    };
    if (success) {
        fs::path xt = homedir / ".emerald/themes" / mt;

        for (auto& entry : fs::directory_iterator{xt}) {
            fs::remove(entry.path());
        }

        fs::path at = homedir / ".emerald/theme";
        for (auto& entry : fs::directory_iterator{at}) {
            fs::path dest = fs::path{xt} / entry.path().filename();
            fs::copy_file(entry.path(), dest);
        }
        info_dialog(_("Theme Saved"));
    }
    version_entry_->set_text(VERSION);
    refresh_theme_list("");
}

void ThemerWindow::cb_delete()
{
    //first normalize the name
    auto row_opt = theme_list_.get_selected_theme_name();
    if (!row_opt) {
        return;
    }
    std::string at = *row_opt;

    if (at.length() >= 1 && at[0] == '*') {
        error_dialog(_("Can't delete system themes\n(or you forgot to enter a name)"));
        return;
    }
    if (at.length() == 0) {
        error_dialog(_("Invalid Theme Name"));
        return;
    }

    fs::path homedir = g_get_home_dir();
    fs::path fn = homedir / ".emerald/themes" / at / "theme.ini";
    if (fs::is_regular_file(fn)) {
        if (!confirm_dialog(_("Are you sure you want to delete %s?"), at)) {
            return;
        } else {
            fs::remove_all(homedir / ".emerald/themes" / at);
        }
        info_dialog(_("Theme deleted"));
        delete_button_->set_sensitive(false);
        refresh_theme_list("");
    } else {
        error_dialog(_("No such theme to delete"));
    }
}

bool ThemerWindow::cb_main_destroy(GdkEventAny*)
{
    main_window_->hide();
    Gtk::Main::quit();
    return true;
}

void ThemerWindow::layout_button_box(SettingsTable& tbl, Gtk::Box& vbox, int b_t)
{
    (void) vbox;
    tbl.append(*Gtk::manage(new Gtk::Label(b_names[b_t])), false);
    std::string bt_nm = std::string(b_names[b_t]) + " Button Pixmap";
    auto& filesel = *Gtk::manage(new Gtk::FileChooserButton(bt_nm,
                                          Gtk::FILE_CHOOSER_ACTION_OPEN));
    tbl.append(filesel, false);

    Gtk::FileFilter filter;
    filter.set_name("Images");
    filter.add_pixbuf_formats();
    filesel.add_filter(filter);

    auto& image = *Gtk::manage(new Gtk::Image());

    SettingItem* item;
    item = SettingItem::register_img_file_setting(filesel, "buttons", b_types[b_t], &image);

    tbl.append(image, true);

    auto& clearer = *Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
    clearer.signal_clicked().connect([=](){ cb_clear_file(item); });
    tbl.append(clearer, false);
}

void ThemerWindow::layout_general_buttons_frame(Gtk::Box& hbox)
{
    {
    auto& btn = *Gtk::manage(new Gtk::CheckButton(_("Use Pixmap Buttons")));
    hbox.pack_start(btn, true, true);
    SettingItem::create(btn, "buttons", "use_pixmap_buttons");
    }{
    auto& btn = *Gtk::manage(new Gtk::CheckButton(_("Use Button Halo/Glow")));
    hbox.pack_start(btn, true, true);
    SettingItem::create(btn, "buttons", "use_button_glow");
    }{
    auto& btn = *Gtk::manage(new Gtk::CheckButton(_("Use Button Halo/Glow For Inactive Windows")));
    hbox.pack_start(btn, true, true);
    SettingItem::create(btn, "buttons", "use_button_inactive_glow");
    }
}

void ThemerWindow::layout_button_pane(Gtk::Box& vbox)
{
    int i;
    /* Yeah, the names should probably be renamed from hbox since it's now
     * a vbox...
     */
    Gtk::Box& hbox = *Gtk::manage(new Gtk::VBox(false, 2));
    vbox.pack_start(hbox, false, false);
    layout_general_buttons_frame(hbox);

    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false);

    vbox.pack_start(*Gtk::manage(new Gtk::Label(_("Button Pixmaps"))), false, false);

    auto& scroller = *Gtk::manage(new Gtk::ScrolledWindow());
    scroller.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    vbox.pack_start(scroller, true, true);

    SettingsTable tbl(4, false, false);
    scroller.add(tbl.table());

    tbl.append(*Gtk::manage(new Gtk::Label(_("Button"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("File"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Preview"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Clear"))), false);

    for (i = 0; i < BX_COUNT; i++) {
        layout_button_box(tbl, vbox, i);
    }
}

void ThemerWindow::layout_window_frame(Gtk::Box& vbox, bool active)
{
    auto& scrollwin = *Gtk::manage(new Gtk::ScrolledWindow());
    vbox.pack_start(scrollwin, true, true);
    scrollwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    SettingsTable tbl(3, false, false);
    scrollwin.add(tbl.table());
    tbl.append_header(_("Colors"));
    tbl.append_acolor(_("Text Fill"), "text", "titlebar", active);
    tbl.append_acolor(_("Text Outline"), "text_halo", "titlebar", active);
    tbl.append_separator();
    tbl.append_acolor(_("Button Fill"), "button", "buttons", active);
    tbl.append_acolor(_("Button Outline"), "button_halo", "buttons", active);
}

void ThemerWindow::add_row(SettingsTable& tbl, Gtk::Box& vbox,
                           Gtk::Widget& item, const std::string& title)
{
    (void) vbox;
    //vbox.pack_start(*Gtk::manage(new Gtk::Label(title)),false,false);
    //vbox.pack_end(item,true,true,0);
    tbl.append(*Gtk::manage(new Gtk::Label(title)), false);
    tbl.append(item, true);
}

void ThemerWindow::add_color_button_row(SettingsTable& tbl, Gtk::Box& vbox,
                                        const std::string& title,
                                        const std::string& key,
                                        const std::string& sect)
{
    auto& color_button = *Gtk::manage(new Gtk::ColorButton());
    SettingItem::create(color_button, sect, key);
    add_row(tbl, vbox, color_button, title);
}

void ThemerWindow::add_int_range_row(SettingsTable& tbl, Gtk::Box& vbox,
                                     const std::string& title,
                                     const std::string& key,
                                     int start, int end, const std::string& sect)
{
    auto& scaler = *scaler_new(start, end, 1);
    SettingItem::create(scaler, sect, key);
    add_row(tbl, vbox, scaler, title);
}

void ThemerWindow::add_float_range_row(SettingsTable& tbl, Gtk::Box& vbox,
                                       const std::string& title,
                                       const std::string& key,
                                       double start, double end, double prec,
                                       const std::string& sect)
{
    auto& scaler = *scaler_new(start, end, prec);
    SettingItem::create(scaler, sect, key);
    add_row(tbl, vbox, scaler, title);
}

void ThemerWindow::layout_shadows_frame(Gtk::Box& vbox)
{
    SettingsTable tbl(2, false, false);
    vbox.pack_start(tbl.table(), false, false);
    add_color_button_row(tbl, vbox, _("Color"), "shadow_color", "shadow");
    add_float_range_row(tbl, vbox, _("Opacity"), "shadow_opacity", 0.01, 6.0, 0.01, "shadow");
    add_float_range_row(tbl, vbox, _("Radius"), "shadow_radius", 0.0, 48.0, 0.1, "shadow");
    add_int_range_row(tbl, vbox, _("X Offset"), "shadow_offset_x", -16, 16, "shadow");
    add_int_range_row(tbl, vbox, _("Y Offset"), "shadow_offset_y", -16, 16, "shadow");
}

void ThemerWindow::layout_title_frame(Gtk::Box& vbox)
{
    Gtk::Scale* scaler;

    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);

    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Title-Text Font"))), false, false, 0);

    auto& fontbtn = *Gtk::manage(new Gtk::FontButton);
    hbox.pack_start(fontbtn, false, false);
    SettingItem::create(fontbtn, "titlebar", "titlebar_font");

    SettingsTable tbl(2, false, false);
    vbox.pack_start(tbl.table(), false, false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Minimum Title-bar Height"))), false);
    scaler = scaler_new(0, 64, 1);
    scaler->set_value(17);
    tbl.append(*scaler, true);
    SettingItem::create(*scaler, "titlebar", "min_titlebar_height");

    tbl.append(*Gtk::manage(new Gtk::Label(_("Vertical Button Offset"))), false);
    scaler = scaler_new(0, 64, 1);
    scaler->set_value(1);
    tbl.append(*scaler, true);
    SettingItem::create(*scaler, "buttons", "vertical_offset");

    tbl.append(*Gtk::manage(new Gtk::Label(_("Horizontal Button Offset"))), false);
    scaler = scaler_new(0, 64, 1);
    scaler->set_value(1);
    tbl.append(*scaler, true);
    SettingItem::create(*scaler, "buttons", "horizontal_offset");

    {
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);
    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Title-bar Object Layout"))), false, false, 0);

    auto& combo = *Gtk::manage(new Gtk::ComboBoxEntryText());
    combo.append_text("IT::HNXC:Normal Layout");
    combo.append_text("CNX:IT:HM:OSX Layout");
    SettingItem::create(combo, "titlebar", "title_object_layout");
    hbox.pack_start(combo, false, false, 0);
    }

    vbox.pack_start(*Gtk::manage(new Gtk::Label(
                            _("Use colons to separate left, center, and right.\n"
                              "Anything after a third colon is ignored.\n"
                              "C=Close, N=Minimize, X/R=Restore\n"
                              "I=Icon, T=Title, H=Help, M=Menu\n"
                              "S=Shade, U/A=On Top(Up/Above)\n"
                              "Y=Sticky, (#)=# pixels space, ex: (5)\n")
                            //"U=On Top, D=On Bottom, S=Shade\n"
                            //"Y=Sticky (on All Desktops)\n"
                        )), false, false);

    /*btn = Gtk::manage(new Gtk::CheckButton("Use active colors for whole active frame, not just titlebar.");
    vbox.pack_start(*btn,false,false,0);
    SettingItem::create(*btn,ST_BOOL,SECT,"use_active_colors");*/
}

void ThemerWindow::add_meta_string_value(SettingsTable& tbl,
                                         const std::string& title,
                                         const std::string& key)
{
    tbl.append(*Gtk::manage(new Gtk::Label(title)), false);
    auto& entry = *Gtk::manage(new Gtk::Entry());
    tbl.append(entry, true);
    SettingItem::create(entry, "theme", key);
}

boost::optional<std::string> ThemerWindow::ask_filename_for_export()
{
    Gtk::FileChooserDialog& dialog =
            *Gtk::manage(new Gtk::FileChooserDialog(*main_window_,
                                                    _("Export Theme..."),
                                                    Gtk::FILE_CHOOSER_ACTION_SAVE));
    std::string pth = std::string(g_get_home_dir()) + "/Desktop/";
    Gtk::FileFilter filter;
    filter.set_name("Theme Packages");
    filter.add_pattern("*.emerald");
    dialog.add_filter(filter);
    dialog.set_do_overwrite_confirmation(true);
    dialog.set_current_folder(pth);

    pth = entry_box_->get_text();
    pth += ".emerald";
    dialog.set_current_name(pth);

    if (dialog.run() == Gtk::RESPONSE_ACCEPT) {
        return boost::optional<std::string>(dialog.get_filename());
    } else {
        return boost::none;
    }
}

void ThemerWindow::layout_file_frame(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);

    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Name:"))), false, false, 0);
    entry_box_ = Gtk::manage(new Gtk::Entry());
    entry_box_->set_text(_("Untitled Theme"));
    hbox.pack_start(*entry_box_, true, true, 0);

    auto& btn = *Gtk::manage(new Gtk::Button(Gtk::Stock::SAVE));
    hbox.pack_start(btn, false, false, 0);
    btn.signal_clicked().connect([=](){ cb_save(); });

    export_button_ = Gtk::manage(new Gtk::Button(_("Export...")));
    Gtk::Image im(Gtk::Stock::SAVE_AS, Gtk::ICON_SIZE_BUTTON);
    export_button_->set_image(im);
    hbox.pack_start(*export_button_, false, false, 0);
    export_button_->signal_clicked().connect([=](){
        auto fn_opt = ask_filename_for_export();
        if (fn_opt) {
            export_theme(*fn_opt);
        }
    });
}

void ThemerWindow::layout_info_frame(Gtk::Box& vbox)
{
    SettingsTable tbl(2, false, false);
    vbox.pack_start(tbl.table(), false, false);
    add_meta_string_value(tbl, _("Creator"), "creator");
    add_meta_string_value(tbl, _("Description"), "description");
    add_meta_string_value(tbl, _("Theme Version"), "theme_version");
    add_meta_string_value(tbl, _("Suggested Widget Theme"), "suggested");

    tbl.append_separator();

    tbl.append(*Gtk::manage(new Gtk::Label(_("Themer Version"))), false);

    version_entry_ = Gtk::manage(new Gtk::Entry);
    version_entry_->set_editable(false);
    version_entry_->set_sensitive(false);
    tbl.append(*version_entry_, true);
}

void ThemerWindow::add_border_slider(SettingsTable& tbl,
                                     const std::string& text,
                                     const std::string& key,
                                     int value)
{
    tbl.append(*Gtk::manage(new Gtk::Label(text)), false);

    auto& scaler = *scaler_new(0, 20, 1);
    tbl.append(scaler, true);
    scaler.set_value(value);
    SettingItem::create(scaler, "borders", key);
}

void ThemerWindow::layout_borders_frame(Gtk::Box& vbox)
{
    SettingsTable tbl(2, false, false);
    vbox.pack_start(tbl.table(), false, false, 0);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Border"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Size"))), false);
    add_border_slider(tbl, _("Top"), "top", 4);
    add_border_slider(tbl, _("Bottom"), "bottom", 6);
    add_border_slider(tbl, _("Left"), "left", 6);
    add_border_slider(tbl, _("Right"), "right", 6);
    vbox.pack_start(*Gtk::manage(new Gtk::Label(
                            _("Note, when changing these values,\n"
                              "it is advised that you do something\n"
                              "like change viewports, as this will\n"
                              "cause the position of the 'event windows'\n"
                              "to update properly, and thus make\n"
                              "moving/resizing windows work properly.\n"
                              "This also applies to when you change\n"
                              "the title-bar height through either\n"
                              "the titlebar-min-height slider\n"
                              "or the titlebar font setting.\n")
                        )), false, false);
}

void ThemerWindow::layout_left_frame_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    layout_shadows_frame(*build_frame(vbox, _("Shadows"), false));
}

void ThemerWindow::layout_right_frame_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    layout_borders_frame(*build_frame(vbox, _("Frame Borders"), false));
}

void ThemerWindow::layout_frame_pane(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(true, 2));
    vbox.pack_start(hbox, true, true);

    layout_left_frame_pane(hbox);
    layout_right_frame_pane(hbox);
}

void ThemerWindow::layout_left_global_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);

    layout_window_frame(*build_frame(vbox, _("Active Window"), false), true);
    layout_window_frame(*build_frame(vbox, _("Inactive Window"), false), false);
    //layout stuff here.
}

void ThemerWindow::layout_right_global_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    layout_title_frame(*build_frame(vbox, _("Title Bar"), false));
}

void ThemerWindow::layout_global_pane(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(true, 2));
    vbox.pack_start(hbox, true, true);
    layout_left_global_pane(hbox);
    layout_right_global_pane(hbox);
}

void ThemerWindow::layout_screenshot_frame(Gtk::Box& vbox)
{
    SettingItem* item;

    auto& image = *Gtk::manage(new Gtk::Image());

    auto& scrollwin = *Gtk::manage(new Gtk::ScrolledWindow());
    scrollwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrollwin.add(image);
    scrollwin.set_size_request(300, 160);
    vbox.pack_start(scrollwin, true, true);

    auto* hbox = Gtk::manage(new Gtk::HBox(true, 2));
    vbox.pack_start(*hbox, false, false);

    auto& filesel = *Gtk::manage(new Gtk::FileChooserButton("Screenshot",
                                                            Gtk::FILE_CHOOSER_ACTION_OPEN));
    hbox->pack_start(filesel, true, true);
    Gtk::FileFilter filter;
    filter.set_name(_("Images"));
    filter.add_pixbuf_formats();
    filesel.add_filter(filter);

    item = SettingItem::register_img_file_setting(filesel, "theme", "screenshot", &image);

    auto& clearer = *Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
    clearer.signal_clicked().connect([=](){ cb_clear_file(item); });
    hbox->pack_start(clearer, true, true);
}

void ThemerWindow::layout_left_theme_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    layout_screenshot_frame(*build_frame(vbox, _("Screenshot"), false));
}

void ThemerWindow::layout_right_theme_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    layout_info_frame(*build_frame(vbox, _("Information"), false));
}

void ThemerWindow::layout_theme_pane(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, true, true);
    layout_left_theme_pane(hbox);
    layout_right_theme_pane(hbox);
}

void ThemerWindow::layout_settings_pane(Gtk::Box& vbox)
{
    vbox.pack_start(*Gtk::manage(new Gtk::Label(
                        _("NOTE - These settings are not part of themes, "
                          "they are stored separately, and control various UI "
                          "preferences for Emerald.")
                    )), false, false, 0);
    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false);

    Gtk::CheckButton* btn;
    btn = Gtk::manage(new Gtk::CheckButton(_("Show Tooltips for Buttons")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create_global(*btn, "buttons", "enable_tooltips");

    btn = Gtk::manage(new Gtk::CheckButton(_("Use Decoration Cropping")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create_global(*btn, "decorations", "use_decoration_cropping");

    btn = Gtk::manage(new Gtk::CheckButton(_("Use Button Fade")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create_global(*btn, "buttons", "use_button_fade");

    btn = Gtk::manage(new Gtk::CheckButton(_("Use Button Fade Pulse")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create_global(*btn, "buttons", "use_button_fade_pulse");

    SettingsTable tbl(2, false, false);
    vbox.pack_start(tbl.table(), false, false);

    tbl.append(*Gtk::manage(new Gtk::Label(_("Button Fade Total Duration"))), false);
    Gtk::Scale* scaler;
    scaler = scaler_new(1, 4000, 1);
    scaler->set_value(250);
    tbl.append(*scaler, true);
    SettingItem::create_global(*scaler, "buttons", "button_fade_total_duration");

    tbl.append(*Gtk::manage(new Gtk::Label(_("Button Fade Step Duration"))), false);
    scaler = scaler_new(1, 2000, 1);
    scaler->set_value(50);
    tbl.append(*scaler, true);
    SettingItem::create(*scaler, "buttons", "button_fade_step_duration");

    tbl.append(*Gtk::manage(new Gtk::Label(_("Button Pulse Wait Duration"))), false);
    scaler = scaler_new(0, 4000, 1);
    scaler->set_value(0);
    tbl.append(*scaler, true);
    SettingItem::create(*scaler, "buttons", "button_fade_pulse_wait_duration");

    tbl.append(*Gtk::manage(new Gtk::Label(_("Button Pulse Min Opacity %"))), false);
    scaler = scaler_new(0, 100, 1);
    scaler->set_value(0);
    tbl.append(*scaler, true);
    SettingItem::create_global(*scaler, "buttons", "button_fade_pulse_min_opacity");

    tbl.append(*Gtk::manage(new Gtk::Label(_("Titlebar Double-Click Action"))), false);

    Gtk::ComboBoxText* combo = Gtk::manage(new Gtk::ComboBoxText());
    for (int i = 0; i < TITLEBAR_ACTION_COUNT; i++) {
        combo->append_text(titlebar_action_name[i]);
    }
    combo->set_active(0);
    tbl.append(*combo, true);
    SettingItem::create_global(*combo, "titlebars", "double_click_action");

    tbl.append(*Gtk::manage(new Gtk::Label(_("Button Hover Cursor"))), false);
    combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->append_text(_("Normal"));
    combo->append_text(_("Pointing Finger"));
    combo->set_active(1);
    tbl.append(*combo, true);
    SettingItem::create_global(*combo, "buttons", "hover_cursor");

    tbl.append(*Gtk::manage(new Gtk::Label(_("Compiz Decoration Blur Type"))), false);
    combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->append_text(_("None"));
    combo->append_text(_("Titlebar only"));
    combo->append_text(_("All decoration"));
    combo->set_active(BLUR_TYPE_NONE);
    tbl.append(*combo, true);
    SettingItem::create_global(*combo, "decorations", "blur_type");
}

void ThemerWindow::layout_engine_pane(Gtk::Box& vbox)
{
    auto& nvbox = *Gtk::manage(new Gtk::VBox(false, 2));
    auto& scwin = *Gtk::manage(new Gtk::ScrolledWindow());
    vbox.pack_start(scwin, true, true);
    scwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scwin.add(nvbox);

    layout_engine_list(nvbox);
    init_engine_list();
}

void ThemerWindow::layout_lower_pane(Gtk::Box& vbox)
{
    auto& notebook = *Gtk::manage(new Gtk::Notebook());
    vbox.pack_start(notebook, true, true);

    layout_engine_pane(*build_notebook_page(_("Frame Engine"), notebook));
    layout_button_pane(*build_notebook_page(_("Buttons"), notebook));
    layout_frame_pane(*build_notebook_page(_("Frame/Shadows"), notebook));
    layout_global_pane(*build_notebook_page(_("Titlebar"), notebook));
    layout_theme_pane(*build_notebook_page(_("Theme"), notebook));

    layout_file_frame(vbox);
}

Gtk::Box* ThemerWindow::build_lower_pane(Gtk::Box& vbox)
{
    auto& expander = *Gtk::manage(new Gtk::Expander(_("Edit")));
    vbox.pack_start(expander, false, false);
    expander.set_expanded(false);

    auto& my_vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    expander.add(my_vbox);

    return &my_vbox;
}

void ThemerWindow::cb_clearbox()
{
    searchbox_->set_text("");
}

void ThemerWindow::cb_import()
{
    //get a filename
    Gtk::FileChooserDialog dialog{*main_window_, _("Import Theme..."),
                                  Gtk::FILE_CHOOSER_ACTION_OPEN};

    std::string homedir = g_get_home_dir();
    std::string path = homedir + "/Desktop/";

    Gtk::FileFilter filter;
    filter.set_name("Theme Packages");
    filter.add_pattern("*.emerald");
    dialog.add_filter(filter);
    dialog.set_current_folder(path);

    if (dialog.run() == Gtk::RESPONSE_ACCEPT) {
        std::string theme = import_theme(dialog.get_filename());
        refresh_theme_list(theme);
    }
}

Gtk::Widget* ThemerWindow::build_tree_view()
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);

    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Search:"))), false, false, 0);

    searchbox_ = Gtk::manage(new Gtk::Entry());
    hbox.pack_start(*searchbox_, true, true, 0);

    auto& clearbut = *Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
    clearbut.signal_clicked().connect([=](){ cb_clearbox(); });
    hbox.pack_start(clearbut, false, false, 0);

    hbox.pack_start(*Gtk::manage(new Gtk::VSeparator()), false, false);

    reload_button_ = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
    hbox.pack_start(*reload_button_, false, false, 0);
    reload_button_->signal_clicked().connect([=](){ refresh_theme_list(""); });

    delete_button_ = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
    hbox.pack_start(*delete_button_, false, false, 0);
    delete_button_->set_sensitive(false);
    delete_button_->signal_clicked().connect([=](){ cb_delete(); });

    import_button_ = Gtk::manage(new Gtk::Button("Import..."));
    auto &img = *Gtk::manage(new Gtk::Image(Gtk::Stock::OPEN, Gtk::ICON_SIZE_BUTTON));
    import_button_->set_image(img);
    hbox.pack_start(*import_button_, false, false, 0);
    import_button_->signal_clicked().connect([=](){ cb_import(); });

    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false);

    theme_list_.signal_get_filter_text().connect([&](){ return searchbox_->get_text(); });
    theme_list_.signal_selection_changed().connect([=](){ cb_load(); });
    theme_list_.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    theme_list_.set_size_request(500, 200);
    vbox.pack_start(theme_list_, true, true);
    return &vbox;
}

void ThemerWindow::import_cache(Gtk::ProgressBar& progbar)
{
    for (auto& entry : fs::directory_iterator{themecache}) {
        if (entry.path().extension() == ".emerald") {
            import_theme(entry.path().native());
            progbar.pulse();
        }
    }
}

bool ThemerWindow::watcher_func(FetcherInfo* fe)
{
    fe->progbar->pulse();
    if (waitpid(fe->pd, NULL, WNOHANG) != 0) {
        import_cache(*(fe->progbar));
        refresh_theme_list("");
        //fe->dialog->destroy_(); // FIXME -- private
        delete fe;
        return false;
    }
    return true;
}

void ThemerWindow::fetch_svn()
{
    std::vector<std::string> command = {"svn", "co", svnpath, themecache};
    Glib::Pid pd;
    FetcherInfo* fe = new FetcherInfo();
    Gtk::Dialog& dialog = *Gtk::manage(new Gtk::Dialog(_("Fetching Themes"),
                                                       *main_window_,
                                                       true));
    auto& lab = *Gtk::manage(new Gtk::Label(_("Fetching themes... \n"
                                             "This may take time depending on \n"
                                             "internet connection speed.")));
    dialog.get_vbox()->pack_start(lab, false, false);
    auto& progbar = *Gtk::manage(new Gtk::ProgressBar());
    dialog.get_action_area()->pack_start(progbar, true, true);
    dialog.show_all();
    Glib::spawn_async(".", command, Glib::SPAWN_SEARCH_PATH | Glib::SPAWN_DO_NOT_REAP_CHILD,
                      sigc::slot<void>{}, &pd);
    fe->dialog = &dialog;
    fe->progbar = &progbar;
    fe->pd = pd;
    Glib::signal_timeout().connect([=](){ return watcher_func(fe); }, 100);
}

void ThemerWindow::fetch_gpl_svn()
{
    svnpath = "http://emerald-themes.googlecode.com/svn/trunk/emerald-themes-repo";
    themecache = g_strconcat(g_get_home_dir(), "/.emerald/themecache", NULL);
    fetch_svn();
}

void ThemerWindow::fetch_ngpl_svn()
{
    svnpath = "https://svn.generation.no/emerald-themes";
    themecache = g_strconcat(g_get_home_dir(), "/.emerald/ngplthemecache", NULL);
    fetch_svn();
}

void ThemerWindow::cb_quit()
{
    main_window_->hide();
}

void ThemerWindow::layout_upper_pane(Gtk::Box& vbox)
{
    vbox.pack_start(*build_tree_view(), true, true);
}

void ThemerWindow::layout_repo_pane(Gtk::Box& vbox)
{
    vbox.pack_start(*Gtk::manage(new Gtk::Label(
                            _("Here are the repositories that you can fetch Emerald Themes from. \n"
                              "Fetching themes would fetch and import themes from SVN repositories \n"
                              "You need Subversion package installed to use this feature."
                             ))), false, false, 0);
    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false);
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, true, true);


    SettingsTable tbl(2, true, false);
    hbox.pack_start(tbl.table(), false, false, 0);

    fetch_button_ = Gtk::manage(new Gtk::Button("Fetch GPL'd Themes"));
    auto& im1 = *Gtk::manage(new Gtk::Image(Gtk::Stock::CONNECT, Gtk::ICON_SIZE_BUTTON));
    fetch_button_->set_image(im1);
    tbl.append(*fetch_button_, false);
    fetch_button_->signal_clicked().connect([=](){ fetch_gpl_svn(); });

    auto* rlabel = Gtk::manage(new Gtk::Label(
                 _("This repository contains GPL'd themes that can be used under \n"
                   "the terms of GNU GPL licence v2.0 or later \n")));
    tbl.append(*rlabel, false);

    fetch_button2_ = Gtk::manage(new Gtk::Button("Fetch non GPL'd Themes"));
    auto& im2 = *Gtk::manage(new Gtk::Image(Gtk::Stock::CONNECT, Gtk::ICON_SIZE_BUTTON));
    fetch_button2_->set_image(im2);
    tbl.append(*fetch_button2_, false);
    fetch_button2_->signal_clicked().connect([=](){ fetch_ngpl_svn(); });

    rlabel = Gtk::manage(new Gtk::Label(
                 _("This repository contains non-GPL'd themes. They might infringe \n"
                   "copyrights and patent laws in some countries.")));
    tbl.append(*rlabel, false);
    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false);

    vbox.pack_start(*Gtk::manage(new Gtk::Label(
                            _("To activate Non-GPL repository please run the following in shell and accept the server certificate permanently: \n"
                              "svn ls https://svn.generation.no/emerald-themes."
                             ))), false, false, 0);
}

void ThemerWindow::layout_themes_pane(Gtk::Box& vbox)
{
    auto& notebook = *Gtk::manage(new Gtk::Notebook());
    vbox.pack_start(notebook, true, true);

    layout_upper_pane(*build_notebook_page(_("Themes"), notebook));
    layout_lower_pane(*build_notebook_page(_("Edit Themes"), notebook));
//  layout_repo_pane(build_notebook_page(_("Repositories"),notebook));
}

void ThemerWindow::create_filechooserdialog1(std::string input)
{
    //get a filename
    Gtk::FileChooserDialog dialog{*main_window_, _("Import Theme..."),
                                  Gtk::FILE_CHOOSER_ACTION_OPEN};

    Gtk::FileFilter filter;
    filter.set_name("Theme Packages");
    filter.add_pattern("*.emerald");
    dialog.add_filter(filter);
    dialog.set_filename(input);

    if (dialog.run() == Gtk::RESPONSE_ACCEPT) {
        std::string theme = import_theme(dialog.get_filename());
        refresh_theme_list(theme);
    }
}

void ThemerWindow::layout_main_window()
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));

    auto& notebook = *Gtk::manage(new Gtk::Notebook());
    vbox.pack_start(notebook, true, true);

    layout_themes_pane(*build_notebook_page(_("Themes Settings"), notebook));
    layout_settings_pane(*build_notebook_page(_("Emerald Settings"), notebook));

    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);

    auto& quit_button = *Gtk::manage(new Gtk::Button(Gtk::Stock::QUIT));
    hbox.pack_end(quit_button, false, false);
    quit_button.signal_clicked().connect([=](){ cb_quit(); });
    main_window_->add(vbox);
}

