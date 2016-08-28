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


#include <windowsx.h>
#include "win_dialog_tags.hpp"

#include "../rc/resource.h"

#include "lifeograph.hpp"
#include "helpers.hpp"
#include "diary.hpp"	// for LIFEO::PASSPHRASE_MIN_SIZE
#include "win_dialog_password.hpp"


using namespace LIFEO;


// ENTER PASSWORD DIALOG ===========================================================================
DialogTags* DialogTags::ptr = NULL;

inline static BOOL CALLBACK
dialog_tags_proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return DialogTags::ptr->proc( hwnd, msg, wParam, lParam );
}

DialogTags::DialogTags()
:   m_tag_cur( NULL ), m_flag_filter( true )
{

}

Result
DialogTags::launch( HWND hw_par, Diary* diary, Entry* entry, const Ustring& name )
{
    ptr = new DialogTags;

    ptr->m_ptr2diary = diary;
    ptr->m_ptr2entry = entry;
    ptr->m_nav.name = name;

    Result res = ( Result ) DialogBox( NULL,
                                       MAKEINTRESOURCE( IDD_TAGS ),
                                       hw_par,
                                       dialog_tags_proc );

    delete ptr;
    ptr = NULL;

    return res;
}

Result
DialogTags::launch( HWND hw_par, Diary* diary )
{
    ptr = new DialogTags;

    ptr->m_ptr2diary = diary;
    ptr->m_ptr2entry = NULL;

    Result res = ( Result ) DialogBox( NULL,
                                       MAKEINTRESOURCE( IDD_TAGS ),
                                       hw_par,
                                       dialog_tags_proc );

    delete ptr;
    ptr = NULL;

    return res;
}

bool
DialogTags::proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_INITDIALOG:
        {
            // HWNDs
            m_hwnd = hwnd;

            m_edit = GetDlgItem( hwnd, IDE_TAG_NAME );
            m_button_theme = GetDlgItem( hwnd, IDB_TAG_THEME );
            m_button_action = GetDlgItem( hwnd, IDB_TAG_ACTION );
            m_button_filter_clear = GetDlgItem( hwnd, IDB_TAG_FILTER_CLEAR );
            m_button_filter_set = GetDlgItem( hwnd, IDB_TAG_FILTER_SET );
            m_list = GetDlgItem( hwnd, IDTV_TAG_LIST );
            
            if( m_ptr2entry )
            {
                ShowWindow( m_button_filter_clear, SW_HIDE );
                ShowWindow( m_button_filter_set, SW_HIDE );
            }
            else
            {
                ShowWindow( m_button_theme, SW_HIDE );
                ShowWindow( m_button_action, SW_HIDE );
                
                if( !m_ptr2diary->get_filter_tag() )
                    EnableWindow( m_button_filter_clear, FALSE );
            }
            
            // IMAGELIST
            HIMAGELIST himagelist = ImageList_Create( 16, 16, ILC_COLOR24, 0, 12 );
            HBITMAP hbitmap = ( HBITMAP ) LoadImage( Lifeograph::p->hInst,
                                                     MAKEINTRESOURCE( IDBM_ENTRY16 ),
                                                     IMAGE_BITMAP, 0, 0,
                                                     LR_LOADTRANSPARENT );
            ImageList_Add( himagelist, hbitmap, NULL );
            DeleteObject( hbitmap );
            TreeView_SetImageList( m_list, himagelist, TVSIL_NORMAL );

            SetWindowText( m_edit, convert_utf8_to_16( m_nav.name ) );

            update_list();

            //EnableWindow( GetDlgItem( m_hwnd, IDOK ), false );

            return TRUE;
        }
        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDE_TAG_NAME:
                    if( HIWORD( wParam ) == EN_UPDATE )
                        handle_entry_changed();
                    return TRUE;
                case IDCANCEL:
                    EndDialog( m_hwnd, HELPERS::ABORTED );
                    return TRUE;
                case IDB_TAG_THEME:
                {
                    // only tags in entry can be set as the theme
                    if( m_tag_operation_cur == TO_REMOVE ) 
                    {
                        Tag* tag{ Diary::d->get_tags()->get_tag( m_nav.name ) };

                        if( tag->get_has_own_theme() )
                            m_ptr2entry->set_theme_tag( tag );

                        EndDialog( m_hwnd, SUCCESS );
                    }
                    return TRUE;
                }
                case IDB_TAG_ACTION:
                {
                    Result result( SUCCESS );

                    switch( m_tag_operation_cur )
                    {
                        case TO_NONE:
                        case TO_INVALID:
                            result = ABORTED;
                            break;
                        case TO_REMOVE:
                            m_ptr2entry->remove_tag( m_tag_cur );
                            break;
                        case TO_CREATE_BOOLEAN:
                        case TO_CREATE_CUMULATIVE:
                            if( m_tag_operation_cur == TO_CREATE_CUMULATIVE )
                                m_tag_cur = Diary::d->create_tag( m_nav.name, nullptr,
                                        ChartPoints::MONTHLY|ChartPoints::CUMULATIVE );
                            else
                                m_tag_cur = Diary::d->create_tag( m_nav.name, nullptr );

                            m_ptr2entry->add_tag( m_tag_cur, m_nav.value );
                            break;
                        case TO_CHANGE_VALUE:
                            m_ptr2entry->remove_tag( m_tag_cur );
                            m_ptr2entry->add_tag( m_tag_cur, m_nav.value );
                            break;
                        case TO_ADD:
                            m_ptr2entry->add_tag( m_tag_cur, m_nav.value );
                            break;
                    }

                    EndDialog( m_hwnd, result );
                    return TRUE;
                }
                case IDB_TAG_FILTER_CLEAR:
                    m_ptr2diary->set_filter_tag( NULL );
                    EndDialog( m_hwnd, SUCCESS );
                    return TRUE;
                case IDB_TAG_FILTER_SET:
                    m_ptr2diary->set_filter_tag( m_tag_cur );
                    EndDialog( m_hwnd, SUCCESS );
                    return TRUE;
            }
            return FALSE;
        case WM_NOTIFY:
            if( wParam == IDTV_TAG_LIST && ( ( LPNMHDR ) lParam )->code == NM_CLICK )
            {
                DWORD dwpos = GetMessagePos();
                TVHITTESTINFO ht = { 0 };
                ht.pt.x = GET_X_LPARAM( dwpos );
                ht.pt.y = GET_Y_LPARAM( dwpos );
                MapWindowPoints( HWND_DESKTOP, m_list, &ht.pt, 1 );

                TreeView_HitTest( m_list, &ht );

                if( ht.flags & TVHT_ONITEM )
                {
                    TVITEM tvi;
                    tvi.mask        = TVIF_PARAM;
                    tvi.hItem       = ht.hItem;
                    if( TreeView_GetItem( m_list, &tvi ) )
                    {
                        DiaryElement* elem = Diary::d->get_element( tvi.lParam );
                        m_flag_filter = false;
                        if( elem && elem->get_type() == DiaryElement::ET_TAG )
                            SetWindowText( m_edit, convert_utf8_to_16( elem->get_name() ) );
                        m_flag_filter = true;
                    }
                }
            }
            return FALSE;
        default:
            return FALSE;
    }
}

void
DialogTags::handle_entry_changed()
{
    wchar_t filter[ 256 ];
    GetWindowText( m_edit, filter, 256 );
    m_nav = NameAndValue::parse( convert_utf16_to_8( filter ) );

    EnableWindow( m_button_theme, false );

    if( ! filter[ 0 ] )
    {
        EnableWindow( m_button_action, false );
        m_tag_operation_cur = TO_NONE;
        m_tag_cur = NULL;

        EnableWindow( m_button_filter_set, false );
    }
    else
    {
        EnableWindow( m_button_action, true );
        m_tag_cur = Diary::d->get_tags()->get_tag( m_nav.name );
        if( m_tag_cur == NULL )
        {
            SetWindowText( m_button_action, L"Create Tag" );
            if( m_nav.value == 1 )
                m_tag_operation_cur = TO_CREATE_BOOLEAN;
            else
                m_tag_operation_cur = TO_CREATE_CUMULATIVE;

            EnableWindow( m_button_filter_set, false );
        }
        else
        {
            if( m_tag_cur->is_boolean() && m_nav.value != 1 )
            {
                SetWindowText( m_button_action, L"Invalid Expression" );
                m_tag_operation_cur = TO_INVALID;
            }
            else if( m_ptr2entry && m_ptr2entry->get_tags().check_for_member( m_tag_cur ) )
            {

                if( m_tag_cur->get_has_own_theme() && m_ptr2entry->get_theme_tag() != m_tag_cur )
                    EnableWindow( m_button_theme, true );

                if( m_tag_cur->get_value( m_ptr2entry ) != m_nav.value )
                {
                    SetWindowText( m_button_action, L"Change Value" );
                    m_tag_operation_cur = TO_CHANGE_VALUE;
                }
                else
                {
                    SetWindowText( m_button_action, L"Remove Tag" );
                    m_tag_operation_cur = TO_REMOVE;
                }
            }
            else
            {
                SetWindowText( m_button_action, L"Add Tag" );
                m_tag_operation_cur = TO_ADD;
            }

            EnableWindow( m_button_filter_set, true );
        }
    }
    
    if( m_flag_filter )
        update_list();
}

HTREEITEM
DialogTags::add_list_elem( DiaryElement* elem, HTREEITEM hti_root, bool flag_bold )
{
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    HTREEITEM hPrev = ( HTREEITEM ) TVI_FIRST;

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvi.pszText = HELPERS::convert_utf8_to_16( elem->get_list_str() );
    tvi.cchTextMax = sizeof( tvi.pszText ) / sizeof( tvi.pszText[ 0 ] );
    tvi.iImage = tvi.iSelectedImage = elem->get_icon();
    tvi.lParam = ( LPARAM ) elem->get_id();

    if( flag_bold )
    {
        tvi.mask |= TVIF_STATE;
        tvi.state = TVIS_BOLD;
    }

    tvins.item = tvi;
    tvins.hInsertAfter = hPrev;
    tvins.hParent = hti_root;

    elem->m_list_data = ( HTREEITEM ) SendMessage( m_list,
                                                   TVM_INSERTITEM,
                                                   0,
                                                   ( LPARAM ) ( LPTVINSERTSTRUCT ) &tvins );

    return elem->m_list_data;
}

void
DialogTags::update_list()
{
    SendMessage( m_list, TVM_DELETEITEM, 0, ( LPARAM ) NULL );

    // ROOT TAGS
    for( auto& kv_tag : *Diary::d->get_tags() )
    {
        Tag* tag( kv_tag.second );
        if( tag->get_category() == NULL )
        {
            if( !m_nav.name.empty() && tag->get_name().find( m_nav.name ) == Ustring::npos )
                continue;

            add_list_elem( tag, ( HTREEITEM ) TVI_ROOT );
        }
    }

    // CATEGORIES
    for( auto& kv_category : *Diary::d->get_tag_categories() )
    {
        CategoryTags* category = kv_category.second;
        HTREEITEM hti_ctg;
        bool ctg_added( false );

        for( Tag* tag : *category )
        {
            if( !m_nav.name.empty() && tag->get_name().find( m_nav.name ) == Ustring::npos )
                continue;

            if( !ctg_added )
            {
                hti_ctg = add_list_elem( category, ( HTREEITEM ) TVI_ROOT, true );
                ctg_added = true;
            }

            add_list_elem( tag, hti_ctg );
        }

        if( category->get_expanded() )
            TreeView_Expand( m_list, hti_ctg, TVE_EXPAND );
    }

    // UNTAGGED PSEUDO TAG
    if( m_nav.name.empty() )
        add_list_elem( Diary::d->get_untagged(), ( HTREEITEM ) TVI_ROOT );
}

