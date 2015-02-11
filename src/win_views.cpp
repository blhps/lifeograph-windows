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
#include <algorithm>

#include "lifeograph.hpp"
#include "win_app_window.hpp"
#include "win_views.hpp"
#include "../rc/resource.h"


using namespace LIFEO;


EntryView::EntryView()
:   m_entry_sync( NULL ), m_flag_entrychanged( false )
{
    Entry::shower = this;

    m_richedit = new RichEdit;
    m_tag_widget = new WidgetTagList;
}

void
EntryView::sync()
{
    if( m_flag_entrychanged )
    {
        m_entry_sync->set_date_changed( time( NULL ) );
        
        Wstring text( m_richedit->get_text() );
        std::replace( text.begin(), text.end(), '\r', '\n' );
        
        m_entry_sync->set_text( convert_utf16_to_8( text.c_str() ) );
        
        m_flag_entrychanged = false;
    }
}

void
EntryView::handle_login()
{

}

void
EntryView::handle_text_change()
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
    
    //WinAppWindow::p->panel_main->set_icon( m_ptr2elem->get_icon32() );
    WinAppWindow::p->update_list_elem( m_ptr2elem );
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
    m_richedit->set_theme( m_ptr2elem->get_theme() );
    m_richedit->reparse();
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
    // TODO: part of this function will be moved to trash_entry() when trash support is added

    if( ! WinAppWindow::p->confirm_dismiss_element( m_ptr2elem ) )
        return;

    if( ! m_ptr2elem )
        return;

    if( Diary::d->get_startup_elem() == m_ptr2elem )
    {
        Diary::d->set_startup_elem( Diary::d->get_id() );
        //AppWindow::p->m_diary_view->update_startup_elem();
    }

    Entry* ptr2entry = m_ptr2elem; // save the pointer
    
    // first make sure that entry to be deleted is completely out of business...
    // ...else dark forces may take action and cause SEGFAULT:

    Diary::d->show();

    //AppWindow::p->remove_element_from_history( m_ptr2elem );
    Diary::d->dismiss_entry( ptr2entry );

    Lifeograph::m_internaloperation++;

    WinAppWindow::p->update_calendar();
    WinAppWindow::p->update_entry_list();

    Lifeograph::m_internaloperation--;
}

void
EntryView::show( Entry& entry )
{
    // do nothing if entry is already the current element:
    if( m_ptr2elem && m_ptr2elem->get_id() == entry.get_id() )
        return;

    m_ptr2elem = &entry; // must be first thing

    m_richedit->set_richtext( m_ptr2elem );
    m_tag_widget->set_entry( m_ptr2elem );
    m_tag_widget->update_full();
    WinAppWindow::p->select_list_elem( m_ptr2elem );

    ShowWindow( WinAppWindow::p->m_button_elem, SW_SHOW );

    ShowWindow( WinAppWindow::p->m_button_title, SW_SHOW );
    SetWindowText( WinAppWindow::p->m_button_title, HELPERS::convert_utf8_to_16( get_title_str() ) );
    EnableWindow( WinAppWindow::p->m_button_title, entry.get_date().is_hidden() ? FALSE : TRUE );
    InvalidateRect( WinAppWindow::p->m_button_title, NULL, TRUE );

    m_entry_sync = m_ptr2elem;
}

void
EntryView::clear()
{
    m_ptr2elem = NULL;
    m_richedit->set_richtext( NULL );
    m_tag_widget->set_entry( NULL );
    m_tag_widget->update();
    m_entry_sync = NULL;
}

// DIARY ===========================================================================================
DiaryView::DiaryView()
{
    Diary::shower = this;
    m_ptr2elem = Diary::d;	// for now no other diary is shown in the view
}

DiaryView::~DiaryView()
{
    Diary::shower = NULL;
}

void
DiaryView::handle_login()
{

}

void
DiaryView::show( Diary& diary )
{
    WinAppWindow::p->m_entry_view->clear();

    ShowWindow( WinAppWindow::p->m_button_title, SW_HIDE );
    ShowWindow( WinAppWindow::p->m_button_elem, SW_HIDE );
}

LIFEO::Result
DiaryView::export_diary()
{
    OPENFILENAME ofn;
    Wchar szFileName[ MAX_PATH ];

    ZeroMemory( &ofn, sizeof( ofn ) );
    szFileName[ 0 ] = 0;

    ofn.lStructSize = sizeof( ofn );
    ofn.hwndOwner = WinAppWindow::p->get_hwnd();
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if( GetSaveFileName( &ofn ) )
    {
        std::string path = convert_utf16_to_8( szFileName );
        return Diary::d->write_txt( path, false );
    }

    return ABORTED;
}

