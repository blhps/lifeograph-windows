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


#ifndef LIFEOGRAPH_WIN_APP_WINDOW_HEADER
#define LIFEOGRAPH_WIN_APP_WINDOW_HEADER

#define LIFEOGRAPH_DEVELOPMENT_VERSION


#include "win_login.hpp"
#include "win_views.hpp"
#include "win_richedit.hpp"
#include "helpers.hpp"
#include "settings.hpp"


namespace LIFEO
{

class WinAppWindow
{
    public:
                                    WinAppWindow();
                                    ~WinAppWindow();
                                    
        int                         run( HINSTANCE );
        HWND                        get_hwnd() const
        { return m_hwnd; }
        LRESULT                     proc( HWND, UINT, WPARAM, LPARAM );
        void                        handle_create();
        void                        handle_resize( short, short );
        void                        handle_notify( int, LPARAM );
        
        BOOL                        init_list();

        void                        update_menu();
        void                        update_entry_list();
        void                        update_calendar();
        void                        update_startup_elem();
        void                        sync_entry();
        void                        update_title();

        bool                        confirm_dismiss_element( const DiaryElement* );

        void                        login();
        bool                        finish_editing( bool = true );
        void                        logout( bool );

        void                        show_about();

        bool                        write_backup();

        // AUTO LOGOUT FUNCTIONALITY
        bool                        handle_idle();
        void                        freeze_auto_logout();
        void                        unfreeze_auto_logout();
        void                        update_auto_logout_count_down();
        void                        disable_auto_logout_for_the_session();
        void                        reenable_auto_logout_for_the_session();
        bool                        is_auto_logout_in_use()
        { return( m_auto_logout_status == 0 ); }

        // PANELS AND VIEWS
        static WinAppWindow*        p;
        WinLogin*                   m_login;
        EntryView*                  m_entry_view;
        DiaryView*                  m_diary_view;

    //protected:
        int                         m_seconds_remaining;
        int                         m_auto_logout_status; // reflects the result of all factors

        HWND                        m_hwnd;
        RichEdit*                   m_richedit;
        HWND                        m_calendar;
        HWND                        m_list;
};

} // end of namespace LIFEO

#endif

