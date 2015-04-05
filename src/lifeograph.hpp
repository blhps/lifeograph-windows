/***********************************************************************************

    Copyright (C) 2007-2012 Ahmet Öztürk (aoz_2@yahoo.com)

    This file is part of Lifeograph.

    Lifeograph is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lifeograph is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lifeograph.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************************/


#ifndef LIFEOGRAPH_BASE_HEADER
#define LIFEOGRAPH_BASE_HEADER

#ifndef LIFEO_WINDOZE
#include <gtkmm.h>
#endif

#include "settings.hpp"
#include "diary.hpp"

#ifndef LIFEO_WINDOZE
#include "panel_main.hpp"
#include "panel_diary.hpp"
#include "panel_extra.hpp"
#include "view_entry.hpp"
#endif

namespace LIFEO
{

    static const char   PROGRAM_NAME[]              = "Lifeograph";

    static const int    BACKUP_INTERVAL             = 30;    // seconds
    static const int    LOGOUT_COUNTDOWN            = 10;    // seconds

    static const char   TARGET_ENTRY_TAG[]          = "GTK_TREE_MODEL_ROW_TAG";
    static const char   TARGET_ENTRY_ENTRY[]        = "GTK_TREE_MODEL_ROW_ENTRY";

struct Icons
{
    Icon                            diary_16;
    Icon                            diary_32;
    Icon                            entry_16;
    Icon                            entry_32;
    Icon                            entry_favorite_16;
    Icon                            entry_favorite_32;
    Icon                            tag_16;
    Icon                            tag_32;
    Icon                            tag_theme_16;
    Icon                            tag_theme_32;
    Icon                            tag_category_16;
    Icon                            tag_category_32;
    Icon                            untagged_16;
    Icon                            untagged_32;
    Icon                            chapter_16;
    Icon                            chapter_32;
    Icon                            month_16;
    Icon                            month_32;
    Icon                            filter_16;
    Icon                            filter_32;
    Icon                            todo_open_16;
    Icon                            todo_open_32;
    Icon                            todo_progressed_16;
    Icon                            todo_progressed_32;
    Icon                            todo_done_16;
    Icon                            todo_done_32;
    Icon                            todo_canceled_16;
    Icon                            todo_canceled_32;
};

typedef std::set< std::string > LanguageList;

class AppWindow; // forward declaration

// Lifeograph Application
class Lifeograph
#ifndef LIFEO_WINDOZE
: public Gtk::Application
#endif
{
    public:
        static const char   PROGRAM_VERSION_STRING[];
//        static const int    PROGRAM_VERSION_INT = 100000;

        enum LoginStatus
        {
            LOGGED_OUT = 0, LOGGED_IN = 1, LOGGED_TIME_OUT = 2
        };

#ifndef LIFEO_WINDOZE
        static Glib::RefPtr< Lifeograph >
                                    create();
#endif

        static bool                 quit();

                                    Lifeograph();
        virtual                     ~Lifeograph();
        static Lifeograph*          p; // static pointer

        void                        on_startup();
        void                        on_activate();
#ifndef LIFEO_WINDOZE
        int             on_command_line( const Glib::RefPtr< Gio::ApplicationCommandLine >& );
#else
        int             on_command_line( int argc, char** argv );
#endif

        static Settings             settings;
        static Icons*               icons;

#ifndef LIFEO_WINDOZE
        static Glib::RefPtr< Gtk::Builder >
                                    builder;

        static void                 create_action(
                bool,
                Glib::RefPtr< Gtk::Action >&,
                const Glib::ustring&,
                const Glib::ustring&,
                const Glib::ustring&,
                const Gtk::AccelKey&,
                const Glib::SignalProxy0< void >::SlotType&,
                Gtk::Activatable* = NULL );

        static void                 create_toggle_action(
                bool,
                Glib::RefPtr< Gtk::ToggleAction >&,
                const Glib::ustring&,
                const Glib::ustring&,
                const Glib::ustring&,
                const Gtk::AccelKey&,
                const Glib::SignalProxy0< void >::SlotType&,
                Gtk::Activatable* = NULL );

        bool                        load_gui( const std::string& );

        static SignalVoid           s_signal_logout;
#endif

        static int                  m_internaloperation;

#ifndef LIFEO_WINDOZE
        // DRAG & DROP
        std::vector< Gtk::TargetEntry >
                                    drag_targets_tag;
        std::vector< Gtk::TargetEntry >
                                    drag_targets_entry;
        static DiaryElement*        s_elem_dragged;
        static bool                 s_flag_dragging;

        // COLORS
        static std::string          s_color_insensitive;

        // ENCHANT
        static EnchantBroker*       s_enchant_broker;
        static LanguageList         s_lang_list;
#endif

        static std::set< std::string > stock_diaries;

#ifndef LIFEO_WINDOZE
        static Icon                 get_thumbnail( const std::string&, int );

        static SignalVoid           signal_logout()
        { return s_signal_logout; }

        static Glib::RefPtr< Gtk::ActionGroup >
                                    m_actions_read;
        static Glib::RefPtr< Gtk::ActionGroup >
                                    m_actions_edit;
#else
        int                         run( HINSTANCE, LPSTR );
        static HINSTANCE            hInst;
        
#endif

        static LoginStatus          loginstatus;

        //std::string                 m_program_path;  // XXX NOT USED

        bool                        m_flag_force_welcome;
        bool                        m_flag_open_directly;
        bool                        m_flag_read_only; // for command-line supplied file

    protected:
#ifndef LIFEO_WINDOZE
        static void                 fill_lang_list_cb( const char* const l, const char* const,
                                                       const char* const, const char* const,
                                                       void* )
        { s_lang_list.insert( l ); }

        // APPLICATION MENU
        Glib::RefPtr< Gio::Menu >   m_menu;

        // POOL OF THUMBNAILS FOR IMAGE LINKS IN DIARY
        typedef std::map< std::string, Icon > MapPixbufs;
        MapPixbufs                  map_thumbnails;
#endif
};

} // end of namespace LIFEO

#endif

