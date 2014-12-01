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


#include "../rc/resource.h"

#include "lifeograph.hpp"
#include "helpers.hpp"
#include "diary.hpp"	// for LIFEO::PASSPHRASE_MIN_SIZE
#include "win_dialog_password.hpp"


using namespace LIFEO;


// ENTER PASSWORD DIALOG ===========================================================================
DialogEnterPassword* DialogEnterPassword::ptr = NULL;

inline static BOOL CALLBACK
dialog_enter_password_proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{   
    return DialogEnterPassword::ptr->proc( hwnd, msg, wParam, lParam );
}

DialogEnterPassword::DialogEnterPassword( )
{

}

int
DialogEnterPassword::launch( HWND hw_par, Diary* diary, bool flag_again )
{
    ptr = new DialogEnterPassword;

    ptr->m_ptr2diary = diary;
    ptr->m_flag_again = flag_again;

    int ret_value = DialogBox( NULL,
                               MAKEINTRESOURCE( IDD_PASSWORD ),
                               hw_par,
                               dialog_enter_password_proc );

    delete ptr;
    ptr = NULL;

    return ret_value;
}

bool
DialogEnterPassword::proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
    {
    	case WM_INITDIALOG:
            // HWNDs
            m_hwnd = hwnd;
            m_edit_current = GetDlgItem( hwnd, IDE_PASSWORD );

            SetDlgItemTextA( hwnd, IDL_DIARY_NAME, m_ptr2diary->get_name().c_str() );

            EnableWindow( GetDlgItem( m_hwnd, IDOK ), false );

            SetFocus( m_edit_current ); // probably unnecessary here

            if( m_flag_again )
            {
                Sleep( SLEEP_DURATION );
                SetDlgItemTextA( m_hwnd, IDL_PASSWORD_MESSAGE, "Wrong password. Please retry..." );
            }

            return TRUE;
        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDE_PASSWORD:
                    if( HIWORD( wParam ) == EN_UPDATE )
                    {
                        handle_entry_changed();
                    }
                    return TRUE;
                case IDCANCEL:
                    EndDialog( m_hwnd, 0 );
                    return TRUE;
                case IDOK: {
                    TCHAR* str = new TCHAR[ 128 ];
                    GetDlgItemText( m_hwnd, IDE_PASSWORD, str, 128 );
                    m_ptr2diary->set_passphrase( HELPERS::convert_utf16_to_8( str ) );
                    EndDialog( m_hwnd, 1 ); // TODO: use constants
                    return TRUE;
                }
            }
            return FALSE;

        default:
            return FALSE;
    }
}

void
DialogEnterPassword::handle_entry_changed()
{
    char* str = new char[ 128 ];
    GetDlgItemTextA( m_hwnd, IDE_PASSWORD, str, 128 );
    EnableWindow( GetDlgItem( m_hwnd, IDOK ), ( strlen( str ) >= PASSPHRASE_MIN_SIZE ) );
}

