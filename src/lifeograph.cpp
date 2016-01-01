/***********************************************************************************

    Copyright (C) 2007-2015 Ahmet Öztürk (aoz_2@yahoo.com)

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


#include <unistd.h>

#include "lifeograph.hpp"
#include "win_app_window.hpp"
#ifndef LIFEO_WINDOZE
#include "dialog_preferences.hpp"
#else
#include "win_wao.hpp"
#endif
#include "helpers.hpp"

#if LIFEOGRAPH_DEBUG_BUILD
#include <build_time.h>
#endif


using namespace LIFEO;

// STATIC MEMBERS
const char                          Lifeograph::PROGRAM_VERSION_STRING[] = "0.3.0"
#if LIFEOGRAPH_DEBUG_BUILD
        "\n" BUILD_TIMESTAMP
#endif
;

Lifeograph*                         Lifeograph::p;
Settings                            Lifeograph::settings;
Icons*                              Lifeograph::icons = NULL;
#ifndef LIFEO_WINDOZE
Glib::RefPtr< Gtk::Builder >        Lifeograph::builder;
Glib::RefPtr< Gtk::ActionGroup >    Lifeograph::m_actions_read;
Glib::RefPtr< Gtk::ActionGroup >    Lifeograph::m_actions_edit;
EnchantBroker*                      Lifeograph::s_enchant_broker = NULL;
SignalVoid                          Lifeograph::s_signal_logout;
#endif
Lifeograph::LoginStatus             Lifeograph::loginstatus = Lifeograph::LOGGED_OUT;
std::set< std::string >             Lifeograph::stock_diaries;
#ifndef LIFEO_WINDOZE
DiaryElement*                       Lifeograph::s_elem_dragged = NULL;
bool                                Lifeograph::s_flag_dragging = false;
LanguageList                        Lifeograph::s_lang_list;
std::string                         Lifeograph::s_color_insensitive;
#else
HINSTANCE                           Lifeograph::hInst = 0;
#endif
int                                 Lifeograph::m_internaloperation = 0;

// CONSTRUCTOR
Lifeograph::Lifeograph()
:
#ifndef LIFEO_WINDOZE
    Gtk::Application( "lifeograph.app",
                      Gio::APPLICATION_NON_UNIQUE | Gio::APPLICATION_HANDLES_COMMAND_LINE ),
#endif
    m_flag_force_welcome( false ),  m_flag_open_directly( false ),
    m_flag_read_only( false )
{
    p = this;

#ifndef LIFEO_WINDOZE
    Glib::set_application_name( LIFEO::PROGRAM_NAME );
    if( Glib::file_test( PIXMAPDIR "/lifeograph.png", Glib::FILE_TEST_EXISTS ) )
        Gtk::Window::set_default_icon_from_file( PIXMAPDIR "/lifeograph.png" );

    m_actions_read = Gtk::ActionGroup::create( "read actions" );
    m_actions_edit = Gtk::ActionGroup::create( "edit actions" );

    m_actions_read->set_sensitive( false );
    m_actions_edit->set_sensitive( false );

    drag_targets_tag.push_back( Gtk::TargetEntry( TARGET_ENTRY_TAG,
                                                  Gtk::TARGET_SAME_APP,
                                                  DiaryElement::ET_TAG ) );
    drag_targets_entry.push_back( Gtk::TargetEntry( TARGET_ENTRY_ENTRY,
                                                    Gtk::TARGET_SAME_APP,
                                                    DiaryElement::ET_ENTRY ) );
#endif

    icons = new Icons;
}

Lifeograph::~Lifeograph()
{
#ifndef LIFEO_WINDOZE
    if( s_enchant_broker )
    {
        enchant_broker_free( s_enchant_broker );
        s_enchant_broker = NULL;
    }
#endif
}

#ifndef LIFEO_WINDOZE
Glib::RefPtr< Lifeograph >
Lifeograph::create()
{
    return Glib::RefPtr< Lifeograph >( new Lifeograph );
}
#else
int
Lifeograph::run( HINSTANCE hInstance, LPSTR cmdline )
{
    int argc;
    char** argv = command_line_to_argvA( cmdline, &argc );

    on_startup();
    on_command_line( argc, argv );

    LocalFree( argv );
    new WinAppWindow;
    hInst = hInstance;
    return WinAppWindow::p->run();
}
#endif

bool
Lifeograph::quit()
{
    PRINT_DEBUG( "Lifeograph::quit()" );

    if( loginstatus == LOGGED_IN )
        if( ! WinAppWindow::p->finish_editing() )
            return false;

#ifndef LIFEO_WINDOZE
    // SAVE SETTINGS
    AppWindow::p->get_size( settings.width, settings.height );
    AppWindow::p->get_position( settings.position_x, settings.position_y );
    settings.state_maximized = (
            AppWindow::p->get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED );

    settings.write();
#endif
    return true;
}

void
Lifeograph::on_startup()
{
#ifndef LIFEO_WINDOZE
    Gtk::Application::on_startup();
#else
    WAO_init();
#endif

    // SETTINGS
    Lifeograph::settings.read();

#ifndef LIFEO_WINDOZE
    // ENCHANT
    s_enchant_broker = enchant_broker_init();
    enchant_broker_list_dicts( s_enchant_broker, fill_lang_list_cb, NULL );

    // APP MENU
    Glib::RefPtr< Gio::SimpleAction > action_prefs = Gio::SimpleAction::create( "preferences" );
    action_prefs->signal_activate().connect( sigc::ptr_fun( &appmenu_preferences ) );
    add_action( action_prefs );

    Glib::RefPtr< Gio::SimpleAction > action_about = Gio::SimpleAction::create( "about" );
    action_about->signal_activate().connect( sigc::ptr_fun( &appmenu_about ) );
    add_action( action_about );

    Glib::RefPtr< Gio::SimpleAction > action_quit = Gio::SimpleAction::create( "quit" );
    action_quit->signal_activate().connect( sigc::ptr_fun( &appmenu_quit ) );
    add_action( action_quit );

    m_menu = Gio::Menu::create();
    m_menu->append( _( "Preferences" ), "app.preferences" );
    m_menu->append( _( "About Lifeograph" ), "app.about" );
    m_menu->append( _( "Quit" ), "app.quit" );

    set_app_menu( m_menu );
#endif
}

#ifndef LIFEO_WINDOZE
void
Lifeograph::on_activate()
{
    add_window( *( new AppWindow ) );
    AppWindow::p->show();
}
#endif

int
#ifndef LIFEO_WINDOZE
Lifeograph::on_command_line( const Glib::RefPtr< Gio::ApplicationCommandLine >& cmd_line )
{
    int argc;
    char** argv( cmd_line->get_arguments( argc ) );

    for( int i = 1; i < argc; i++ )

#else
Lifeograph::on_command_line( int argc, char** argv )
{
    for( int i = 0; i < argc; i++ )
#endif

    {
        if( ! strcmp( argv[ i ], "--open" ) || ! strcmp( argv[ i ], "-o" ) )
        {
            if( ( i + 1 ) < argc )
            {
                if( access( argv[ ++i ], F_OK ) == 0 ) // check existance
                {
                    if( ! is_dir( argv[ i ] ) )
                    {
                        WinAppWindow::p->m_login->m_path_cur = argv[ i ];
                        m_flag_open_directly = true;
                    }
                }
                if( ! m_flag_open_directly )
                    Error( "File cannot be opened" );
            }
            else
                Error( "No path provided" );
        }
        else
        if( ! strcmp( argv[ i ], "--force-welcome" ) )
        {
            m_flag_force_welcome = true;
        }
        else
        if( ! strcmp( argv[ i ], "--ignore-locks" ) )
        {
            Diary::s_flag_ignore_locks = true;
        }
        else
        if( ! strcmp( argv[ i ], "--read-only" ) || ! strcmp( argv[ i ], "-r" ) )
        {
            m_flag_read_only = true;
        }
        else
        if( access( argv[ i ], F_OK ) == 0 && ! is_dir( argv[ i ] ) )
        {
            WinAppWindow::p->m_login->m_path_cur = argv[ i ];
            m_flag_open_directly = true;
        }
        else
        {
            print_info( std::string( "Unrecognized argument: " ) + argv[ i ] );
        }
    }

#ifndef LIFEO_WINDOZE
    activate();
#endif

    return 0;
}

#ifndef LIFEO_WINDOZE
bool
Lifeograph::load_gui( const std::string& path )
{
    builder = Gtk::Builder::create();
    try
    {
        builder->add_from_file( path );
    }
    catch( const Glib::FileError& ex )
    {
        print_error( "FileError: " + ex.what() );
        return false;
    }
    catch( const Gtk::BuilderError& ex )
    {
        print_error( "BuilderError: " + ex.what() );
        return false;
    }

    return true;
}

// ACTIONS
void
Lifeograph::create_action( bool                             flag_edit,
                           Glib::RefPtr< Gtk::Action >&     action,
                           const Glib::ustring&             name,
                           const Glib::ustring&             label,
                           const Glib::ustring&             tooltip,
                           const Gtk::AccelKey&             accelkey,
                           const Glib::SignalProxy0< void >::SlotType&
                                                            slot,
                           Gtk::Activatable*                widget_proxy )
{
    action = Gtk::Action::create( name, label, tooltip );

    if( flag_edit )
        m_actions_edit->add( action, accelkey );
    else
        m_actions_read->add( action, accelkey );
    action->set_accel_group( AppWindow::p->get_accel_group() );
    action->connect_accelerator();
    if( widget_proxy != NULL )
        widget_proxy->set_related_action( action );
    action->signal_activate().connect( slot );
}

void
Lifeograph::create_toggle_action( bool                                  flag_edit,
                                  Glib::RefPtr< Gtk::ToggleAction >&    action,
                                  const Glib::ustring&                  name,
                                  const Glib::ustring&                  label,
                                  const Glib::ustring&                  tooltip,
                                  const Gtk::AccelKey&                  accelkey,
                                  const Glib::SignalProxy0< void >::SlotType&
                                                                        slot,
                                 Gtk::Activatable*                      widget_proxy )
{
    action = Gtk::ToggleAction::create( name, label, tooltip );

    if( flag_edit )
        m_actions_edit->add( action, accelkey );
    else
        m_actions_read->add( action, accelkey );
    action->set_accel_group( AppWindow::p->get_accel_group() );
    action->connect_accelerator();
    if( widget_proxy != NULL )
        widget_proxy->set_related_action( action );
    action->signal_activate().connect( slot );
}

Glib::RefPtr< Gdk::Pixbuf >
Lifeograph::get_thumbnail( const std::string& path, int width )
{
    Glib::RefPtr< Gdk::Pixbuf > buf;
    MapPixbufs::iterator iter( p->map_thumbnails.find( path ) );

    if( iter == p->map_thumbnails.end() )
    {
        buf = Gdk::Pixbuf::create_from_file( path );
        p->map_thumbnails[ path ] = buf;
    }
    else
        buf = iter->second;

    if( buf->get_width() > width )
        return buf->scale_simple( width,
                                  ( buf->get_height() * width ) / buf->get_width(),
                                  Gdk::INTERP_BILINEAR );
    else
        return buf;
}

// DIALOGEVENT =====================================================================================
HELPERS::DialogEvent::DialogEvent( const Glib::ustring& title )
:   Gtk::Dialog( title )
{
    set_transient_for( *AppWindow::p );
    Lifeograph::signal_logout().connect( sigc::mem_fun( this, &DialogEvent::handle_logout ) );
}

HELPERS::DialogEvent::DialogEvent( BaseObjectType* cobject,
                                   const Glib::RefPtr< Gtk::Builder >& )
:   Gtk::Dialog( cobject )
{
    set_transient_for( * AppWindow::p );
    //Lifeograph::signal_logout().connect( sigc::mem_fun( this, &DialogEvent::handle_logout ) );
}

bool
HELPERS::DialogEvent::on_event( GdkEvent* e )
{
    AppWindow::p->on_event( e );
    return false;
}
#endif
