/***********************************************************************************

    Copyright (C) 2014-2017 Ahmet Öztürk (aoz_2@yahoo.com)

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


#include "helpers.hpp"
#include "lifeograph.hpp"
#include "strings.hpp"
#include "win_app_window.hpp"
#include "win_dialog_password.hpp"


using namespace LIFEO;
using namespace HELPERS;


std::string WinLogin::m_path_cur;


WinLogin::WinLogin()
:   m_flag_info_is_visible( false ), m_sort_date_type( SDT_SAVE )
{

}

void
WinLogin::initialize()
{
	/* TODO
    if( Lifeograph::p->m_flag_open_directly )
        if( open_selected_diary( Lifeograph::p->m_flag_read_only ) == LIFEO::SUCCESS )
            return;

    populate_diaries();
    */
}

void
WinLogin::handle_login()
{
	
}

void
WinLogin::handle_logout()
{
	
}

void
WinLogin::populate_diaries()
{

}

LIFEO::Result
WinLogin::open_selected_diary( bool read_only )
{
    // BEWARE: clear the diary before returning any result but SUCCESS

    // SET PATH
    Result result( Diary::d->set_path( m_path_cur,
                                       read_only ? Diary::SPT_READ_ONLY : Diary::SPT_NORMAL ) );
    switch( result )
    {
        case LIFEO::SUCCESS:
            break;
        case FILE_NOT_FOUND:
            MessageBoxA( WinAppWindow::p->get_hwnd(), STRING::DIARY_NOT_FOUND, "",
                         MB_OK|MB_ICONERROR );
            break;
        case FILE_NOT_READABLE:
        	MessageBoxA( WinAppWindow::p->get_hwnd(), STRING::DIARY_NOT_READABLE, "",
                         MB_OK|MB_ICONERROR );
            break;
        case FILE_LOCKED:
            // TODO: to be improved
            MessageBoxA( WinAppWindow::p->get_hwnd(), "Diary LOCKED!", "", MB_OK|MB_ICONWARNING );
            break;
        default:
            MessageBoxA( WinAppWindow::p->get_hwnd(), STRING::FAILED_TO_OPEN_DIARY, "",
                         MB_OK|MB_ICONWARNING );
            break;
    }
    if( result != LIFEO::SUCCESS )
    {
        Diary::d->clear();
        return result;
    }

    // READ HEADER
    switch( result = Diary::d->read_header() )
    {
        case LIFEO::SUCCESS:
            break;
        case INCOMPATIBLE_FILE_OLD:
            MessageBoxA( WinAppWindow::p->get_hwnd(), STRING::INCOMPATIBLE_DIARY_OLD, "",
                         MB_OK|MB_ICONERROR );
            break;
        case INCOMPATIBLE_FILE_NEW:
            MessageBoxA( WinAppWindow::p->get_hwnd(), STRING::INCOMPATIBLE_DIARY_NEW, "",
                         MB_OK|MB_ICONERROR );
            break;
        case CORRUPT_FILE:
            MessageBoxA( WinAppWindow::p->get_hwnd(), STRING::CORRUPT_DIARY, "",
                         MB_OK|MB_ICONERROR );
            break;
        default:
            MessageBoxA( WinAppWindow::p->get_hwnd(), STRING::FAILED_TO_OPEN_DIARY, "",
                         MB_OK|MB_ICONERROR );
            break;
    }
    if( result != LIFEO::SUCCESS )
    {
        return result;
    }

    // HANDLE OLD DIARY
    if( Diary::d->is_old() && read_only == false )
    {
        int response =
                MessageBoxA( WinAppWindow::p->get_hwnd(),
                             STRING::UPGRADE_DIARY_CONFIRM,
                             "Are You Sure You Want to Upgrade The Diary?",
                             MB_YESNO | MB_ICONWARNING );

        if( response !=  IDYES )
        {
            Diary::d->clear();
            return LIFEO::ABORTED;
        }
    }

    // HANDLE ENCRYPTION
    if( Diary::d->is_encrypted() )
    {
        if( DialogPassword::launch( WinAppWindow::p->get_hwnd(),
                                    Diary::d,
                                    DialogPassword::PD_OPEN,
                                    m_password_attempt_no ) !=
            HELPERS::OK )
        {
            Diary::d->clear();
            return HELPERS::ABORTED;
        }
    }

    // FINALLY READ BODY
    switch( result = Diary::d->read_body() )
    {
        case LIFEO::SUCCESS:
            WinAppWindow::p->login();
            break;
        case WRONG_PASSWORD:
            m_password_attempt_no++;
            return open_selected_diary( read_only );
        case CORRUPT_FILE:
            MessageBoxA( WinAppWindow::p->get_hwnd(), STRING::CORRUPT_DIARY, "",
                         MB_OK|MB_ICONERROR );
            // no break
        default:
            return result;
    }

    m_password_attempt_no = 0;
    
    return LIFEO::SUCCESS;
}

void
WinLogin::remove_selected_diary()
{
    // LATER
}

void
WinLogin::create_new_diary()
{
    OPENFILENAMEA ofn;
    char szFileName[ MAX_PATH ];

    ZeroMemory( &ofn, sizeof( ofn ) );
    szFileName[ 0 ] = 0;

    ofn.lStructSize = sizeof( ofn );
    ofn.hwndOwner = WinAppWindow::p->get_hwnd();
    ofn.lpstrFilter = "Diary Files (*.diary)\0*.diary\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "diary";
    ofn.Flags = OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY;

    if( GetSaveFileNameA( &ofn ) )
    {
        std::string path = szFileName;

        switch( Diary::d->init_new( path ) )
        {
            case LIFEO::SUCCESS:
                WinAppWindow::p->login();
                break;
            case LIFEO::FILE_LOCKED:
                // TODO: to be improved
                MessageBoxA( WinAppWindow::p->get_hwnd(), "Diary LOCKED!", "",
                             MB_OK|MB_ICONWARNING );
                break;
            default:
                MessageBoxA( WinAppWindow::p->get_hwnd(), STRING::FAILED_TO_OPEN_DIARY, "",
                             MB_OK|MB_ICONERROR );
                break;
        }
    }
}

void
WinLogin::add_existing_diary()
{
    OPENFILENAMEA ofn;
    char szFileName[ MAX_PATH ];

    ZeroMemory( &ofn, sizeof( ofn ) );
    szFileName[ 0 ] = 0;

    ofn.lStructSize = sizeof( ofn );
    ofn.hwndOwner = WinAppWindow::p->get_hwnd();
    ofn.lpstrFilter = "Diary Files (*.diary)\0*.diary\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "diary";
    ofn.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST;
    
    if( GetOpenFileNameA( &ofn ) )
    {
        m_path_cur = szFileName;
        if( open_selected_diary( ofn.Flags & OFN_READONLY ) != LIFEO::SUCCESS )
            MessageBoxA( WinAppWindow::p->get_hwnd(), "Load of file failed.", "",
                         MB_OK|MB_ICONERROR );
    }
}

void
WinLogin::handle_start()
{
    if( Lifeograph::settings.flag_maiden_voyage || Lifeograph::p->m_flag_force_welcome )
    {
        MessageBoxA( WinAppWindow::p->get_hwnd(), "Welcome to Lifeograph!", "Welcome", MB_OK );
    }
    
    if( Lifeograph::p->m_flag_open_directly )
        if( open_selected_diary( Lifeograph::p->m_flag_read_only ) == LIFEO::SUCCESS )
            return;

    WinAppWindow::p->m_entry_view->m_richedit->set_richtext( NULL );
}

