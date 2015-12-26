/***********************************************************************************

    Copyright (C) 2014-2015 Ahmet �zt�rk (aoz_2@yahoo.com)

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

#include <cmath>
#include <cassert>

#include "lifeograph.hpp"
#include "win_app_window.hpp"
#include "win_views.hpp"
#include "win_richedit.hpp"
#include "strings.hpp"


using namespace LIFEO;


// TEXTBUFFERDIARY =================================================================================
RichEdit::RichEdit()
:   parser_open_tag_is_link( false ),
    m_flag_settextoperation( false ), m_flag_ongoing_operation( 0 ), m_flag_parsing( false ),
    m_ptr2entry( NULL )
{
    // FORMATS
    memset( &m_format_default, 0, sizeof( CHARFORMAT2 ) );
    m_format_default.cbSize = sizeof( CHARFORMAT2 );
    m_format_default.dwMask = CFM_COLOR | CFM_SIZE | CFM_BOLD | CFM_ITALIC | CFM_STRIKEOUT |
                              CFM_UNDERLINE | CFM_SUPERSCRIPT | CFM_BACKCOLOR;
    m_format_heading.dwEffects = 0;
    m_format_default.yHeight = 200;
    
    memset( &m_format_heading, 0, sizeof( CHARFORMAT2 ) );
    m_format_heading.cbSize = sizeof( CHARFORMAT2 );
    m_format_heading.dwMask = CFM_COLOR | CFM_BOLD | CFM_SIZE;
    m_format_heading.dwEffects = CFE_BOLD;
    m_format_heading.yHeight = 400;
    
    memset( &m_format_subheading, 0, sizeof( CHARFORMAT2 ) );
    m_format_subheading.cbSize = sizeof( CHARFORMAT2 );
    m_format_subheading.dwMask = CFM_COLOR | CFM_BOLD | CFM_SIZE;
    m_format_subheading.dwEffects = CFE_BOLD;
    m_format_subheading.yHeight = 300;

    memset( &m_format_match, 0, sizeof( CHARFORMAT2 ) );
    m_format_match.cbSize = sizeof( CHARFORMAT2 );
    m_format_match.dwMask = CFM_COLOR | CFM_BACKCOLOR;

    memset( &m_format_markup, 0, sizeof( CHARFORMAT2 ) );
    m_format_markup.cbSize = sizeof( CHARFORMAT2 );
    m_format_markup.dwMask = CFM_COLOR | CFM_SIZE;
    m_format_markup.yHeight = 150;

    memset( &m_format_markup_link, 0, sizeof( CHARFORMAT2 ) );
    m_format_markup_link.cbSize = sizeof( CHARFORMAT2 );
    m_format_markup_link.dwMask = CFM_COLOR;

    memset( &m_format_bold, 0, sizeof( CHARFORMAT2 ) );
    m_format_bold.cbSize = sizeof( CHARFORMAT2 );
    m_format_bold.dwMask = CFM_BOLD;
    m_format_bold.dwEffects = CFE_BOLD;

    memset( &m_format_italic, 0, sizeof( CHARFORMAT2 ) );
    m_format_italic.cbSize = sizeof( CHARFORMAT2 );
    m_format_italic.dwMask = CFM_ITALIC;
    m_format_italic.dwEffects = CFE_ITALIC;

    memset( &m_format_highlight, 0, sizeof( CHARFORMAT2 ) );
    m_format_highlight.cbSize = sizeof( CHARFORMAT2 );
    m_format_highlight.dwMask = CFM_BACKCOLOR;

    memset( &m_format_strikethrough, 0, sizeof( CHARFORMAT2 ) );
    m_format_strikethrough.cbSize = sizeof( CHARFORMAT2 );
    m_format_strikethrough.dwMask = CFM_STRIKEOUT;
    m_format_strikethrough.dwEffects = CFE_STRIKEOUT;

    memset( &m_format_comment, 0, sizeof( CHARFORMAT2 ) );
    m_format_comment.cbSize = sizeof( CHARFORMAT2 );
    m_format_comment.dwMask = CFM_COLOR|CFM_SUPERSCRIPT;
    m_format_comment.dwEffects = CFE_SUPERSCRIPT;
    
    memset( &m_format_region, 0, sizeof( CHARFORMAT2 ) );
    m_format_region.cbSize = sizeof( CHARFORMAT2 );
    m_format_region.dwMask = CFM_BACKCOLOR;

    memset( &m_format_link, 0, sizeof( CHARFORMAT2 ) );
    m_format_link.cbSize = sizeof( CHARFORMAT2 );
    m_format_link.dwMask = CFM_UNDERLINE|CFM_COLOR;
    m_format_link.dwEffects = CFE_UNDERLINE;

    memset( &m_format_link_broken, 0, sizeof( CHARFORMAT2 ) );
    m_format_link_broken.cbSize = sizeof( CHARFORMAT2 );
    m_format_link_broken.dwMask = CFM_UNDERLINE|CFM_COLOR;
    m_format_link_broken.dwEffects = CFE_UNDERLINE;

    memset( &m_format_done, 0, sizeof( CHARFORMAT2 ) );
    m_format_done.cbSize = sizeof( CHARFORMAT2 );
    m_format_done.dwMask = CFM_BACKCOLOR;

    memset( &m_format_checkbox_todo, 0, sizeof( CHARFORMAT2 ) );
    m_format_checkbox_todo.cbSize = sizeof( CHARFORMAT2 );
    m_format_checkbox_todo.dwMask = CFM_COLOR | CFM_BACKCOLOR | CFM_BOLD;
    m_format_checkbox_todo.dwEffects = CFM_BOLD;
    const COLORREF color_todo( parse_color( Theme::s_color_todo ) );
    m_format_checkbox_todo.crTextColor = color_todo;

    memset( &m_format_checkbox_progressed, 0, sizeof( CHARFORMAT2 ) );
    m_format_checkbox_progressed.cbSize = sizeof( CHARFORMAT2 );
    m_format_checkbox_progressed.dwMask = CFM_COLOR | CFM_BACKCOLOR | CFM_BOLD;
    m_format_checkbox_progressed.dwEffects = CFM_BOLD;
    const COLORREF color_progressed( parse_color( Theme::s_color_progressed ) );
    m_format_checkbox_progressed.crTextColor = color_progressed;

    memset( &m_format_checkbox_done, 0, sizeof( CHARFORMAT2 ) );
    m_format_checkbox_done.cbSize = sizeof( CHARFORMAT2 );
    m_format_checkbox_done.dwMask = CFM_COLOR | CFM_BACKCOLOR | CFM_BOLD;
    m_format_checkbox_done.dwEffects = CFM_BOLD;
    const COLORREF color_done( parse_color( Theme::s_color_done ) );
    m_format_checkbox_done.crTextColor = color_done;

    memset( &m_format_checkbox_canceled, 0, sizeof( CHARFORMAT2 ) );
    m_format_checkbox_canceled.cbSize = sizeof( CHARFORMAT2 );
    m_format_checkbox_canceled.dwMask = CFM_COLOR | CFM_BACKCOLOR | CFM_BOLD;
    m_format_checkbox_canceled.dwEffects = CFM_BOLD;
    const COLORREF color_canceled( parse_color( Theme::s_color_canceled ) );
    m_format_checkbox_canceled.crTextColor = color_canceled;
}

void
RichEdit::handle_login()
{

}

void
RichEdit::handle_logout()
{
    EntryParser::set_search_str( L"" );
    m_ptr2entry = NULL;
}

void
RichEdit::set_search_str( const Wstring& str )
{
    EntryParser::set_search_str( str );
    //place_cursor( begin() );
    reparse();
}

bool
RichEdit::select_searchstr_previous()
{
    return false;
}

bool
RichEdit::select_searchstr_next()
{
    return false;
}

void
RichEdit::parse( LONG start, LONG end )
{
    m_flag_parsing = true;

    // TWEAKS TO SPEED UP
    SendMessage( m_hwnd, EM_SETEVENTMASK, 0, 0 );
    SendMessage( m_hwnd, WM_SETREDRAW, false, 0 );
    
    // STORE THE SELECTION
    CHARRANGE sel;
    SendMessage( m_hwnd, EM_EXGETSEL, 0, ( LPARAM ) &sel );
    
    // CLEAR EXISTING FORMATTING AND PARSE
    remove_all_formats( start, end );
    EntryParser::parse( start, end );
    
    // RESTORE SELECTION
    SendMessage( m_hwnd, EM_EXSETSEL, 0, ( LPARAM ) &sel );
    
    // REVERT SPEED UP TWEAKS
    SendMessage( m_hwnd, WM_SETREDRAW, true, 0 );
    InvalidateRect( m_hwnd, 0, true );
    SendMessage( m_hwnd, EM_SETEVENTMASK, 0, ( LPARAM ) ENM_CHANGE );
    
    m_flag_parsing = false;
}

void
RichEdit::reparse()
{
    m_flag_settextoperation = true;
    parse( 0, get_length() );
    m_flag_settextoperation = false;
}

Wchar
RichEdit::get_char_at( int i )
{
    return get_text()[ i ];
}

inline COLORREF
RichEdit::midcontrast( const Theme* theme, const COLORREF color_target )
{
    COLORREF color_lt = midtone( parse_color( "#FFFFFF" ), color_target, 0.8 );
    return midtone( color_lt, parse_color( theme->color_base ), 0.85 );
}

// WINDOWS SPECIFIC HELPERS
void
RichEdit::apply_format( CHARFORMAT2& format, LONG start, LONG end )
{
    CHARRANGE range = { start, end };
    SendMessage( m_hwnd, EM_EXSETSEL, 0, ( LPARAM ) &range );
    SendMessage( m_hwnd, EM_SETCHARFORMAT, SCF_SELECTION, ( LPARAM ) &format );
}

void
RichEdit::remove_all_formats( LONG start, LONG end )
{
    SendMessage( m_hwnd, EM_SETCHARFORMAT, SCF_ALL, ( LPARAM ) &m_format_default );
}

LONG
RichEdit::get_length()
{
    GETTEXTLENGTHEX gtl;
    gtl.flags = GTL_PRECISE | GTL_NUMCHARS;
    gtl.codepage = 1200;
    
    LONG ret = SendMessage( m_hwnd, EM_GETTEXTLENGTHEX, ( WPARAM ) &gtl, 0 );
    return ret;
}

Wstring
RichEdit::get_text()
{
    LONG length = get_length() + 1;
    Wchar* str = new Wchar[ length ];
    
    GETTEXTEX gt;
    gt.cb = length * sizeof( Wchar );
    gt.flags = GT_DEFAULT;
    gt.codepage = 1200;
    gt.lpDefaultChar = NULL;
    gt.lpUsedDefChar = NULL;
    
    SendMessage( m_hwnd, EM_GETTEXTEX, ( WPARAM ) &gt, ( LPARAM ) str );
    
    Wstring wstr( str );
    
    delete str;
    
    return wstr;
}

// PARSING APPLIERS
void
RichEdit::apply_heading()
{
    LONG end( 0 );
    Wstring text( get_text() );
    
    if( !text.empty() )
    {
        if( text[ 0 ] != L'\r' )
            end = text.find_first_of( L'\r' );
        if( end == text.npos )
            end = get_length();

        apply_format( m_format_heading, 0, end );
    }

    if( ! m_flag_settextoperation )
    {
        m_ptr2entry->set_name( end > 0 ?
                convert_utf16_to_8( text.substr( 0, end ).c_str() ) :
                text.empty() ? STRING::EMPTY_ENTRY_TITLE : "" );

        WinAppWindow::p->update_list_elem( m_ptr2entry );
        //AppWindow::p->panel_main->refresh_title();
    }
}

void
RichEdit::apply_subheading()
{
    Wstring text( get_text() );
    LONG end = text.find_first_of( L'\r', m_pos_start );
    
    if( end == text.npos )
        end = get_length();
    
    apply_format( m_format_subheading, m_pos_start, end );
}

void
RichEdit::apply_bold()
{
    apply_markup( m_format_bold );
}

void
RichEdit::apply_italic()
{
    apply_markup( m_format_italic );
}

void
RichEdit::apply_strikethrough()
{
    apply_markup( m_format_strikethrough );
}

void
RichEdit::apply_highlight()
{
    apply_markup( m_format_highlight );
}

void
RichEdit::apply_comment()
{
    apply_format( m_format_comment, m_pos_start, pos_current + 1 );
}

void
RichEdit::apply_ignore()
{
    // TODO
    /*Gtk::TextIter iter_start( get_iter_at_offset( m_pos_start ) );
    Gtk::TextIter iter_end( iter_start );
    iter_end.forward_to_line_end();
    apply_format( m_format_region, iter_start, iter_end );*/
}

/*
void
RichEdit::apply_link()
{
    // HANDLE RELATIVE PATHS
    if( word_last.find( "rel://" ) == 0 )
    {
        word_last.replace( 0, 5, "file://" + Glib::path_get_dirname( Diary::d->get_path() ) );
    }

    Gtk::TextIter iter_start( get_iter_at_offset( m_pos_start ) );
    Gtk::TextIter iter_current( get_iter_at_offset( pos_current ) );

    if( m_flag_hidden_link )
    {
        Gtk::TextIter iter_end( get_iter_at_offset( pos_current + 1 ) );
        Gtk::TextIter iter_url_start( get_iter_at_offset( m_pos_start + 1 ) );
        Gtk::TextIter iter_tab( get_iter_at_offset( pos_tab ) );
        m_list_links.push_back( new LinkUri( create_mark( iter_tab ),
                                             create_mark( iter_current ),
                                             get_slice( iter_url_start, iter_tab ) ) );

        apply_hidden_link_tags( iter_end, m_format_link );
    }
    else
    {
        PRINT_DEBUG( "url: " + word_last );

        if( word_last.find( "file:///" ) == 0  &&
            // there can be no text on the same line as the image:
            iter_start.starts_line() && iter_current.ends_line() )
        {
            try
            {
                Glib::RefPtr< Gdk::Pixbuf > buf(
                        Lifeograph::get_thumbnail( Glib::filename_from_uri( word_last ),
                                                  m_max_thumbnail_width ) );

                apply_tag( m_format_hidden, iter_start, iter_current );
                remove_tag( m_format_misspelled, iter_start, iter_current );

                m_list_images.insert( new InlineImage( create_mark( iter_current ) ) );
                insert_pixbuf( iter_current, buf );

                pos_current++;
                m_pos_end++;

                iter_start = get_iter_at_offset( m_pos_start );
                iter_current = get_iter_at_offset( pos_current );

                apply_tag( m_format_image, iter_start, iter_current );
            }
            catch( Glib::FileError &er )
            {
                print_error( "Link target not found" );
                apply_tag( m_format_link, iter_start, iter_current );
                remove_tag( m_format_misspelled, iter_start, iter_current );
            }
            catch( Gdk::PixbufError &er )
            {
                PRINT_DEBUG( "Link is not an image" );
                apply_tag( m_format_link, iter_start, iter_current );
                remove_tag( m_format_misspelled, iter_start, iter_current );
            }
        }
        else
        {
            apply_tag( m_format_link, iter_start, iter_current );
            remove_tag( m_format_misspelled, iter_start, iter_current );
        }

        m_list_links.push_back( new LinkUri( create_mark( iter_start ),
                                             create_mark( iter_current ),
                                             word_last ) );
    }
}

void
TextbufferDiary::apply_link_id()
{
    DiaryElement *element( Diary::d->get_element( id_last ) );

    if( element != NULL )
    {
        if( element->get_type() == DiaryElement::ET_ENTRY )
        {
            Gtk::TextIter iter_tab( get_iter_at_offset( pos_tab ) );
            Gtk::TextIter iter_current( get_iter_at_offset( pos_current ) );
            Gtk::TextIter iter_end( get_iter_at_offset( pos_current + 1 ) );

            m_list_links.push_back( new LinkID( create_mark( iter_tab ),
                                                create_mark( iter_current ),
                                                id_last ) );

            apply_hidden_link_tags( iter_end, m_format_link );
            return;
        }
    }
    // indicate dead links
    Gtk::TextIter iter_start( get_iter_at_offset( m_pos_start ) );
    Gtk::TextIter iter_end( get_iter_at_offset( pos_current + 1 ) );
    apply_tag( m_format_link_broken, iter_start, iter_end );
}

void
TextbufferDiary::apply_link_date()
{
    LinkStatus status( LS_OK );
    Entry *ptr2entry( Diary::d->get_entry( date_last.m_date + 1 ) ); // + 1 fixes order
    if( ptr2entry == NULL )
        status = LS_ENTRY_UNAVAILABLE;
    else
    if( date_last.get_pure() == m_ptr2entry->get_date().get_pure() )
        status = Diary::d->get_day_has_multiple_entries( date_last ) ? LS_OK : LS_CYCLIC;

    Gtk::TextIter iter_start( get_iter_at_offset( m_pos_start ) );
    Gtk::TextIter iter_current( get_iter_at_offset( pos_current ) );
    Gtk::TextIter iter_tab( get_iter_at_offset( pos_tab ) );

    if( status < LS_INVALID )
    {
        Gtk::TextIter iter_end( iter_current );
        iter_end++;

        if( m_flag_hidden_link )
        {
            m_list_links.push_back( new LinkEntry( create_mark( iter_tab ),
                                                   create_mark( iter_current ),
                                                   date_last ) );
            apply_hidden_link_tags( iter_end,
                    status == LS_OK ? m_format_link : m_format_link_broken );
        }
        else
        {
            m_list_links.push_back(
                    new LinkEntry( create_mark( iter_start ),
                                   create_mark( iter_end ),
                                   date_last ) );

            apply_tag( status == LS_OK ? m_format_link :
                            m_format_link_broken, iter_start, iter_end );
        }
    }
}*/

void
RichEdit::apply_check( CHARFORMAT2& format_box, CHARFORMAT2* tag, int c )
{
    Wstring text( get_text() );
    LONG iter_start = pos_current - 3;
    LONG iter_box = pos_current;
    LONG iter_end = text.find_first_of( L'\r', iter_box );
    if( iter_end == text.npos )
        iter_end = get_length();

    /*if( ! Diary::d->is_read_only() )
        m_list_links.push_back( new LinkCheck( create_mark( iter_start ),
                                               create_mark( iter_box ),
                                               c ) );*/

    apply_format( format_box, iter_start, iter_box );
    if( tag )
        apply_format( *tag, ++iter_box, iter_end ); // ++ to skip separating space char
}

void
RichEdit::apply_check_unf()
{
    apply_check( m_format_checkbox_todo, &m_format_bold, 0 );
}

void
RichEdit::apply_check_prg()
{
    apply_check( m_format_checkbox_progressed, NULL, 1 );
}

void
RichEdit::apply_check_fin()
{
    apply_check( m_format_checkbox_done, &m_format_done, 2 );
}

void
RichEdit::apply_check_ccl()
{
    apply_check( m_format_checkbox_canceled, &m_format_strikethrough, 3 );
}

void
RichEdit::apply_match()
{
    apply_format( m_format_match, pos_search, pos_current + 1 );
}

inline void
RichEdit::apply_markup( CHARFORMAT2& format )
{
    apply_format( m_format_markup, m_pos_start, m_pos_start + 1 );
    //apply_format( m_format_hidden, m_pos_start, m_pos_start + 1 );

    apply_format( format, m_pos_start + 1, pos_current );

    apply_format( m_format_markup, pos_current, pos_current + 1 );
    //apply_format( m_format_hidden, pos_current, pos_current + 1 );
}

// EVENT HANDLERS
void
RichEdit::handle_change()
{
    if( m_flag_ongoing_operation || m_flag_parsing || !m_ptr2entry )
        return;

    parse( 0, get_length() );

    WinAppWindow::p->m_entry_view->handle_text_change();
}

bool
RichEdit::handle_new_line()
{
    Wstring fulltext( get_text() );
    CHARRANGE cr;
    SendMessage( m_hwnd, EM_EXGETSEL, 0, ( LPARAM ) &cr );
    LONG iter_end = cr.cpMin;
    if( iter_end <= 1 )
        return false;

    LONG iter_start( fulltext.find_last_of( L'\r', iter_end - 1 ) );
    if( iter_start == Wstring::npos /*||
        iter_end.get_line_offset() < 3    NOT EASY in WIN32*/ )
        return false;

    iter_start++;   // get rid of the new line char
    const Wstring::size_type offset_start( iter_start );   // save for future

    if( fulltext[ iter_start ] == L'\t' )
    {
        Wstring text( L"\r\t" );
        int value = 0;
        char char_lf = '*';
        iter_start++;   // first tab is already handled, so skip it

        for( ; iter_start != iter_end; ++iter_start )
        {
            switch( fulltext[ iter_start ] )
            {
                // BULLETED LIST
                case 0x2022:
                    if( char_lf != '*' )
                        return false;
                    char_lf = ' ';
                    text += L"\u2022 ";
                    break;
                // CHECK LIST
                case L'[':
                    if( char_lf != '*' )
                        return false;
                    char_lf = 'c';
                    break;
                case L'~':
                case L'+':
                case L'x':
                case L'X':
                    if( char_lf != 'c' )
                        return false;
                    char_lf = ']';
                    break;
                case L']':
                    if( char_lf != ']' )
                        return false;
                    char_lf = ' ';
                    text += L"[ ] ";
                    break;
                // NUMBERED LIST
                case L'0': case L'1': case L'2': case L'3': case L'4':
                case L'5': case L'6': case L'7': case L'8': case L'9':
                    if( char_lf != '*' && char_lf != '1' )
                        return false;
                    char_lf = '1';
                    value *= 10;
                    value += fulltext[ iter_start ] - L'0';
                    break;
                case L'-':
                    if( char_lf == '*' )
                    {
                        char_lf = ' ';
                        text += L"- ";
                        break;
                    }
                    // no break
                case L'.':
                case L')':
                    if( char_lf != '1' )
                        return false;
                    char_lf = ' ';
                    text += std::to_wstring( ++value );
                    text += fulltext[ iter_start ];
                    text += L" ";
                    break;
                case L'\t':
                    if( char_lf != '*' )
                        return false;
                    text += L'\t';
                    break;
                case L' ':
                    if( char_lf == 'c' )
                    {
                        char_lf = ']';
                        break;
                    }
                    else if( char_lf != ' ' )
                        return false;
                    // remove the last bullet if no text follows it:
                    if( iter_start == iter_end - 1 )
                    {
                        iter_start = offset_start;
                        m_flag_ongoing_operation++;
                        CHARRANGE range = { iter_start, iter_end };
                        SendMessage( m_hwnd, EM_EXSETSEL, 0, ( LPARAM ) &range );
                        SendMessage( m_hwnd, EM_REPLACESEL, ( WPARAM ) TRUE, ( LPARAM ) L"\r" );
                        m_flag_ongoing_operation--;
                        return true;
                    }
                    else
                    {
                        SendMessage( m_hwnd, EM_REPLACESEL, ( WPARAM ) TRUE,
                                     ( LPARAM ) text.c_str() );
                        /*iter_start += text.size();
                        if( value > 0 )
                        {
                            iter_start++;
                            while( increment_numbered_line( iter_start, value++ ) )
                            {
                                iter_start.forward_to_line_end();
                                iter_start++;
                            }
                        }*/
                        return true;
                    }
                    break;
                default:
                    return false;
            }
        }
    }
    return false;
}
/*
void
RichEdit::on_erase( const Gtk::TextIter& iter_start,
                    const Gtk::TextIter& iter_end )
{
    const Glib::ustring::size_type offset_itr = iter_start.get_offset();

    if( UndoManager::m->is_freezed() )
    {
        Gtk::TextIter iter_scroll( iter_start );    // to remove constness
        m_ptr2textview->scroll_to( iter_scroll );
    }
    else
    {
        UndoErase * undo_erase = new UndoErase( offset_itr, get_slice( iter_start, iter_end ) );
        UndoManager::m->add_action( undo_erase );
    }

    Gtk::TextBuffer::on_erase( iter_start, iter_end );

    // set_text() calls on_erase too:
    if( m_flag_settextoperation || m_flag_ongoing_operation || m_flag_parsing )
        return;

    parser_open_tag_begin = parser_open_tag_end = std::string::npos;

    //  CALCULATING LIMITS & PARSING
    Gtk::TextIter iter_start2( get_iter_at_offset( offset_itr ) );
    Gtk::TextIter iter_end2( iter_start2 );
    calculate_para_bounds( iter_start2, iter_end2 );
    parse( iter_start2, iter_end2 );
}*/

/*void
TextbufferDiary::on_apply_tag( const Glib::RefPtr< TextBuffer::Tag >& tag,
                               const Gtk::TextIter& iter_begin,
                               const Gtk::TextIter& iter_end )
{
    // do not check spelling of links:
    if( ( iter_begin.has_tag( m_format_link ) || iter_begin.has_tag( m_format_hidden ) ) &&
        tag == m_format_misspelled )
        return;
    else
        Gtk::TextBuffer::on_apply_tag( tag, iter_begin, iter_end );
}*/

/*
Link*
RichEdit::get_link( int offset ) const
{
    for ( ListLinks::const_iterator iter = m_list_links.begin();
          iter != m_list_links.end();
          ++iter )
    {
        if( offset >= ( *iter )->m_mark_start->get_iter().get_offset() &&
            offset <= ( *iter )->m_mark_end->get_iter().get_offset() )
            return( *iter );
    }

    return NULL;
}

void
RichEdit::clear_links( int start, int end )
{
    ListLinks::iterator iter_tmp;
    for( ListLinks::iterator iter = m_list_links.begin(); iter != m_list_links.end(); )
        if( start <= ( *iter )->m_mark_start->get_iter().get_offset() &&
            end >= ( *iter )->m_mark_end->get_iter().get_offset() )
        {
            iter_tmp = iter;
            ++iter;
            delete( *iter_tmp );
            m_list_links.erase( iter_tmp );
        }
        else
            ++iter;
}

void
RichEdit::clear_links()
{
    for( ListLinks::iterator iter = m_list_links.begin();
         iter != m_list_links.end();
         ++iter )
    {
        delete( *iter );
    }
    m_list_links.clear();
}*/

// FORMATTING
/*
void
RichEdit::toggle_format( Glib::RefPtr< Tag > tag, const Glib::ustring& markup )
{
    if( ! m_ptr2textview->has_focus() )
        return;

    Gtk::TextIter iter_start, iter_end;
    if( get_has_selection() )
    {
        int start( -2 ), end( -1 );
        bool properly_separated( false );

        get_selection_bounds( iter_start, iter_end );
        int p_start = iter_start.get_offset();
        int p_end = iter_end.get_offset() - 1;

        Gtk::TextIter firstnl = begin();
        if( ! firstnl.forward_find_char( s_predicate_nl ) ) // there is only heading
            return;
        int p_first_nl = firstnl.get_offset();

        if( p_end <= p_first_nl )
            return;
        else if( p_start > p_first_nl )
            p_start--;   // also evaluate the previous character
        else // p_start <= p_first_nl
        {
            p_start = p_first_nl + 1;
            properly_separated = true;
            start = -1;
        }

        for( ; ; p_start++ )
        {
            if( get_iter_at_offset( p_start ).has_tag( m_format_bold ) ||
                get_iter_at_offset( p_start ).has_tag( m_format_italic ) ||
                get_iter_at_offset( p_start ).has_tag( m_format_highlight ) ||
                get_iter_at_offset( p_start ).has_tag( m_format_strikethrough ) )
                return;
            switch( get_iter_at_offset( p_start ).get_char() )
            {
                case '\n': // selection spreads over more than one line
                    if( start >= 0 )
                    {
                        if( properly_separated )
                        {
                            insert( get_iter_at_offset( start ), markup );
                            end += 2;
                            p_start += 2;
                            p_end += 2;
                        }
                        else
                        {
                            insert( get_iter_at_offset( start ), " " + markup );
                            end += 3;
                            p_start += 3;
                            p_end += 3;
                        }

                        insert( get_iter_at_offset( end ), markup );

                        properly_separated = true;
                        start = -1;
                        break;
                    }
                    // else no break
                case ' ':
                case '\t':
                    if( start == -2 )
                    {
                        properly_separated = true;
                        start = -1;
                    }
                    break;
                default: // non-space
                    if( start == -2 )
                        start = -1;
                    else
                    if( start == -1 )
                        start = p_start;
                    end = p_start;
                    break;
            }
            if( p_start == p_end )
                break;
        }
        // add markup chars to the beginning and end:
        if( start >= 0 )
        {
            if( properly_separated )
            {
                insert( get_iter_at_offset( start ), markup );
                end += 2;
            }
            else
            {
                insert( get_iter_at_offset( start ), " " + markup );
                end += 3;
            }

            insert( get_iter_at_offset( end ), markup );
            place_cursor( get_iter_at_offset( end ) );
        }
    }
    else    // no selection case
    {
        Glib::RefPtr< Gtk::TextMark > mark = get_insert();
        iter_start = mark->get_iter();
        if( Glib::Unicode::isspace( iter_start.get_char() ) || iter_start.is_end() )
        {
            if( iter_start.starts_line() )
                return;
            iter_start--;
            if( iter_start.has_tag( TextbufferDiary::m_format_markup ) )
                iter_start--;
        }
        else if( iter_start.has_tag( TextbufferDiary::m_format_markup ) )
        {
            if( iter_start.starts_line() )
                return;
            iter_start--;
            if( Glib::Unicode::isspace( iter_start.get_char() ) )
            {
                iter_start.forward_chars( 2 );
            }
        }
        if( iter_start.has_tag( tag ) ) // if already has the tag, remove it
        {
            m_flag_ongoing_operation++;

            // necessary when cursor is between a space char and non-space char:
            if( iter_start.starts_word() )
                iter_start++;

            int offset_start( iter_start.get_line_offset() );

            iter_start.backward_to_tag_toggle( tag );
            backspace( iter_start );

            iter_end = get_iter_at_offset( offset_start );
            iter_end.forward_to_tag_toggle( TextbufferDiary::m_format_markup );

            m_flag_ongoing_operation--;

            backspace( ++iter_end );
        }
        else
        if( iter_start.has_tag( m_format_bold ) == false &&
            iter_start.has_tag( m_format_italic ) == false &&
            iter_start.has_tag( m_format_strikethrough ) == false &&
            iter_start.has_tag( m_format_highlight ) == false &&
            iter_start.has_tag( m_format_heading ) == false &&
            iter_start.has_tag( m_format_link ) == false ) // formatting tags cannot be nested
        {
            // find word boundaries:
            if( !( iter_start.starts_word() || iter_start.starts_line() ) )
                iter_start.backward_word_start();
            insert( iter_start, markup );

            iter_end = mark->get_iter();
            if( !( iter_end.ends_word() || iter_end.ends_line() ) )
            {
                iter_end.forward_word_end();
                insert( iter_end, markup );
            }
            else
            {
                int offset = iter_end.get_offset();
                insert( iter_end, markup );
                place_cursor( get_iter_at_offset( offset ) );
            }
        }
    }
}

void
RichEdit::toggle_bold()
{
    toggle_format( m_format_bold, "*" );
}

void
RichEdit::toggle_italic()
{
    toggle_format( m_format_italic, "_" );
}

void
RichEdit::toggle_strikethrough()
{
    toggle_format( m_format_strikethrough, "=" );
}

void
RichEdit::toggle_highlight()
{
    toggle_format( m_format_highlight, "#" );
}

void
RichEdit::set_comment_visibility( bool visible )
{
    m_format_comment->property_invisible() = visible;
}

void
RichEdit::calculate_para_bounds( LONG& iter_begin, LONG& iter_end )
{
    Wstring text( get_text() );
    
    text.find_first_of( '\r',  )
    if( ! iter_begin.backward_find_char( s_predicate_nl ) )
        iter_begin.set_offset( 0 );

    if( iter_end.ends_line() )
        iter_end++;
    else if( iter_end.forward_find_char( s_predicate_nl ) )
        iter_end++;
    else
        iter_end.forward_to_end();
}*/

/*bool
RichEdit::calculate_multiline_selection_bounds( LONG& iter_begin,
                                                       LONG& iter_end )
{
    
    bool flag_starts_first_para( false );

    if( get_has_selection() )
        get_selection_bounds( iter_begin, iter_end );
    else
        iter_begin = iter_end = get_iter_at_mark( get_insert() );

    if( ! iter_begin.backward_find_char( s_predicate_nl ) )
    {
        if( ! iter_begin.forward_find_char( s_predicate_nl ) )
            return false;
        else
            flag_starts_first_para = true;
    }
    else
        iter_begin++;

    if( iter_end.get_char() != '\n' )
    {
        if( ! iter_end.forward_find_char( s_predicate_nl ) )
        {
            if( flag_starts_first_para )
                return false; // ignore first line
            iter_end.forward_to_end();
        }
    }

    return( !( iter_begin == iter_end && flag_starts_first_para ) );
    
}*/

/*
void
TextbufferDiary::handle_indent()
{
    if( ! m_ptr2textview->has_focus() )
        return;

    Gtk::TextIter iter, iter_end;
    calculate_multiline_selection_bounds( iter, iter_end );

    if ( iter == iter_end ) // empty line
    {
        insert( iter, "\t" );
        return;
    }

    Glib::RefPtr< Gtk::TextMark > mark_end = create_mark( iter_end );

    iter = insert( iter, "\t" );    // first line

    while( iter != get_iter_at_mark( mark_end ) )
    {
        if( iter.get_char() == '\n' )
        {
            ++iter;
            iter = insert( iter, "\t" );
        }
        else
            ++iter;
    }

    delete_mark( mark_end );
}

void
TextbufferDiary::handle_unindent()
{
    if( ! m_ptr2textview->has_focus() )
        return;

    Gtk::TextIter iter, iter_end;
    calculate_multiline_selection_bounds( iter, iter_end );

    if ( iter == iter_end ) // empty line
    {
        if( iter.get_char() == '\t' )
            erase( iter, ++iter_end );

        return;
    }

    Glib::RefPtr< Gtk::TextMark > mark_end = create_mark( iter_end );

    if( iter.get_char() == '\t' )
    {
        iter_end = iter;
        iter = erase( iter, ++iter_end ); // first line
    }

    while( iter != get_iter_at_mark( mark_end ) )
    {
        if( iter.get_char() == '\n' )
        {
            ++iter;
            if( iter.get_char() == '\t' )
            {
                iter_end = iter;
                iter = erase( iter, ++iter_end );    // first line
            }
        }
        else
            ++iter;
    }
}

void
TextbufferDiary::add_bullet()
{
    Gtk::TextIter iter, iter_end;
    if( ! calculate_multiline_selection_bounds( iter, iter_end ) )
        return;

    if ( iter == iter_end ) // empty line
    {
        insert( iter, "\t• " );
        return;
    }

    Glib::RefPtr< Gtk::TextMark > mark_end = create_mark( iter_end );
    Gtk::TextIter iter_erase_begin( iter );
    char char_lf( 't' );    // tab
    while( iter != get_iter_at_mark( mark_end ) )
    {
        switch( iter.get_char() )
        {
            case L'•':  // remove bullet
                if( char_lf == 'b' )
                    char_lf = 's';  // space
                else
                    char_lf = 'n'; // new line
                break;
            case ' ':
                if( char_lf == 's' || char_lf == 'S' )
                {
                    char_lf += ( 'g' - 's' ); // g or G depending on the previous value
                    iter = erase( iter_erase_begin, ++iter );
                    continue;
                }
                else
                    break;
            case '\n':
                char_lf = 't';  // tab
                break;
            case L'☐':
            case L'☑':
            case L'☒':
                if( char_lf == 'b' )
                    char_lf = 'S';  // capital s: space 2
                else
                    char_lf = 'n'; // new line
                break;
            case '\t':
                if( char_lf == 't'  || char_lf == 'b' )
                {
                    char_lf = 'b';  // bullet
                    iter_erase_begin = iter;
                }
                break;
            case 0: // end
            default:
                if( char_lf == 'G' || char_lf == 't' || char_lf == 'b' )
                    iter = insert( iter, "\t• " );
                char_lf = 'n';
                break;
        }
        ++iter;
    }

    delete_mark( mark_end );
}

void
TextbufferDiary::add_checkbox()
{
    Gtk::TextIter iter, iter_end;
    if( ! calculate_multiline_selection_bounds( iter, iter_end ) )
        return;

    if ( iter == iter_end ) // empty line
    {
        insert( iter, "\t☐ " );
        return;
    }

    Glib::RefPtr< Gtk::TextMark > mark_end = create_mark( iter_end );
    Gtk::TextIter iter_erase_begin( iter );
    char char_lf( 't' );    // tab
    while( iter != get_iter_at_mark( mark_end ) )
    {
        switch( iter.get_char() )
        {
            case L'☐':
            case L'☑':
            case L'☒':  // remove checkbox
                if( char_lf == 'c' )
                    char_lf = 's';  // space
                else
                    char_lf = 'n'; // new line
                break;
            case ' ':
                if( char_lf == 's' || char_lf == 'S' )
                {
                    char_lf += ( 'g' - 's' ); // g or G depending on the previous value
                    iter = erase( iter_erase_begin, ++iter );
                    continue;
                }
                else
                    break;
            case '\n':
                char_lf = 't';  // tab
                break;
            case L'•':
                if( char_lf == 'c' )
                    char_lf = 'S';  // capital s: space 2
                else
                    char_lf = 'n'; // new line
                                    break;
            case '\t':
                if( char_lf == 't'  || char_lf == 'c' )
                {
                    char_lf = 'c';  // bullet
                    iter_erase_begin = iter;
                }
                break;
            case 0: // end
            default:
                if( char_lf == 'G' || char_lf == 't' || char_lf == 'c' )
                    iter = insert( iter, "\t☐ " );
                char_lf = 'n';
                break;
        }
        ++iter;
    }

    delete_mark( mark_end );
}

void
TextbufferDiary::add_empty_line_above()
{
    if( ! m_ptr2textview->has_focus() )
        return;

    Gtk::TextIter iter( get_iter_at_mark( get_insert() ) );
    if( iter.backward_line() )
        iter.forward_line();
    insert( iter, "\n" );
}

void
TextbufferDiary::remove_empty_line_above()
{
    if( ! m_ptr2textview->has_focus() )
        return;

    Gtk::TextIter iter( get_iter_at_mark( get_insert() ) );
    if( iter.backward_line() )
        iter.forward_line();

    if( iter.get_line() < 1 )
        return;

    Gtk::TextIter iter_begin( --iter );
    iter_begin--;
    if( iter_begin.get_char() == '\n' )
        erase( iter_begin, iter );
}

void
TextbufferDiary::move_line_up()
{
    if( ! m_ptr2textview->has_focus() )
        return;

    Gtk::TextIter iter( get_iter_at_mark( get_insert() ) );
    if( iter.get_line() < 2 )
        return;

    if( iter.backward_line() )
    {
        int offset( iter.get_offset() );

        iter.forward_line();
        iter--;

        Gtk::TextIter iter_end( get_iter_at_mark( get_insert() ) );
        if( !iter_end.ends_line() )
            iter_end.forward_to_line_end();

        Glib::ustring text( get_text( iter, iter_end ) );

        // image will vanish by itself:
        if( get_iter_at_offset( iter.get_offset() + 1 ).has_tag( m_format_image ) )
            iter_end--;

        erase( iter, iter_end );

        iter = get_iter_at_offset( offset - 1 );

        insert( iter, text );

        place_cursor( get_iter_at_offset( offset ) );
    }
}

void
TextbufferDiary::move_line_down()
{
    if( ! m_ptr2textview->has_focus() )
        return;

    Gtk::TextIter iter( get_iter_at_mark( get_insert() ) );

    if( iter.backward_line() )
    {
        iter.forward_line();
        iter--;

        Gtk::TextIter iter_end( get_iter_at_mark( get_insert() ) );
        if( !iter_end.ends_line() )
            if( !iter_end.forward_to_line_end() )
                return;

        int offset( iter.get_offset() + 1 );

        Glib::ustring text( get_text( iter, iter_end ) );

        // image will vanish by itself:
        if( get_iter_at_offset( iter.get_offset() + 1 ).has_tag( m_format_image )  )
            iter_end--;

        erase( iter, iter_end );

        iter = get_iter_at_offset( offset );
        if( !iter.ends_line() )
            iter.forward_to_line_end();
        offset = iter.get_offset();

        insert( iter, text );

        place_cursor( get_iter_at_offset( offset + 1 ) );
    }
}*/

void
RichEdit::insert_link( DiaryElement* element )
{
    /*
    // TODO: implement a custom insert function that adds spaces where necessary
    Gtk::TextIter iter( get_iter_at_mark( get_insert() ) );
    if( iter.get_tags().size() > 0 )
        return;
    if( iter.get_offset() > 0 )
        iter--;
    char c( iter.get_char() );
    if( c != ' ' && c != '\n' && c != '\t' )
        insert( get_iter_at_mark( get_insert() ), " ");
    insert( get_iter_at_mark( get_insert() ),
                Glib::ustring::compose( "<deid:%1\t%2>", element->get_id(), element->get_name() ) );*/
}

void
RichEdit::insert_time_stamp()
{
    /*if( ! m_ptr2textview->has_focus() )
        return;

    // TODO: implement a custom insert function that adds spaces where necessary
    Gtk::TextIter iter( get_iter_at_mark( get_insert() ) );
    //if( iter.get_tags().size() > 0 )
        //return;
    if( iter.get_offset() > 0 )
        iter--;
    char c( iter.get_char() );
    if( c != ' ' && c != '\n' && c != '\t' )
        insert( get_iter_at_mark( get_insert() ), " ");

    insert( get_iter_at_mark( get_insert() ), Date::format_string_dt( time( NULL ) ) );*/
}

void
RichEdit::set_richtext( Entry* entry )
{
    if( m_ptr2entry && ! Diary::d->is_read_only() )
        WinAppWindow::p->m_entry_view->sync();

    m_flag_settextoperation = true;
    //clear_links();

    parser_offset_insert_prev = 0;

    m_ptr2entry = entry;
    
    if( entry )
    {
        //set_language( Diary::d->is_read_only() ? "" : entry->get_lang_final() );
        set_theme( entry->get_theme() );

        SetWindowText( m_hwnd, HELPERS::convert_utf8_to_16( entry->get_text() ) );

        //place_cursor( begin() );
        EnableWindow( m_hwnd, true );
    }
    else
    {
        SetWindowText( m_hwnd, L"" );
        EnableWindow( m_hwnd, false );
    }

    m_flag_settextoperation = false;
}

void
RichEdit::set_theme( const Theme* theme )
{
    const COLORREF color_base( parse_color( theme->color_base ) );
    const COLORREF color_text( parse_color( theme->color_text ) );
    const COLORREF color_mid( midtone( color_base, color_text, 0.5 ) );

    m_format_default.crTextColor = color_text;
    m_format_default.crBackColor = color_base; // not elegant but no way for transparent
    m_format_heading.crTextColor = parse_color( theme->color_heading );
    m_format_subheading.crTextColor = parse_color( theme->color_subheading );
    m_format_highlight.crBackColor = parse_color( theme->color_highlight );
    m_format_markup.crTextColor = color_mid;
    m_format_comment.crTextColor = color_mid;

    SendMessage( m_hwnd, EM_SETCHARFORMAT, SCF_ALL, ( LPARAM ) &m_format_default );
    SendMessage( m_hwnd, EM_SETBKGNDCOLOR, 0, ( LPARAM ) color_base );

/*    m_ptr2textview->override_font( theme->font );

    m_format_comment->property_background_rgba() = theme->color_base; // to disable highlighting

    m_format_region->property_paragraph_background_rgba() = midtone(
            theme->color_base, theme->color_text, 0.9 );*/
    m_format_match.crTextColor = color_base;
    m_format_match.crBackColor = contrast2( color_base,
                                            parse_color( Theme::s_color_match1 ),
                                            parse_color( Theme::s_color_match2 ) );
    /*m_format_markup_link->property_foreground_rgba() = color_mid;
    m_format_link->property_foreground_rgba() = contrast2(
            theme->color_base, Theme::s_color_link1, Theme::s_color_link2 );
    m_format_link_broken->property_foreground_rgba() = contrast2(
            theme->color_base, Theme::s_color_broken1, Theme::s_color_broken2 ); */
            

    m_format_done.crBackColor = contrast2( color_text,
                                           parse_color( Theme::s_color_done1 ),
                                           parse_color( Theme::s_color_done2 ) );
    m_format_checkbox_todo.crBackColor =
            midcontrast( theme, m_format_checkbox_todo.crTextColor );
    m_format_checkbox_progressed.crBackColor =
            midcontrast( theme, m_format_checkbox_progressed.crTextColor );
    m_format_checkbox_done.crBackColor =
            midcontrast( theme, m_format_checkbox_done.crTextColor );
    m_format_checkbox_canceled.crBackColor =
            midcontrast( theme, m_format_checkbox_canceled.crTextColor );
}
/*
inline void
TextviewDiary::update_link()
{
    Gtk::TextIter       iter;
    const Glib::RefPtr< Gdk::Cursor >
                        *ptr2cursor = &m_cursor_xterm;
    int                 pointer_x, pointer_y;
    int                 trailing, buffer_x, buffer_y;
    Gdk::ModifierType   modifiers;

    Gtk::Widget::get_window()->get_pointer( pointer_x, pointer_y, modifiers );
    window_to_buffer_coords( Gtk::TEXT_WINDOW_WIDGET,
                             pointer_x, pointer_y,
                             buffer_x, buffer_y );
    get_iter_at_position( iter, trailing, buffer_x, buffer_y );

    // FIX ITER IF NEEDED:
    if( iter.has_tag( m_buffer->m_format_hidden ) )
        iter.forward_to_tag_toggle( m_buffer->m_format_hidden );
    Gtk::TextIter iter2( iter );
    iter2++;
    if( ( iter2.ends_tag( m_buffer->m_format_link ) ||
          iter2.ends_tag( m_buffer->m_format_link_broken ) ) && trailing > 0 )
        iter++;

    m_link_hovered = m_buffer->get_link( iter );

    if( m_link_hovered != NULL )
    {
        if( Diary::d->is_read_only() || m_link_hovered->type == Link::LT_CHECK )
        {
            if( !( modifiers & Gdk::CONTROL_MASK ) )
                ptr2cursor = &m_cursor_hand;
        }
        else
        {
            if( modifiers & Gdk::CONTROL_MASK )
                ptr2cursor = &m_cursor_hand;
        }
    }

    if( ptr2cursor != m_ptr2cursor_last )
    {
        m_ptr2cursor_last = ptr2cursor;
        get_window( Gtk::TEXT_WINDOW_TEXT )->set_cursor( *ptr2cursor );
    }
}*/

