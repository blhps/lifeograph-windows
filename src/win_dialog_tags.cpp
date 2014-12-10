/***********************************************************************************

    Copyright (C) 2014-2015 Ahmet Öztürk (aoz_2@yahoo.com)

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


#include "win_dialog_tags.hpp"

#include "../rc/resource.h"

#include "lifeograph.hpp"
#include "helpers.hpp"
#include "diary.hpp"	// for LIFEO::PASSPHRASE_MIN_SIZE
#include "win_dialog_password.hpp"


using namespace LIFEO;


// ENTER PASSWORD DIALOG ===========================================================================
DialogTags* DialogTags::ptr = NULL;

inline static BOOL CALLBACK
dialog_tags_proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return DialogTags::ptr->proc( hwnd, msg, wParam, lParam );
}

DialogTags::DialogTags()
{

}

Result
DialogTags::launch( HWND hw_par, Diary* diary, Entry* entry, const Wstring& name )
{
    ptr = new DialogTags;

    ptr->m_ptr2diary = diary;
    ptr->m_ptr2entry = entry;
    ptr->m_name = name;

    Result res = ( Result ) DialogBox( NULL,
                                       MAKEINTRESOURCE( IDD_TAGS ),
                                       hw_par,
                                       dialog_tags_proc );

    delete ptr;
    ptr = NULL;

    return res;
}

bool
DialogTags::proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
    {
    	case WM_INITDIALOG:
            // HWNDs
            m_hwnd = hwnd;

            m_edit = GetDlgItem( hwnd, IDE_TAG_NAME );
            m_button_theme = GetDlgItem( hwnd, IDB_TAG_THEME );
            m_button_action = GetDlgItem( hwnd, IDB_TAG_ACTION );

            SetWindowText( m_edit, m_name.c_str() );

            //EnableWindow( GetDlgItem( m_hwnd, IDOK ), false );

            return TRUE;
        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDE_TAG_NAME:
                    if( HIWORD( wParam ) == EN_UPDATE )
                        handle_entry_changed();
                    return TRUE;
                case IDCANCEL:
                    EndDialog( m_hwnd, HELPERS::ABORTED );
                    return TRUE;
                case IDB_TAG_THEME: {
/*                    TCHAR str[ 128 ];
                    GetWindowText( m_edit1, str, 128 );
                    if( m_type == PD_AUTHORIZE )
                    {
                        EndDialog( m_hwnd,
                            m_ptr2diary->compare_passphrase( HELPERS::convert_utf16_to_8( str ) )
                            ? HELPERS::SUCCESS : HELPERS::INVALID );
                    }
                    else
                    {
                        m_ptr2diary->set_passphrase( HELPERS::convert_utf16_to_8( str ) );
                        EndDialog( m_hwnd, HELPERS::OK );
                    }*/

                    return TRUE;
                }
                case IDB_TAG_ACTION:
                {
                    wchar_t buffer[ 128 ];
                    GetWindowText( m_edit, buffer, 128 );
                    Ustring filter = convert_utf16_to_8( buffer );

                    Tag* tag;
                    Result result( SUCCESS );

                    switch( m_tag_operation_cur )
                    {
                        case TO_NONE:
                            result = ABORTED;
                            break;
                        case TO_REMOVE:
                            tag = Diary::d->get_tags()->get_tag( filter );
                            m_ptr2entry->remove_tag( tag );
                            break;
                        case TO_CREATE_AND_ADD:
                            tag = Diary::d->create_tag( filter );
                            m_ptr2entry->add_tag( tag );
                            break;
                        case TO_ADD:
                            tag = Diary::d->get_tags()->get_tag( filter );
                            m_ptr2entry->add_tag( tag );
                            break;
                    }

                    EndDialog( m_hwnd, result );
                    return TRUE;
                }
            }
            return FALSE;

        default:
            return FALSE;
    }
}

void
DialogTags::handle_entry_changed()
{
    wchar_t filter[ 128 ];
    GetWindowText( m_edit, filter, 128 );

    EnableWindow( m_button_theme, false );

    if( ! filter[ 0 ] )
    {
        EnableWindow( m_button_action, false );
        m_tag_operation_cur = TO_NONE;
    }
    else
    {
        EnableWindow( m_button_action, true );
        Tag* tag = Diary::d->get_tags()->get_tag( convert_utf16_to_8( filter ) );
        if( tag == NULL )
        {
            SetWindowTextA( m_button_action, "Create Tag" );
            m_tag_operation_cur = TO_CREATE_AND_ADD;
        }
        else
        {
            if( m_ptr2entry->get_tags().checkfor_member( tag ) )
            {
                SetWindowTextA( m_button_action, "Remove Tag" );
                m_tag_operation_cur = TO_REMOVE;

                if( tag->get_has_own_theme() && m_ptr2entry->get_theme_tag() != tag )
                    EnableWindow( m_button_theme, true );
            }
            else
            {
                SetWindowTextA( m_button_action, "Add Tag" );
                m_tag_operation_cur = TO_ADD;
            }
        }
    }
}

