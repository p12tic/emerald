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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <libengine/emerald.h>
#include <libengine/libengine.h>
#include <libengine/filesystem.h>
#include <libengine/format.h>
#include <gtkmm.h>
#include <functional>
#include "window.h"

#define LAST_COMPAT_VER "0.1.0"

namespace fs = boost::filesystem;

int main(int argc, char* argv[])
{
    set_changed(false);
    set_apply(false);

    std::string input_file;
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
                std::cout << format("File To Install %s\n", input_file);
                install_file = 1;
            } else {
                std::cout << "Usage: -i /file/to/install\n";
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

    fs::path homedir = g_get_home_dir();
    fs::create_directories(homedir / ".emerald/theme");
    fs::create_directories(homedir / ".emerald/themes");

    init_key_files();

#ifdef USE_DBUS
    setup_dbus();
#endif

    ThemerWindow win(install_file, input_file);

    kit.run();
    return 0;
}
