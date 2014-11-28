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


#ifndef LIFEOGRAPH_WIN_LOGIN_HEADER
#define LIFEOGRAPH_WIN_LOGIN_HEADER


#include <string>

#include "helpers.hpp"


namespace LIFEO
{

class WinLogin
{
        enum InfoResponse { RESP_OK = 10, RESP_UNDO = 20 };
        enum SortDateType { SDT_ACCESS, SDT_SAVE, SDT_NAME };

    public:
                                    WinLogin();

        void                        handle_start();
        void                        handle_login();
        void                        handle_logout();

        void                        populate_diaries();

        void                        create_new_diary();
        void                        add_existing_diary();

        static std::string          m_path_cur;

    protected:
        // these two functions are to mask bool return type of open_selected_diary()
        // from signal connection system
        void                        handle_button_read()
        { open_selected_diary( true ); }
        void                        handle_button_edit()
        { open_selected_diary( false ); }
        HELPERS::Result             open_selected_diary( bool );
        void                        open_activated_diary();
        void                        remove_selected_diary();

        void                        initialize();

        std::string                 m_path_removed;
        bool                        m_flag_info_is_visible;
        bool                        m_flag_diary_activated;
        int                         m_password_attempt_no;
        InfoResponse                m_resp_cur; // FIXME: a most unfortunate solution!!
        SortDateType                m_sort_date_type;
};

} // end of namespace LIFEO

#endif

