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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <windows.h>

#include "lifeograph.hpp"
#include "win_app_window.hpp"
#include "win_entry_view.hpp"


using namespace LIFEO;


EntryView::EntryView()
:   m_entry_sync( NULL ), m_flag_entrychanged( false )
{
    Entry::shower = this;
}

void
EntryView::sync()
{
    if( m_flag_entrychanged )
    {
        m_entry_sync->set_date_changed( time( NULL ) );
        //m_entry_sync->set_text( m_textview->m_buffer->get_text() );
        m_flag_entrychanged = false;
        PRINT_DEBUG( "entry synced" );
    }
}

void
EntryView::handle_login()
{

}

void
EntryView::handle_textview_changed()
{
    if( Lifeograph::m_internaloperation ) return;

    m_flag_entrychanged = true;

    PRINT_DEBUG( "entry changed" );

    // TODO:
    //if( m_hbox_tagtools->is_visible() )
        //m_hbox_tagtools->hide();
}

void
EntryView::toggle_favoredness()
{
    if( Lifeograph::m_internaloperation )
        return;

    m_ptr2elem->toggle_favored();
    //AppWindow::p->panel_main->update_overlay_icon();
}

void
EntryView::set_todo_status( ElemStatus status )
{
    if( Lifeograph::m_internaloperation )
        return;

    m_ptr2elem->set_todo_status( status );
    //AppWindow::p->panel_main->set_icon( m_ptr2elem->get_icon32() );
}

void
EntryView::print()
{

}

void
EntryView::hide()
{
    Diary::d->get_filter()->add_entry( m_ptr2elem );
    //AppWindow::p->panel_diary->update_calendar();
    //AppWindow::p->panel_diary->update_entry_list();
}

void
EntryView::hide_before()
{
    Diary::d->set_filter_date_begin( m_ptr2elem->get_date_t() );
    //AppWindow::p->panel_diary->update_calendar();
    //AppWindow::p->panel_diary->update_entry_list();
}

void
EntryView::hide_after()
{
    Diary::d->set_filter_date_end( m_ptr2elem->get_date_t() );
    //AppWindow::p->panel_diary->update_calendar();
    //AppWindow::p->panel_diary->update_entry_list();
}

void
EntryView::update_theme()
{
    //m_textview->m_buffer->set_theme( m_ptr2elem->get_theme() );
}

void
EntryView::trash_entry()
{

}

void
EntryView::restore_entry()
{

}

void
EntryView::dismiss_entry()
{

}

void
EntryView::show( Entry& entry )
{
    // do nothing if entry is already the current element:
    //if( AppWindow::p->panel_main->is_cur_elem( &entry ) )
        //return;

    m_ptr2elem = &entry; // must be first thing

    // BODY
    WinAppWindow::p->m_richedit->set_richtext( m_ptr2elem );

    m_entry_sync = m_ptr2elem;
}

