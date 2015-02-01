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


//#define _WIN32_IE 0x0400
#include <commctrl.h>

#include "win_login.hpp"
#include "win_views.hpp"
#include "helpers.hpp"
#include "settings.hpp"


namespace LIFEO
{

class WinAppWindow
{
    public:
        static constexpr float EDITOR_RATIO = 0.6;
        static constexpr int TOOLBAR_HEIGHT = 35;

                                    WinAppWindow();
                                    ~WinAppWindow();

        int                         run();
        HWND                        get_hwnd() const
        { return m_hwnd; }
        LRESULT                     proc( HWND, UINT, WPARAM, LPARAM );
        BOOL                        proc_toolbar( HWND, UINT, WPARAM, LPARAM );
        void                        handle_create();
        void                        handle_resize( short, short );
        void                        handle_notify( int, LPARAM );

        BOOL                        init_list();
        bool                        select_list_elem( const DiaryElement* );
        bool                        update_list_elem( const DiaryElement* );
        HTREEITEM                   add_list_elem( DiaryElement*, HTREEITEM, bool = false );
        void                        add_chapter_category_to_list( const CategoryChapters*,
                                                                  HTREEITEM );
        void                        update_entry_list();

        void                        update_menu();

        void                        fill_monthdaystate( int, int, MONTHDAYSTATE[], int );
        void                        update_calendar();
        void                        update_title();
        
        void                        handle_calendar_doubleclick();
        
        void                        start_tag_dialog( const Wstring& );

        bool                        authorize();

        bool                        confirm_dismiss_element( const DiaryElement* );
        VOID APIENTRY               display_context_menu( HWND, HWND, int id );

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

        HWND                        m_toolbar;
        HWND                        m_hwnd;
        HWND                        m_calendar;
        HWND                        m_list;

        HMENU                       m_hmenu;
};

} // end of namespace LIFEO

#endif

