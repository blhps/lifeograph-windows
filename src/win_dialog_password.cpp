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
DialogPassword* DialogPassword::ptr = NULL;

inline static BOOL CALLBACK
dialog_password_proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{   
    return DialogPassword::ptr->proc( hwnd, msg, wParam, lParam );
}

DialogPassword::DialogPassword( PDType type )
:   m_flag_again( false ), m_type( type )
{

}

Result
DialogPassword::launch( HWND hw_par, Diary* diary, PDType type, bool flag_again )
{
    ptr = new DialogPassword( type );

    ptr->m_ptr2diary = diary;
    ptr->m_flag_again = flag_again;

    Result res = ( Result ) DialogBox( NULL,
                                       MAKEINTRESOURCE( type == PD_NEW ?
                                                            IDD_NEW_PASSWORD : IDD_PASSWORD ),
                                       hw_par,
                                       dialog_password_proc );

    delete ptr;
    ptr = NULL;

    return res;
}

bool
DialogPassword::proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
    {
    	case WM_INITDIALOG:
            // HWNDs
            m_hwnd = hwnd;
            
            m_edit1 = GetDlgItem( hwnd, IDE_PASSWORD );
                
            if( m_type == PD_NEW )
            {
                m_edit2 = GetDlgItem( hwnd, IDE_PASSWORD2 );
            }
            else
            {
                SetDlgItemTextA( hwnd, IDL_DIARY_NAME, m_ptr2diary->get_name().c_str() );
            }
            
            SetFocus( m_edit1 );

            EnableWindow( GetDlgItem( m_hwnd, IDOK ), false );

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
                case IDE_PASSWORD2:
                    if( HIWORD( wParam ) == EN_UPDATE )
                        handle_entry_changed();
                    return TRUE;
                case IDCANCEL:
                    EndDialog( m_hwnd, HELPERS::ABORTED );
                    return TRUE;
                case IDOK: {
                    TCHAR str[ 128 ];
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
                    }
                    
                    return TRUE;
                }
            }
            return FALSE;

        default:
            return FALSE;
    }
}

void
DialogPassword::handle_entry_changed()
{
    char str[ 128 ];
    GetWindowTextA( m_edit1, str, 128 );
    
    if( m_type == PD_NEW )
    {
        char str2[ 128 ];
        GetWindowTextA( m_edit2, str2, 128 );
        EnableWindow( GetDlgItem( m_hwnd, IDOK ), ( strlen( str ) >= PASSPHRASE_MIN_SIZE ) &&
                                                    ! strcmp( str, str2 ) );
    }
    else
    {
        EnableWindow( GetDlgItem( m_hwnd, IDOK ), ( strlen( str ) >= PASSPHRASE_MIN_SIZE ) );
    }
}

