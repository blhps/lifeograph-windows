/***********************************************************************************

    Copyright (C) 2007-2011 Ahmet Öztürk (aoz_2@yahoo.com)

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

#include <cstdio>   // for file operations
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <gcrypt.h>
#include <cerrno>
#include <cassert>
#include <cfloat>

#include "diary.hpp"
#include "helpers.hpp"
#include "lifeograph.hpp"

#ifdef LIFEO_WINDOZE
#include <unistd.h>
#include "strings.hpp"
#endif

using namespace LIFEO;


// STATIC MEMBERS
Diary*                  Diary::d;
#if LIFEOGRAPH_DEBUG_BUILD
bool                    Diary::s_flag_ignore_locks( true );
#else
bool                    Diary::s_flag_ignore_locks( false );
#endif
ElementView< Diary >* Diary::shower( nullptr );

// PARSING HELPERS
Date
get_db_line_date( const Ustring& line )
{
    Date date( 0 );

    for( unsigned int i = 2;
         i < line.size() && i < 12 && int ( line[ i ] ) >= '0' && int ( line[ i ] ) <= '9';
         i++ )
    {
        date.m_date = ( date.m_date * 10 ) + int ( line[ i ] ) - '0';
    }

    return( date );
}

Ustring
get_db_line_name( const Ustring& line )
{
    Ustring::size_type begin( line.find( '\t' ) );
    if( begin == std::string::npos )
        begin = 2;
    else
        begin++;

    return( line.substr( begin ) );
}

// DIARY ===========================================================================================
Diary::Diary()
:   DiaryElement( nullptr, DEID_DIARY )
{
    m_filter_active = new Filter( NULL, _( "Active Filter" ) );
    m_filter_default = new Filter( NULL, "Default Filter" );

    m_topics = new CategoryChapters( this, Date::TOPIC_MIN );
    m_groups = new CategoryChapters( this, Date::GROUP_MIN );
}

Diary::~Diary()
{
    remove_lock();

    delete m_topics;
    delete m_groups;
}

Result
Diary::init_new( const std::string& path )
{
    clear();
    Result result( set_path( path, SPT_NEW ) );

    if( result != LIFEO::SUCCESS )
    {
        clear();
        return result;
    }

    // every diary must at least have one chapter category:
    m_ptr2chapter_ctg_cur = create_chapter_ctg( _( STRING::DEFAULT_CHAPTER_CTG_NAME ) );

    add_today(); // must come after m_ptr2chapter_ctg_cur is set

    return LIFEO::SUCCESS;
}

void
Diary::clear()
{
    close_file();

    if( remove_lock() )
        m_path.clear();

    m_read_version = 0;

    m_current_id = DEID_FIRST;
    m_force_id = DEID_UNSET;
    m_ids.clear();
    m_ids[ DEID_DIARY ] = this; // add diary back to IDs pool

    m_entries.clear();
    m_tags.clear();
    m_tag_categories.clear();
    m_untagged.reset();

    m_ptr2chapter_ctg_cur = NULL;
    m_chapter_categories.clear();
    m_topics->clear();
    m_groups->clear();
    m_orphans.clear();

    m_startup_elem_id = HOME_CURRENT_ELEM;
    m_last_elem_id = DEID_DIARY;

    m_passphrase.clear();

    m_search_text.clear();
    m_filter_default->reset();
    m_filter_active->reset();

    // NOTE: only reset body options here:
    m_language.clear();
    m_option_sorting_criteria = SC_DATE;
}

#ifndef LIFEO_WINDOZE
const Icon&
Diary::get_icon() const
{
    return Lifeograph::icons->diary_16;
}
const Icon&
Diary::get_icon32() const
{
    return Lifeograph::icons->diary_32;
}
#else
int
Diary::get_icon() const
{
    return ICON_NO_DIARY;
}
#endif

LIFEO::Result
Diary::set_path( const std::string& path0, SetPathType type )
{
    std::string path( path0 );

#ifndef LIFEO_WINDOZE
    // RESOLVE SYMBOLIC LINK
    if( g_file_test( path0.c_str(), G_FILE_TEST_IS_SYMLINK ) )
    {
        GError* err = NULL;
        path = g_file_read_link( path0.c_str(), &err );
        print_info( "Symbolic link resolved to path: ", path );
    }
#endif

    // CHECK FILE SYSTEM PERMISSIONS
    if( access( path.c_str(), F_OK ) != 0 ) // check existence
    {
        if( errno == ENOENT )
        {
            if( type != SPT_NEW )
            {
                PRINT_DEBUG( "File is not found" );
                return LIFEO::FILE_NOT_FOUND;
            }
        }
        else
            return LIFEO::FAILURE; // should not be the case
    }
    else if( access( path.c_str(), R_OK ) != 0 ) // check read access
    {
        PRINT_DEBUG( "File is not readable" );
        return LIFEO::FILE_NOT_READABLE;
    }
    else if( type != SPT_READ_ONLY && access( path.c_str(), W_OK ) != 0 ) // check write access
    {
        if( type == SPT_NEW )
        {
            PRINT_DEBUG( "File is not writable" );
            return LIFEO::FILE_NOT_WRITABLE;
        }
        // errno == EACCES or errno == EROFS but no need to bother
        PRINT_DEBUG( "File is not writable, opening read-only..." );
        type = SPT_READ_ONLY;
    }

    // CHECK AND "TOUCH" THE NEW LOCK
    if( type != SPT_READ_ONLY )
    {
        std::string path_lock( path + LOCK_SUFFIX );
        if( access( path_lock.c_str(), F_OK ) == 0 )
        {
            if( s_flag_ignore_locks )
                print_info( "Ignored file lock" );
            else
                return LIFEO::FILE_LOCKED;
        }

        if( type == SPT_NORMAL )
        {
            FILE* fp = fopen( path_lock.c_str(), "a+" );
            if( fp )
                fclose( fp );
            else
                print_error( "Could not create lock file" );
        }
    }

    // REMOVE PREVIOUS LOCK IF ANY
    remove_lock();

    // ACCEPT PATH
    m_path = path;
    m_name = get_filename_base( path );
    m_flag_read_only = ( type == SPT_READ_ONLY );

    return LIFEO::SUCCESS;
}

const std::string&
Diary::get_path() const
{
    return m_path;
}

bool
Diary::is_path_set() const
{
    return( ( bool ) m_path.size() );
}

bool
Diary::set_passphrase( const std::string& passphrase )
{
    if( passphrase.size() >= PASSPHRASE_MIN_SIZE )
    {
        m_passphrase = passphrase;
        return true;
    }
    else
        return false;
}

void
Diary::clear_passphrase()
{
    m_passphrase.clear();
}

std::string
Diary::get_passphrase() const
{
    return m_passphrase;
}

bool
Diary::compare_passphrase( const std::string& passphrase ) const
{
    return( m_passphrase == passphrase );
}

bool
Diary::is_passphrase_set() const
{
    return( ( bool ) m_passphrase.size() );
}

void
Diary::close_file()
{
    if( m_ifstream )
    {
        m_ifstream->close();
        delete m_ifstream;
        m_ifstream = NULL;
    }
}

LIFEO::Result
Diary::read_header()
{
    m_ifstream = new std::ifstream( m_path.c_str() );

    if( ! m_ifstream->is_open() )
    {
        print_error( "failed to open diary file: ", m_path );
        clear();
        return LIFEO::COULD_NOT_START;
    }
    std::string line;

    getline( *m_ifstream, line );

    if( line != LIFEO::DB_FILE_HEADER )
    {
        clear();
        return LIFEO::CORRUPT_FILE;
    }

    while( getline( *m_ifstream, line ) )
    {
        switch( line[ 0 ] )
        {
            case 'V':
                m_read_version = convert_string( line.substr( 2 ) );
                if( m_read_version < LIFEO::DB_FILE_VERSION_INT_MIN )
                {
                    clear();
                    return LIFEO::INCOMPATIBLE_FILE_OLD;
                }
                else if( m_read_version > LIFEO::DB_FILE_VERSION_INT )
                {
                    clear();
                    return LIFEO::INCOMPATIBLE_FILE_NEW;
                }
                break;
            case 'E':
                if( line[ 2 ] == 'y' )
                    // passphrase is set to a dummy value to indicate that diary
                    // is an encrypted one until user enters the real passphrase
                    m_passphrase = " ";
                else
                    m_passphrase.clear();
                break;
            case 0: // end of header
                return LIFEO::SUCCESS;
            default:
                print_error( "unrecognized header line: ", line );
                break;
        }
    }

    clear();
    return LIFEO::CORRUPT_FILE;
}

LIFEO::Result
Diary::read_body()
{
    Result res( m_passphrase.empty() ? read_plain() : read_encrypted() );

    close_file();

    return res;
}

LIFEO::Result
Diary::read_plain()
{
    if( ! m_ifstream->is_open() )
    {
        print_error( "Internal error while reading the diary file: ", m_path );
        clear();
        return LIFEO::COULD_NOT_START;
    }

    return parse_db_body_text( *m_ifstream );
}

LIFEO::Result
Diary::read_encrypted()
{
    if( ! m_ifstream->is_open() )
    {
        print_error( "Failed to open diary file: ", m_path );
        clear();
        return LIFEO::COULD_NOT_START;
    }

    CipherBuffers buf;

#ifdef LIFEO_WINDOZE
    m_ifstream->close();
    m_ifstream->open( m_path.c_str(),
                      std::ifstream::in | std::ifstream::binary | std::ifstream::ate );

    size_t fsize = m_ifstream->tellg();
    m_ifstream->seekg( 0, std::ios_base::beg );

    // seek... this is preposterous!
    char ch;
    int ch_count = 0;
    while( m_ifstream->get( ch ) )
    {
        if( ch == '\n' )
        {
            if( ch_count )
                break;
            else
                ch_count = 1;
        }
        else
            ch_count = 0;
    }
#else
    size_t fsize = LIFEO::get_file_size( *m_ifstream );
#endif

    try
    {
        // allocate memory for salt
        buf.salt = new unsigned char[ LIFEO::Cipher::cSALT_SIZE ];
        // read salt value
        m_ifstream->read( ( char* ) buf.salt, LIFEO::Cipher::cSALT_SIZE );

        buf.iv = new unsigned char[ LIFEO::Cipher::cIV_SIZE ];
        // read IV
        m_ifstream->read( ( char* ) buf.iv, LIFEO::Cipher::cIV_SIZE );

        LIFEO::Cipher::expand_key( m_passphrase.c_str(), buf.salt, &buf.key );

        // calculate bytes of data in file
        size_t size = fsize - m_ifstream->tellg();
        if( size <= 3 )
        {
            buf.clear();
            clear();
            return LIFEO::CORRUPT_FILE;
        }
        buf.buffer = new unsigned char[ size + 1 ];
        if( ! buf.buffer )
            throw LIFEO::Error( "Unable to allocate memory for buffer" );

        m_ifstream->read( ( char* ) buf.buffer, size );
        LIFEO::Cipher::decrypt_buffer( buf.buffer, size, buf.key, buf.iv );

        // passphrase check
        if( buf.buffer[ 0 ] != m_passphrase[ 0 ] && buf.buffer[ 1 ] != '\n' )
        {
            buf.clear();
            clear();
            return LIFEO::WRONG_PASSWORD;
        }

        buf.buffer[ size ] = 0;   // terminating zero
    }
    catch( ... )
    {
        buf.clear();
        clear();
        return LIFEO::COULD_NOT_START;
    }

    std::stringstream stream;
    buf.buffer += 2;    // ignore first two chars which are for passphrase checking
    stream << buf.buffer;
    buf.buffer -= 2;    // restore pointer to the start of the buffer before deletion

    buf.clear();
    return parse_db_body_text( stream );
}

 // PARSING HELPERS
inline void
parse_todo_status( DiaryElement* elem, char c )
{
    switch( c )
    {
        case 't':
            elem->set_todo_status( ES::TODO );
            break;
        case 'p':
            elem->set_todo_status( ES::PROGRESSED );
            break;
        case 'd':
            elem->set_todo_status( ES::DONE );
            break;
        case 'c':
            elem->set_todo_status( ES::CANCELED );
            break;
    }
}

Date
fix_pre_1020_date( Date d )
{
    if( d.is_ordinal() )
    {
        if( d.m_date & Date::VISIBLE_FLAG )
            d.m_date -= Date::VISIBLE_FLAG;
        else
            d.m_date |= Date::VISIBLE_FLAG;
    }

    return d;
}

void
Diary::do_standard_checks_after_parse()
{
    // every diary must at least have one chapter category:
    if( m_chapter_categories.empty() )
        m_ptr2chapter_ctg_cur = create_chapter_ctg( _( STRING::DEFAULT_CHAPTER_CTG_NAME ) );

    if( m_startup_elem_id > HOME_FIXED_ELEM )
        if( get_element( m_startup_elem_id ) == NULL )
        {
            print_error( "startup element cannot be found in db" );
            m_startup_elem_id = DEID_DIARY;
        }

    if( m_entries.size() < 1 )
    {
        print_info( "a dummy entry added to the diary" );
        add_today();
    }
}

inline void
parse_theme( Theme* ptr2theme, const std::string& line )
{
    switch( line[ 1 ] )
    {
#ifndef LIFEO_WINDOZE
        case 'f':   // font
            ptr2theme->font = Pango::FontDescription( line.substr( 2 ) );
            break;
        case 'b':   // base color
            ptr2theme->color_base.set( line.substr( 2 ) );
            break;
        case 't':   // text color
            ptr2theme->color_text.set( line.substr( 2 ) );
            break;
        case 'h':   // heading color
            ptr2theme->color_heading.set( line.substr( 2 ) );
            break;
        case 's':   // subheading color
            ptr2theme->color_subheading.set( line.substr( 2 ) );
            break;
        case 'l':   // highlight color
            ptr2theme->color_highlight.set( line.substr( 2 ) );
            break;
#else
        case 'f':   // font
            ptr2theme->font = line.substr( 2 );
            break;
        case 'b':   // base color
            ptr2theme->color_base = line.substr( 2 );
            break;
        case 't':   // text color
            ptr2theme->color_text = line.substr( 2 );
            break;
        case 'h':   // heading color
            ptr2theme->color_heading = line.substr( 2 );
            break;
        case 's':   // subheading color
            ptr2theme->color_subheading = line.substr( 2 );
            break;
        case 'l':   // highlight color
            ptr2theme->color_highlight = line.substr( 2 );
            break;
#endif
    }
}

// PARSING FUNCTIONS
inline LIFEO::Result
Diary::parse_db_body_text( std::istream& stream )
{
    switch( m_read_version )
    {
        case 1040:
        case 1030:
        case 1020:
            return parse_db_body_text_1040( stream );
        case 1011:
        case 1010:
            return parse_db_body_text_1010( stream );
        default:
            return parse_db_body_text_110( stream );
    }
}

LIFEO::Result
Diary::parse_db_body_text_1040( std::istream& stream )
{
    std::string         line( "" );
    Entry*              entry_new = NULL;
    CategoryChapters*   ptr2chapter_ctg = NULL;
    Chapter*            ptr2chapter = NULL;
    CategoryTags*       ptr2tag_ctg = NULL;
    Tag*                ptr2tag = NULL;
    bool                flag_first_paragraph( false );

    // TAG DEFINITIONS & CHAPTERS
    while( getline( stream, line ) )
    {
        if( line[ 0 ] == 0 )    // end of section
            break;
        else if( line.size() >= 3 )
        {
            switch( line[ 0 ] )
            {
                case 'I':   // id
                    set_force_id( LIFEO::convert_string( line.substr( 2 ) ) );
                break;
                // TAGS
                case 'T':   // tag category
                    ptr2tag_ctg = create_tag_ctg( line.substr( 2 ) );
                    ptr2tag_ctg->set_expanded( line[ 1 ] == 'e' );
                    break;
                case 't':   // tag
                    switch( line[ 1 ] )
                    {
                        case ' ':
                            ptr2tag = create_tag( line.substr( 2 ), ptr2tag_ctg );
                            break;
                        case 'c':
                            if( ptr2tag )
                                ptr2tag->set_chart_type(
                                        LIFEO::convert_string( line.substr( 2 ) ) );
                            break;
                        case 'u':
                            if( ptr2tag )
                                ptr2tag->set_unit( line.substr( 2 ) );
                            break;
                    }
                    break;
                case 'u':
                    ptr2tag = &m_untagged;
                    if( line[ 1 ] == 'c' )
                    {
                        ptr2tag->set_chart_type( LIFEO::convert_string( line.substr( 2 ) ) );
                        break;
                    }
                    // no break
                case 'm':
                    if( ptr2tag == nullptr )
                    {
                        print_error( "No tag declared for theme" );
                        break;
                    }
                    parse_theme( ptr2tag->get_own_theme(), line );
                    break;
                case 'f':   // default filter
                    switch( line[ 1 ] )
                    {
                        case 's':   // status
                            if( line.size() < 11 )
                            {
                                print_error( "status filter length error" );
                                continue;
                            }
                            m_filter_default->set_trash( line[ 2 ] == 'T', line[ 3 ] == 't' );
                            m_filter_default->set_favorites( line[ 4 ] == 'F', line[ 5 ] == 'f' );
                            m_filter_default->set_todo( line[ 6 ]  == 'N',
                                                        line[ 7 ]  == 'T',
                                                        line[ 8 ]  == 'P',
                                                        line[ 9 ]  == 'D',
                                                        line[ 10 ] == 'C' );
                            break;
                        case 't':   // tag
                        {
                            PoolTags::iterator iter_tag( m_tags.find( line.substr( 2 ) ) );
                            if( iter_tag != m_tags.end() )
                                m_filter_default->set_tag( iter_tag->second );
                            else
                                print_error( "Reference to undefined tag: ", line.substr( 2 ) );
                            break;
                        }
                        case 'b':   // begin date
                            m_filter_default->set_date_begin(
                                    LIFEO::convert_string( line.substr( 2 ) ) );
                            break;
                        case 'e':   // end date
                            m_filter_default->set_date_end(
                                    LIFEO::convert_string( line.substr( 2 ) ) );
                            break;
                    }
                    break;
                case 'C':   // chapters...
                    switch( line[ 1 ] )
                    {
                        case 'C':   // chapter category
                            ptr2chapter_ctg = create_chapter_ctg( line.substr( 3 ) );
                            if( line[ 2 ] == 'c' )
                                m_ptr2chapter_ctg_cur = ptr2chapter_ctg;
                            break;
                        case 'c':   // chapter color
                            if( ptr2chapter == nullptr )
                            {
                                print_error( "No chapter defined" );
                                break;
                            }
                            ptr2chapter->set_color( Color( line.substr( 2 ) ) );
                            break;
                        case 'T':   // temporal chapter
                            if( ptr2chapter_ctg == nullptr )
                            {
                                print_error( "No chapter category defined" );
                                break;
                            }
                            ptr2chapter = ptr2chapter_ctg->create_chapter(
                                    get_db_line_name( line ), get_db_line_date( line ).m_date );
                            break;
                        case 'O':   // ordinal chapter (used to be called topic)
                            ptr2chapter = m_topics->create_chapter(
                                    get_db_line_name( line ), get_db_line_date( line ).m_date );
                            break;
                        case 'G':   // free chapter (replaced todo_group in v1020)
                        case 'S':
                            ptr2chapter = m_groups->create_chapter(
                                    get_db_line_name( line ), get_db_line_date( line ).m_date );
                            break;
                        case 'p':   // chapter preferences
                            ptr2chapter->set_expanded( line[ 2 ] == 'e' );
                            parse_todo_status( ptr2chapter, line[ 3 ] );
                            if( line.size() > 4 && line[ 4 ] == 'Y' )
                                ptr2chapter->set_chart_type( ChartPoints::YEARLY );
                            break;
                    }
                    break;
                case 'O':   // options
                    m_option_sorting_criteria = line[ 2 ];
                    if( line.size() > 3 && line[ 3 ] == 'Y' )
                        set_chart_type( ChartPoints::YEARLY );
                    break;
                case 'l':   // language
                    m_language = line.substr( 2 );
                    break;
                case 'S':   // startup action
                    m_startup_elem_id = LIFEO::convert_string( line.substr( 2 ) );
                    break;
                case 'L':
                    m_last_elem_id = LIFEO::convert_string( line.substr( 2 ) );
                    break;
                default:
                    print_error( "unrecognized line:\n", line );
                    clear();
                    return LIFEO::CORRUPT_FILE;
            }
        }
    }

    // ENTRIES
    while( getline( stream, line ) )
    {
        if( line.size() < 2 )
            continue;
        else if( line[ 0 ] != 'I' && line[ 0 ] != 'E' && line[ 0 ] != 'e' && entry_new == nullptr )
        {
            print_error( "No entry declared for the attribute" );
            continue;
        }

        switch( line[ 0 ] )
        {
            case 'I':
                set_force_id( LIFEO::convert_string( line.substr( 2 ) ) );
                break;
            case 'E':   // new entry
            case 'e':   // trashed
                if( line.size() < 5 )
                    continue;

                entry_new = new Entry( this, LIFEO::convert_string( line.substr( 4 ) ),
                                       line[ 1 ] == 'f' );

                m_entries[ entry_new->m_date.m_date ] = entry_new;
                add_entry_to_related_chapter( entry_new );
                m_untagged.add_entry( entry_new );

                if( line[ 0 ] == 'e' )
                    entry_new->set_trashed( true );
                if( line[ 2 ] == 'h' )
                    m_filter_default->add_entry( entry_new );

                parse_todo_status( entry_new, line[ 3 ] );

                flag_first_paragraph = true;
                break;
            case 'D':   // creation & change dates (optional)
                switch( line[ 1 ] )
                {
                    case 'r':
                        entry_new->m_date_created = LIFEO::convert_string( line.substr( 2 ) );
                        break;
                    case 'h':
                        entry_new->m_date_edited = LIFEO::convert_string( line.substr( 2 ) );
                        break;
                    case 's':
                        entry_new->m_date_status = LIFEO::convert_string( line.substr( 2 ) );
                        break;
                }
                break;
            case 'T':   // tag
            {
                NameAndValue nav{ NameAndValue::parse( line.substr( 2 ) ) };
                PoolTags::iterator iter_tag{ m_tags.find( nav.name ) };
                if( iter_tag != m_tags.end() )
                {
                    entry_new->add_tag( iter_tag->second, nav.value );
                    if( line[ 1 ] == 'T' )
                        entry_new->set_theme_tag( iter_tag->second );
                }
                else
                    print_error( "Reference to undefined tag: ", nav.name );
                break;
            }
            case 'L':   // location
                entry_new->m_location = line.substr( 2 );
                break;
            case 'l':   // language
                entry_new->set_lang( line.substr( 2 ) );
                break;
            case 'P':    // paragraph
                if( flag_first_paragraph )
                {
                    if( line.size() > 2 )
                        entry_new->m_text = line.substr( 2 );
                    entry_new->m_name = entry_new->m_text;
                    flag_first_paragraph = false;
                }
                else
                {
                    entry_new->m_text += "\n";
                    entry_new->m_text += line.substr( 2 );
                }
                break;
            default:
                print_error( "Unrecognized line:\n", line );
                clear();
                return LIFEO::CORRUPT_FILE;
        }
    }

    do_standard_checks_after_parse();

    m_filter_active->set( m_filter_default );   // employ the default filter

    if( m_read_version == 1020 )
        upgrade_entries();

    return LIFEO::SUCCESS;
}

LIFEO::Result
Diary::parse_db_body_text_1010( std::istream& stream )
{
    std::string         line( "" );
    Entry*              entry_new = NULL;
    CategoryChapters*   ptr2chapter_ctg = NULL;
    Chapter*            ptr2chapter = NULL;
    CategoryTags*       ptr2tag_ctg = NULL;
    Tag*                ptr2tag = NULL;
    bool                flag_first_paragraph( false );

    // TAG DEFINITIONS & CHAPTERS
    while( getline( stream, line ) )
    {
        if( line[ 0 ] == 0 )    // end of section
            break;
        else if( line.size() >= 3 )
        {
            switch( line[ 0 ] )
            {
                case 'I':   // id
                    set_force_id( LIFEO::convert_string( line.substr( 2 ) ) );
                break;
                // TAGS
                case 'T':   // tag category
                    ptr2tag_ctg = create_tag_ctg( line.substr( 2 ) );
                    ptr2tag_ctg->set_expanded( line[ 1 ] == 'e' );
                    break;
                case 't':   // tag
                    ptr2tag = create_tag( line.substr( 2 ), ptr2tag_ctg );
                    break;
                case 'u':
                    ptr2tag = &m_untagged;
                    // no break
                case 'm':
                    if( ptr2tag == NULL )
                    {
                        print_error( "No tag declared for theme" );
                        break;
                    }
                    parse_theme( ptr2tag->get_own_theme(), line );
                    break;
                case 'f':   // default filter
                    switch( line[ 1 ] )
                    {
                        case 's':   // status
                            if( line.size() < 9 )
                            {
                                print_error( "status filter length error" );
                                continue;
                            }
                            m_filter_default->set_trash( line[ 2 ] == 'T', line[ 3 ] == 't' );
                            m_filter_default->set_favorites( line[ 4 ] == 'F', line[ 5 ] == 'f' );
                            // made in-progress entries depend on the preference for open ones
                            m_filter_default->set_todo( true, line[ 6 ]  == 'T', line[ 6 ]  == 'T',
                                                        line[ 7 ]  == 'D', line[ 8 ] == 'C' );
                            break;
                        case 't':   // tag
                        {
                            PoolTags::iterator iter_tag( m_tags.find( line.substr( 2 ) ) );
                            if( iter_tag != m_tags.end() )
                                m_filter_default->set_tag( iter_tag->second );
                            else
                                print_error( "Reference to undefined tag: ", line.substr( 2 ) );
                            break;
                        }
                        case 'b':   // begin date
                            m_filter_default->set_date_begin(
                                    LIFEO::convert_string( line.substr( 2 ) ) );
                            break;
                        case 'e':   // end date
                            m_filter_default->set_date_end(
                                    LIFEO::convert_string( line.substr( 2 ) ) );
                            break;
                    }
                    break;
                case 'o':   // ordinal chapter (topic)
                    ptr2chapter = m_topics->create_chapter(
                            get_db_line_name( line ),
                            fix_pre_1020_date( get_db_line_date( line ) ).m_date );
                    ptr2chapter->set_expanded( line[ 1 ] == 'e' );
                    break;
                case 'd':   // to-do group
                    if( line[ 1 ] == ':' ) // declaration
                    {
                        ptr2chapter = m_groups->create_chapter(
                                get_db_line_name( line ),
                                fix_pre_1020_date( get_db_line_date( line ) ).m_date );
                    }
                    else // options
                    {
                        ptr2chapter->set_expanded( line[ 2 ] == 'e' );
                        if( line[ 3 ] == 'd' )
                            ptr2chapter->set_todo_status( ES::DONE );
                        else if( line[ 3 ] == 'c' )
                            ptr2chapter->set_todo_status( ES::CANCELED );
                        else
                            ptr2chapter->set_todo_status( ES::TODO );
                    }
                    break;
                case 'C':   // chapter category
                    ptr2chapter_ctg = create_chapter_ctg( line.substr( 2 ) );
                    if( line[ 1 ] == 'c' )
                        m_ptr2chapter_ctg_cur = ptr2chapter_ctg;
                    break;
                case 'c':   // chapter
                    if( ptr2chapter_ctg == NULL )
                    {
                        print_error( "No chapter category defined" );
                        break;
                    }
                    ptr2chapter = ptr2chapter_ctg->create_chapter(
                            get_db_line_name( line ),
                            fix_pre_1020_date( get_db_line_date( line ) ).m_date );
                    ptr2chapter->set_expanded( line[ 1 ] == 'e' );
                    break;
                case 'O':   // options
                    m_option_sorting_criteria = line[ 2 ];
                    break;
                case 'l':   // language
                    m_language = line.substr( 2 );
                    break;
                case 'S':   // startup action
                    m_startup_elem_id = LIFEO::convert_string( line.substr( 2 ) );
                    break;
                case 'L':
                    m_last_elem_id = LIFEO::convert_string( line.substr( 2 ) );
                    break;
                default:
                    print_error( "unrecognized line:\n", line );
                    clear();
                    return LIFEO::CORRUPT_FILE;
            }
        }
    }

    // ENTRIES
    while( getline( stream, line ) )
    {
        if( line.size() < 2 )
            continue;

        switch( line[ 0 ] )
        {
            case 'I':
                set_force_id( LIFEO::convert_string( line.substr( 2 ) ) );
                break;
            case 'E':   // new entry
            case 'e':   // trashed
            {
                if( line.size() < 5 )
                    continue;

                Date date( LIFEO::convert_string( line.substr( 4 ) ) );

                entry_new = new Entry( this, fix_pre_1020_date( date ).m_date,
                                       line[ 1 ] == 'f' );

                m_entries[ entry_new->m_date.m_date ] = entry_new;
                add_entry_to_related_chapter( entry_new );
                m_untagged.add_entry( entry_new );

                if( line[ 0 ] == 'e' )
                    entry_new->set_trashed( true );
                if( line[ 2 ] == 'h' )
                    m_filter_default->add_entry( entry_new );
                if( line[ 3 ] == 'd' )
                    entry_new->set_todo_status( ES::DONE );
                else if( line[ 3 ] == 'c' )
                    entry_new->set_todo_status( ES::CANCELED );
                // hidden flag used to denote to do items:
                else if( entry_new->get_date().is_hidden() )
                    entry_new->set_todo_status( ES::TODO );

                flag_first_paragraph = true;
                break;
            }
            case 'D':   // creation & change dates (optional)
                if( entry_new == NULL )
                {
                    print_error( "No entry declared" );
                    break;
                }
                if( line[ 1 ] == 'r' )
                    entry_new->m_date_created = LIFEO::convert_string( line.substr( 2 ) );
                else    // it should be 'h'
                    entry_new->m_date_edited = LIFEO::convert_string( line.substr( 2 ) );
                break;
            case 'T':   // tag
                if( entry_new == NULL )
                    print_error( "No entry declared" );
                else
                {
                    PoolTags::iterator iter_tag( m_tags.find( line.substr( 2 ) ) );
                    if( iter_tag != m_tags.end() )
                    {
                        entry_new->add_tag( iter_tag->second );
                        if( line[ 1 ] == 'T' )
                            entry_new->set_theme_tag( iter_tag->second );
                    }
                    else
                        print_error( "Reference to undefined tag: ", line.substr( 2 ) );
                }
                break;
            case 'l':   // language
                if( entry_new == NULL )
                    print_error( "No entry declared" );
                else
                    entry_new->set_lang( line.substr( 2 ) );
                break;
            case 'P':    // paragraph
                if( entry_new == NULL )
                {
                    print_error( "No entry declared" );
                    break;
                }
                if( flag_first_paragraph )
                {
                    if( line.size() > 2 )
                        entry_new->m_text = line.substr( 2 );
                    entry_new->m_name = entry_new->m_text;
                    flag_first_paragraph = false;
                }
                else
                {
                    entry_new->m_text += "\n";
                    entry_new->m_text += line.substr( 2 );
                }
                break;
            default:
                print_error( "unrecognized line:\n", line );
                clear();
                return LIFEO::CORRUPT_FILE;
        }
    }

    do_standard_checks_after_parse();

    m_filter_active->set( m_filter_default );   // employ the default filter

    upgrade_entries();

    return LIFEO::SUCCESS;
}

LIFEO::Result
Diary::parse_db_body_text_110( std::istream& stream )
{
    std::string         line( "" );
    Entry*              entry_new = NULL;
    CategoryChapters*   ptr2chapter_ctg = NULL;
    Chapter*            ptr2chapter = NULL;
    CategoryTags*       ptr2tag_ctg = NULL;
    Theme*              ptr2theme = NULL;
    Theme*              ptr2default_theme = NULL;
    bool                flag_first_paragraph( false );

    // add tag for system theme
    create_tag( "[ - 0 - ]", NULL )->create_own_theme_duplicating( ThemeSystem::get() );

    // TAG DEFINITIONS & CHAPTERS
    while( getline( stream, line ) )
    {
        if( line[ 0 ] == 0 )    // end of section
            break;
        else if( line.size() >= 3 )
        {
            switch( line[ 0 ] )
            {
                case 'I':
                    set_force_id( LIFEO::convert_string( line.substr( 2 ) ) );
                break;
                case 'T':   // tag category
                    ptr2tag_ctg = create_tag_ctg( line.substr( 2 ) );
                    ptr2tag_ctg->set_expanded( line[ 1 ] == 'e' );
                    break;
                case 't':   // tag
                    create_tag( line.substr( 2 ), ptr2tag_ctg );
                    break;
                case 'C':   // chapter category
                    ptr2chapter_ctg = create_chapter_ctg( line.substr( 2 ) );
                    if( line[ 1 ] == 'c' )
                        m_ptr2chapter_ctg_cur = ptr2chapter_ctg;
                    break;
                case 'o':   // ordinal chapter (topic)
                    ptr2chapter = m_topics->create_chapter(
                            get_db_line_name( line ),
                            fix_pre_1020_date( get_db_line_date( line ) ).m_date );
                    ptr2chapter->set_expanded( line[ 1 ] == 'e' );
                    break;
                case 'c':   // chapter
                    if( ptr2chapter_ctg == NULL )
                    {
                        print_error( "No chapter category defined" );
                        break;
                    }
                    ptr2chapter = ptr2chapter_ctg->create_chapter(
                            get_db_line_name( line ),
                            fix_pre_1020_date( get_db_line_date( line ) ).m_date );
                    ptr2chapter->set_expanded( line[ 1 ] == 'e' );
                    break;
                case 'M':
                    // themes with same name as tags are merged into existing tags
                    ptr2theme = create_tag( line.substr( 2 ), nullptr )->get_own_theme();

                    if( line[ 1 ] == 'd' )
                        ptr2default_theme = ptr2theme;
                    break;
                case 'm':
                    if( ptr2theme == NULL )
                    {
                        print_error( "No theme declared" );
                        break;
                    }
                    parse_theme( ptr2theme, line );
                    break;
                case 'O':   // options
                    if( line.size() < 4 )
                        break;
                    m_option_sorting_criteria = line[ 3 ];
                    break;
                case 'l':   // language
                    m_language = line.substr( 2 );
                    break;
                case 'S':   // startup action
                    m_startup_elem_id = LIFEO::convert_string( line.substr( 2 ) );
                    break;
                case 'L':
                    m_last_elem_id = LIFEO::convert_string( line.substr( 2 ) );
                    break;
                default:
                    print_error( "unrecognized line:\n", line );
                    clear();
                    return LIFEO::CORRUPT_FILE;
            }
        }
    }

    // ENTRIES
    while( getline( stream, line ) )
    {
        if( line.size() < 2 )
            continue;

        switch( line[ 0 ] )
        {
            case 'I':
                set_force_id( LIFEO::convert_string( line.substr( 2 ) ) );
                break;
            case 'E':   // new entry
            case 'e':   // trashed
            {
                Date date( LIFEO::convert_string( line.substr( 2 ) ) );

                entry_new = new Entry( this, fix_pre_1020_date( date ).m_date, line[ 1 ] == 'f' );

                m_entries[ entry_new->m_date.m_date ] = entry_new;
                add_entry_to_related_chapter( entry_new );
                m_untagged.add_entry( entry_new );

                if( line[ 0 ] == 'e' )
                    entry_new->set_trashed( true );

                flag_first_paragraph = true;
                break;
            }
            case 'D':   // creation & change dates (optional)
                if( entry_new == NULL )
                {
                    print_error( "No entry declared" );
                    break;
                }
                if( line[ 1 ] == 'r' )
                    entry_new->m_date_created = LIFEO::convert_string( line.substr( 2 ) );
                else    // it should be 'h'
                    entry_new->m_date_edited = LIFEO::convert_string( line.substr( 2 ) );
                break;
            case 'M':   // themes are converted into tags
            case 'T':   // tag
                if( entry_new == NULL )
                    print_error( "No entry declared" );
                else
                {
                    PoolTags::iterator iter_tag( m_tags.find( line.substr( 2 ) ) );
                    if( iter_tag != m_tags.end() )
                        entry_new->add_tag( iter_tag->second );
                    else
                        print_error( "Reference to undefined tag: ", line.substr( 2 ) );
                }
                break;
            case 'l':   // language
                if( entry_new == NULL )
                    print_error( "No entry declared" );
                else
                    entry_new->set_lang( line.substr( 2 ) );
                break;
            case 'P':    // paragraph
                if( entry_new == NULL )
                {
                    print_error( "No entry declared" );
                    break;
                }
                if( flag_first_paragraph )
                {
                    if( line.size() > 2 )
                        entry_new->m_text = line.substr( 2 );
                    entry_new->m_name = entry_new->m_text;
                    flag_first_paragraph = false;
                }
                else
                {
                    entry_new->m_text += "\n";
                    entry_new->m_text += line.substr( 2 );
                }
                break;
            default:
                print_error( "unrecognized line (110):\n", line );
                clear();
                return LIFEO::CORRUPT_FILE;
        }
    }

    do_standard_checks_after_parse();

    // if default theme is different than the system theme, set the untagged accordingly
    if( ptr2default_theme )
        m_untagged.create_own_theme_duplicating( ptr2default_theme );

    upgrade_entries();

    return LIFEO::SUCCESS;
}

void
Diary::upgrade_entries()
{
    for( EntryIterConst iter = m_entries.begin(); iter != m_entries.end(); ++iter )
    {
        Entry* entry = ( *iter ).second;

        entry->m_date_status = entry->m_date_created; // initialize the status date

        Wchar c;
        Wchar lf( 'n' );
        Ustring check;
        Ustring new_content;

        for( unsigned int i = 0; i < entry->m_text.size(); i++ )
        {
            switch( c = entry->m_text[ i ] )
            {
                case '\n':
                case '\r':
                    new_content += '\n';
                    // no break
                case 0:     // should never be the case
                    lf = 't'; // tab
                    break;
                case '\t':
                    new_content += c;
                    lf = ( lf == 't' || lf == 'c' ) ? 'c' : 'n';
                    break;
                case L'☐':
                    check = "[ ] ";
                    new_content += c;
                    lf = lf == 'c' ? 's' : 'n';
                    break;
                case L'☑':
                    check = "[+] ";
                    new_content += c;
                    lf = lf == 'c' ? 's' : 'n';
                    break;
                case L'☒':
                    check = "[x] ";
                    new_content += c;
                    lf = lf == 'c' ? 's' : 'n';
                    break;
                case ' ':
                    if( lf == 's' )
                    {
                        new_content.erase( new_content.size() - 1, 1 );
                        new_content += check;
                    }
                    else
                        new_content += c;
                    lf = 'n';
                    break;
                default:
                    new_content += c;
                    lf = 'n';
                    break;
            }
        }

        entry->m_text = new_content;
    }
}

inline bool
create_db_header_text( std::stringstream& output, bool encrypted )
{
    output << LIFEO::DB_FILE_HEADER;
    output << "\nV " << LIFEO::DB_FILE_VERSION_INT;
    output << ( encrypted ? "\nE yes" : "\nE no" );
    output << "\n\n"; // end of header

    return true;
}

inline void
create_db_tag_text( const Tag* tag, std::stringstream& output )
{
    char type{ 'm' };

    if( tag->get_type() == DiaryElement::ET_UNTAGGED )
    {
        output << "uc";
        type = 'u';
    }
    else
    {
        output << "ID" << tag->get_id() << "\nt " << tag->get_name_std();
        output << "\ntc";
    }
    output << tag->get_chart_type() << '\n';

    if( ! tag->is_boolean() && ! tag->get_unit().empty() ) // never true for untagged
        output << "tu" << tag->get_unit() << '\n';

    if( tag->get_has_own_theme() )
    {
        Theme* theme( tag->get_theme() );

#ifndef LIFEO_WINDOZE
        output << type << 'f' << theme->font.to_string() << '\n';
        output << type << 'b' << convert_gdkrgba_to_string( theme->color_base ) << '\n';
        output << type << 't' << convert_gdkrgba_to_string( theme->color_text ) << '\n';
        output << type << 'h' << convert_gdkrgba_to_string( theme->color_heading ) << '\n';
        output << type << 's' << convert_gdkrgba_to_string( theme->color_subheading ) << '\n';
        output << type << 'l' << convert_gdkrgba_to_string( theme->color_highlight ) << '\n';
#else
        output << type << 'f' << theme->font << '\n';
        output << type << 'b' << theme->color_base << '\n';
        output << type << 't' << theme->color_text << '\n';
        output << type << 'h' << theme->color_heading << '\n';
        output << type << 's' << theme->color_subheading << '\n';
        output << type << 'l' << theme->color_highlight << '\n';
#endif
    }
}

inline void
create_db_todo_status_text( const DiaryElement* elem, std::stringstream& output )
{
    switch( elem->get_todo_status() )
    {
        case ES::NOT_TODO:
            output << 'n';
            break;
        case ES::TODO:
            output << 't';
            break;
        case ES::PROGRESSED:
            output << 'p';
            break;
        case ES::DONE:
            output << 'd';
            break;
        case ES::CANCELED:
            output << 'c';
            break;
    }
}

inline void
create_db_chapterctg_text( char type, const CategoryChapters* ctg, std::stringstream& output )
{
    Chapter* chapter;

    for( auto& kv_chapter : *ctg )
    {
        chapter = kv_chapter.second;
        output << "ID" << chapter->get_id()
               << "\nC" << type << kv_chapter.first // type + date
               << '\t' << chapter->get_name_std()   // name
               << "\nCp" << ( chapter->get_expanded() ? 'e' : '_' );
        create_db_todo_status_text( chapter, output );
        if( chapter->get_chart_type() & ChartPoints::YEARLY )
            output << 'Y';
        output << '\n';

        if( chapter->get_color() != Color( "White" ) )
            output << "Cc" <<
#ifndef LIFEO_WINDOZE
            convert_gdkrgba_to_string( chapter->get_color() )
#else
            chapter->get_color()
#endif
            << '\n';
    }
}

bool
Diary::create_db_body_text( std::stringstream& output )
{
    // OPTIONS
    output << "O " << m_option_sorting_criteria;
    if( m_chart_type & ChartPoints::YEARLY )
        output << 'Y';
    output << '\n';

    if( !m_language.empty() )
        output << "l " << m_language << '\n';

    // STARTUP ACTION (HOME ITEM)
    output << "S " << m_startup_elem_id << '\n';
    output << "L " << m_last_elem_id << '\n';

    // ROOT TAGS
    for( auto& kv_tag : m_tags )
    {
        if( kv_tag.second->get_category() == NULL )
            create_db_tag_text( kv_tag.second, output );
    }
    // CATEGORIZED TAGS
    for( auto& kv_tag_ctg : m_tag_categories )
    {
        // tag category:
        CategoryTags* ctg( kv_tag_ctg.second );
        output << "ID" << ctg->get_id()
               << "\nT" << ( ctg->get_expanded() ? 'e' : '_' )
               << ctg->get_name_std() << '\n';
        // tags in it:
        for( Tag* tag : *ctg )
        {
            create_db_tag_text( tag, output );
        }
    }
    // UNTAGGED THEME
    create_db_tag_text( &m_untagged, output );

    // TOPICS
    create_db_chapterctg_text( 'O', m_topics, output );

    // FREE CHAPTERS
    create_db_chapterctg_text( 'G', m_groups, output );

    // CHAPTERS
    PRINT_DEBUG( "***CURRENT CHAPTER:", m_ptr2chapter_ctg_cur->get_name() );
    for( auto& kv_cc : m_chapter_categories )
    {
        // chapter category:
        CategoryChapters* ctg( kv_cc.second );
        output << "ID" << ctg->get_id()
               << "\nCC" << ( ctg == m_ptr2chapter_ctg_cur ? 'c' : '_' )
               << ctg->get_name_std() << '\n';
        // chapters in it:
        create_db_chapterctg_text( 'T', ctg, output );
    }

    // FILTER
    const ElemStatus fs( m_filter_default->get_status() );
    output << "fs" << ( fs & ES::SHOW_TRASHED ? 'T' : '_' )
                   << ( fs & ES::SHOW_NOT_TRASHED ? 't' : '_' )
                   << ( fs & ES::SHOW_FAVORED ? 'F' : '_' )
                   << ( fs & ES::SHOW_NOT_FAVORED ? 'f' : '_' )
                   << ( fs & ES::SHOW_NOT_TODO ? 'N' : '_' )
                   << ( fs & ES::SHOW_TODO ? 'T' : '_' )
                   << ( fs & ES::SHOW_PROGRESSED ? 'P' : '_' )
                   << ( fs & ES::SHOW_DONE ? 'D' : '_' )
                   << ( fs & ES::SHOW_CANCELED ? 'C' : '_' )
                   << '\n';
    if( fs & ES::FILTER_TAG )
        output << "ft" << m_filter_default->get_tag()->get_name_std() << '\n';
    if( fs & ES::FILTER_DATE_BEGIN )
        output << "fb" << m_filter_default->get_date_begin() << '\n';
    if( fs & ES::FILTER_DATE_END )
        output << "fe" << m_filter_default->get_date_end() << '\n';

    // END OF SECTION
    output << '\n';

    // ENTRIES
    for( auto& kv_entry : m_entries )
    {
        Entry* entry = kv_entry.second;

        // purge empty entries:
        if( entry->is_empty() ) continue;
        // optionally only save filtered entries:
        else if( entry->get_filtered_out() && m_flag_only_save_filtered ) continue;

        // ENTRY DATE
        output << "ID" << entry->get_id() << '\n'
               << ( entry->is_trashed() ? 'e' : 'E' )
               << ( entry->is_favored() ? 'f' : '_' )
               << ( m_filter_default->is_entry_filtered( entry ) ? 'h' : '_' );
        create_db_todo_status_text( entry, output );
        output << entry->m_date.m_date << '\n';

        output << "Dr" << entry->m_date_created << '\n';
        output << "Dh" << entry->m_date_edited << '\n';
        output << "Ds" << entry->m_date_status << '\n';

        // TAGS
        for( Tag* tag : entry->m_tags )
        {
            output << "T" << ( tag == entry->get_theme_tag() ? 'T' : '_' )
                   << tag->get_name_and_value( entry, true, false ) << '\n';
        }

        // LOCATION
        if( ! entry->m_location.empty() )
            output << "L " << entry->m_location << '\n';

        // LANGUAGE
        if( entry->get_lang() != LANG_INHERIT_DIARY )
            output << "l " << entry->get_lang() << '\n';

        // CONTENT
        if( entry->m_text.empty() )
            output << "\n";
        else
        {
            // NOTE: for some reason, implicit conversion from Glib:ustring...
            // ...fails while substr()ing when LANG=C
            // we might reconsider storing text of entries as std::string.
            // for now we convert entry text to std::string here:
            std::string             content( entry->m_text );
            std::string::size_type  pt_start( 0 ), pt_end( 0 );

            while( true )
            {
                pt_end = content.find( '\n', pt_start );
                if( pt_end == std::string::npos )
                {
                    pt_end = content.size();
                    output << "P " << content.substr( pt_start, content.size() - pt_start )
                           << "\n\n";
                    break; // end of while( true )
                }
                else
                {
                    pt_end++;
                    output << "P " << content.substr( pt_start, pt_end - pt_start );
                    pt_start = pt_end;
                }
            }
        }
    }

    return true;
}

LIFEO::Result
Diary::write()
{
    assert( m_flag_read_only == false );

    // UPGRADE BACKUP
    if( m_read_version != DB_FILE_VERSION_INT )
        copy_file_suffix( m_path, ".", m_read_version );

    // BACKUP THE PREVIOUS VERSION
    if( access( m_path.c_str(), F_OK ) == 0 )
    {
        std::string path_old( m_path + ".~previousversion~" );
        rename( m_path.c_str(), path_old.c_str() );
    }

    // WRITE THE FILE
    Result result( write( m_path ) );

    // DAILY BACKUP SAVES
#if LIFEOGRAPH_DEBUG_BUILD
    if( copy_file_suffix(
            m_path, "." + Date::format_string( Date::get_today(), "YMD", '-' ), -1 ) )
        print_info( "daily backup has been written successfully" );
#endif

    return result;
}

LIFEO::Result
Diary::write( const std::string& path )
{
    m_flag_only_save_filtered = false;

    if( m_passphrase.empty() )
        return write_plain( path );
    else
        return write_encrypted( path );
}

LIFEO::Result
Diary::write_copy( const std::string& path, const std::string& passphrase, bool flag_filtered )
{
    m_flag_only_save_filtered = flag_filtered;

    Result result;

    if( passphrase.empty() )
        result = write_plain( path );
    else
    {
        std::string passphrase_actual( m_passphrase );
        m_passphrase = passphrase;
        result = write_encrypted( path );
        m_passphrase = passphrase_actual;
    }

    return result;
}

LIFEO::Result
Diary::write_txt( const std::string& path, bool flag_filtered_only )
{
    std::ofstream file( path.c_str(), std::ios::out | std::ios::trunc );
    if( ! file.is_open() )
    {
        print_error( "i/o error: ", path );
        return LIFEO::FILE_NOT_WRITABLE;
    }

    // HELPERS
    CategoryChapters* dummy_ctg_orphans = new CategoryChapters( NULL, "" );
    dummy_ctg_orphans->insert( CategoryChapters::value_type( 0, &m_orphans ) );
    CategoryChapters* chapters[] =
            { dummy_ctg_orphans, m_ptr2chapter_ctg_cur, m_topics, m_groups };
    const std::string separator         = "---------------------------------------------\n";
    const std::string separator_favored = "+++++++++++++++++++++++++++++++++++++++++++++\n";
    const std::string separator_thick   = "=============================================\n";
    const std::string separator_chapter = ":::::::::::::::::::::::::::::::::::::::::::::\n";

    // DIARY TITLE
    file << separator_thick << get_filename_base( path ) << '\n' << separator_thick;

    // ENTRIES
    for( int i = 0; i < 4; i++ )
    {
        // CHAPTERS
        for( CategoryChapters::reverse_iterator iter_chapter = chapters[ i ]->rbegin();
             iter_chapter != chapters[ i ]->rend(); ++iter_chapter )
        {
            Chapter* chapter = iter_chapter->second;

            if( !chapter->empty() )
            {
                file << "\n\n" << separator_chapter
                     << chapter->get_date().format_string()
                     << " - " << chapter->get_name() << '\n'
                     << separator_chapter << "\n\n";
            }

            // ENTRIES
            for(  Chapter::reverse_iterator iter_entry = chapter->rbegin();
                  iter_entry != chapter->rend(); ++iter_entry )
            {
                Entry* entry = *iter_entry;

                // PURGE EMPTY OR FILTERED OUT ENTRIES
                if( entry->is_empty() || ( entry->get_filtered_out() && flag_filtered_only ) )
                    continue;

                if( entry->is_favored() )
                    file << separator_favored;
                else
                    file << separator;

                // DATE AND FAVOREDNESS
                file << entry->get_date().format_string();
                if( entry->is_favored() )
                    file << '\n' << separator_favored;
                else
                    file << '\n' << separator;

                // TO-DO STATUS
                switch( entry->get_status() & ES::FILTER_TODO)
                {
                    case ES::TODO:
                        file << "[ ] ";
                        break;
                    case ES::PROGRESSED:
                        file << "[~] ";
                        break;
                    case ES::DONE:
                        file << "[+] ";
                        break;
                    case ES::CANCELED:
                        file << "[X] ";
                        break;
                }

                // CONTENT
                file << entry->get_text();

                // TAGS
                bool first_tag = true;
                for( Tag* tag : entry->m_tags )
                {
                    if( first_tag )
                    {
                        file << "\n\n" << _( "TAGS" ) << ": ";
                        first_tag = false;
                    }
                    else
                        file << ", ";

                    file << tag->get_name_and_value( entry, false, true );
                }

                file << "\n\n";
            }
        }
    }

    file << '\n';

    file.close();

    return SUCCESS;
}

LIFEO::Result
Diary::write_plain( const std::string& path, bool flag_header_only )
{
    // NOTE: ios::binary prevents windows to use \r\n for line ends
    std::ofstream file( path.c_str(), std::ios::binary | std::ios::out | std::ios::trunc );
    if( ! file.is_open() )
    {
        print_error( "i/o error: ", path );
        return LIFEO::COULD_NOT_START;
    }

    std::stringstream output;
    create_db_header_text( output, flag_header_only );
    // header only mode is for encrypted diaries
    if( ! flag_header_only )
    {
        create_db_body_text( output );
    }

    file << output.str();
    file.close();

    return LIFEO::SUCCESS;
}

LIFEO::Result
Diary::write_encrypted( const std::string& path )
{
    // writing header:
    write_plain( path, true );
    std::ofstream file( path.c_str(), std::ios::out | std::ios::app | std::ios::binary );
    if( ! file.is_open() )
    {
        print_error( "i/o error: ", path );
        return LIFEO::COULD_NOT_START;
    }
    std::stringstream output;
    CipherBuffers buf;

    // first char of passphrase for validity checking
    output << m_passphrase[ 0 ] << '\n';
    create_db_body_text( output );

    // encryption
    try {
        size_t size =  output.str().size() + 1;

        LIFEO::Cipher::create_new_key( m_passphrase.c_str(), &buf.salt, &buf.key );

        LIFEO::Cipher::create_iv( &buf.iv );

        buf.buffer = new unsigned char[ size ];
        memcpy( buf.buffer, output.str().c_str(), size );

        LIFEO::Cipher::encrypt_buffer( buf.buffer, size, buf.key, buf.iv );

        file.write( ( char* ) buf.salt, LIFEO::Cipher::cSALT_SIZE );
        file.write( ( char* ) buf.iv, LIFEO::Cipher::cIV_SIZE );
        file.write( ( char* ) buf.buffer, size );
    }
    catch( ... )
    {
        buf.clear();
        return LIFEO::FAILURE;
    }

    file.close();
    buf.clear();
    return LIFEO::SUCCESS;
}

DiaryElement*
Diary::get_element( DEID id ) const
{
    PoolDEIDs::const_iterator iter( m_ids.find( id ) );
    return( iter == m_ids.end() ? NULL : iter->second );
}

DiaryElement*
Diary::get_startup_elem() const
{
    DiaryElement* elem = NULL;

    switch( m_startup_elem_id )
    {
        case HOME_CURRENT_ELEM:
            elem = get_most_current_elem();
            break;
        case HOME_LAST_ELEM:
            elem = get_element( m_last_elem_id );
            break;
        case DEID_UNSET:
            break;
        default:
            elem = get_element( m_startup_elem_id );
            break;
    }

    return ( elem ? elem : d );
}

void
Diary::set_startup_elem( const DEID id )
{
    m_startup_elem_id = id;
}

DiaryElement*
Diary::get_most_current_elem() const
{
    Date::date_t date( Date::get_today() );
    Date::date_t diff1( Date::ORDINAL_FLAG );
    long diff2;
    DiaryElement* elem( NULL );
    bool descending( false );
    for( auto& kv_entry : m_entries )
    {
        Entry* entry( kv_entry.second );

        if( ! entry->get_filtered_out() && ! entry->get_date().is_ordinal() )
        {
            diff2 = entry->get_date_t() - date;
            if( diff2 < 0 ) diff2 *= -1;
            if( static_cast< unsigned long >( diff2 ) < diff1 )
            {
                diff1 = diff2;
                elem = entry;
                descending = true;
            }
            else
            if( descending )
                break;
        }
    }

    if( elem )
        return elem;
    else
        return( const_cast< Diary* >( this ) );
}

DiaryElement*
Diary::get_prev_session_elem() const
{
    return get_element( m_last_elem_id );
}

void
Diary::set_last_elem( const DiaryElement* elem )
{
    m_last_elem_id = elem->get_id();
}

// LOCKS ===========================================================================================
inline bool
Diary::remove_lock()
{
    if( m_path.empty() )
        return false;

    std::string path_lock( m_path + LOCK_SUFFIX );
    if( access( path_lock.c_str(), F_OK ) == 0 )
        remove( path_lock.c_str() );
    return true;
}

// ENTRIES =========================================================================================
Entry*
Diary::add_today()
{
    return create_entry( Date::get_today() );
}

Entry*
Diary::get_entry( const Date::date_t date, bool filtered_too )
{
    EntryIter iter( m_entries.find( date ) );
    if( iter != m_entries.end() )
        if( filtered_too || iter->second->get_filtered_out() == false )
            return iter->second;

    return NULL;
}

Entry*
Diary::get_entry_today()
{
    // FIXME: handle filtered out case
    return get_entry( Date::get_today( 1 ) );  // 1 is the order
}

EntryVector*
Diary::get_entries( Date::date_t date ) // takes pure date
{
    EntryVector* entries = new EntryVector;

    EntryIter iter = m_entries.find( date + 1 );
    if( iter == m_entries.end() )
        return entries; // return empty vector

    for( ; ; --iter )
    {
        if( iter->second->get_date().get_pure() == date )
        {
            if( iter->second->get_filtered_out() == false )
                entries->push_back( iter->second );
        }
        else
            break;
        if( iter == m_entries.begin() )
            break;
    }
    return entries;
}

bool
Diary::get_day_has_multiple_entries( const Date& date_impure )
{
    Date::date_t date = date_impure.get_pure();
    EntryIterConst iter = m_entries.find( date + 2 );
    if( iter == m_entries.end() )
        return false;

    for( ; iter != m_entries.begin(); --iter )
    {
        if( iter->second->get_date().get_pure() == date )
        {
            if( iter->second->get_filtered_out() == false )
                return true;
        }
        else
            break;
    }

    return false;
}

Entry*
Diary::get_entry_next_in_day( const Date& date )
{
    EntryIter entry_1st( m_entries.find( date.get_pure() + 1 ) );
    if( entry_1st == m_entries.end() )
        return NULL;
    EntryIter entry_next( m_entries.find( date.m_date + 1 ) );
    if( entry_next != m_entries.end() )
        return entry_next->second->get_filtered_out() ? NULL : entry_next->second;
    else
    if( date.get_order() > 1 )
        return entry_1st->second->get_filtered_out() ? NULL : entry_1st->second;
    else
        return NULL;
}

Entry*
Diary::get_entry_first()
{
    // return first unfiltered entry
    for( auto& kv_entry : m_entries )
    {
        Entry* entry = kv_entry.second;
        if( entry->get_filtered_out() == false )
            return( entry );
    }
    return nullptr;
}

Entry*
Diary::get_entry_latest() const
{
    for( auto& kv_entry : m_entries )
    {
        if( ! kv_entry.second->get_date().is_ordinal() )
            return kv_entry.second;
    }
    return nullptr;
}

bool
Diary::is_first( Entry const* const entry ) const
{
    return( entry == m_entries.begin()->second );
}

bool
Diary::is_last( Entry const* const entry ) const
{
    return( entry == m_entries.rbegin()->second );
}

Entry*
Diary::create_entry( Date::date_t date, const Ustring& content, bool flag_favorite )
{
    // make it the last entry of its day:
    Date::reset_order_1( date );
    while( m_entries.find( date ) != m_entries.end() )
        ++date;

    Entry* entry = new Entry( this, date, content, flag_favorite );

    m_entries[ date ] = entry;

    add_entry_to_related_chapter( entry );

    m_untagged.add_entry( entry );

    return( entry );
}

bool
Diary::dismiss_entry( Entry* entry )
{
    Date::date_t date = entry->get_date_t();

    // fix startup element:
    if( m_startup_elem_id == entry->get_id() )
        m_startup_elem_id = DEID_DIARY;

    // remove from tags:
    for( Tag* tag : entry->m_tags )
        tag->erase( entry );

    // remove from untagged:
    m_untagged.erase( entry );

    // remove from chapters:
    remove_entry_from_chapters( entry );

    // remove from filters:
    if( m_filter_active->is_entry_filtered( entry ) )
        m_filter_active->remove_entry( entry );
    if( m_filter_default->is_entry_filtered( entry ) )
        m_filter_default->remove_entry( entry );

    // erase entry from map:
    m_entries.erase( date );

    // fix entry order:
    int i = 1;
    for( EntryIter iter = m_entries.find( date + i );
         iter != m_entries.end();
         iter = m_entries.find( date + i ) )
    {
        Entry* entry2fix = iter->second;
        m_entries.erase( iter );
        entry2fix->m_date.m_date--;
        m_entries[ entry2fix->get_date_t() ] = entry2fix;
        ++i;
    }

    delete entry;
    return true;
}

void
Diary::set_entry_date( Entry* entry, const Date& date )
{
    EntryIter iter;
    Date::date_t d( entry->m_date.m_date );
    m_entries.erase( d );

    for( iter = m_entries.find( ++d );
         iter != m_entries.end();
         iter = m_entries.find( ++d ) )
    {
        Entry* entry_shift( iter->second );
        m_entries.erase( d );
        entry_shift->set_date( d - 1 );
        m_entries[ d - 1 ] = entry_shift;
    }

    // find the last entry in the date/order
    d = date.m_date;
    bool flag_replace( false );
    while( m_entries.find( d ) != m_entries.end() )
    {
        flag_replace = true;
        d++;
    }

    if( flag_replace )
    {
        for( iter = m_entries.find( --d );
             d >= date.m_date;
             iter = m_entries.find( --d ) )
        {
            Entry* entry_shift( iter->second );
            m_entries.erase( d );
            entry_shift->set_date( d + 1 );
            m_entries[ d + 1 ] = entry_shift;
        }
    }

    entry->set_date( date.m_date );
    m_entries[ date.m_date ] = entry;
    update_entries_in_chapters(); // date changes require full update
}

bool
Diary::make_free_entry_order( Date& date ) const
{
    date.reset_order_1();
    while( m_entries.find( date.m_date ) != m_entries.end() )
        ++date.m_date;

    return true;    // reserved for bounds checking
}

int
Diary::get_time_span() const
{
    int timespan{ 0 };
    Entry* entry_latest{ get_entry_latest() };
    if( entry_latest )
        timespan = m_entries.rbegin()->second->get_date().
                calculate_days_between( entry_latest->get_date() );
    return timespan;
}

// FILTERING =======================================================================================
void
Diary::set_search_text( const Ustring& text )
{
    m_search_text = text;
    m_filter_active->set_status_outstanding();
}

int
Diary::replace_text( const Ustring& newtext )
{
    Ustring::size_type iter_str;
    const int chardiff = newtext.size() - m_search_text.size();

    Entry* entry;

    for( auto& kv_entry : m_entries )
    {
        entry = kv_entry.second;
        if( entry->get_filtered_out() )
            continue;
#ifndef LIFEO_WINDOZE
        Ustring entrytext = entry->get_text().lowercase();
#else
        Ustring entrytext = entry->get_text();
#endif
        iter_str = 0;
        int count = 0;
        while( ( iter_str = entrytext.find( m_search_text, iter_str ) ) != std::string::npos )
        {
            entry->get_text().erase(
                iter_str + ( chardiff * count ), m_search_text.size() );
            entry->get_text().insert(
                iter_str + ( chardiff * count ), newtext );
            count++;
            iter_str += m_search_text.size();
        }
    }

    return 0;   // reserved
}

// TAGS ============================================================================================
Tag*
Diary::create_tag( const Ustring& name, CategoryTags* category, int chart_type )
{
    PoolTags::iterator iter = m_tags.find( name );
    if( iter != m_tags.end() )
    {
        PRINT_DEBUG( "Tag already exists: " + name );
        return( iter->second );
    }
    Tag* tag( new Tag( this, name, category, chart_type ) );
    m_tags.insert( PoolTags::value_type( name, tag ) );
    return tag;
}

void
Diary::dismiss_tag( Tag* tag, bool flag_dismiss_associated )
{
    // fix or dismiss associated entries
    if( flag_dismiss_associated )
    {
        while( ! tag->empty() )
            dismiss_entry( ( * tag->begin() ).first );
    }
    else
    {
        // we have to work on a copy because the original list is modified in the loop
        std::vector< Entry* > entries;
        for( auto& kv_entry : *tag )
            entries.push_back( kv_entry.first );
        for( Entry* entry : entries )
            entry->remove_tag( tag );
    }

    // remove from category if any
    if( tag->get_category() != NULL )
        tag->get_category()->erase( tag );

    // clear filters if necessary
    if( tag == m_filter_active->get_tag() )
        m_filter_active->set_tag( NULL );
    if( tag == m_filter_default->get_tag() )
        m_filter_default->set_tag( NULL );

    m_tags.erase( tag->get_name() );
    delete tag;
}

CategoryTags*
Diary::create_tag_ctg()
{
    Ustring name = create_unique_name_for_map( m_tag_categories,
                                               _( STRING::NEW_CATEGORY_NAME ) );
    CategoryTags* new_category = new CategoryTags( this, name );
    m_tag_categories.insert( PoolCategoriesTags::value_type( name, new_category ) );

    return new_category;
}

CategoryTags*
Diary::create_tag_ctg( const Ustring& name )  // used while reading diary file
{
    try
    {
        CategoryTags* new_category = new CategoryTags( this, name );
        m_tag_categories.insert( PoolCategoriesTags::value_type( name, new_category ) );
        return new_category;
    }
    catch( std::exception &ex )
    {
        throw LIFEO::Error( ex.what() );
    }
}

void
Diary::dismiss_tag_ctg( CategoryTags* ctg, bool flag_dismiss_contained )
{
    // fix or dismiss contained tags
    if( flag_dismiss_contained )
    {
        while( ! ctg->empty() )
            dismiss_tag( * ctg->begin() );
    }
    else
    {
        for( Tag* tag : *ctg )
            tag->set_category( NULL );
    }

    // remove from the list and delete
    m_tag_categories.erase( ctg->get_name() );
    delete ctg;
}

// CHAPTERS ========================================================================================
CategoryChapters*
Diary::create_chapter_ctg()
{
    Ustring name = create_unique_name_for_map( m_chapter_categories,
                                               _( STRING::NEW_CATEGORY_NAME ) );
    CategoryChapters* category = new CategoryChapters( this, name );
    m_chapter_categories.insert( PoolCategoriesChapters::value_type( name, category ) );

    return category;
}

CategoryChapters*
Diary::create_chapter_ctg( const Ustring& name )
{
    // name's availability must be checked beforehand
    try
    {
        CategoryChapters* category = new CategoryChapters( this, name );
        m_chapter_categories.insert( PoolCategoriesChapters::value_type( name, category ) );
        return category;
    }
    catch( std::exception& ex )
    {
        throw LIFEO::Error( ex.what() );
    }
}

void
Diary::dismiss_chapter_ctg( CategoryChapters* category )
{
    if( m_chapter_categories.size() < 2 )
        return;

    if( category == m_ptr2chapter_ctg_cur )
        m_ptr2chapter_ctg_cur = m_chapter_categories.begin()->second;

    m_chapter_categories.erase( category->get_name() );
    delete category;
}

bool
Diary::rename_chapter_ctg( CategoryChapters* category, const Ustring& new_name )
{
    if( m_chapter_categories.count( new_name ) > 0 )
        return false;

    m_chapter_categories.erase( category->get_name() );
    category->set_name( new_name );
    m_chapter_categories.insert(
            PoolCategoriesChapters::value_type( new_name, category ) );

    return true;
}

void
Diary::dismiss_chapter( Chapter* chapter, bool flag_dismiss_contained )
{
    if( chapter->is_ordinal() ) // topic or group
    {
        CategoryChapters* ptr2ctg( chapter->get_date().is_hidden() ? m_groups : m_topics );

        // ITERATORS
        CategoryChapters::reverse_iterator iter_c( ptr2ctg->find( chapter->get_date_t() ) );
        CategoryChapters::reverse_iterator iter_n( iter_c );
        iter_c--;   // fix the order

        if( iter_c == ptr2ctg->rend() )
        {
            print_error( "chapter could not be found in assumed category" );
            return;
        }

        const bool flag_erasing_oldest_cpt(
                iter_n == ptr2ctg->rend() && iter_c != ptr2ctg->rbegin() );

        // CALCULATE THE LAST ENTRY DATE
        Date::date_t last_entry_date;
        // last entry date is taken from previous chapter's last entry when
        // the chapter to be deleted is the last chapter
        if( flag_erasing_oldest_cpt )
        {
            CategoryChapters::reverse_iterator iter_p( iter_c );
            iter_p--;

            // use the chapters date if it does not contain any entry
            last_entry_date = iter_p->second->empty() ? iter_p->second->get_date_t()
                    : ( * iter_p->second->begin() )->get_date_t();
        }
        else
        {
            last_entry_date = iter_c->second->empty() ? iter_c->second->get_date_t()
                    : ( * iter_c->second->begin() )->get_date_t();
        }

        // SHIFT ENTRY DATES
        bool flag_first( true );
        for( CategoryChapters::reverse_iterator iter = iter_c; iter != ptr2ctg->rend(); ++iter )
        {
            Chapter* chpt( iter->second );
            bool flag_neighbor( chpt->get_date_t() == chapter->get_date_t()
                                + Date::ORDINAL_STEP  );

            for( Chapter::iterator iter2 = chpt->begin(); iter2 != chpt->end(); ++iter2 )
            {
                if( flag_first && flag_dismiss_contained )
                    dismiss_entry( *iter2 );
                else if( !flag_first || flag_erasing_oldest_cpt )
                {
                    Entry* entry( *iter2 );
                    m_entries.erase( entry->get_date_t() );
                    if( !flag_dismiss_contained && ( flag_neighbor || flag_erasing_oldest_cpt ) )
                        entry->set_date( entry->get_date().get_order() + last_entry_date );
                    else
                        entry->set_date( entry->get_date_t() - Date::ORDINAL_STEP );
                    m_entries[ entry->get_date_t() ] = entry;
                }
            }
            flag_first = false;
        }

        // COPY AND SHIFT CHAPTER DATES
        std::vector< Chapter* > tmp_chapter_storage;
        for( CategoryChapters::reverse_iterator iter = iter_n; iter != ptr2ctg->rend(); ++iter )
        {
            Chapter* chpt( iter->second );
            tmp_chapter_storage.push_back( chpt );

            chpt->set_date( chpt->get_date_t() - Date::ORDINAL_STEP );
        }

        // ERASE STARTING FROM THE ONE TO BE ACTUALLY DISMISSED
        ptr2ctg->erase( ptr2ctg->begin(), iter_c.base() );

        // ADD BACK ALL BUT THE ACTUAL ONE
        for( Chapter* chpt : tmp_chapter_storage )
        {
            ( *ptr2ctg )[ chpt->get_date_t() ] = chpt;
        }
    }
    else // TEMPORAL CHAPTER
    {
        CategoryChapters::iterator iter(
                m_ptr2chapter_ctg_cur->find( chapter->get_date_t() ) );
        if( iter == m_ptr2chapter_ctg_cur->end() )
        {
            print_error( "chapter could not be found in assumed category" );
            return;
        }
        else if( ( ++iter ) != m_ptr2chapter_ctg_cur->end() )  // fix time span
        {
            Chapter *chapter_earlier( iter->second );
            if( chapter->get_time_span() > 0 )
                chapter_earlier->set_time_span(
                        chapter_earlier->get_time_span() + chapter->get_time_span() );
            else
                chapter_earlier->set_time_span( 0 );
        }

        if( flag_dismiss_contained )
        {
            for( Entry* entry : *chapter )
                dismiss_entry( entry );
        }

        m_ptr2chapter_ctg_cur->erase( chapter->get_date_t() );
    }

    delete chapter;
    update_entries_in_chapters();
}

void
Diary::update_entries_in_chapters()
{
    PRINT_DEBUG( "Diary::update_entries_in_chapters()" );

     Chapter*   chapter;
     EntryIter  itr_entry   = m_entries.begin();
     Entry*     entry       = itr_entry != m_entries.end() ? itr_entry->second : NULL;
     CategoryChapters* chapters[ 3 ] = { m_topics, m_groups, m_ptr2chapter_ctg_cur };

     for( int i = 0; i < 3; i++ )
     {
         for( auto& kv_chapter : *chapters[ i ] )
         {
             chapter = kv_chapter.second;
             chapter->clear();

             if( entry == NULL )
                 continue;

             while( entry->get_date() > chapter->get_date() )
             {
                 chapter->insert( entry );

                 if( ++itr_entry == m_entries.end() )
                 {
                     entry = NULL;
                     break;
                 }
                 else
                     entry = itr_entry->second;
             }
         }
     }

     m_orphans.clear();

     if( entry != NULL )
     {
         for( ; itr_entry != m_entries.end(); ++itr_entry )
         {
             entry = itr_entry->second;

             m_orphans.insert( entry );
         }

         m_orphans.set_date( entry->get_date_t() );
     }
     else
         m_orphans.set_date( Date::DATE_MAX );
}

void
Diary::add_entry_to_related_chapter( Entry* entry )
{
    // NOTE: works as per the current listing options needs to be updated when something
    // changes the arrangement such as a change in the current chapter category

    CategoryChapters* ptr2ctg;

    if( entry->get_date().is_ordinal() ) // in groups or topics
        ptr2ctg = ( entry->get_date().is_hidden() ? m_groups : m_topics );
    else // in chapters
        ptr2ctg = m_ptr2chapter_ctg_cur;

    for( auto& kv_chapter : *ptr2ctg )
    {
        Chapter* chapter( kv_chapter.second );

        if( entry->get_date() > chapter->get_date() )
        {
            chapter->insert( entry );
            return;
        }
    }

    // if does not belong to any of the defined chapters:
    m_orphans.insert( entry );
    if( entry->m_date.m_date < m_orphans.get_date().m_date )
        m_orphans.set_date( entry->m_date.m_date );
}

void
Diary::remove_entry_from_chapters( Entry* entry )
{
    CategoryChapters* ptr2ctg;

    if( entry->get_date().is_ordinal() ) // in groups or topics
        ptr2ctg = ( entry->get_date().is_hidden() ? m_groups : m_topics );
    else // in chapters
        ptr2ctg = m_ptr2chapter_ctg_cur;

    for( auto& kv_chapter : *ptr2ctg )
    {
        Chapter* chapter( kv_chapter.second );

        if( chapter->find( entry ) != chapter->end() )
        {
            chapter->erase( entry );
            return;
        }
    }

    // if does not belong to any of the defined chapters:
    m_orphans.erase( entry );
}

void
Diary::set_topic_order( Chapter* chapter, Date::date_t date )
{
    assert( chapter->is_ordinal() );

    CategoryChapters* ptr2ctg( chapter->get_date().is_hidden() ? m_groups : m_topics );

    long step( chapter->get_date_t() > date ? -Date::ORDINAL_STEP : Date::ORDINAL_STEP );
    EntryVector ev;

    Date::date_t d_e( chapter->get_date_t() + 1 );
    EntryIter iter_entry( m_entries.find( d_e ) );

    while( iter_entry != m_entries.end() )
    {
        Entry* entry( iter_entry->second );

        m_entries.erase( d_e );
        ev.push_back( entry );

        iter_entry = m_entries.find( ++d_e );
    }

    ptr2ctg->erase( chapter->get_date_t() );

    // SHIFT TOPICS
    for( Date::date_t d = chapter->get_date_t() + step; d != ( date + step ); d += step )
    {
        CategoryChapters::iterator iter( ptr2ctg->find( d ) );
        if( iter == ptr2ctg->end() )
            break;
        Chapter* chpt( iter->second );
        ptr2ctg->erase( d );
        chpt->set_date( d - step );
        ( *ptr2ctg )[ d - step ] = chpt;

        d_e = d + 1;
        iter_entry = m_entries.find( d_e );
        while( iter_entry != m_entries.end() )
        {
            Entry* entry( iter_entry->second );

            m_entries.erase( d_e );
            entry->set_date( d_e - step );
            m_entries[ d_e - step ] = entry;

            iter_entry = m_entries.find( ++d_e );
        }
    }

    chapter->set_date( date );
    ( *ptr2ctg )[ date ] = chapter;
    for( Entry* entry : ev )
    {
        entry->set_date( ++date );
        m_entries[ date ] = entry;
    }
}

/*Date Diary::get_free_chapter_order_temporal()
{
    time_t t = time( NULL );
    struct tm* ti = localtime( &t );
    Date date( ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday );

    while( m_ptr2chapter_ctg_cur->get_chapter( date.m_date ) )
    {
        date.forward_day();
    }

    return date;
}*/

// SHOW ============================================================================================
void
Diary::fill_up_chart_points( ChartPoints* cp ) const
{
    if( !cp )
        return;

    if( cp->get_span() < 1 )
        return;

    for( auto kv_chapter = m_ptr2chapter_ctg_cur->rbegin();
         kv_chapter != m_ptr2chapter_ctg_cur->rend(); ++kv_chapter )
    {
        Date d_chapter{ kv_chapter->second->get_date() };
        float pos{ float( cp->calculate_distance_neg( Date( cp->start_date ), d_chapter ) ) };
        if( cp->type & ChartPoints::MONTHLY )
            pos += ( float( d_chapter.get_day() - 1.0 ) / d_chapter.get_days_in_month() );
        else
            pos += ( float( d_chapter.get_month() - 1.0 ) / 12.0 );

        cp->chapters.push_back(
                ChartPoints::PairChapter( pos, kv_chapter->second->get_color() ) );
    }

    // dummy entry just to have the last chapter drawn:
    cp->chapters.push_back( ChartPoints::PairChapter( FLT_MAX, Color( "#000000" ) ) );
}

ChartPoints*
Diary::create_chart_data() const
{
    if( m_entries.empty() )
        return nullptr;

    ChartPoints* cp{ new ChartPoints( m_chart_type ) };
    Date d_last{ Date::NOT_SET };

    for( auto iter = m_entries.rbegin(); iter != m_entries.rend(); ++iter )
        cp->add_plain( d_last, iter->second->get_date() );

    fill_up_chart_points( cp );

    return cp;
}

void
Diary::show()
{
    if( shower != NULL )
        shower->show( *this );
}

// IMPORTING =======================================================================================
bool
Diary::import_tag( Tag* tag )
{
    Tag* new_tag( create_tag( tag->get_name(), nullptr ) );
    if( new_tag == NULL )
        return false;

    if( tag->get_has_own_theme() )
        new_tag->create_own_theme_duplicating( tag->get_theme() );

    return true;
}

bool
Diary::import_entries( const Diary& diary,
                       bool flag_import_tags,
                       const Ustring& tag_all_str )
{
    Entry* entry_new;
    Entry* entry_ext;
    Tag* tag_all = NULL;

    if( ! tag_all_str.empty() )
        tag_all = create_tag( tag_all_str, nullptr );

    for( EntryIterConstRev iter = diary.m_entries.rbegin();
         iter != diary.m_entries.rend();
         ++iter )
    {
        entry_ext = iter->second;
        entry_new = new Entry( this,
                               entry_ext->m_date.m_date,
                               entry_ext->m_text,
                               entry_ext->is_favored() );

        // fix order:
        entry_new->m_date.reset_order_1();
        while( m_entries.find( entry_new->m_date.m_date ) != m_entries.end() )
            entry_new->m_date.m_date++;

        // copy other data:
        entry_new->m_date_created = entry_ext->m_date_created;
        entry_new->m_date_edited = entry_ext->m_date_edited;
        entry_new->m_date_status = entry_ext->m_date_status;

        // insert it into the diary:
        m_entries[ entry_new->m_date.m_date ] = entry_new;
        add_entry_to_related_chapter( entry_new );

        if( flag_import_tags )
        {
            Tag* tag;

            for( Tag* tag_ext : entry_ext->m_tags )
            {
                tag = ( m_tags.find( tag_ext->get_name() ) )->second;
                entry_new->add_tag( tag );
            }

            // preserve the theme:
            if( entry_ext->get_theme_is_set() )
            {
                tag = ( m_tags.find( entry_ext->get_theme_tag()->get_name() ) )->second;
                entry_new->set_theme_tag( tag );
            }
        }
        if( tag_all )
            entry_new->add_tag( tag_all );

        // add to untagged if that is the case:
        if( entry_new->get_tags().empty() )
            m_untagged.add_entry( entry_new );
    }

    return true;
}

bool
Diary::import_chapters( const Diary& diary )
{
    for( auto& kv_cc : diary.m_chapter_categories )
    {
        CategoryChapters* cc( kv_cc.second );
        CategoryChapters* cc_new = create_chapter_ctg(
                create_unique_name_for_map( m_chapter_categories, cc->get_name() ) );

        for( auto& kv_chapter : *cc )
        {
            Chapter* chapter( kv_chapter.second );
            cc_new->create_chapter( chapter->get_name(), chapter->get_date().m_date );
        }
    }

    return true;
}

