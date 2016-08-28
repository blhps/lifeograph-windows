/***********************************************************************************

    Copyright (C) 2010 Ahmet Öztürk (aoz_2@yahoo.com)

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


#include "settings.hpp"

#include <fstream>
#ifndef LIFEO_WINDOZE
#include <glib/gstdio.h>    // g_mkdir()
#else
#include <shlwapi.h>
#include <unistd.h>
#endif


using namespace LIFEO;


const std::string Settings::s_date_format_orders[ 3 ]  = { "YMD", "DMY", "MDY" };
const std::string Settings::s_date_format_separators   = "./-";

bool
Settings::read()
{
#ifndef LIFEO_WINDOZE
    m_path = Glib::getenv( "LIFEOGRAPH_CONFIG_FILE" );
#endif
    if( m_path.empty() )
    {
        flag_maiden_voyage = true; // maiden voyage is not used when custom config path is given
#ifndef LIFEO_WINDOZE
        m_path = Glib::build_filename( Glib::get_user_config_dir(), "lifeograph", "lifeo.conf" );
#else
        char exe_path[ MAX_PATH ];
        GetModuleFileNameA( NULL, exe_path, MAX_PATH );
        PathRemoveFileSpecA( exe_path );
        PathAppendA( exe_path, "lifeo.conf" );
        m_path = exe_path;
#endif
    }

    std::ifstream file( m_path.c_str() );

    if( ! file.is_open() )
    {
        PRINT_DEBUG( "Configuration file does not exist!" );
        return false;
    }
    else
        flag_maiden_voyage = false;

    reset();

    std::string line;

    while( getline( file, line ) )
    {
        if( line.size() < 3 )
            continue;

        switch( line[ 0 ] )
        {
            case 'D':
                if( line[1] == 'o' )
                    date_format_order = convert_string( line.substr( 2 ) );
                else
                    date_format_separator = convert_string( line.substr( 2 ) );
                break;
            case 'E':
                // if only a dot (.) no extension will be added automatically
                diary_extension = line.substr( 2 );
                if( diary_extension[ 0 ] != '.' )
                    diary_extension.insert( 0, "." );
                break;
            case 'F':
                show_formatting_toolbar = ( line[ 2 ] == 'y' || line[ 2 ] == 'Y' );
                break;
            // TODO: position & size settings may go to the diary
            // TODO: check if these are within screen boundaries
            case 'H':
                height = convert_string( line.substr( 2 ) );
                break;
            case 'I':
                idletime = convert_string( line.substr( 2 ) );
                if( idletime < IDLETIME_MIN || idletime > IDLETIME_MAX )
                    idletime = IDLETIME_DEFAULT;
                break;
            case 'L':
                autologout = ( line[ 2 ] == 'y' || line[ 2 ] == 'Y' );
                break;
            case 'O': // special options
                if( line.find( "big lists" ) != std::string::npos )
                    small_lists = false;
                break;
            case 'P':   // uppercase
                position_pane = convert_string( line.substr( 2 ) );
                break;
            case 'p':   // lowercase
                position_pane_tags =  convert_string( line.substr( 2 ) );
                // zero width is not accepted by Gtk::Paned
                if( position_pane_tags == 0 )
                    position_pane_tags = 1;
                break;
            case 'R':
                recentfiles.insert( line.substr( 2 ) );
                break;
            case 'S':
                state_maximized = ( line[ 2 ] == 'm' || line[ 2 ] == 'M' );
                break;
            case 'T':
                icon_theme = line.substr( 2 );
                break;
            case 'W':
                width = convert_string( line.substr( 2 ) );
                break;
            case 'X':
                position_x = convert_string( line.substr( 2 ) );
                break;
            case 'Y':
                position_y = convert_string( line.substr( 2 ) );
                break;

            case 0:     // empty line
            case '#':   // comment
                break;
            default:
                print_info( "unrecognized option:\n", line );
                break;
        }
    }

    update_date_format();

    file.close();
    return true;
}

bool
Settings::write()
{
#ifndef LIFEO_WINDOZE
    if( access( m_path.c_str(), F_OK ) != 0 )
    {
        if( ! Glib::file_test( Glib::path_get_dirname( m_path ), Glib::FILE_TEST_IS_DIR ) )
            g_mkdir_with_parents( Glib::path_get_dirname( m_path ).c_str(), 0700 );
    }
#endif

    std::ofstream file( m_path.c_str(), std::ios::out | std::ios::trunc );

    if( ! file.is_open() )
    {
        print_error( "Failed to save configuration!" );
        return false;
    }

    file << "W " << width;
    file << "\nH " << height;
    file << "\nX " << position_x;
    file << "\nY " << position_y;
    file << "\nS " << ( state_maximized ? 'm' : 'u' );
    file << "\nP " << position_pane;
    file << "\np " << position_pane_tags;

    // RECENT FILES
    for( const std::string& path : recentfiles )
        file << "\nR " << path;

    file << "\nL " << ( autologout ? 'y' : 'n' );
    file << "\nI " << idletime;
    file << "\nF " << ( show_formatting_toolbar ? 'y' : 'n' );

    file << "\nDo" << date_format_order;
    file << "\nDs" << date_format_separator;

    if( diary_extension != EXTENSION_DEFAULT )
        file << "\nE " << diary_extension;

    if( ! icon_theme.empty() )
        file << "\nT " << icon_theme;

    if( small_lists == false )
        file << "\nO big lists";

    file.close();
    return true;
}

void
Settings::reset()
{
    recentfiles.clear();
    autologout = true;
    idletime = IDLETIME_DEFAULT;
    show_formatting_toolbar = true;
    date_format_order = 0;
    date_format_separator = 0;
    icon_theme.clear();
    diary_extension = EXTENSION_DEFAULT;
}

void
Settings::update_date_format()
{
    Date::s_date_format_order = s_date_format_orders[ date_format_order ];
    Date::s_date_format_separator = s_date_format_separators[ date_format_separator ];
}

// maybe later:
//bool
//Settings::get_bool_opt( int i )
//{
//    if( i >= m_bool_opts.size() )
//        throw LIFEO::Error( "Requested option is out of bounds" );
//    else
//        return m_bool_opts[ i ];
//}
