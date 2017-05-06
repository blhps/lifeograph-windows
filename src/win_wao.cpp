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


#include <winsock2.h>   // just to silence the compiler
#include <windows.h>
#include <commctrl.h>

#include "win_wao.hpp"


LRESULT CALLBACK
waoWC_button0Proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static TRACKMOUSEEVENT tME;
    switch( msg )
    {
        case WM_CREATE:
        {
            tME.cbSize = sizeof( TRACKMOUSEEVENT );
            //tME.dwFlags = TME_HOVER;
            tME.dwHoverTime = 0;
            SendMessage( GetParent (hwnd), WAOM_TBBTNCREATED,
                    MAKEWPARAM( WAOM_TBBTNCREATED_LW,
                            GetWindowLong( hwnd, GWL_ID ) ), ( LPARAM ) hwnd );
            return FALSE;
        }
        case WM_ENABLE:
            if( GetWindowLong( hwnd, GWL_USERDATA ) & WAO_TBBS_CHECKED )
                SetWindowLong( hwnd, GWL_USERDATA,
                        wParam ? ( WAO_TBBS_NORMAL | WAO_TBBS_CHECKED ) :
                        ( WAO_TBBS_DISABLED | WAO_TBBS_CHECKED ) );
            else
                SetWindowLong( hwnd, GWL_USERDATA, wParam ? WAO_TBBS_NORMAL : WAO_TBBS_DISABLED );
            RedrawWindow( hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE );
            return FALSE;
        case WM_MOUSEMOVE:
            if( GetWindowLong( hwnd, GWL_USERDATA ) != WAO_TBBS_NORMAL )
                return FALSE;
            SetWindowLong( hwnd, GWL_USERDATA, WAO_TBBS_HOVERED );
            RedrawWindow( hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE );
            tME.dwFlags = TME_LEAVE;
            tME.hwndTrack = hwnd;
            TrackMouseEvent( &tME );
            return FALSE;
        case WM_MOUSELEAVE:
            SetWindowLong( hwnd, GWL_USERDATA, WAO_TBBS_NORMAL );
            RedrawWindow( hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE );
            return FALSE;
        case WM_RBUTTONDOWN:
            if( GetWindowLong( hwnd, GWL_STYLE ) & BS_RIGHTBUTTON )
            {
                long btnStt = GetWindowLong( hwnd, GWL_USERDATA );
                SetWindowLong( hwnd, GWL_USERDATA,
                               WAO_TBBS_CLICKED | ( btnStt & WAO_TBBS_CHECKED ) );
            }
            return FALSE;
        case WM_LBUTTONDOWN:
        {
            long btnStt = GetWindowLong( hwnd, GWL_USERDATA );
            SetWindowLong( hwnd, GWL_USERDATA, WAO_TBBS_CLICKED | ( btnStt & WAO_TBBS_CHECKED ) );
            RedrawWindow( hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE );
            if( GetWindowLong( hwnd, GWL_STYLE ) & BS_NOTIFY )
                SetTimer( hwnd, WAO_TMR_TBRPW, WAO_TBBN_RPWTIME, NULL );
            return FALSE;
        }
        case WM_LBUTTONUP:
            // should be clicked first:
            if( !( GetWindowLong( hwnd, GWL_USERDATA ) & WAO_TBBS_CLICKED ) )
                return FALSE;
        case WM_CLEAR: // tweak to check with one internal message
        {
            SetWindowLong( hwnd, GWL_USERDATA, WAO_TBBS_NORMAL );
            RedrawWindow( hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE );
            DLGPROC parProc = ( DLGPROC ) GetWindowLong ( GetParent( hwnd ), DWL_DLGPROC );
            if( msg != WM_CLEAR ) // what about making this a seperate thread?
                parProc( GetParent ( hwnd ), WM_COMMAND,
                         MAKEWPARAM( GetWindowLong( hwnd, GWL_ID ),
                         WAO_TBBN_LCLCKED ),
                         ( LPARAM ) hwnd );
            if( GetWindowLong( hwnd, GWL_STYLE ) & BS_NOTIFY )
            {
                KillTimer( hwnd, WAO_TMR_TBRPW );
                KillTimer( hwnd, WAO_TMR_TBRPT );
            }
            return FALSE;
        }
        case WM_RBUTTONUP:
            // should be clicked first:
            if( !( GetWindowLong( hwnd, GWL_USERDATA ) & WAO_TBBS_CLICKED ) )
                return FALSE;
            if( GetWindowLong( hwnd, GWL_STYLE ) & BS_RIGHTBUTTON )
            {
                DLGPROC parProc = ( DLGPROC ) GetWindowLong( GetParent( hwnd ), DWL_DLGPROC );
                parProc( GetParent( hwnd ),
                         WM_COMMAND,
                         MAKEWPARAM( GetWindowLong ( hwnd, GWL_ID ), WAO_TBBN_RCLCKED ),
                         ( LPARAM ) hwnd );
            }
            return FALSE;
        case WM_TIMER:
        {
            if( wParam == WAO_TMR_TBRPW ) // repeat wait
            {
                KillTimer( hwnd, WAO_TMR_TBRPW );
                SetTimer( hwnd, WAO_TMR_TBRPT, WAO_TBBN_RPTTIME, NULL );
            }
            DLGPROC parProc = ( DLGPROC ) GetWindowLong( GetParent( hwnd ), DWL_DLGPROC );
            parProc( GetParent( hwnd ),
                     WM_COMMAND,
                     MAKEWPARAM( GetWindowLong( hwnd, GWL_ID ), WAO_TBBN_LCLCKED ),
                     ( LPARAM ) hwnd );
            return FALSE;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            BeginPaint( hwnd, &paintStruct );
            RECT rcWnd;
            GetClientRect( hwnd, &rcWnd ); // should this be calculated every time?
            int btnStt = GetWindowLong( hwnd, GWL_USERDATA );

            if( btnStt & WAO_TBBS_CHECKED )
            {
                FillRect( paintStruct.hdc, &rcWnd, CreateSolidBrush( 0x99ffff ) );
                DrawEdge( paintStruct.hdc, &rcWnd, BDR_SUNKENOUTER, BF_RECT );
            }
            else if( btnStt == WAO_TBBS_CLICKED )
            {
                DrawEdge( paintStruct.hdc, &rcWnd, BDR_SUNKENOUTER, BF_RECT );
            }
            else if( btnStt == WAO_TBBS_HOVERED )
            {
                DrawEdge( paintStruct.hdc, &rcWnd, BDR_RAISEDINNER, BF_RECT );
            }
            // drawing icon
            if( GetWindowLong( hwnd, GWL_STYLE ) & BS_ICON ) // maybe later bitmap too
            {
                HICON hIco = ( HICON ) GetPropA( hwnd, WAO_PROP_ICON );

                DrawIconEx( paintStruct.hdc,
                            ( rcWnd.right - rcWnd.left - 16 ) / 2,
                            ( rcWnd.bottom - rcWnd.top - 16 ) / 2,
                            hIco, 16, 16, 0, NULL, DI_NORMAL );
            }
            // drawing text
            else
            {
                int tmpLen = GetWindowTextLength( hwnd );
                wchar_t buffer[ tmpLen + 1 ];
                SIZE tmpSze;
                GetWindowText( hwnd, buffer, tmpLen + 1 );
                SetBkMode( paintStruct.hdc, TRANSPARENT );
                SelectObject( paintStruct.hdc, GetStockObject( DEFAULT_GUI_FONT ) );

                GetTextExtentPoint32( paintStruct.hdc, buffer, tmpLen, &tmpSze );
                DrawState( paintStruct.hdc, NULL, NULL,
                         ( LPARAM ) buffer, tmpLen,
                         ( rcWnd.right-rcWnd.left-tmpSze.cx ) / 2,
                         ( rcWnd.bottom-rcWnd.top-tmpSze.cy ) / 2,
                         tmpSze.cx, tmpSze.cy, DST_TEXT|(
                                ( btnStt & WAO_TBBS_DISABLED ) ? DSS_DISABLED : 0 ) );
            }
            EndPaint( hwnd, &paintStruct );
            return FALSE;
        }
        default:
            return DefWindowProc( hwnd, msg, wParam, lParam );
    }
}

LRESULT CALLBACK
waoWC_buttonChkProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{   static TRACKMOUSEEVENT tME;
    switch( msg )
    {
        case WM_CREATE:
        {
            tME.cbSize = sizeof( TRACKMOUSEEVENT );
            tME.dwHoverTime = 0;
            SendMessage( GetParent( hwnd ),
                         WAOM_TBBTNCREATED,
                         MAKEWPARAM( WAOM_TBBTNCREATED_LW, GetWindowLong( hwnd, GWL_ID ) ),
                         ( LPARAM ) hwnd );
        }
        case WM_MOUSELEAVE:
        if( !( GetWindowLong( hwnd, GWL_USERDATA ) & WAO_TBBS_CHECKED ) )
        {
            SetWindowLong( hwnd, GWL_USERDATA, WAO_TBBS_NORMAL );
            RedrawWindow( hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE );
        }
        else
            SetWindowLong( hwnd, GWL_USERDATA, WAO_TBBS_CHECKED | WAO_TBBS_NORMAL );
        break;
        case WM_LBUTTONUP:
        {
            // should be clicked first:
            if( !( GetWindowLong( hwnd, GWL_USERDATA ) & WAO_TBBS_CLICKED ) )
                return FALSE;
            if( GetWindowLong( hwnd, GWL_USERDATA ) & WAO_TBBS_CHECKED )
            {
                if( !( GetWindowLong( hwnd, GWL_STYLE ) & BS_AUTORADIOBUTTON ) )
                {
                    SetWindowLong( hwnd, GWL_USERDATA, WAO_TBBS_HOVERED );
                    tME.dwFlags = TME_LEAVE;
                    tME.hwndTrack = hwnd;
                    TrackMouseEvent( &tME );
                }
            }
            else
            {
                SetWindowLong( hwnd, GWL_USERDATA, WAO_TBBS_CHECKED );
                if( GetWindowLong( hwnd, GWL_STYLE ) & BS_RADIOBUTTON )
                {
                    long wStyle;
                    HWND t_hndWnd = hwnd;
                    while( ( t_hndWnd = GetWindow( t_hndWnd, GW_HWNDPREV ) ) )
                    {
                        wchar_t buffer[ 16 ];
                        GetClassName( t_hndWnd, buffer, 15 );
                        if( lstrcmp( buffer, L"wao_BUTTON_Chk" ) )
                            break;
                        wStyle = GetWindowLong( t_hndWnd, GWL_STYLE );
                        if( !( wStyle & BS_RADIOBUTTON ) )
                            break;
                        SetWindowLong( t_hndWnd, GWL_USERDATA, WAO_TBBS_NORMAL );
                        RedrawWindow( t_hndWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE );
                        if( wStyle & WS_GROUP )
                            break;
                    }
                    t_hndWnd = hwnd;
                    while( ( t_hndWnd = GetWindow( t_hndWnd, GW_HWNDNEXT ) ) )
                    {
                        wchar_t buffer[ 16 ];
                        GetClassName( t_hndWnd, buffer, 15 );
                        if( lstrcmp( buffer, L"wao_BUTTON_Chk" ) )
                            break;
                        wStyle = GetWindowLong( t_hndWnd, GWL_STYLE );
                        if( !( wStyle & BS_RADIOBUTTON ) )
                            break;
                        SetWindowLong( t_hndWnd, GWL_USERDATA, WAO_TBBS_NORMAL );
                        RedrawWindow( t_hndWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE );
                        if( wStyle & WS_GROUP )
                            break;
                    }
                }
            }
            RedrawWindow( hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE );
// RADIO BUTTON
            DLGPROC parProc = ( DLGPROC ) GetWindowLong( GetParent( hwnd ), DWL_DLGPROC );
            parProc( GetParent( hwnd ),
                     WM_COMMAND,
                     MAKEWPARAM( GetWindowLong( hwnd, GWL_ID ), WAO_TBBN_LCLCKED ),
                     ( LPARAM ) hwnd );

        }
        break;
        default:
        return( waoWC_button0Proc( hwnd, msg, wParam, lParam ) );
    }
    
    return 0;   // just to silence the compiler
}

LRESULT CALLBACK
WAO_advanced_edit_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                        UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
    switch( msg )
    {
        case WM_KEYDOWN:
            if( wparam == VK_RETURN )
                SendMessage( GetParent( hwnd ),
                             WAOM_EDITACTIVATED,
                             MAKEWPARAM( GetWindowLong( hwnd, GWL_ID ), WAOM_EDITACTIVATED_HW ),
                             ( LPARAM ) hwnd );
            else if( wparam == VK_ESCAPE )
                SendMessage( GetParent( hwnd ),
                             WAOM_EDITABORTED,
                             MAKEWPARAM( GetWindowLong( hwnd, GWL_ID ), WAOM_EDITACTIVATED_HW ),
                             ( LPARAM ) hwnd );
            break;
        default:
            return DefSubclassProc( hwnd, msg, wparam, lparam );
    }
    return 0;
}

#include <ctime>

bool
WAO_init()
{
    WNDCLASS waoWC_button0, waoWC_buttonChk;

    waoWC_button0.style         = CS_OWNDC | CS_BYTEALIGNWINDOW | CS_PARENTDC;
    waoWC_button0.lpfnWndProc   = waoWC_button0Proc;
    waoWC_button0.cbClsExtra    = 0;
    waoWC_button0.cbWndExtra    = 0;	// state bits?
    waoWC_button0.hInstance     = GetModuleHandle( NULL );
    waoWC_button0.hIcon         = NULL;
    waoWC_button0.hCursor       = LoadCursor( NULL, IDC_HAND );
    waoWC_button0.hbrBackground = CreateSolidBrush( GetSysColor( COLOR_BTNFACE ) );
    waoWC_button0.lpszMenuName  = NULL;
    waoWC_button0.lpszClassName = L"wao_BUTTON_0";

    waoWC_buttonChk = waoWC_button0;
    waoWC_buttonChk.lpfnWndProc   = waoWC_buttonChkProc;
    waoWC_buttonChk.lpszClassName = L"wao_BUTTON_Chk";

    if( !RegisterClass( &waoWC_button0 ) )
    {
        MessageBoxA( NULL, "waoWC_button0 Registration Failed!", "WAO", MB_OK );
        return false;
    }
    if( !RegisterClass( &waoWC_buttonChk ) )
    {
        MessageBoxA( NULL, "waoWC_buttonChk Registration Failed!", "WAO", MB_OK );
        return false;
    }

    return true;
}

BOOL CALLBACK
WAO_toolbar_proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return FALSE;
}

BOOL
WAO_set_icon( HWND hwnd, HICON hIcon )
{
    BOOL retv = SetPropA( hwnd, WAO_PROP_ICON, hIcon );
    InvalidateRect( hwnd, NULL, TRUE );
    return retv;
}

// INPUT DIALOG ====================================================================================
WAO_InputDlg::WAO_InputDlg( const Wstring& title, const Wstring& text )
:   m_text( text ), m_title( title )
{
}

bool
WAO_InputDlg::launch( HWND hWPar, DLGPROC redirProc )
{
    DialogBox( GetModuleHandle( NULL ), MAKEINTRESOURCE( WAO_IDD_INPUT ), hWPar, redirProc );
    return m_result;
}

bool
WAO_InputDlg::proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{   
    switch( msg )
    {
        case WM_INITDIALOG:
            m_hwnd = hwnd;
            SetWindowText( m_hwnd, m_title.c_str() );
            SetDlgItemText( m_hwnd, WAO_IDE_INPUT, m_text.c_str() );
            m_result = false;
            return TRUE;
        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDOK:
                {
                    m_result = true;
                    wchar_t buffer[ 1024 ];
                    GetDlgItemText( m_hwnd, WAO_IDE_INPUT, buffer, 1023 );
                    m_text = buffer;
                    EndDialog( m_hwnd, 0 );
                }
                return FALSE;
            }
            return FALSE;
        case WM_DESTROY:
            m_hwnd = NULL;	// whenever window is destroyed hW is nullified
            return FALSE;
        default:
            return FALSE;
    }
}

// MENU ============================================================================================
WAO_Menu::WAO_Menu()
{
    m_hmenu = NULL;
}

HMENU
WAO_Menu::init()
{
    return( m_hmenu = CreatePopupMenu() );
}
// APPEND
bool
WAO_Menu::append( UINT flags, UINT newItm, LPCTSTR content )
{
    return AppendMenu( m_hmenu, flags, newItm, content );
}
// TRACK MENU
int
WAO_Menu::track( UINT flags, HWND hWOwn, int posX, int posY )
{
    //hMenuAct = hMenu;
    //hMenu = NULL;	// to avoid recreation during append()
    if( posX < 0 )
    {
        POINT pnt;
        GetCursorPos (&pnt);
        posX = pnt.x;
        posY = pnt.y;
    }
    return TrackPopupMenu( m_hmenu, flags, posX, posY, 0, hWOwn, NULL );
}
// SET DEFAULT ITEM
bool
WAO_Menu::set_default_item( UINT itm, UINT byPos )
{
    return SetMenuDefaultItem( m_hmenu, itm, byPos );
}
// WHETHER THE MENU IS ACTIVE
bool
WAO_Menu::is_active()
{
    return( ( bool ) m_hmenu );
}
// DESTROY MENU IF IT IS ACTIVE
bool
WAO_Menu::destroy()
{
    if( m_hmenu )
    {
        DestroyMenu( m_hmenu );
        m_hmenu = 0;
        return true;        // it was active
    }
    else
        return false;       // it was already inactive
}
