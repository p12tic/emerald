#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define NEED_BUTTON_FILE_NAMES
#define NEED_BUTTON_NAMES
#define NEED_TITLEBAR_ACTION_NAMES
#include <emerald.h>
#include <engine.h>
#include <gtkmm.h>
#include <functional>

#define LAST_COMPAT_VER "0.1.0"

struct FetcherInfo {
    Gtk::Dialog* dialog;
    Gtk::ProgressBar* progbar;
    GPid        pd;
};

class ThemeColumns :
    public Gtk::TreeModel::ColumnRecord	{
public:
    ThemeColumns()
    {
        add(name);
        add(engine);
        add(engine_version);
        add(creator);
        add(description);
        add(theme_version);
        add(suggested);
        add(markup);
        add(pixbuf);
    }

    Gtk::TreeModelColumn<std::string> name;
    Gtk::TreeModelColumn<std::string> engine;
    Gtk::TreeModelColumn<std::string> engine_version;
    Gtk::TreeModelColumn<std::string> creator;
    Gtk::TreeModelColumn<std::string> description;
    Gtk::TreeModelColumn<std::string> theme_version;
    Gtk::TreeModelColumn<std::string> suggested;
    Gtk::TreeModelColumn<std::string> markup;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> pixbuf;
};

ThemeColumns theme_columns_;
Gtk::TreeView* theme_selector_;
Glib::RefPtr<Gtk::ListStore> theme_model_;
Glib::RefPtr<Gtk::TreeSelection> theme_select_;
Gtk::Window* main_window_;
Gtk::Entry* entry_box_;
Gtk::Entry* version_entry_;
Gtk::Button* reload_button_;
Gtk::Button* delete_button_;
Gtk::Button* import_button_;
Gtk::Button* fetch_button_;
Gtk::Button* fetch_button2_;
Gtk::Button* export_button_;
const char* svnpath;
const char* themecache;

static void theme_list_append(const char* value, const char* dir,
                              const char* fil)
{
    char* path = g_strjoin("/", dir, fil, "theme.ini", NULL);
    std::string imgpath = std::string(dir) + "/" + fil + "/theme.screenshot.png";
    char* val;
    char* val2;
    GdkPixbuf* p;
    EngineMetaInfo emi;
    GKeyFile* f = g_key_file_new();
    g_key_file_load_from_file(f, path, G_KEY_FILE_NONE, NULL);

    Gtk::TreeModel::Row row = *(theme_model_->append());
    row[theme_columns_.name] = value;
    val = g_key_file_get_string(f, "engine", "engine", NULL);
    row[theme_columns_.engine] = val ? val : "";

    if (val) {
        const char* tver;
        const char* ostr1;
        const char* ostr2;
        const char* ostr;
        const char* elc;
        get_engine_meta_info(val, &emi);
        val2 = g_key_file_get_string(f, "engine_version", val, NULL);
        if (!val2) {
            val2 = g_strdup("0.0.0");
        }
        tver = g_key_file_get_string(f, "theme", "version", NULL);
        if (!tver) {
            tver = g_strup("0.0.0");
        }
        elc = emi.last_compat;
        if (!emi.last_compat) {
            elc = "0.0.0";
        }
        ostr1 = g_strdup_printf(strverscmp(val2, elc) >= 0 ?
                                "Engine: YES (%s)\n" : "Engine: NO (%s)\n", val2);
        ostr2 = g_strdup_printf(strverscmp(tver, LAST_COMPAT_VER) >= 0 ?
                                "Emerald: YES (%s)" : "Emerald: NO (%s)", tver);
        ostr = g_strdup_printf("%s%s", ostr1, ostr2);
        g_free(ostr1);
        g_free(ostr2);
        row[theme_columns_.engine_version] = ostr;
        g_free(ostr);
        g_free(tver);
        g_free(val2);
    } else {
        val = g_key_file_get_string(f, "theme", "version", NULL);
        if (!val) {
            val = g_strdup("0.0.0");
        }
        val2 = g_strdup_printf(strverscmp(val, LAST_COMPAT_VER) >= 0 ?
                               "No Engine\nEmerald: YES (%s)" : "No Engine\nEmerald: NO (%s)", val ? val : "NONE");
        row[theme_columns_.engine_version] = val2;
        g_free(val2);
    }
    if (val) {
        g_free(val);
    }
    val = g_key_file_get_string(f, "theme", "creator", NULL);
    row[theme_columns_.creator] = val ? val : "";
    if (val) {
        g_free(val);
    }
    val = g_key_file_get_string(f, "theme", "description", NULL);
    row[theme_columns_.description] = val ? val : "";
    if (val) {
        g_free(val);
    }
    val = g_key_file_get_string(f, "theme", "theme_version", NULL);
    row[theme_columns_.theme_version] = val ? val : "";
    if (val) {
        g_free(val);
    }
    val = g_key_file_get_string(f, "theme", "suggested", NULL);
    row[theme_columns_.suggested] = val ? val : "";
    if (val) {
        g_free(val);
    }
    g_free(path);
    {
        //create the Theme column data
        char* format =
            _("<b><big>%s</big></b>\n"
              "<i>%s</i>\n"
              "<small>"
              "<b>Version</b> %s\n"
              "<b>Created By</b> %s\n"
              "<b>Use With</b> %s\n"
              "</small>");
        if (value[0] == '*') {
            value += 2;
            format =
                _("<b><big>%s</big></b> (System Theme)\n"
                  "<i>%s</i>\n"
                  "<small>"
                  "<b>Version</b> %s\n"
                  "<b>Created By</b> %s\n"
                  "<b>Use With</b> %s\n"
                  "</small>");
        }
        char* creator = g_key_file_get_string(f, "theme", "creator", NULL);
        char* tver = g_key_file_get_string(f, "theme", "theme_version", NULL);
        char* rwid = g_key_file_get_string(f, "theme", "suggested", NULL);
        char* desc = g_key_file_get_string(f, "theme", "description", NULL);
        if (creator && !strlen(creator)) {
            g_free(creator);
            creator = NULL;
        }
        if (!creator) {
            creator = g_strdup(_("Unknown"));
        }
        if (tver && !strlen(tver)) {
            g_free(tver);
            tver = NULL;
        }
        if (!tver) {
            tver = g_strdup(_("Unknown"));
        }
        if (rwid && !strlen(rwid)) {
            g_free(rwid);
            rwid = NULL;
        }
        if (!rwid) {
            rwid = g_strdup(_("Whatever (no hint)"));
        }
        if (desc && !strlen(desc)) {
            g_free(desc);
            desc = NULL;
        }
        if (!desc) {
            desc = g_strdup(_("No Description"));
        }
        val = g_markup_printf_escaped(format, value, desc, tver, creator, rwid);
        row[theme_columns_.markup] = val ? val : "";
        g_free(val);
        g_free(tver);
        g_free(creator);
        g_free(rwid);
        g_free(desc);
    }
    g_key_file_free(f);
    auto pix = Gdk::Pixbuf::create_from_file(imgpath, -1, 100);
    if (pix) {
        row[theme_columns_.pixbuf] = pix;
    } else {
        //p = gdk_pixbuf_new(GDK_COLORSPACE_RGB,true,8,1,1);
        //p = gdk_pixbuf_new_from_file(,NULL);
        auto icon_theme = Gtk::IconTheme::create();
        pix = icon_theme->load_icon(GTK_STOCK_MISSING_IMAGE, Gtk::ICON_SIZE_LARGE_TOOLBAR);
        row[theme_columns_.pixbuf] = pix;
    }
}

static void theme_scan_dir(char* dir, bool writable)
{
    GDir* d;
    d = g_dir_open(dir, 0, NULL);
    if (d) {
        char* n;
        while ((n = (char*) g_dir_read_name(d))) {
            char* fn = g_strdup_printf("%s/%s/theme.ini", dir, n);
            if (g_file_test(fn, G_FILE_TEST_IS_REGULAR)) {
                //actually add it here
                char* o;
                if (writable) {
                    o = g_strdup(n);
                } else {
                    o = g_strdup_printf("* %s", n);
                }
                theme_list_append(o, dir, n);
                g_free(o);
            }
            g_free(fn);
        }
        g_dir_close(d);
    }
}
static void scroll_to_theme(const char* thn)
{
    GtkTreeIter i;
    bool c;
    auto it = theme_model_->children().begin();
    auto it_end = theme_model_->children().end();
    for (; it != it_end; ++it) {
        std::string s = (*it)[theme_columns_.name];
        if (s == thn) {
            Gtk::TreePath p = theme_model_->get_path(it);
            theme_selector_->scroll_to_row(p);
        }
    }
}

static void refresh_theme_list(const char* thn)
{
    char* path;
    theme_model_->clear();
    theme_scan_dir(DATA_DIR "/emerald/themes/", false);
    path = g_strdup_printf("%s/.emerald/themes/", g_get_home_dir());
    theme_scan_dir(path, true);
    g_free(path);
    if (thn) {
        scroll_to_theme(thn);
    }
}

static void cb_refresh()
{
    refresh_theme_list(NULL);
}

static bool confirm_dialog(const char* val, const char* val2)
{
    std::string rv = val;
    rv += val2;
    Gtk::MessageDialog dlg(*main_window_, rv, false, Gtk::MESSAGE_QUESTION,
                           Gtk::BUTTONS_YES_NO, true);
    return (dlg.run() == Gtk::RESPONSE_YES);
}

static void info_dialog(const char* val)
{
    Gtk::MessageDialog dlg(*main_window_, val, false, Gtk::MESSAGE_INFO,
                           Gtk::BUTTONS_CLOSE, true);
    dlg.run();
}

static void error_dialog(const char* val)
{
    Gtk::MessageDialog dlg(*main_window_, val, false, Gtk::MESSAGE_ERROR,
                           Gtk::BUTTONS_CLOSE, true);
    dlg.run();
}

static void cb_load()
{
    GKeyFile* f;
    GDir* dr;
    char* fn, *xt, *gt;
    bool dist;
    dist = false;

    //first normalize the name
    auto it = theme_select_->get_selected();
    if (!it) {
        return;
    }
    std::string z_at = (*it)[theme_columns_.name];
    const char* at = z_at.c_str();
    const char* mt = at;
    if (strlen(at) >= 1 && at[0] == '*') {
        if (strlen(at) >= 2) {
            mt = at + 2;
            dist = true;
        } else {
            error_dialog(_("Invalid Theme Name"));
            return;
        }
    }
    if (strlen(mt) == 0) {
        error_dialog(_("Invalid Theme Name"));
        return;
    }
    delete_button_->set_sensitive(!dist);
    xt = g_strdup_printf("%s/.emerald/theme/", g_get_home_dir());
    dr = g_dir_open(xt, 0, NULL);
    while (dr && (gt = (char*)g_dir_read_name(dr))) {
        char* ft;
        ft = g_strdup_printf("%s/%s", xt, gt);
        g_unlink(ft);
        g_free(ft);
    }
    if (dr) {
        g_dir_close(dr);
    }
    if (dist) {
        fn = g_strdup_printf(DATA_DIR "/emerald/themes/%s/theme.ini", mt);
        at = g_strdup_printf(DATA_DIR "/emerald/themes/%s/", mt);
    } else {
        fn = g_strdup_printf("%s/.emerald/themes/%s/theme.ini", g_get_home_dir(), mt);
        at = g_strdup_printf("%s/.emerald/themes/%s/", g_get_home_dir(), mt);
    }
    dr = g_dir_open(at, 0, NULL);
    while (dr && (gt = (char*)g_dir_read_name(dr))) {
        char* nt;
        gsize len;
        char* ft;
        ft = g_strdup_printf("%s/%s", at, gt);
        if (g_file_get_contents(ft, &nt, &len, NULL)) {
            g_free(ft);
            ft = g_strdup_printf("%s/%s", xt, gt);
            g_file_set_contents(ft, nt, len, NULL);
            g_free(nt);
        }
        g_free(ft);
    }
    if (dr) {
        g_dir_close(dr);
    }
    g_free(xt);
    f = g_key_file_new();
    if (!g_key_file_load_from_file(f, fn, 0, NULL)) {
        g_free(fn);
        g_key_file_free(f);
        error_dialog("Invalid Theme File / Name");
        return;
    }
    entry_box_->set_text(mt);
    set_changed(true);
    set_apply(false);
    for (auto& item : get_setting_list()) {
        item.read_setting((void*)f);
    }
    {
        char* c;
        c = g_key_file_get_string(f, "theme", "version", NULL);
        if (c) {
            version_entry_->set_text(c);
        } else {
            version_entry_->set_text("Pre-0.8");
        }
    }
    set_apply(true);
    send_reload_signal();
    g_key_file_free(f);
    g_free(fn);
}
static char* import_theme(char* file)
{
    //first make sure we have our location
    char* fn, * at, * ot, *pot, *rstr;
    int ex;
    ot = g_strdup(file);
    pot = ot;
    if (!g_str_has_suffix(ot, ".emerald")) {
        g_free(pot);
        error_dialog(_("Invalid theme file.  Does not end in .emerald"));
        return NULL;
    }
    ot[strlen(ot) - strlen(".emerald")] = '\0';
    ot = g_strrstr(ot, "/");
    if (!ot) {
        ot = pot;
    } else {
        ot++;
    }
    rstr = g_strdup(ot);
    fn = g_strdup_printf("%s/.emerald/themes/%s/", g_get_home_dir(), ot);
    g_free(pot);
    g_mkdir_with_parents(fn, 00755);
    at = g_shell_quote(fn);
    g_free(fn);
    fn = g_shell_quote(file);
    ot = g_strdup_printf("tar -xzf %s -C %s", fn, at);
    if (!g_spawn_command_line_sync(ot, NULL, NULL, &ex, NULL) ||
            (WIFEXITED(ex) && WEXITSTATUS(ex))) {
        g_free(fn);
        g_free(ot);
        g_free(at);
        error_dialog("Error calling tar.");
        return NULL;
    }
    g_free(fn);
    g_free(ot);
    g_free(at);
//   info_dialog(_("Theme Imported"));
    return rstr;
}
static void export_theme(const char* file)
{
    std::string themename = entry_box_->get_text();
    char* fn, *at, *ot;
    int ex;
    if (themename.empty() || themename[0] == '*'
            || themename.find_first_of('/') != std::string::npos) {
        error_dialog(_("Invalid Theme Name\nCould Not Export"));
        //these conditions should already have been handled but another check is ok
        return;
    }
    if (!g_str_has_suffix(file, ".emerald")) {
        error_dialog(_("Invalid File Name\nMust End in .emerald"));
        return;
    }
    fn = g_strdup_printf("%s/.emerald/theme/", g_get_home_dir());
    at = g_shell_quote(fn);
    g_free(fn);
    fn = g_shell_quote(file);
    ot = g_strdup_printf("tar -czf %s -C %s ./ --exclude=*~", fn, at);
    if (!g_spawn_command_line_sync(ot, NULL, NULL, &ex, NULL) ||
            (WIFEXITED(ex) && WEXITSTATUS(ex))) {
        g_free(fn);
        g_free(ot);
        g_free(at);
        error_dialog(_("Error calling tar."));
        return;
    }
    g_free(ot);
    g_free(fn);
    g_free(at);
    info_dialog(_("Theme Exported"));
}
static void cb_save()
{
    GKeyFile* f;
    char* fn, *at, *mt, *gt;
    GDir* dr;
    //first normalize the name
    std::string at_s = entry_box_->get_text();
    if (at_s.empty()) {
        return;
    }
    at = at_s.c_str();
    if (strlen(at) >= 1 && at[0] == '*') {
        error_dialog(_("Can't save over system themes\n(or you forgot to enter a name)"));
        return;
    }
    if (strlen(at) == 0 || strchr(at, '/')) {
        error_dialog(_("Invalid Theme Name"));
        return;
    }
    fn = g_strdup_printf("%s/.emerald/themes/%s", g_get_home_dir(), at);
    g_mkdir_with_parents(fn, 00755);
    g_free(fn);
    fn = g_strdup_printf("%s/.emerald/themes/%s/theme.ini", g_get_home_dir(), at);
    f = g_key_file_new();
    g_key_file_load_from_file(f, fn, 0, NULL);
    for (auto& item : get_setting_list()) {
        item.write_setting(f);
    }
    g_key_file_set_string(f, "theme", "version", VERSION);
    if (g_file_test(fn, G_FILE_TEST_EXISTS)) {
        if (!confirm_dialog(_("Overwrite Theme %s?"), at)) {
            return;
        }
    }
    mt = at;
    at = g_key_file_to_data(f, NULL, NULL);
    //little fix since we're now copying from ~/.emerald/theme/*
    g_free(fn);
    fn = g_strdup_printf("%s/.emerald/theme/theme.ini", g_get_home_dir());
    if (at && !g_file_set_contents(fn, at, -1, NULL)) {
        error_dialog(_("Couldn't Write Theme"));
        g_free(at);
    } else if (!at) {
        error_dialog(_("Couldn't Form Theme"));
    } else {
        char* xt;
        xt = g_strdup_printf("%s/.emerald/themes/%s/", g_get_home_dir(), mt);
        dr = g_dir_open(xt, 0, NULL);
        while (dr && (gt = (char*)g_dir_read_name(dr))) {
            char* ft;
            ft = g_strdup_printf("%s/%s", xt, gt);
            g_unlink(ft);
            g_free(ft);
        }
        if (dr) {
            g_dir_close(dr);
        }
        at = g_strdup_printf("%s/.emerald/theme/", g_get_home_dir());
        dr = g_dir_open(at, 0, NULL);
        while (dr && (gt = (char*)g_dir_read_name(dr))) {
            char* nt;
            gsize len;
            char* ft;
            ft = g_strdup_printf("%s/%s", at, gt);
            if (g_file_get_contents(ft, &nt, &len, NULL)) {
                g_free(ft);
                ft = g_strdup_printf("%s/%s", xt, gt);
                g_file_set_contents(ft, nt, len, NULL);
                g_free(nt);
            }
            g_free(ft);
        }
        if (dr) {
            g_dir_close(dr);
        }
        g_free(xt);
        g_free(at);
        info_dialog(_("Theme Saved"));
    }
    version_entry_->set_text(VERSION);
    g_key_file_free(f);
    g_free(fn);
    refresh_theme_list(NULL);
}
static void cb_delete(Gtk::Widget* w)
{
    GtkTreeIter iter;
    GtkTreeModel* model;
    char* fn;
    //first normalize the name
    auto row = theme_select_->get_selected();
    if (!row) {
        return;
    }
    std::string name = (*row)[theme_columns_.name];
    const char* at = name.c_str();

    if (strlen(at) >= 1 && at[0] == '*') {
        error_dialog(_("Can't delete system themes\n(or you forgot to enter a name)"));
        return;
    }
    if (strlen(at) == 0) {
        error_dialog(_("Invalid Theme Name"));
        return;
    }
    fn = g_strdup_printf("%s/.emerald/themes/%s/theme.ini", g_get_home_dir(), at);
    if (g_file_test(fn, G_FILE_TEST_EXISTS)) {
        if (!confirm_dialog(_("Are you sure you want to delete %s?"), at)) {
            g_free(fn);
            return;
        } else {
            GDir* dir;
            char* ot, * mt, * pt;
            pt = g_strdup_printf("%s/.emerald/themes/%s/", g_get_home_dir(), at);
            dir = g_dir_open(pt, 0, NULL);
            while (dir && (ot = (char*)g_dir_read_name(dir))) {
                mt = g_strdup_printf("%s/%s", pt, ot);
                g_unlink(mt);
                g_free(mt);
            }
            if (dir) {
                g_dir_close(dir);
                g_rmdir(pt);
            }
            g_free(pt);
        }
        info_dialog(_("Theme deleted"));
        w->set_sensitive(false);
        refresh_theme_list(NULL);
    } else {
        error_dialog(_("No such theme to delete"));
    }
    g_free(fn);
}

static bool cb_main_destroy(GdkEventAny*)
{
    main_window_->hide();
    gtk_main_quit();
    return true;
}

static void layout_button_box(Gtk::Box& vbox, int b_t)
{

    table_append(*Gtk::manage(new Gtk::Label(b_names[b_t])), false);
    std::string bt_nm = std::string(b_names[b_t]) + " Button Pixmap";
    auto& filesel = *Gtk::manage(new Gtk::FileChooserButton(bt_nm,
                                          Gtk::FILE_CHOOSER_ACTION_OPEN));
    table_append(filesel, false);

    Gtk::FileFilter filter;
    filter.set_name("Images");
    filter.add_pixbuf_formats();
    filesel.add_filter(filter);

    auto& image = *Gtk::manage(new Gtk::Image());

    SettingItem* item;
    item = SettingItem::register_img_file_setting(filesel, "buttons", b_types[b_t], &image);

    table_append(image, true);

    auto& clearer = *Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
    clearer.signal_clicked().connect(sigc::bind(&cb_clear_file, item));
    table_append(clearer, false);
}

void layout_general_buttons_frame(Gtk::Box& hbox)
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

void layout_button_pane(Gtk::Box& vbox)
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

    table_new(4, false, false);
    scroller.add(get_current_table());

    table_append(*Gtk::manage(new Gtk::Label(_("Button"))), false);
    table_append(*Gtk::manage(new Gtk::Label(_("File"))), false);
    table_append(*Gtk::manage(new Gtk::Label(_("Preview"))), false);
    table_append(*Gtk::manage(new Gtk::Label(_("Clear"))), false);

    for (i = 0; i < BX_COUNT; i++) {
        layout_button_box(vbox, i);
    }
}

void layout_window_frame(Gtk::Box& vbox, bool active)
{
    auto& scrollwin = *Gtk::manage(new Gtk::ScrolledWindow());
    vbox.pack_start(scrollwin, true, true);
    scrollwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

    table_new(3, false, false);
    scrollwin.add(get_current_table());
    make_labels(_("Colors"));
    add_color_alpha_value(_("Text Fill"), "text", "titlebar", active);
    add_color_alpha_value(_("Text Outline"), "text_halo", "titlebar", active);
    table_append_separator();
    add_color_alpha_value(_("Button Fill"), "button", "buttons", active);
    add_color_alpha_value(_("Button Outline"), "button_halo", "buttons", active);
}

void add_row(Gtk::Box& vbox, Gtk::Widget& item, const char* title)
{
    //vbox.pack_start(*Gtk::manage(new Gtk::Label(title)),false,false);
    //vbox.pack_end(item,true,true,0);
    table_append(*Gtk::manage(new Gtk::Label(title)), false);
    table_append(item, true);
}

void add_color_button_row(Gtk::Box& vbox, const char* title,
                          const char* key, const char* sect)
{
    auto& color_button = *Gtk::manage(new Gtk::ColorButton());
    SettingItem::create(color_button, sect, key);
    add_row(vbox, color_button, title);
}

void add_int_range_row(Gtk::Box& vbox, const char* title, const char* key,
                       int start, int end, const char* sect)
{
    auto& scaler = *scaler_new(start, end, 1);
    SettingItem::create(scaler, sect, key);
    add_row(vbox, scaler, title);
}

void add_float_range_row(Gtk::Box& vbox, const char* title, const char* key,
                         double start, double end, double prec, const char* sect)
{
    auto& scaler = *scaler_new(start, end, prec);
    SettingItem::create(scaler, sect, key);
    add_row(vbox, scaler, title);
}

void layout_shadows_frame(Gtk::Box& vbox)
{
    table_new(2, false, false);
    vbox.pack_start(get_current_table(), false, false);
    add_color_button_row(vbox, _("Color"), "shadow_color", "shadow");
    add_float_range_row(vbox, _("Opacity"), "shadow_opacity", 0.01, 6.0, 0.01, "shadow");
    add_float_range_row(vbox, _("Radius"), "shadow_radius", 0.0, 48.0, 0.1, "shadow");
    add_int_range_row(vbox, _("X Offset"), "shadow_offset_x", -16, 16, "shadow");
    add_int_range_row(vbox, _("Y Offset"), "shadow_offset_y", -16, 16, "shadow");
}

void layout_title_frame(Gtk::Box& vbox)
{
    Gtk::Scale* scaler;

    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);

    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Title-Text Font"))), false, false, 0);

    auto& fontbtn = *Gtk::manage(new Gtk::FontButton);
    hbox.pack_start(fontbtn, false, false);
    SettingItem::create(fontbtn, "titlebar", "titlebar_font");

    table_new(2, false, false);
    vbox.pack_start(get_current_table(), false, false);
    table_append(*Gtk::manage(new Gtk::Label(_("Minimum Title-bar Height"))), false);
    scaler = scaler_new(0, 64, 1);
    scaler->set_value(17);
    table_append(*scaler, true);
    SettingItem::create(*scaler, "titlebar", "min_titlebar_height");

    table_append(*Gtk::manage(new Gtk::Label(_("Vertical Button Offset"))), false);
    scaler = scaler_new(0, 64, 1);
    scaler->set_value(1);
    table_append(*scaler, true);
    SettingItem::create(*scaler, "buttons", "vertical_offset");

    table_append(*Gtk::manage(new Gtk::Label(_("Horizontal Button Offset"))), false);
    scaler = scaler_new(0, 64, 1);
    scaler->set_value(1);
    table_append(*scaler, true);
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
void add_meta_string_value(const char* title, const char* key)
{
    table_append(*Gtk::manage(new Gtk::Label(title)), false);
    auto& entry = *Gtk::manage(new Gtk::Entry());
    table_append(entry, true);
    SettingItem::create(entry, "theme", key);
}

static void cb_export()
{
    //get a filename
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
        std::string fn = dialog.get_filename();
        export_theme(fn.c_str());
    }
}

void layout_file_frame(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);

    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Name:"))), false, false, 0);
    entry_box_ = Gtk::manage(new Gtk::Entry());
    entry_box_->set_text(_("Untitled Theme"));
    hbox.pack_start(*entry_box_, true, true, 0);

    auto& btn = *Gtk::manage(new Gtk::Button(Gtk::Stock::SAVE));
    hbox.pack_start(btn, false, false, 0);
    btn.signal_clicked().connect(&cb_save);

    export_button_ = Gtk::manage(new Gtk::Button(_("Export...")));
    Gtk::Image im(Gtk::Stock::SAVE_AS, Gtk::ICON_SIZE_BUTTON);
    export_button_->set_image(im);
    hbox.pack_start(*export_button_, false, false, 0);
    export_button_->signal_clicked().connect(&cb_export);
}

void layout_info_frame(Gtk::Box& vbox)
{
    table_new(2, false, false);
    vbox.pack_start(get_current_table(), false, false);
    add_meta_string_value(_("Creator"), "creator");
    add_meta_string_value(_("Description"), "description");
    add_meta_string_value(_("Theme Version"), "theme_version");
    add_meta_string_value(_("Suggested Widget Theme"), "suggested");

    table_append_separator();

    table_append(*Gtk::manage(new Gtk::Label(_("Themer Version"))), false);

    version_entry_ = Gtk::manage(new Gtk::Entry);
    version_entry_->set_editable(false);
    version_entry_->set_sensitive(false);
    table_append(*version_entry_, true);
}

void add_border_slider(const char* text, const char* key, int value)
{
    table_append(*Gtk::manage(new Gtk::Label(text)), false);

    auto& scaler = *scaler_new(0, 20, 1);
    table_append(scaler, true);
    scaler.set_value(value);
    SettingItem::create(scaler, "borders", key);
}

void layout_borders_frame(Gtk::Box& vbox)
{
    table_new(2, false, false);
    vbox.pack_start(get_current_table(), false, false, 0);
    table_append(*Gtk::manage(new Gtk::Label(_("Border"))), false);
    table_append(*Gtk::manage(new Gtk::Label(_("Size"))), false);
    add_border_slider(_("Top"), "top", 4);
    add_border_slider(_("Bottom"), "bottom", 6);
    add_border_slider(_("Left"), "left", 6);
    add_border_slider(_("Right"), "right", 6);
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
void layout_left_frame_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    layout_shadows_frame(*build_frame(vbox, _("Shadows"), false));
}

void layout_right_frame_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    layout_borders_frame(*build_frame(vbox, _("Frame Borders"), false));
}

void layout_frame_pane(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(true, 2));
    vbox.pack_start(hbox, true, true);

    layout_left_frame_pane(hbox);
    layout_right_frame_pane(hbox);
}

void layout_left_global_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);

    layout_window_frame(*build_frame(vbox, _("Active Window"), false), true);
    layout_window_frame(*build_frame(vbox, _("Inactive Window"), false), false);
    //layout stuff here.
}

void layout_right_global_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    layout_title_frame(*build_frame(vbox, _("Title Bar"), false));
}

void layout_global_pane(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(true, 2));
    vbox.pack_start(hbox, true, true);
    layout_left_global_pane(hbox);
    layout_right_global_pane(hbox);
}

void layout_screenshot_frame(Gtk::Box& vbox)
{
    GtkFileFilter* imgfilter;
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
    clearer.signal_clicked().connect(sigc::bind(&cb_clear_file, item));
    hbox->pack_start(clearer, true, true);
}
void layout_left_theme_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    layout_screenshot_frame(*build_frame(vbox, _("Screenshot"), false));
}

void layout_right_theme_pane(Gtk::Box& hbox)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    layout_info_frame(*build_frame(vbox, _("Information"), false));
}

void layout_theme_pane(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, true, true);
    layout_left_theme_pane(hbox);
    layout_right_theme_pane(hbox);
}

void layout_settings_pane(Gtk::Box& vbox)
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

    table_new(2, false, false);
    vbox.pack_start(get_current_table(), false, false);

    table_append(*Gtk::manage(new Gtk::Label(_("Button Fade Total Duration"))), false);
    Gtk::Scale* scaler;
    scaler = scaler_new(1, 4000, 1);
    scaler->set_value(250);
    table_append(*scaler, true);
    SettingItem::create_global(*scaler, "buttons", "button_fade_total_duration");

    table_append(*Gtk::manage(new Gtk::Label(_("Button Fade Step Duration"))), false);
    scaler = scaler_new(1, 2000, 1);
    scaler->set_value(50);
    table_append(*scaler, true);
    SettingItem::create(*scaler, "buttons", "button_fade_step_duration");

    table_append(*Gtk::manage(new Gtk::Label(_("Button Pulse Wait Duration"))), false);
    scaler = scaler_new(0, 4000, 1);
    scaler->set_value(0);
    table_append(*scaler, true);
    SettingItem::create(*scaler, "buttons", "button_fade_pulse_wait_duration");

    table_append(*Gtk::manage(new Gtk::Label(_("Button Pulse Min Opacity %"))), false);
    scaler = scaler_new(0, 100, 1);
    scaler->set_value(0);
    table_append(*scaler, true);
    SettingItem::create_global(*scaler, "buttons", "button_fade_pulse_min_opacity");

    table_append(*Gtk::manage(new Gtk::Label(_("Titlebar Double-Click Action"))), false);

    Gtk::ComboBoxText* combo = Gtk::manage(new Gtk::ComboBoxText());
    for (int i = 0; i < TITLEBAR_ACTION_COUNT; i++) {
        combo->append_text(titlebar_action_name[i]);
    }
    combo->set_active(0);
    table_append(*combo, true);
    SettingItem::create_global(*combo, "titlebars", "double_click_action");

    table_append(*Gtk::manage(new Gtk::Label(_("Button Hover Cursor"))), false);
    combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->append_text(_("Normal"));
    combo->append_text(_("Pointing Finger"));
    combo->set_active(1);
    table_append(*combo, true);
    SettingItem::create_global(*combo, "buttons", "hover_cursor");

    table_append(*Gtk::manage(new Gtk::Label(_("Compiz Decoration Blur Type"))), false);
    combo = Gtk::manage(new Gtk::ComboBoxText());
    combo->append_text(_("None"));
    combo->append_text(_("Titlebar only"));
    combo->append_text(_("All decoration"));
    combo->set_active(BLUR_TYPE_NONE);
    table_append(*combo, true);
    SettingItem::create_global(*combo, "decorations", "blur_type");
}

void layout_engine_pane(Gtk::Box& vbox)
{
    auto& nvbox = *Gtk::manage(new Gtk::VBox(false, 2));
    auto& scwin = *Gtk::manage(new Gtk::ScrolledWindow());
    vbox.pack_start(scwin, true, true);
    scwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scwin.add(nvbox);

    layout_engine_list(nvbox);
    init_engine_list();
}

void layout_lower_pane(Gtk::Box& vbox)
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

Gtk::Box* build_lower_pane(Gtk::Box& vbox)
{
    auto& expander = *Gtk::manage(new Gtk::Expander(_("Edit")));
    vbox.pack_start(expander, false, false);
    expander.set_expanded(false);

    auto& my_vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    expander.add(my_vbox);

    return &my_vbox;
}

void cb_refilter(Glib::RefPtr<Gtk::TreeModelFilter> filt)
{
    filt->refilter();
}

bool is_visible(const Gtk::TreeModel::const_iterator& iter, Gtk::Entry& e)
{
    int i;

    std::string tx = e.get_text();
    if (tx.empty()) {
        return true;
    }
    for (auto& ch : tx) { ch = std::tolower(ch); }

    std::vector<std::string> strs;
    strs.push_back((*iter)[theme_columns_.name]);
    strs.push_back((*iter)[theme_columns_.engine]);
    strs.push_back((*iter)[theme_columns_.creator]);
    strs.push_back((*iter)[theme_columns_.description]);
    strs.push_back((*iter)[theme_columns_.suggested]);

    for (auto& str : strs) {
        for (auto& ch : str) { ch = std::tolower(ch); }
        if (str.find(tx) != std::string::npos) {
            return true;
        }
    }
    return false;
}

void cb_clearbox(Gtk::Entry& w)
{
    w.set_text("");
}

static void cb_import()
{
    //get a filename
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
                            _("Import Theme..."), main_window_->gobj(),
                            GTK_FILE_CHOOSER_ACTION_OPEN,
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                            NULL);
    char* pth = g_strdup_printf("%s/Desktop/", g_get_home_dir());
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Theme Packages");
    gtk_file_filter_add_pattern(filter, "*.emerald");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                        pth);
    g_free(pth);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename;
        char* thn;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        thn = import_theme(filename);
        g_free(filename);
        refresh_theme_list(thn);
        if (thn) {
            g_free(thn);
        }
    }
    gtk_widget_destroy(dialog);
}

Gtk::Widget* build_tree_view()
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);

    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Search:"))), false, false, 0);

    //create data store
    theme_model_ = Gtk::ListStore::create(theme_columns_);

    auto& searchbox = *Gtk::manage(new Gtk::Entry());
    hbox.pack_start(searchbox, true, true, 0);

    auto& clearbut = *Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
    clearbut.signal_clicked().connect(sigc::bind(&cb_clearbox, std::ref(searchbox)));
    hbox.pack_start(clearbut, false, false, 0);

    hbox.pack_start(*Gtk::manage(new Gtk::VSeparator()), false, false);

    reload_button_ = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
    hbox.pack_start(*reload_button_, false, false, 0);
    reload_button_->signal_clicked().connect(&cb_refresh);

    delete_button_ = Gtk::manage(new Gtk::Button(Gtk::Stock::DELETE));
    hbox.pack_start(*delete_button_, false, false, 0);
    delete_button_->set_sensitive(false);
    delete_button_->signal_clicked().connect(sigc::bind(&cb_delete, delete_button_));

    import_button_ = Gtk::manage(new Gtk::Button("Import..."));
    auto &img = *Gtk::manage(new Gtk::Image(Gtk::Stock::OPEN, Gtk::ICON_SIZE_BUTTON));
    import_button_->set_image(img);
    hbox.pack_start(*import_button_, false, false, 0);
    import_button_->signal_clicked().connect(&cb_import);

    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false);

    theme_selector_ = Gtk::manage(new Gtk::TreeView(theme_model_));
    theme_selector_->set_headers_clickable();
    theme_selector_->set_reorderable();

    {
    auto &renderer = *Gtk::manage(new Gtk::CellRendererText());
    renderer.property_ellipsize().set_value(Pango::ELLIPSIZE_END);
    int id = theme_selector_->append_column("Theme", renderer);
    auto &column = *(theme_selector_->get_column(id-1));
    column.add_attribute(renderer.property_markup(), theme_columns_.markup);
    column.set_sort_column_id(theme_columns_.markup);
    column.set_resizable();
    column.set_reorderable();
    } {
    auto &renderer = *Gtk::manage(new Gtk::CellRendererPixbuf());
    renderer.set_alignment(0, 0.5);
    int id = theme_selector_->append_column("Screenshot", renderer);
    auto &column = *(theme_selector_->get_column(id-1));
    column.add_attribute(renderer.property_pixbuf(), theme_columns_.pixbuf);
    column.set_sort_column_id(theme_columns_.pixbuf);
    column.set_max_width(400);
    column.set_resizable();
    column.set_reorderable();
    } {
    auto &renderer = *Gtk::manage(new Gtk::CellRendererText());
    int id = theme_selector_->append_column("Up-to-Date", renderer);
    auto &column = *(theme_selector_->get_column(id-1));
    column.add_attribute(renderer.property_text(), theme_columns_.engine_version);
    column.set_sort_column_id(theme_columns_.engine_version);
    column.set_resizable();
    column.set_reorderable();
    } {
    auto &renderer = *Gtk::manage(new Gtk::CellRendererText());
    int id = theme_selector_->append_column("Engine", renderer);
    auto &column = *(theme_selector_->get_column(id-1));
    column.add_attribute(renderer.property_text(), theme_columns_.engine);
    column.set_sort_column_id(theme_columns_.engine);
    column.set_resizable();
    column.set_reorderable();
    }

    theme_select_ = theme_selector_->get_selection();
    theme_select_->set_mode(Gtk::SELECTION_SINGLE);
    theme_select_->signal_changed().connect(&cb_load);

    auto& scrollwin = *Gtk::manage(new Gtk::ScrolledWindow());
    scrollwin.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrollwin.add(*theme_selector_);
    scrollwin.set_size_request(500, 200);

    vbox.pack_start(scrollwin, true, true);
    return &vbox;
}

void import_cache(Gtk::ProgressBar& progbar)
{
    GDir* d;
    d = g_dir_open(themecache, 0, NULL);
    if (d) {
        char* n;
        while ((n = (char*) g_dir_read_name(d))) {

            char* fn;
            if (g_str_has_suffix(n, ".emerald")) {
                fn = g_strconcat(themecache, "/", NULL);
                fn = g_strconcat(fn, n, NULL);
                import_theme(fn);
                g_free(fn);
                progbar.pulse();
            }
        }
        g_free(n);
        g_dir_close(d);
    }
}

bool watcher_func(FetcherInfo* fe)
{
    fe->progbar->pulse();
    if (waitpid(fe->pd, NULL, WNOHANG) != 0) {
        import_cache(*(fe->progbar));
        refresh_theme_list(NULL);
        //fe->dialog->destroy_(); // FIXME -- private
        g_free(themecache);
        delete fe;
        return false;
    }
    return true;
}

void fetch_svn()
{
    char* themefetcher[] = {g_strdup("svn"), g_strdup("co"), g_strdup(svnpath), g_strdup(themecache), NULL };
    GPid pd;
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
    g_spawn_async(NULL, themefetcher, NULL,
                  G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                  NULL, NULL, &pd, NULL);
    g_free(themefetcher[4]);
    fe->dialog = &dialog;
    fe->progbar = &progbar;
    fe->pd = pd;
    g_timeout_add(100, watcher_func, fe);
}

void fetch_gpl_svn()
{
    svnpath = "http://emerald-themes.googlecode.com/svn/trunk/emerald-themes-repo";
    themecache = g_strconcat(g_get_home_dir(), "/.emerald/themecache", NULL);
    fetch_svn();
}

void fetch_ngpl_svn()
{
    svnpath = "https://svn.generation.no/emerald-themes";
    themecache = g_strconcat(g_get_home_dir(), "/.emerald/ngplthemecache", NULL);
    fetch_svn();
}

void cb_quit()
{
    main_window_->hide();
}

void layout_upper_pane(Gtk::Box& vbox)
{
    vbox.pack_start(*build_tree_view(), true, true);
}
void layout_repo_pane(Gtk::Box& vbox)
{
    vbox.pack_start(*Gtk::manage(new Gtk::Label(
                            _("Here are the repositories that you can fetch Emerald Themes from. \n"
                              "Fetching themes would fetch and import themes from SVN repositories \n"
                              "You need Subversion package installed to use this feature."
                             ))), false, false, 0);
    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false);
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, true, true);


    table_new(2, true, false);
    hbox.pack_start(get_current_table(), false, false, 0);

    fetch_button_ = Gtk::manage(new Gtk::Button("Fetch GPL'd Themes"));
    auto& im1 = *Gtk::manage(new Gtk::Image(Gtk::Stock::CONNECT, Gtk::ICON_SIZE_BUTTON));
    fetch_button_->set_image(im1);
    table_append(*fetch_button_, false);
    fetch_button_->signal_clicked().connect(&fetch_gpl_svn);

    auto* rlabel = Gtk::manage(new Gtk::Label(
                 _("This repository contains GPL'd themes that can be used under \n"
                   "the terms of GNU GPL licence v2.0 or later \n")));
    table_append(*rlabel, false);

    fetch_button2_ = Gtk::manage(new Gtk::Button("Fetch non GPL'd Themes"));
    auto& im2 = *Gtk::manage(new Gtk::Image(Gtk::Stock::CONNECT, Gtk::ICON_SIZE_BUTTON));
    fetch_button2_->set_image(im2);
    table_append(*fetch_button2_, false);
    fetch_button2_->signal_clicked().connect(&fetch_ngpl_svn);

    rlabel = Gtk::manage(new Gtk::Label(
                 _("This repository contains non-GPL'd themes. They might infringe \n"
                   "copyrights and patent laws in some countries.")));
    table_append(*rlabel, false);
    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false);

    vbox.pack_start(*Gtk::manage(new Gtk::Label(
                            _("To activate Non-GPL repository please run the following in shell and accept the server certificate permanently: \n"
                              "svn ls https://svn.generation.no/emerald-themes."
                             ))), false, false, 0);
}
void layout_themes_pane(Gtk::Box& vbox)
{
    auto& notebook = *Gtk::manage(new Gtk::Notebook());
    vbox.pack_start(notebook, true, true);

    layout_upper_pane(*build_notebook_page(_("Themes"), notebook));
    layout_lower_pane(*build_notebook_page(_("Edit Themes"), notebook));
//  layout_repo_pane(build_notebook_page(_("Repositories"),notebook));
}

GtkWidget* create_filechooserdialog1(const char* input)
{

    //get a filename
    GtkWidget* dialog_startup = gtk_file_chooser_dialog_new(
                                    _("Import Theme..."), NULL,
                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                    NULL);

    char* pth = g_strdup_printf("%s", input);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Theme Packages");
    gtk_file_filter_add_pattern(filter, "*.emerald");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog_startup), filter);
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog_startup),
                                  pth);
    g_free(pth);
    if (gtk_dialog_run(GTK_DIALOG(dialog_startup)) == GTK_RESPONSE_ACCEPT) {
        char* filename;
        char* thn;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog_startup));
        thn = import_theme(filename);
        g_free(filename);
        refresh_theme_list(thn);
        if (thn) {
            g_free(thn);
        }
    }

    gtk_widget_destroy(dialog_startup);
    return dialog_startup;
}

void layout_main_window()
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
    quit_button.signal_clicked().connect(&cb_quit);
    main_window_->add(vbox);
}

int main(int argc, char* argv[])
{
    set_changed(false);
    set_apply(false);

    char* input_file = NULL;
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    int install_file = 0;
    int loop_count = 0;

    while (loop_count < argc) {
        if (strcmp(argv[loop_count], "-i") == 0) {
            //-i arg found so next option should be the file to install
            if (loop_count + 1 < argc) {
                input_file = argv[loop_count + 1];
                printf("File To Install %s\n", input_file);
                install_file = 1;
            } else {
                printf("Usage: -i /file/to/install\n");
            }
        }
        loop_count++;
    }

    Gtk::Main kit(argc, argv);

#ifdef USE_DBUS
    if (!g_thread_supported()) {
        g_thread_init(NULL);
    }
    dbus_g_thread_init();
#endif

    g_mkdir_with_parents(g_strdup_printf("%s/.emerald/theme/", g_get_home_dir()), 00755);
    g_mkdir_with_parents(g_strdup_printf("%s/.emerald/themes/", g_get_home_dir()), 00755);

    init_key_files();

    main_window_ = new Gtk::Window();

    if (install_file == 1) {
        GtkWidget* filechooserdialog1;
        filechooserdialog1 = create_filechooserdialog1(input_file);
        gtk_widget_show(filechooserdialog1);
    }

#ifdef USE_DBUS
    setup_dbus();
#endif

    main_window_->set_title("Emerald Themer " VERSION);
    main_window_->set_resizable(true);
    main_window_->set_default_size(700, 500);
    main_window_->signal_delete_event().connect(&cb_main_destroy);
    main_window_->set_default_icon_from_file(PIXMAPS_DIR "/emerald-theme-manager-icon.png");

    main_window_->set_border_width(5);

    layout_main_window();
    main_window_->show_all();

    refresh_theme_list(NULL);
    copy_from_defaults_if_needed();
    init_settings();
    set_changed(false);
    set_apply(true);

    kit.run();
    return 0;
}
