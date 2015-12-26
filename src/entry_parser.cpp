/***********************************************************************************

    Copyright (C) 2007-2012 Ahmet Öztürk (aoz_2@yahoo.com)

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


#include "entry_parser.hpp"


using namespace LIFEO;


void
EntryParser::reset( Ustring::size_type start, Ustring::size_type end )
{
    m_pos_start = start;
    m_pos_end = end;
    pos_current = pos_word = pos_alpha = pos_regular = start;

    m_cf_last = CF_NOT_SET;
    m_cf_req = CF_ANY;
    m_word_last.clear();
    m_word_count = 0;
    alpha_last.clear();
    int_last = 0;
    date_last = 0;
    id_last = 0;
    m_chars_looked_for.clear();

    if( start == 0 )
    {
        m_chars_looked_for.push_back( AbsChar( CF_IGNORE, NULL ) ); // to prevent formatting within title
        m_applier_nl = &EntryParser::apply_heading_end;
        apply_heading();
    }
    else
    {
        m_chars_looked_for.push_back( AbsChar( CF_NOTHING, NULL ) );
        m_applier_nl = NULL;
    }
}

void
EntryParser::set_search_str( const Wstring& str )
{
    m_search_str = str;
    i_search = 0;
    i_search_end = str.size() - 1;
}

void
EntryParser::parse( Wstring::size_type start, Wstring::size_type end )
{
    reset( start, end );

    for( ; pos_current < m_pos_end; ++pos_current )
    {
        char_current = get_char_at( pos_current );

        if( !m_search_str.empty() )
        {
            if( m_search_str[ i_search ] == char_lower( char_current ) )
            {
                if( i_search == 0 )
                    pos_search = pos_current;
                if( i_search == i_search_end )
                {
                    apply_match();
                    i_search = 0;
                }
                else
                    i_search++;
            }
            else
            {
                i_search = 0;
            }
        }

        // MARKUP PARSING
        switch( char_current )
        {
            case 0:     // should never be the case
            case '\n':
            case '\r':
                process_char( CF_NEWLINE,
                              CF_NUM_CKBX|CF_ALPHA|CF_FORMATCHAR|
                              CF_SLASH|CF_DOTDATE|CF_MORE|CF_TAB|CF_IGNORE,
                              0, NULL );
                break;
            case ' ':
                process_char( CF_SPACE,
                              CF_ALPHA|CF_NUMBER|CF_SLASH|CF_DOTDATE|CF_TODO,
                              CF_NOTHING, &EntryParser::trigger_subheading );
                break;
            case '*': // SIGN
                process_char( CF_ASTERISK,
                              CF_NUM_CKBX|CF_ALPHA|CF_SLASH|CF_DOTDATE,
                              CF_NOTHING, &EntryParser::trigger_bold );
                break;
            case '_': // SIGN
                process_char( CF_UNDERSCORE,
                              CF_NUM_CKBX|CF_SLASH|CF_DOTDATE,
                              CF_NOTHING, &EntryParser::trigger_italic );
                break;
            case '=': // SIGN
                process_char( CF_EQUALS,
                              CF_NUM_CKBX|CF_ALPHA|CF_SLASH|CF_DOTDATE,
                              CF_NOTHING, &EntryParser::trigger_strikethrough );
                break;
            case '#': // SIGN
                process_char( CF_HASH,
                              CF_NUM_CKBX|CF_ALPHA|CF_SLASH|CF_DOTDATE,
                              CF_NOTHING, &EntryParser::trigger_highlight );
                break;
            case '[': // SIGN
                process_char( CF_SBB,
                              CF_NUM_CKBX|CF_ALPHA|CF_SLASH|CF_DOTDATE,
                              CF_NOTHING, &EntryParser::trigger_comment );
                break;
            case ']': // SIGN
                process_char( CF_SBE,
                              CF_NUM_CKBX|CF_ALPHA|CF_SLASH|CF_DOTDATE,
                              0, NULL );
                break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                handle_number();   // calculates numeric value
                process_char( CF_NUMBER,
                              CF_SLASH|CF_ALPHA|CF_DOTDATE|CF_TODO,
                              CF_NOTHING, &EntryParser::trigger_link_date );
                break;
            case '.': // SIGN
                process_char( CF_DOTDATE,
                              CF_NUM_CKBX|CF_ALPHA|CF_SLASH,
                              CF_NOTHING, &EntryParser::trigger_ignore );
                break;
            case '-': // SIGN - CF_SIGNSPELL does not seem to be necessary
                process_char( CF_DOTDATE,
                              CF_NUM_CKBX|CF_ALPHA|CF_SLASH,
                              0, NULL );
                break;
            case '/': // SIGN
                process_char( CF_SLASH|CF_DOTDATE,
                              CF_NUM_CKBX|CF_ALPHA,
                              0, NULL );
                break;
            case ':': // SIGN
                process_char( CF_PUNCTUATION_RAW,
                              CF_NUM_CKBX|CF_ALPHA|CF_SLASH|CF_DOTDATE,
                              CF_NOTHING, &EntryParser::trigger_link );
                break;
            case '@': // SIGN
                process_char( CF_AT,
                              CF_NUM_CKBX|CF_ALPHA|CF_SLASH|CF_DOTDATE,
                              CF_NOTHING, &EntryParser::trigger_link_at );
                break;
            case '>': // SIGN
                process_char( CF_MORE,
                              CF_NUM_CKBX|CF_ALPHA|CF_SLASH|CF_DOTDATE,
                              0, NULL );
                break;
            case '\t':
                process_char( CF_TAB,
                              CF_NUM_SLSH|CF_ALPHA|CF_DOTDATE,
                              CF_NOTHING, &EntryParser::trigger_list );
                break;
            // LIST CHARS
            case '~':
            case '+':
                process_char( CF_TODO|CF_PUNCTUATION_RAW,
                              CF_ALPHA|CF_NUM_CKBX|CF_DOTDATE|CF_SLASH,
                              0, NULL );
                break;
            case 'x':
            case 'X':
                process_char( CF_TODO|CF_ALPHA,
                              CF_NUM_CKBX|CF_DOTDATE|CF_SLASH,
                              0, NULL );
                break;
            // MAYBE LATER:
            /*case '\'': // SIGN
                process_char( CF_PUNCTUATION_RAW,
                              CF_ALPHA|CF_NUM_CKBX|CF_DOTDATE|CF_SLASH,
                              0, NULL,
                              CC_SIGNSPELL );
                break;*/
            default:
                if( is_char_alpha( char_current ) )
                {
                    process_char( CF_ALPHA,
                                  CF_NUM_CKBX|CF_DOTDATE|CF_SLASH,
                                  0, NULL );
                }
                else // SIGN
                {
                    process_char( CF_PUNCTUATION_RAW,
                                  CF_ALPHA|CF_NUM_CKBX|CF_DOTDATE|CF_SLASH,
                                  0, NULL );
                }
                break;
        }
    }
    // end of the text -treated like new line
    if( m_pos_end > 0 ) // only when finish is not forced
    {
        char_current = '\n';    // treat end of text as new line for all means and purposes

        process_char( CF_NEWLINE|CF_EOT,
                      CF_NUM_CKBX|CF_ALPHA|CF_FORMATCHAR|
                      CF_SLASH|CF_DOTDATE|CF_MORE|CF_TAB|CF_IGNORE,
                      0, NULL );
    }
}

inline void
EntryParser::process_char( unsigned int char_flags, unsigned int breaks,
                           unsigned int triggers_on, FPtr_void triggerer )
{
    // NOTE: m_cc_current *must* be set before calling this function

    // UPDATE ALPHA LAST
    if( m_flag_handle_word )
    {
        if( char_flags & CF_ALPHA ) // CF_SPELLCHECK may be used in the future
        {
            if( alpha_last.empty() )
                pos_alpha = pos_current;
            alpha_last += char_current;
        }
        else if( char_current == '\'' && !alpha_last.empty() ) // not so elegant
            alpha_last += char_current;
        else
        {
            if( !alpha_last.empty() )
            {
                // too hackish a solution:
                if( alpha_last[ alpha_last.size() - 1 ] == '\'' )
                    alpha_last.erase( alpha_last.size() - 1, 1 );
                handle_word();
                alpha_last.clear();
            }
        }
    }

    // COLLECT INFORMATION ON WHAT TO DO
    unsigned int    cf( m_chars_looked_for.front().flags );
    FPtr_void       applier( m_chars_looked_for.front().applier );
    bool            flag_clear_chars( false );
    bool            flag_trigger( false );
    bool            flag_apply( false );

    if( char_flags & cf )
    {
        if( applier != NULL )
        {
            if( m_chars_looked_for.front().junction )
                flag_apply = true;
            else if( m_cf_last & m_cf_req ) // not junction = final applier
            {
                flag_clear_chars = true;
                flag_apply = true;
            }
        }
        else
        {
            m_chars_looked_for.pop_front();
            if( triggers_on & cf )
                flag_trigger = true;
        }
    }
    else
    if( ( breaks & cf ) == cf || ( cf & CF_IMMEDIATE ) )
    {
        flag_clear_chars = true;
        if( triggers_on & CF_NOTHING )
            flag_trigger = true;
    }
    else
    if( triggers_on & cf )
    {
        flag_trigger = true;
    }

    if( char_flags & CF_NEWLINE )
    {
        flag_clear_chars = true;

        if( m_applier_nl != NULL )
        {
            ( this->*m_applier_nl )();
            m_applier_nl = NULL;
        }
        else if( ( char_flags & CF_EOT ) && !flag_apply )
        {
            m_pos_start = pos_current + 1;
            apply_regular();
        }
    }

    // DO AS COLLECTED INFORMATION REQUIRES
    if( flag_clear_chars )
    {
        m_chars_looked_for.clear();
        m_chars_looked_for.push_back( AbsChar( CF_NOTHING, NULL ) );
    }
    if( flag_trigger )
        ( this->*triggerer )();
    if( flag_apply )
        ( this->*applier )();

    // UPDATE WORD LAST
    if( char_flags & CF_SEPARATOR )
        m_word_last.clear();
    else
    {
        if( m_word_last.empty() )
        {
            pos_word = pos_current;
            m_word_count++;
        }
        m_word_last += char_current;
    }

    // UPDATE CHAR CLASS
    m_cf_last = char_flags;
}

// TRIGGERERS ======================================================================================
void
EntryParser::trigger_subheading()
{
    if( m_cf_last == CF_NEWLINE )
    {
        m_chars_looked_for.clear();
        m_chars_looked_for.push_back( AbsChar( CF_NONSPACE, &EntryParser::apply_subheading_0 ) );
        m_cf_req = CF_ANY;
        m_pos_start = pos_current;
    }
}

void
EntryParser::apply_subheading_0()
{
    m_applier_nl = &EntryParser::apply_subheading_end;
    apply_subheading();
}

inline void
EntryParser::trigger_markup( unsigned int lf, FPtr_void applier )
{
    if( m_cf_last & CF_NOT_SEPARATOR )
        return;

    m_chars_looked_for.clear();
    m_chars_looked_for.push_back( AbsChar( CF_NONSPACE - lf, NULL ) );
    m_chars_looked_for.push_back( AbsChar( lf, applier ) );
    m_cf_req = CF_NOT_SEPARATOR;
    m_pos_start = pos_current;
}

void
EntryParser::trigger_bold()
{
    trigger_markup( CF_ASTERISK, &EntryParser::apply_bold );
}

void
EntryParser::trigger_italic()
{
    trigger_markup( CF_UNDERSCORE, &EntryParser::apply_italic );
}

void
EntryParser::trigger_strikethrough()
{
    trigger_markup( CF_EQUALS, &EntryParser::apply_strikethrough );
}

void
EntryParser::trigger_highlight()
{
    trigger_markup( CF_HASH, &EntryParser::apply_highlight );
}

void
EntryParser::trigger_comment()
{
    m_chars_looked_for.clear();
    m_chars_looked_for.push_back( AbsChar( CF_SBB|CF_IMMEDIATE, NULL ) );
    m_chars_looked_for.push_back( AbsChar( CF_SBE, NULL ) );
    m_chars_looked_for.push_back( AbsChar( CF_SBE|CF_IMMEDIATE, &EntryParser::apply_comment ) );
    m_cf_req = CF_ANY;
    m_pos_start = pos_current;
}

void
EntryParser::trigger_link()
{
    PRINT_DEBUG( "word_last: " + m_word_last );
    m_flag_hidden_link = m_word_last[ 0 ] == '<';
    if( m_flag_hidden_link )
        m_word_last.erase( 0, 1 );

    m_cf_req = CF_ANY;

    if( m_word_last == W( "http" ) || m_word_last == W( "https" ) ||
        m_word_last == W( "ftp" ) || m_word_last == W( "file" ) || m_word_last == W( "rel" ) )
    {
        m_chars_looked_for.clear();
        m_chars_looked_for.push_back( AbsChar( CF_SLASH, NULL ) );
        m_chars_looked_for.push_back( AbsChar( CF_SLASH, NULL ) );
        if( m_word_last == W( "file" ) )
        {
            m_chars_looked_for.push_back( AbsChar( CF_SLASH, NULL ) );
            m_chars_looked_for.push_back( AbsChar( CF_NONSPACE, NULL ) );
        }
        else
            m_chars_looked_for.push_back( AbsChar( CF_ALPHA|CF_NUMBER, NULL ) ); // TODO: add dash
    }
    else
    if( m_word_last == W( "mailto" ) )
    {
        m_chars_looked_for.clear();
        m_chars_looked_for.push_back( AbsChar( CF_UNDERSCORE|CF_ALPHA|CF_NUMBER, NULL ) );
        m_chars_looked_for.push_back( AbsChar( CF_AT, NULL ) );
        m_chars_looked_for.push_back( AbsChar( CF_ALPHA|CF_NUMBER, NULL ) ); // TODO: add dash
    }
    else
    if( m_word_last == W( "deid" ) && m_flag_hidden_link )
    {
        m_chars_looked_for.clear();
        m_chars_looked_for.push_back( AbsChar( CF_NUMBER, NULL ) );
        m_chars_looked_for.push_back( AbsChar( CF_TAB,
                                               &EntryParser::junction_link_hidden_tab,
                                               true ) ); // junction
        m_chars_looked_for.push_back( AbsChar( CF_NONSPACE - CF_MORE, NULL ) );
        m_chars_looked_for.push_back( AbsChar( CF_MORE, &EntryParser::apply_link_id ) );
        m_pos_start = pos_word;
        return;
    }
    else
        return;

    if( m_flag_hidden_link )
    {
        m_chars_looked_for.push_back( AbsChar( CF_TAB,
                                               &EntryParser::junction_link_hidden_tab,
                                               true ) ); // junction
        m_chars_looked_for.push_back( AbsChar( CF_NONSPACE - CF_MORE, NULL ) );
        m_chars_looked_for.push_back( AbsChar( CF_MORE, &EntryParser::apply_link ) );
    }
    else
    {
        m_chars_looked_for.push_back( AbsChar( CF_TAB|CF_NEWLINE|CF_SPACE,
                                               &EntryParser::apply_link ) );
    }
    m_pos_start = pos_word;
}

void
EntryParser::trigger_link_at()
{
    PRINT_DEBUG( "word_last [@]: " + m_word_last );
    if( m_cf_last & CF_SEPARATOR )
        return;

    m_flag_hidden_link = false;
    m_word_last.insert( 0, W( "mailto:" ) );
    m_chars_looked_for.clear();
    m_chars_looked_for.push_back( AbsChar( CF_ALPHA|CF_NUMBER, NULL ) ); // TODO: add dash
    m_chars_looked_for.push_back( AbsChar( CF_TAB|CF_NEWLINE|CF_SPACE,
                                           &EntryParser::apply_link ) );
    m_cf_req = CF_ANY;
    m_pos_start = pos_word;
}

void
EntryParser::trigger_link_date()
{
    m_cf_req = CF_ANY;
    m_chars_looked_for.clear();
    m_chars_looked_for.push_back( AbsChar( CF_NUMBER, NULL ) );
    m_chars_looked_for.push_back( AbsChar( CF_NUMBER, NULL ) );
    m_chars_looked_for.push_back( AbsChar( CF_NUMBER, NULL ) );
    m_chars_looked_for.push_back( AbsChar( CF_DOTYM, &EntryParser::junction_date_dotym, true ) );
    m_chars_looked_for.push_back( AbsChar( CF_NUMBER, NULL ) );
    m_chars_looked_for.push_back( AbsChar( CF_NUMBER, NULL ) );
    m_chars_looked_for.push_back( AbsChar( CF_DOTMD, &EntryParser::junction_date_dotmd, true ) );
    m_chars_looked_for.push_back( AbsChar( CF_NUMBER, NULL ) );
    m_chars_looked_for.push_back( AbsChar( CF_NUMBER, &EntryParser::junction_link_date, true ) );

    m_flag_hidden_link = ( m_word_last == W( "<" ) );
    if( m_flag_hidden_link )
    {
        m_chars_looked_for.push_back( AbsChar( CF_TAB,
                                               &EntryParser::junction_link_hidden_tab,
                                               true ) );    // junction
        m_chars_looked_for.push_back( AbsChar( CF_NONSPACE, NULL ) );
        m_chars_looked_for.push_back( AbsChar( CF_MORE, &EntryParser::apply_link_date ) );
        m_pos_start = pos_current - 1;
    }
    else
    {
        m_pos_start = pos_current;
    }
}

void
EntryParser::trigger_list()
{
    if( m_cf_last != CF_NEWLINE )
        return;

    m_chars_looked_for.clear();
    m_chars_looked_for.push_back( AbsChar( CF_NONTAB, &EntryParser::junction_list, true ) );
    m_cf_req = CF_ANY;
    m_pos_start = pos_current;
}

void
EntryParser::trigger_ignore()
{
    if( m_cf_last == CF_NEWLINE )
    {
        m_chars_looked_for.clear();
        m_chars_looked_for.push_back( AbsChar( CF_TAB|CF_IMMEDIATE,
                                               &EntryParser::junction_ignore,
                                               true ) ); // junction
        m_cf_req = CF_ANY;
        m_pos_start = pos_current;
    }
}

void
EntryParser::junction_link_date()
{
    date_last.set_day( int_last );

    if( date_last.is_valid() )
    {
        if( m_flag_hidden_link )
        {
            m_chars_looked_for.pop_front();
            return;
        }
        else
            apply_link_date();
    }

    m_chars_looked_for.clear();
    m_chars_looked_for.push_back( AbsChar( CF_NOTHING, NULL ) );
}

void
EntryParser::junction_link_hidden_tab()
{
    m_chars_looked_for.pop_front();
    pos_tab = pos_current;
    id_last = int_last;     // if not id link assignment is in vain
}

void
EntryParser::junction_list()
{
    apply_indent();
    m_cf_req = CF_ANY;

    switch( char_current )
    {
        case '[':
            // reuse
            m_chars_looked_for.front().flags = CF_SPACE|CF_TODO|CF_IMMEDIATE;
            m_chars_looked_for.front().applier = &EntryParser::junction_list2;

            m_chars_looked_for.push_back( AbsChar( CF_SBE|CF_IMMEDIATE, NULL ) );
            m_chars_looked_for.push_back( AbsChar( CF_SPACE|CF_IMMEDIATE, NULL ) );
            break;
        default:
            m_chars_looked_for.clear();
            m_chars_looked_for.push_back( AbsChar( CF_NOTHING, NULL ) );
            break;
    }
}

void
EntryParser::junction_list2()
{
    m_cf_req = CF_ANY;

    switch( char_current )
    {
        case ' ':
            m_chars_looked_for.pop_front();
            m_chars_looked_for.back().applier = &EntryParser::apply_check_unf;
            break;
        case '~':
            m_chars_looked_for.pop_front();
            m_chars_looked_for.back().applier = &EntryParser::apply_check_prg;
            break;
        case '+':
            m_chars_looked_for.pop_front();
            m_chars_looked_for.back().applier = &EntryParser::apply_check_fin_0;
            break;
        case 'x':
        case 'X':
            m_chars_looked_for.pop_front();
            m_chars_looked_for.back().applier = &EntryParser::apply_check_ccl_0;
            break;
        default:
            m_chars_looked_for.clear();
            m_chars_looked_for.push_back( AbsChar( CF_NOTHING, NULL ) );
            break;
    }
}

void
EntryParser::junction_date_dotym()
{
    if( int_last >= Date::YEAR_MIN && int_last <= Date::YEAR_MAX )
    {
        date_last.set_year( int_last );
        m_chars_looked_for.pop_front();
    }
    else
    {
        m_chars_looked_for.clear();
        m_chars_looked_for.push_back( AbsChar( CF_NOTHING, NULL ) );
    }
}

void
EntryParser::junction_date_dotmd()
{
    if( int_last >= 1 && int_last <= 12 &&
    // two separators must be the same:
        char_current == m_word_last[ m_word_last.size() - 3 ] )
    {
        date_last.set_month( int_last );
        m_chars_looked_for.pop_front();
    }
    else
    {
        m_chars_looked_for.clear();
        m_chars_looked_for.push_back( AbsChar( CF_NOTHING, NULL ) );
    }
}

void
EntryParser::junction_ignore()
{
//    m_applier_nl = &EntryParser::apply_ignore_end;
    m_chars_looked_for.front().flags = CF_IGNORE;
    m_chars_looked_for.front().applier = NULL;
    apply_ignore();
}

// TODO: this is a temporary solution until begin and end appliers, and junction functions will
// be bundled in a class also eliminating the need for separate deques for all of them:
void
EntryParser::apply_check_ccl_0()
{
    m_applier_nl = &EntryParser::apply_check_ccl_end;
    apply_check_ccl();
}

void
EntryParser::apply_check_fin_0()
{
    m_applier_nl = &EntryParser::apply_check_fin_end;
    apply_check_fin();
}

// HELPERS =========================================================================================
inline void
EntryParser::handle_number()
{
    if( m_cf_last == CF_NUMBER )
    {
        int_last *= 10;
        int_last += ( char_current - '0' );
    }
    else
        int_last = ( char_current - '0' );
}
