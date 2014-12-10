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


#include "win_widget_tag_list.hpp"
#include "win_app_window.hpp"

using namespace LIFEO;
using namespace HELPERS;


WidgetTagList::WidgetTagList( )
:   m_ptr2entry( NULL ), m_pos_x( MARGIN ), m_pos_y( MARGIN ), m_y_offset( 0 ),
    m_hovered_tag( NULL ), m_flag_editable( true )
{

}

void
WidgetTagList::set_entry( Entry* ptr2entry )
{
    m_ptr2entry = ptr2entry;
    m_pos_x = MARGIN;
}

void
WidgetTagList::update()
{
    RECT rect;
    rect.left = 0;
    rect.top = m_y_offset;
    rect.right = m_width;
    rect.bottom = m_y_offset + m_height;
    InvalidateRect( WinAppWindow::p->m_hwnd, &rect, true );
}

void
WidgetTagList::update_full()
{
    RECT rect;
    GetClientRect( WinAppWindow::p->m_hwnd, &rect );

    const int editor_width = rect.right * WinAppWindow::EDITOR_RATIO;
    handle_resize( editor_width, rect.bottom );
    const int editor_height = rect.bottom - m_height;

    MoveWindow( WinAppWindow::p->m_entry_view->m_richedit->m_hwnd, 0, 0,
                editor_width, editor_height, TRUE );

    rect.top = editor_height;
    rect.right = editor_width;
    InvalidateRect( WinAppWindow::p->m_hwnd, &rect, true );
}

void
WidgetTagList::handle_resize( int width, int client_height )
{
    m_width = width;
    m_height = calculate_height();
    m_y_offset = client_height - m_height;
}

int
WidgetTagList::calculate_height()
{
    if( m_ptr2entry == NULL )
        return( MARGIN + TEXT_HEIGHT + MARGIN );

    int pos_x( MARGIN ), pos_y( MARGIN );
    
    RECT rect;
    
    HDC hdc = GetDC( WinAppWindow::p->m_hwnd );
    
    SelectObject( hdc, GetStockObject( DEFAULT_GUI_FONT ) );

    for( Tag* tag : m_ptr2entry->get_tags() )
    {
        // CALCULATE THE WIDTH
        DrawText( hdc, convert_utf8_to_16( tag->get_name() ), -1,
                  &rect, DT_NOPREFIX | DT_SINGLELINE | DT_CALCRECT );
        int text_width = rect.right - rect.left;

        if( pos_x + ICON_SIZE + LABEL_OFFSET + text_width + MARGIN > m_width )
        {
            pos_x = MARGIN;
            pos_y += ( ICON_SIZE + VSPACING );
        }

        pos_x += ( text_width + HALF_HEIGHT + HSPACING );
    }
    
    // Add item
    if( m_flag_editable )
    {
        DrawText( hdc, L"Add Tag", -1, &rect, DT_NOPREFIX | DT_SINGLELINE | DT_CALCRECT );
        int text_width = rect.right - rect.left;

        if( pos_x + ICON_SIZE + LABEL_OFFSET + text_width + MARGIN > m_width )
            pos_y += ( ICON_SIZE + VSPACING );
    }
    
    ReleaseDC( WinAppWindow::p->m_hwnd, hdc );

    return( pos_y + TEXT_HEIGHT + MARGIN );
}

int
WidgetTagList::get_height()
{
    return m_height;
}

bool
WidgetTagList::handle_draw()
{
    if( m_ptr2entry == NULL )
        return false;
        
    PAINTSTRUCT ps;
	HDC hDC;

	// DEVICE CONTEXT
    hDC = BeginPaint( WinAppWindow::p->m_hwnd, &ps );
    if( !hDC )
        return false;

    SetBkMode( hDC, TRANSPARENT );
    SelectObject( hDC, GetStockObject( DEFAULT_GUI_FONT ) );

    m_pos_x = m_pos_y = MARGIN;
    m_pos_y += m_y_offset;
    m_items.clear();

    const Tagset& ptr2tagset = m_ptr2entry->get_tags();

    if( ptr2tagset.empty() && !m_flag_editable )
    {
        SetTextColor( hDC, 0x0 );
        TextOut( hDC, m_pos_x, m_pos_y, L"Not tagged", -1 );
    }

    for( Tag* tag : ptr2tagset )
        add_item( hDC, convert_utf8_to_16( tag->get_name() ), tag );

    if( m_flag_editable )
        add_item( hDC, L"Add Tag", &m_add_tag_item );

    EndPaint( WinAppWindow::p->m_hwnd, &ps );

    return true;
}

void
WidgetTagList::add_item( const HDC hdc, const Wstring& label, const Tag* tag )
{
    // CALCULATE THE WIDTH
    RECT rect;
    DrawText( hdc, label.c_str(), -1, &rect, DT_NOPREFIX | DT_SINGLELINE | DT_CALCRECT );
    int text_width = rect.right - rect.left;
    
    // FILL THE ITEM
    TagItem titem;

    if( m_pos_x + ICON_SIZE + LABEL_OFFSET + text_width + MARGIN > m_width )
    {
        m_pos_x = MARGIN;
        m_pos_y += ( ICON_SIZE + VSPACING );
    }

    titem.tag = tag;
    titem.xl = m_pos_x - ITEM_BORDER;
    titem.xr = m_pos_x + text_width + HALF_HEIGHT;
    titem.yl = m_pos_y - ITEM_BORDER;
    titem.yr = titem.yl + ITEM_HEIGHT;

    if( tag == &m_add_tag_item )
        titem.xr += ( ICON_SIZE + LABEL_OFFSET );

    m_items.push_back( titem );

    // BACKGROUND
    if( !( tag != m_hovered_tag && tag == &m_add_tag_item ) )
    {
        // REGION
        POINT pts[ 5 ];
        int width( titem.xr - titem.xl - HALF_HEIGHT );
        int bs = ( tag == m_hovered_tag && m_flag_editable ) ? 2 : 1;

        pts[ 0 ].x = titem.xl; pts[ 0 ].y = titem.yl;
        pts[ 1 ].x = titem.xl + width; pts[ 1 ].y = titem.yl;
        pts[ 2 ].x = titem.xl + width + HALF_HEIGHT; pts[ 2 ].y = titem.yl + HALF_HEIGHT;
        pts[ 3 ].x = titem.xl + width; pts[ 3 ].y = titem.yl + 2 * HALF_HEIGHT;
        pts[ 4 ].x = titem.xl; pts[ 4 ].y = titem.yl + 2 * HALF_HEIGHT;

        if( tag->get_has_own_theme() )
        {
            HBRUSH hbrush = CreateSolidBrush( parse_color( tag->get_theme()->color_base ) );
            SelectObject( hdc, hbrush );
            
            HPEN hpen = CreatePen( PS_SOLID, bs, parse_color( tag->get_theme()->color_highlight ) );
            SelectObject( hdc, hpen );
            Polygon( hdc, pts, 5 );
            DeleteObject( hpen );
            
            hpen = CreatePen( PS_DOT, bs, parse_color( tag->get_theme()->color_heading ) );
            SelectObject( hdc, hpen );
            Polygon( hdc, pts, 5 );
            DeleteObject( hpen );
            
            DeleteObject( hbrush );
        }
        else
        {
            HRGN region = CreatePolygonRgn( pts, 5, ALTERNATE );
            HBRUSH hbrush = CreateSolidBrush( 0x0 );
            FrameRgn( hdc, region, hbrush, bs, bs );
            DeleteObject( hbrush );
            DeleteObject( region );
        }
    }

    /*// ICON
    if( tag == &m_add_tag_item )
    {
        cr->set_source( m_image_surface_add, m_pos_x, m_pos_y );
        cr->rectangle( m_pos_x, m_pos_y, ICON_SIZE, ICON_SIZE );
        cr->clip();
        cr->paint();
        cr->reset_clip();

        m_pos_x += ( ICON_SIZE + LABEL_OFFSET );
    }*/

    // LABEL
    if( tag->get_has_own_theme() )
        SetTextColor( hdc, parse_color( tag->get_theme()->color_text ) );
    else
        SetTextColor( hdc, 0x0 );
    
    TextOut( hdc, m_pos_x, m_pos_y, label.c_str(), label.size() );

    m_pos_x += ( text_width + HALF_HEIGHT + HSPACING );
}

void
WidgetTagList::handle_mouse_move( int x, int y )
{
    if( m_ptr2entry != NULL )
    {
        const Tag* hovered_tag( NULL );

        for( TagItem& ti : m_items )
        {
            if( ti.xl < x && ti.xr > x &&
                ti.yl < y && ti.yr > y )
            {
                hovered_tag = ti.tag;
                break;
            }
        }

        if( hovered_tag != m_hovered_tag )
        {
            m_hovered_tag = hovered_tag;
            
            update();
        }
    }
}

void
WidgetTagList::handle_click( int x, int y )
{
    if( m_ptr2entry != NULL )
    {
        const Tag* hovered_tag( NULL );

        for( TagItem& ti : m_items )
        {
            if( ti.xl < x && ti.xr > x &&
                ti.yl < y && ti.yr > y )
            {
                WinAppWindow::p->start_tag_dialog( convert_utf8_to_16( ti.tag->get_name() ) );
                break;
            }
        }
    }
}
