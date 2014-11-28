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


#ifndef LIFEOGRAPH_WIN_DIALOG_PASSWORD_HEADER
#define LIFEOGRAPH_WIN_DIALOG_PASSWORD_HEADER


#include "helpers.hpp"


namespace LIFEO
{

// PASSWORD DIALOG =================================================================================
class DialogEnterPassword
{
    public:
                                    DialogEnterPassword();

        static int                  launch( HWND, Diary*, bool = false );
        bool                        proc( HWND, UINT, WPARAM, LPARAM );

        static DialogEnterPassword* ptr;

    protected:
        void                        handle_entry_changed();

        Diary*                      m_ptr2diary;
        bool                        m_flag_again;
        
        HWND                        m_hwnd;
        HWND                        m_edit_current;

        static const int            SLEEP_DURATION = 2000;
};

} // end of namespace LIFEO

#endif

