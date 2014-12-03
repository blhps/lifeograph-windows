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


#ifndef LIFEOGRAPH_LISTITEMDIARY_HEADER
#define LIFEOGRAPH_LISTITEMDIARY_HEADER


#include "entry.hpp"
#include "diary.hpp"


namespace LIFEO
{

class EntryView : public ElementShower< Entry >
{
    public:
                                EntryView();

        void                    sync();

        void                    focus_tag();

        Ustring                 get_title_str() const
        {
            Ustring title;

            if( ! m_ptr2elem->get_date().is_hidden() )
            {
                title = m_ptr2elem->get_date().format_string();
                title += "  ";
            }

            if( m_ptr2elem->get_date().is_ordinal() )
                title += m_ptr2elem->get_name();
            else
                title += m_ptr2elem->get_date().get_weekday_str();

            return title;
        }

        void                    show( Entry& );

        void                    handle_login();

        void                    update_formatting_toolbar();
        void                    update_highlight_button();
        void                    toggle_comments();

        void                    trash_entry();
        void                    restore_entry();
        void                    dismiss_entry();
        void                    print();
        void                    set_todo_status( ElemStatus );
        void                    toggle_favoredness();
        void                    hide();
        void                    hide_before();
        void                    hide_after();

        void                    update_theme();

    protected:
        enum TagOperation { TO_NONE, TO_ADD, TO_REMOVE, TO_CREATE_AND_ADD };
        
        void                    handle_textview_changed();
        void                    handle_entry_tag_changed();
        void                    handle_button_tag_clicked();
        void                    handle_set_theme();

        void                    focus_tag_entry();
        void                    edit_tag_in_entry( const Tag* );
        void                    remove_selected_tags();

        // WIDGETS

        bool                    m_flag_entrychanged;
        Entry*                  m_entry_sync;

    friend class Lifeograph;
};

class DiaryView : public ElementShower< Diary >
{
    public:
                                DiaryView();
                                ~DiaryView();

        Ustring                 get_title_str() const
        { return m_ptr2elem->m_name; }

        void                    handle_login();

        void                    show( Diary& );

        void                    update_startup_elem();

    protected:
        // DIALOGS

        void                    open_diary_folder();
        void                    handle_language_changed();
        void                    handle_startup_type_changed();
};

} // end of namespace LIFEO

#endif

