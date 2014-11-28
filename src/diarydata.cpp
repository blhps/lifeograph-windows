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


#include "diarydata.hpp"
#include "lifeograph.hpp"
#include "strings.hpp"

#include <cassert>


using namespace LIFEO;


bool
LIFEO::compare_listitems( DiaryElement* item_l, DiaryElement* item_r )
{
    return( item_l->get_date() > item_r->get_date() );
}

bool
LIFEO::compare_listitems_by_name( DiaryElement* item_l, DiaryElement* item_r )
{
    return( item_l->get_name() < item_r->get_name() );
}

bool
LIFEO::compare_names( const Ustring& name_l, const Ustring& name_r )
{
    return( name_l < name_r );
}

// DIARYELEMENT ====================================================================================
// STATIC MEMBERS
#ifndef LIFEO_WINDOZE
ListData::Colrec*                   ListData::colrec;
#endif
bool                                DiaryElement::FLAG_ALLOCATE_GUI_FOR_DIARY = true;
const Icon                          DiaryElement::s_pixbuf_null = NULL;
const Ustring DiaryElement::s_type_names[] =
{   "", _( "Tag" ), _( "Untagged" ), _( "Tag Category" ), _( "Chapter Category" ),
    // TRANSLATORS: filter is a noun here
    _( "Filter" ),
    _( "Diary" ), _( "Dated Chapter" ), _( "Numbered Chapter" ),
    _( "Free Chapter" ), _( "Entry" ), "Multiple Entries", "", ""
};

DiaryElement::DiaryElement()
:   NamedElement( "" ), m_list_data( NULL ), m_ptr2diary( NULL ), m_id( DEID_UNSET ),
    m_status( ES::VOID_ )
{
}
DiaryElement::DiaryElement( Diary* const ptr2diary,
                            const Ustring& name,
                            ElemStatus status )
:   NamedElement( name ), m_ptr2diary( ptr2diary ),
    m_id( ptr2diary ? ptr2diary->create_new_id( this ) :  DEID_UNSET ),
    m_status( status )
{
#ifndef LIFEO_WINDOZE
    if( FLAG_ALLOCATE_GUI_FOR_DIARY )
        m_list_data = new ListData;
#endif
}
DiaryElement::DiaryElement( Diary* const ptr2diary, DEID id, ElemStatus status )
:   NamedElement( "" ), m_ptr2diary( ptr2diary ), m_id( id ), m_status( status )
{
#ifndef LIFEO_WINDOZE
    if( FLAG_ALLOCATE_GUI_FOR_DIARY )
        m_list_data = new ListData;
#endif
}

DiaryElement::~DiaryElement()
{
    if( m_ptr2diary != NULL )
        m_ptr2diary->erase_id( m_id );
}

void
DiaryElement::set_todo_status( ElemStatus s )
{
    m_status -= ( m_status & ES::FILTER_TODO );
    m_status |= s;
}

// TAG =============================================================================================
// STATIC MEMBERS
ElementShower< Tag >* Tag::shower( NULL );

Tag::Tag( Diary* const d, const Ustring& name, CategoryTags* category )
:   DiaryElementReferrer( d, name, ES::VOID_ ), m_ptr2category( category ), m_theme( NULL )
{
    if( category != NULL )
        category->insert( this );
}

Tag::~Tag()
{
}

const Icon&
Tag::get_icon() const
{
    return( get_has_own_theme() ? Lifeograph::icons->tag_theme_16 : Lifeograph::icons->tag_16 );
}

const Icon&
Tag::get_icon32() const
{
    return( get_has_own_theme() ? Lifeograph::icons->tag_theme_32 : Lifeograph::icons->tag_32 );
}

void
Tag::show()
{
    if( shower != NULL )
        shower->show( *this );
    else
        PRINT_DEBUG( "Tag has no graphical data!" );
}

void
Tag::set_name( const Ustring& name )
{
    m_name = name;
}

void
Tag::set_category( CategoryTags* category_new )
{
    if( m_ptr2category )
        m_ptr2category->erase( this );
    if( category_new )
        category_new->insert( this );
    m_ptr2category = category_new;
}

Theme*
Tag::get_theme() const
{
    return( m_theme ? m_theme : ThemeSystem::get() );
}
Theme*
Tag::get_own_theme()
{
    if( m_theme == NULL )
    {
        m_theme = new Theme;

        for( Set::iterator iter = begin(); iter != end(); ++iter )
        {
            Entry* entry = *iter;
            entry->update_theme();
        }
    }

    return m_theme;
}

Theme*
Tag::create_own_theme_duplicating( const Theme* theme )
{
    if( m_theme != NULL )
        delete m_theme;

    m_theme = new Theme( theme );

    for( Set::iterator iter = begin(); iter != end(); ++iter )
    {
        Entry* entry = *iter;
        entry->update_theme();
    }

    return m_theme;
}

void
Tag::reset_theme()
{
    if( m_theme )
    {
        delete m_theme;
        m_theme = NULL;

        for( Set::const_iterator iter = begin(); iter != end(); ++iter )
        {
            Entry* entry = *iter;
            entry->update_theme();
        }
    }
}

Untagged::Untagged()
:   Tag( NULL, _( "Untagged" ), NULL )
{

}

Ustring
Untagged::get_list_str() const
{
#ifndef LIFEO_WINDOZE
    return( "<i>" + Glib::Markup::escape_text( m_name ) + "</i>" );
#else
    return( m_name ); // FIXME
#endif
}

const Icon&
Untagged::get_icon() const
{
    return Lifeograph::icons->untagged_16;
}

const Icon&
Untagged::get_icon32() const
{
    return Lifeograph::icons->untagged_32;
}

// TAGPOOL =========================================================================================
PoolTags::~PoolTags()
{
    for( iterator iter = begin(); iter != end(); ++iter )
        delete iter->second;
}

Tag*
PoolTags::get_tag( unsigned int tagorder )
{
    if( tagorder >= size() )
        return NULL;

    const_iterator iter = begin();
    advance( iter, tagorder );
    return iter->second;
}

Tag*
PoolTags::get_tag( const Ustring& name )
{
    iterator iter = find( name );
    if( iter != end() )
        return( iter->second );
    else
        return NULL;
}

bool
PoolTags::rename( Tag* tag, const Ustring& new_name )
{
    erase( tag->m_name );
    tag->m_name = new_name;
    return( insert( value_type( new_name, tag ) ).second );
}

void
PoolTags::clear()
{
    for( iterator iter = begin(); iter != end(); ++iter )
        delete iter->second;

    std::map< Ustring, Tag*, FuncCompareStrings >::clear();
}

// CATEGORYTAGS ====================================================================================
// STATIC MEMBERS
ElementShower< CategoryTags >* CategoryTags::shower( NULL );

CategoryTags::CategoryTags( Diary* const d, const Ustring& name )
:   DiaryElementReferrer( d, name, ES::EXPANDED, compare_listitems_by_name )
{
}

const Icon&
CategoryTags::get_icon() const
{
    return Lifeograph::icons->tag_category_16;
}
const Icon&
CategoryTags::get_icon32() const
{
    return Lifeograph::icons->tag_category_32;
}

void
CategoryTags::show()
{
    if( shower != NULL )
        shower->show( *this );
    else
        PRINT_DEBUG( "Category has no graphical data!" );
}

// POOL OF DEFINED TAG CATEGORIES
PoolCategoriesTags::PoolCategoriesTags()
:   std::map< Ustring, CategoryTags*, FuncCompareStrings >( compare_names )
{
}

PoolCategoriesTags::~PoolCategoriesTags()
{
    for( iterator iter = begin(); iter != end(); ++iter )
    {
        delete iter->second;
    }
}

void
PoolCategoriesTags::clear()
{
    for( iterator iter = begin(); iter != end(); ++iter )
    {
        delete iter->second;
    }

    std::map< Ustring, CategoryTags*, FuncCompareStrings >::clear();
}

bool
PoolCategoriesTags::rename_category( CategoryTags* category, const Ustring& new_name )
{
    erase( category->m_name );
    category->m_name = new_name;
    return( insert( value_type( new_name, category ) ).second );
}

// TAGSET
Tagset::~Tagset()
{
}

bool
Tagset::add( Tag* tag )
{
    if( insert( tag ).second ) // if did not exist previously
        return true;
    else
    {
        PRINT_DEBUG( " tagset already has the tag " + tag->get_name() );
        return false;
    }

}

bool
Tagset::checkfor_member( const Tag* tag ) const
{
    return( count( const_cast< Tag* >( tag ) ) > 0 );
}

const Tag*
Tagset::get_tag( unsigned int tagorder ) const
{
    unsigned int i = 0;

    for( Tagset::const_iterator iter = this->begin(); iter != this->end(); ++iter )
    {
        if( i == tagorder )
        {
            return( *iter );
        }
        i++;
    }
    return NULL;
}

// CHAPTER =========================================================================================
// STATIC MEMBERS
ElementShower< Chapter >* Chapter::shower( NULL );

Chapter::Chapter( Diary* const d, const Ustring& name, Date::date_t date )
:   DiaryElementReferrer( d, name, ES::CHAPTER_DEFAULT ), m_date_begin( date ), m_time_span( 0 )
{
    update_type();
}

const Icon&
Chapter::get_icon() const
{
    switch( m_status & ES::FILTER_TODO )
    {
        case ES::TODO:
            return Lifeograph::icons->todo_open_16;
        case ES::PROGRESSED:
            return Lifeograph::icons->todo_progressed_16;
        case ES::DONE:
            return Lifeograph::icons->todo_done_16;
        case ES::CANCELED:
            return Lifeograph::icons->todo_canceled_16;
        default:
            return( m_date_begin.is_ordinal() ?
                    Lifeograph::icons->chapter_16 : Lifeograph::icons->month_16 );
    }
}
const Icon&
Chapter::get_icon32() const
{
    switch( m_status & ES::FILTER_TODO )
    {
        case ES::TODO:
            return Lifeograph::icons->todo_open_32;
        case ES::PROGRESSED:
            return Lifeograph::icons->todo_progressed_32;
        case ES::DONE:
            return Lifeograph::icons->todo_done_32;
        case ES::CANCELED:
            return Lifeograph::icons->todo_canceled_32;
        default:
            return( m_date_begin.is_ordinal() ?
                    Lifeograph::icons->chapter_32 : Lifeograph::icons->month_32 );
    }

}

Ustring
Chapter::get_list_str() const
{
    if( m_date_begin.is_hidden() )
#ifndef LIFEO_WINDOZE
        return Glib::ustring::compose( "<b>%1</b>", Glib::Markup::escape_text( m_name ) );
//    else
        return Glib::ustring::compose( "<b>%1 -  %2</b>", m_date_begin.format_string(),
                    Glib::Markup::escape_text( m_name ) );
#else // FIXME
        return m_name;
//    else
        return STR::compose( m_date_begin.format_string(), m_name );
#endif
}

void
Chapter::show()
{
    if( shower != NULL )
        shower->show( *this );
    else
        PRINT_DEBUG( "Chapter has no graphical data!" );

}

void
Chapter::set_name( const Ustring& name )
{
    m_name = name;
}

Ustring
Chapter::get_date_str() const
{
    return m_date_begin.format_string();
}

Date
Chapter::get_date() const
{
    return m_date_begin;
}

void
Chapter::update_type()
{
    if( m_date_begin.is_hidden() )
        m_type = ET_GROUP;
    else if( m_date_begin.is_ordinal() )
        m_type = ET_TOPIC;
    else
        m_type = ET_CHAPTER;
}

Date
Chapter::get_free_order() const
{
    Date date( m_date_begin );
    m_ptr2diary->make_free_entry_order( date );
    return date;
}

void
Chapter::recalculate_span( const Chapter* next )
{
    if( next == NULL )
        m_time_span = 0;    // unlimited
    else if( next->m_date_begin.is_ordinal() )
        m_time_span = 0;    // last temporal chapter: unlimited
    else
        m_time_span = m_date_begin.calculate_days_between( next->m_date_begin );
}

// CHAPTER CATEGORY ================================================================================
CategoryChapters::CategoryChapters( Diary* const d, const Ustring& name )
:   DiaryElement( d, name ),
    std::map< Date::date_t, Chapter*, FuncCompareDates >( compare_dates ),
    m_date_min( 0 )
{
}
CategoryChapters::CategoryChapters( Diary* const d, Date::date_t date_min )
:   DiaryElement( d, DEID_UNSET ),
    std::map< Date::date_t, Chapter*, FuncCompareDates >( compare_dates ),
    m_date_min( date_min )
{
}

CategoryChapters::~CategoryChapters()
{
    for( CategoryChapters::iterator iter = begin(); iter != end(); ++iter )
        delete iter->second;
}

Chapter*
CategoryChapters::get_chapter( const Date::date_t date ) const
{
    const_iterator iter( find( date ) );
    return( iter == end() ? NULL : iter->second );
}

Chapter*
CategoryChapters::create_chapter( const Ustring& name, const Date& date )
{
    Chapter* chapter = new Chapter( m_ptr2diary, name, date.m_date );

    add( chapter );

    return chapter;
}
Chapter*
CategoryChapters::create_chapter_ordinal( const Ustring& name )
{
    return create_chapter( name, get_free_order_ordinal() );
}

bool
CategoryChapters::set_chapter_date( Chapter* chapter, Date::date_t date )
{
    assert( chapter->is_ordinal() == false );

    if( chapter->m_date_begin.m_date != Date::NOT_SET )
    {
        iterator iter( find( chapter->m_date_begin.m_date ) );
        if( iter == end() )
            return false; // chapter is not a member of the set

        if( ( ++iter ) != end() ) // fix time span
        {
            Chapter* chapter_earlier( iter->second );
            if( chapter->m_time_span > 0 )
                chapter_earlier->m_time_span += chapter->m_time_span;
            else
                chapter_earlier->m_time_span = 0;
        }

        erase( chapter->m_date_begin.m_date );

        if( find( date ) != end() ) // if target is taken move chapters
        {
            // FIXME "chapter->m_date_begin.is_ordinal() ?" conflicts with the assertion above:
            Date::date_t d( chapter->m_date_begin.is_ordinal() ?
                            chapter->m_date_begin.m_date : get_free_order_ordinal().m_date );
                            // if a temporal chapter is being converted..
                            // ..assume that it was the last
            Date::date_t step( chapter->m_date_begin.m_date > date ?
                                        -Date::ORDINAL_STEP : Date::ORDINAL_STEP );

            for( d += step; ; d += step )
            {
                iter = find( d );
                if( iter == end() )
                    break;

                Chapter *chapter_shift( iter->second );
                erase( d );
                chapter_shift->set_date( d - step );
                insert( CategoryChapters::value_type( chapter_shift->m_date_begin.m_date,
                                                      chapter_shift ) );

                if( d == date )
                    break;
            }
        }
    }

    chapter->set_date( date );

    add( chapter );

    return true;
}

bool
CategoryChapters::add( Chapter* chapter )
{
    iterator iter = insert( CategoryChapters::value_type(
            chapter->m_date_begin.m_date, chapter ) ).first;

    if( iter == begin() ) // latest
        chapter->recalculate_span( NULL );
    else
    {
        iterator iter_next( iter );
        iter_next--;
        chapter->recalculate_span( iter_next->second );
    }

    if( ( ++iter ) != end() ) // if not earliest fix previous
        iter->second->recalculate_span( chapter );

    return true; // reserved
}

void
CategoryChapters::clear()
{
    for( CategoryChapters::iterator iter = begin(); iter != end(); ++iter )
        delete iter->second;

    std::map< Date::date_t, Chapter*, FuncCompareDates >::clear();
}

Date
CategoryChapters::get_free_order_ordinal() const
{
    assert( m_date_min );   // only for ordinal chapter categories

    if( empty() )
        return Date( m_date_min );

    Date d( begin()->first );
    d.forward_ordinal_order();
    return d;

}

PoolCategoriesChapters::~PoolCategoriesChapters()
{
    for( PoolCategoriesChapters::iterator iter = begin(); iter != end(); ++iter )
        delete( iter->second );
}

void
PoolCategoriesChapters::clear()
{
    for( PoolCategoriesChapters::iterator iter = begin(); iter != end(); ++iter )
        delete( iter->second );

    std::map< Ustring, CategoryChapters*, FuncCompareStrings >::clear();
}

// THEMES ==========================================================================================
// STATIC MEMBERS
const Color Theme::s_color_match1( "#33FF33" );
const Color Theme::s_color_match2( "#009900" );
const Color Theme::s_color_link1( "#3333FF" );
const Color Theme::s_color_link2( "#000099" );
const Color Theme::s_color_broken1( "#FF3333" );
const Color Theme::s_color_broken2( "#990000" );

Theme::Theme()
{
}

Theme::Theme( const Ustring& str_font,
              const std::string& str_base,
              const std::string& str_text,
              const std::string& str_heading,
              const std::string& str_subheading,
              const std::string& str_highlight )
:   font( str_font ), color_base( str_base ), color_text( str_text ),
    color_heading( str_heading ), color_subheading( str_subheading ),
    color_highlight( str_highlight )
{
}

Theme::Theme( const Theme* theme )
:   font( theme->font ),
    color_base( theme->color_base ),
    color_text( theme->color_text ),
    color_heading( theme->color_heading ),
    color_subheading( theme->color_subheading ),
    color_highlight( theme->color_highlight )
{
}

ThemeSystem::ThemeSystem( const Ustring& f,
                          const std::string& cb,
                          const std::string& ct,
                          const std::string& ch,
                          const std::string& csh,
                          const std::string& chl )
:   Theme( f, cb, ct, ch, csh, chl )
{
}

ThemeSystem*
ThemeSystem::get()
{
    static bool s_flag_initialized( false );
    static ThemeSystem *s_theme;

    if( ! s_flag_initialized )
    {
#ifndef LIFEO_WINDOZE
        // this may not be the best method to detect the default font:
        Gtk::TextView *tv = new Gtk::TextView;
        tv->show();
        s_theme = new ThemeSystem( tv->get_style_context()->get_font().to_string(),
#else
        s_theme = new ThemeSystem( "sans 11",
#endif
                                   "white", "black",
                                   "blue", "#F066FC",
                                   "#FFF955" );
#ifndef LIFEO_WINDOZE
        delete tv;
#endif
        s_flag_initialized = true;
        return s_theme;
    }
    else
    {
        return s_theme;
    }
}

// FILTERS =========================================================================================
ElementShower< Filter >* Filter::shower( NULL );

Filter::Filter( Diary* const d, const Ustring& name )
:   DiaryElement( d, name, ES::FILTER_RESET ), m_tag( NULL ),
    m_date_begin( 0 ), m_date_end( Date::DATE_MAX )
{
}

const Icon&
Filter::get_icon() const
{
    return Lifeograph::icons->filter_16;
}

const Icon&
Filter::get_icon32() const
{
    return Lifeograph::icons->filter_32;
}

void
Filter::show()
{
    if( shower != NULL )
        shower->show( *this );
    else
        PRINT_DEBUG( "Filter has no graphical data!" );
}

void
Filter::reset()
{
    m_status = ES::FILTER_RESET;
    m_tag = NULL;
    m_date_begin = 0;
    m_date_end = Date::DATE_MAX;
    m_entries.clear();
}

void
Filter::set( const Filter* source )
{
    m_status = source->m_status;
    m_tag = source->m_tag;
    m_date_begin = source->m_date_begin;
    m_date_end = source->m_date_end;
    m_entries = source->m_entries;
}

void
Filter::set_tag( const Tag* tag )
{
    m_tag = tag;

    if( tag == NULL )
        m_status &= ( ~ES::FILTER_TAG );
    else
        m_status |= ES::FILTER_TAG;

    m_status |= ES::FILTER_OUTSTANDING;
}

void
Filter::set_favorites( bool flag_show_fav, bool flag_show_not_fav )
{
    // clear previous values
    m_status &= ( ES::FILTER_FAVORED ^ ES::FILTER_MAX );

    if( flag_show_fav )
        m_status |= ES::SHOW_FAVORED;
    if( flag_show_not_fav )
        m_status |= ES::SHOW_NOT_FAVORED;

    m_status |= ES::FILTER_OUTSTANDING;
}

void
Filter::set_trash( bool flag_show_trashed, bool flag_show_not_trashed )
{
    // clear previous values
    m_status &= ( ES::FILTER_TRASHED ^ ES::FILTER_MAX );

    if( flag_show_trashed )
        m_status |= ES::SHOW_TRASHED;
    if( flag_show_not_trashed )
        m_status |= ES::SHOW_NOT_TRASHED;

    m_status |= ES::FILTER_OUTSTANDING;
}

void
Filter::set_todo( bool flag_show_not_todo, bool flag_show_todo,
                  bool flag_show_progressed, bool flag_show_done, bool flag_show_canceled )
{
    // clear previous values
    m_status &= ( ES::FILTER_TODO ^ ES::FILTER_MAX );

    if( flag_show_not_todo )
        m_status |= ES::SHOW_NOT_TODO;
    if( flag_show_todo )
        m_status |= ES::SHOW_TODO;
    if( flag_show_progressed )
        m_status |= ES::SHOW_PROGRESSED;
    if( flag_show_done )
        m_status |= ES::SHOW_DONE;
    if( flag_show_canceled )
        m_status |= ES::SHOW_CANCELED;

    m_status |= ES::FILTER_OUTSTANDING;
}

void
Filter::clear_dates()
{
    m_date_begin = 0;
    m_date_end = Date::DATE_MAX;
    if( m_status & ES::FILTER_DATE_BEGIN )
        m_status -= ES::FILTER_DATE_BEGIN;
    if( m_status & ES::FILTER_DATE_END )
        m_status -= ES::FILTER_DATE_END;

    m_status |= ES::FILTER_OUTSTANDING;
}

void
Filter::set_date_begin( Date::date_t d )
{
    m_status |= ES::FILTER_DATE_BEGIN;
    m_status |= ES::FILTER_OUTSTANDING;
    m_date_begin = d;
}

void
Filter::set_date_end( Date::date_t d )
{
    m_status |= ES::FILTER_DATE_END;
    m_status |= ES::FILTER_OUTSTANDING;
    m_date_end = d;
}

void
Filter::add_entry( Entry* entry )
{
    m_status |= ES::FILTER_INDIVIDUAL;
    m_status |= ES::FILTER_OUTSTANDING;
    m_entries.insert( entry );
}

void
Filter::remove_entry( Entry* entry )
{
    if( m_entries.find( entry ) != m_entries.end() )
    {
        m_status |= ES::FILTER_OUTSTANDING;
        m_entries.erase( entry );
    }
}

void
Filter::clear_entries()
{
    if( ! m_entries.empty() )
    {
        m_status |= ES::FILTER_OUTSTANDING;
        m_entries.clear();
    }
}

bool
Filter::is_entry_filtered( Entry* entry ) const
{
    return( m_entries.find( entry ) != m_entries.end() );
}
