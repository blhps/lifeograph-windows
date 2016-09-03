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


#ifndef LIFEOGRAPH_WIN_TOOLBARS_HEADER
#define LIFEOGRAPH_WIN_TOOLBARS_HEADER


#include "../rc/resource.h"
#include "helpers.hpp"


// MESSAGES
#define WAOM_EDITACTIVATED      0x7FA0  // enter pressed
#define WAOM_EDITACTIVATED_HW   9847    // to check if really WAO msg (hiword of wparam)
#define WAOM_EDITABORTED        0x7FA1  // escape pressed
#define WAON_EDITACTIVATED      157559847   // this is sent via WM_COMMAND
#define WAOM_TBBTNCREATED       0x7FA2  // toolbar button created
#define WAOM_TBBTNCREATED_LW    9867    // to check if really WAO msg (loword of wparam)

// TIMER IDS
#define WAO_TMR_TBRPW   1808  // toolbar button repeat wait
#define WAO_TMR_TBRPT   1809  // toolbar button repeat

// toolbar button states
#define WAO_TBBS_NORMAL     0
#define WAO_TBBS_HOVERED    1
#define WAO_TBBS_CLICKED    2
#define WAO_TBBS_CHECKED    32
#define WAO_TBBS_DISABLED   256
// toolbar button notification codes
#define WAO_TBBN_LCLCKED    1
#define WAO_TBBN_RCLCKED    2

#define WAO_TBBSFLTR_CLK_   287 // check filter (256+31)
// toolbar wait times
#define WAO_TBBN_RPWTIME    800    // ms
#define WAO_TBBN_RPTTIME    100    // less this value, greater repeat speed


bool WAO_init ();		// WAO INITIALIZATION

BOOL CALLBACK WAO_toolbar_proc( HWND, UINT, WPARAM, LPARAM );

LRESULT CALLBACK WAO_advanced_edit_proc( HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR );

class WAO_InputDlg
{
    public:
                            WAO_InputDlg( const Wstring&, const Wstring& );

        bool                launch( HWND, DLGPROC );

        bool                proc( HWND, UINT, WPARAM, LPARAM );

        Wstring             m_text;

    private:
        Wstring             m_title;
        bool                m_result;
        HWND                m_hwnd;
};

// MENU CLASS=======================================================================================
class WAO_Menu
{
    public:
        HMENU               m_hmenu;
                            WAO_Menu();		// for only drop down menus now
        HMENU               init();
        bool                append( UINT, UINT, LPCTSTR );
        int                 track( UINT, HWND, int=-1, int=-1 );
        bool                set_default_item( UINT, UINT );
        bool                is_active();
        bool                destroy();
};
#endif

