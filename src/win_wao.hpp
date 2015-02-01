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


// MESSAGES
#define WAOM_TBBTNCREATED       0x7FA0  // toolbar button created
#define WAOM_TBBTNCREATED_LW    9867    // really WAO msg (loword of wparam)

// TIMER IDS
#define WAO_TMR_TBRPW   1808  // toolbar button repeat wait
#define WAO_TMR_TBRPT   1809  // toolbar button repeat

// toolbar button messages
#define WAOM_TBBTNCREATED       0x7FA0  // toolbar button created
#define WAOM_TBBTNCREATED_LW    9867    // really WAO msg (loword of wparam)
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

#endif

