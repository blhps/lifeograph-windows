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


#ifndef LIFEOGRAPH_WIN_DIALOG_TAGS_HEADER
#define LIFEOGRAPH_WIN_DIALOG_TAGS_HEADER


#include "helpers.hpp"
#include "diary.hpp"


namespace LIFEO
{

// PASSWORD DIALOG =================================================================================
class DialogTags
{
    public:
        enum TagOperation { TO_NONE, TO_INVALID,
                            TO_ADD, TO_REMOVE, TO_CHANGE_VALUE,
                            TO_CREATE_BOOLEAN, TO_CREATE_CUMULATIVE };

                                    DialogTags();

        static Result               launch( HWND, Diary*, Entry*, const Ustring& ); // Assigning
        static Result               launch( HWND, Diary* ); // Filtering

        bool                        proc( HWND, UINT, WPARAM, LPARAM );

        static DialogTags*          ptr;

    protected:
        void                        handle_entry_changed();
        void                        update_list();
        HTREEITEM                   add_list_elem( DiaryElement*, HTREEITEM, bool = false );
        void                        handle_tag_menu_result( int, TVITEM );

        Diary*                      m_ptr2diary;
        Entry*                      m_ptr2entry;
        TagOperation                m_tag_operation_cur;
        Tag*                        m_tag_cur;
        NameAndValue                m_nav;

        HWND                        m_hwnd;
        HWND                        m_edit;
        HWND                        m_button_theme;
        HWND                        m_button_action;
        HWND                        m_button_filter_clear;
        HWND                        m_button_filter_set;
        HWND                        m_list;
        
        bool                        m_flag_filter;
};

} // end of namespace LIFEO

#endif

