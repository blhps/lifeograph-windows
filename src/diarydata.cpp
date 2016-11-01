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
const Icon                          DiaryElement::s_pixbuf_null;
#else
const Icon                          DiaryElement::s_pixbuf_null = NULL;
#endif
bool                                DiaryElement::FLAG_ALLOCATE_GUI_FOR_DIARY = true;
const Ustring                       DiaryElement::s_type_names[] =
{
    "", _( "Tag" ), _( "Untagged" ), _( "Tag Category" ), _( "Chapter Category" ),
    // TRANSLATORS: filter is a noun here
    _( "Filter" ),
    _( "Diary" ), _( "Dated Chapter" ), _( "Numbered Chapter" ),
    _( "Free Chapter" ), _( "Entry" ), "Multiple Entries", "", ""
};

DiaryElement::DiaryElement() : NamedElement( "" ) {}

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

// CHART POINTS ====================================================================================
unsigned int
ChartPoints::calculate_distance( const Date& d1, const Date& d2 ) const
{
    switch( type & PERIOD_MASK )
    {
        case MONTHLY:
            return d1.calculate_months_between( d2.m_date );
        case YEARLY:
            return labs( int( d1.get_year() ) - int( d2.get_year() ) );
    }

    return 0; // just to silence the compiler warning
}
int
ChartPoints::calculate_distance_neg( const Date& d1, const Date& d2 ) const
{
    switch( type & PERIOD_MASK )
    {
        case MONTHLY:
            return Date::calculate_months_between_neg( d1.m_date, d2.m_date );
        case YEARLY:
            return( d2.get_year() - d1.get_year() );
    }

    return 0; // just to silence the compiler warning
}

void
ChartPoints::add( int limit, bool flag_sustain, const Value a, const Value b )
{
    for( int i = 1; i < limit; i++ )
    {
        if( flag_sustain ) // interpolation
            push_back( a + ( i * ( ( b - a ) / limit ) ) );
        else
            push_back( 0 );
    }

    push_back( b );
}

void
ChartPoints::add_plain( Date& d_last, const Date&& d )
{
    if( d.is_ordinal() )
        return;

    if( start_date == 0 )
        start_date = d.m_date;

    if( values.empty() ) // first value is being entered i.e. v_before is not set
        push_back( 1 );
    else if( calculate_distance( d, d_last ) > 0 )
        add( calculate_distance( d, d_last ), false, 0, 1 );
    else
    {
        Value v = values.back() + 1;
        values.back() = v;
        if( v < value_min )
            value_min = v;
        if( v > value_max )
            value_max = v;
    }

    d_last = d;
}

// NAME AND VALUE ==================================================================================
NameAndValue
NameAndValue::parse( const Ustring& text )
{
    NameAndValue nav;
    char lf{ '=' }; // =, \, #, $(unit)
    int divider{ 0 };
    int trim_length{ 0 };
    int trim_length_unit{ 0 };
    bool negative{ false };
    Wchar c;

    for( Ustring::size_type i = 0; i < text.size(); i++ )
    {
        c = text.at( i );
        switch( c )
        {
            case '\\':
                if( lf == '#' || lf == '$' )
                {
                    nav.unit += c;
                    trim_length_unit = 0;
                    lf = '$';
                }
                else if( lf == '\\' )
                {
                    nav.name += c;
                    trim_length = 0;
                    lf = '=';
                }
                else // i.e. ( lf == '=' )
                    lf = '\\';
                break;
            case '=':
                if( nav.name.empty() || lf == '\\' )
                {
                    nav.name += c;
                    trim_length = 0;
                    lf = '=';
                }
                else if( lf == '#' || lf == '$' )
                {
                    nav.unit += c;
                    trim_length_unit = 0;
                    lf = '$';
                }
                else // i.e. ( lf == '=' )
                    lf = '#';
                break;
            case ' ':
            case '\t':
                // if( lf == '#' ) just ignore
                if( lf == '=' || lf == '\\' )
                {
                    if( !nav.name.empty() ) // else ignore
                    {
                        nav.name += c;
                        trim_length++;
                    }
                }
                else if( lf == '$' )
                {
                    nav.unit += c;
                    trim_length_unit++;
                }
                break;
            case ',':
            case '.':
                if( divider || lf == '$' ) // note that if divider, lf must be #
                {
                    nav.unit += c;
                    trim_length_unit = 0;
                    lf = '$';
                }
                else if( lf == '#' )
                    divider = 1;
                else
                {
                    nav.name += c;
                    trim_length = 0;
                    lf = '=';
                }
                break;
            case '-':
                if( negative || lf == '$' ) // note that if negative, lf must be #
                {
                    nav.unit += c;
                    trim_length_unit = 0;
                    lf = '$';
                }
                else if( lf == '#' )
                    negative = true;
                else
                {
                    nav.name += c;
                    trim_length = 0;
                    lf = '=';
                }
                break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                if( lf == '#' )
                {
                    nav.status = NameAndValue::HAS_VALUE;
                    nav.value *= 10;
                    nav.value += ( c - '0' );
                    if( divider )
                        divider *= 10;
                }
                else if( lf == '$' )
                {
                    nav.unit += c;
                    trim_length_unit = 0;
                }
                else
                {
                    nav.name += c;
                    trim_length = 0;
                    lf = '='; // reset ( lf == \ ) case
                }
                break;
            default:
                if( lf == '#' || lf == '$' )
                {
                    nav.unit += c;
                    trim_length_unit = 0;
                    lf = '$';
                }
                else
                {
                    nav.name += c;
                    trim_length = 0;
                    lf = '=';
                }
                break;
        }
    }

    if( lf == '$' )
        nav.status |= ( NameAndValue::HAS_NAME | NameAndValue::HAS_UNIT );
    else if( ! nav.name.empty() )
        nav.status |= NameAndValue::HAS_NAME;

    if( trim_length )
        nav.name.erase( nav.name.size() - trim_length, trim_length );
    if( trim_length_unit )
        nav.unit.erase( nav.unit.size() - trim_length_unit, trim_length_unit );

    if( lf == '=' && ! nav.name.empty() ) // implicit boolean tag
        nav.value = 1;
    else
    {
        if( divider > 1 )
            nav.value /= divider;
        if( negative )
            nav.value *= -1;
    }

    PRINT_DEBUG( "tag parsed | name: ", nav.name, "; value: ", nav.value, "; unit: ", nav.unit );

    return nav;
}

// TAG =============================================================================================
// STATIC MEMBERS
ElementView< Tag >* Tag::shower( nullptr );

Tag::Tag( Diary* const d, const Ustring& name, CategoryTags* category, int chart_type )
:   DiaryElementMapper( d, name, ES::_VOID_ ), DiaryElementChart( chart_type ),
    m_ptr2category( category )
{
    if( category != nullptr )
        category->insert( this );
}

Tag::~Tag()
{
}

#ifndef LIFEO_WINDOZE
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
#else
int
Tag::get_icon() const
{
    return( get_has_own_theme() ? ICON_NO_TAG_THEME : ICON_NO_TAG );
}
#endif

Ustring
Tag::escape_name( const Ustring& name )
{
    Ustring result;
    Wchar c;

    for( Ustring::size_type i = 0; i < name.size(); i++ )
    {
        c = name.at( i );
        if( c == '=' || c == '\\' )
            result += '\\';
        result += c;
    }

    return result;
}

Ustring
Tag::get_name_and_value( const Entry* entry, bool flag_escape, bool flag_unit ) const
{
    Ustring result{ flag_escape ? escape_name( m_name ) : m_name };

    if( !is_boolean() )
    {
        result += STR::compose( " = ", get_value( const_cast< Entry* >( entry ) ) );

        if( flag_unit && !m_unit.empty() )
            result += " " + m_unit;
    }

    return result;
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

        for( auto& kv_entry : *this )
            kv_entry.first->update_theme();
    }

    return m_theme;
}

Theme*
Tag::create_own_theme_duplicating( const Theme* theme )
{
    if( m_theme != NULL )
        delete m_theme;

    m_theme = new Theme( theme );

    for( auto& kv_entry : *this )
        kv_entry.first->update_theme();

    return m_theme;
}

void
Tag::reset_theme()
{
    if( m_theme )
    {
        delete m_theme;
        m_theme = NULL;

        for( auto& kv_entry : *this )
            kv_entry.first->update_theme();
    }
}

Value
Tag::get_value( Entry* entry ) const
{
    auto iter = find( entry );
    if( iter != end() )
        return iter->second;
    else
        return -404;
}

ChartPoints*
Tag::create_chart_data() const
{
    if( empty() )
        return nullptr;

    ChartPoints* cp{ new ChartPoints( m_chart_type ) };
    if( ( m_chart_type & ChartPoints::BOOLEAN ) == 0 )
        cp->unit = m_unit;

    // order from old to new: d/v_before > d/v_last > d/v
    Date d_before{ Date::NOT_SET }, d_last{ Date::NOT_SET },  d{ Date::NOT_SET };
    Value v_before{ 0 }, v_last{ 0 }, v{ 0 };
    int no_of_entries{ 0 };

    auto add_value = [ & ]()
        {
            bool flag_sustain{ ( m_chart_type & ChartPoints::VALUE_TYPE_MASK ) ==
                    ChartPoints::AVERAGE };
            if( flag_sustain && no_of_entries > 1 )
                v_last /= no_of_entries;

            if( cp->values.empty() ) // first value is being entered i.e. v_before is not set
                cp->add( 0, 0, 0, v_last );
            else
                cp->add( cp->calculate_distance( d_last,  d_before ),
                         flag_sustain, v_before, v_last );

            v_before = v_last;
            v_last = v;
            d_before = d_last;
            d_last = d;
            no_of_entries = 1;
        };

    for( auto iter = rbegin(); iter != rend(); ++iter )
    {
        d = iter->first->get_date();

        if( d.is_ordinal() )
            break;

        if( cp->start_date == 0 )
            cp->start_date = d.m_date;
        if( ! d_last.is_set() )
            d_last = d;

        v = is_boolean() ? 1.0 : iter->second;

        if( cp->calculate_distance( d, d_last ) > 0 )
            add_value();
        else
        {
            v_last += v;
            no_of_entries++;
        }
    }

    add_value();

    Diary::d->fill_up_chart_points( cp );

    return cp;
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

#ifndef LIFEO_WINDOZE
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
#else
int
Untagged::get_icon() const
{
    return ICON_NO_UNTAGGED;
}
#endif

// TAGPOOL =========================================================================================
PoolTags::~PoolTags()
{
    for( iterator iter = begin(); iter != end(); ++iter )
        delete iter->second;
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
ElementView< CategoryTags >* CategoryTags::shower( nullptr );

CategoryTags::CategoryTags( Diary* const d, const Ustring& name )
:   DiaryElementReferrer( d, name, ES::EXPANDED, compare_listitems_by_name )
{
}

#ifndef LIFEO_WINDOZE
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
#else
int
CategoryTags::get_icon() const
{
    return ICON_NO_TAG_CTG;
}
#endif

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
        delete iter->second;
}

void
PoolCategoriesTags::clear()
{
    for( iterator iter = begin(); iter != end(); ++iter )
        delete iter->second;

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
bool
TagSet::add( Tag* tag )
{
    if( insert( tag ).second ) // if did not exist previously
        return true;
    else
    {
        PRINT_DEBUG( "set already has the tag: " + tag->get_name() );
        return false;
    }

}

bool
TagSet::check_for_member( const Tag* tag ) const
{
    return( count( const_cast< Tag* >( tag ) ) > 0 );
}

// CHAPTER =========================================================================================
// STATIC MEMBERS
ElementView< Chapter >* Chapter::shower( nullptr );

Chapter::Chapter( Diary* const d, const Ustring& name, Date::date_t date )
:   DiaryElementReferrer( d, name, ES::CHAPTER_DEFAULT ), m_date_begin( date )
{
    update_type();
}

#ifndef LIFEO_WINDOZE
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
#else
int
Chapter::get_icon() const
{
    switch( m_status & ES::FILTER_TODO )
    {
        case ES::TODO:
            return ICON_NO_TODO;
        case ES::PROGRESSED:
            return ICON_NO_PROGRESSED;
        case ES::DONE:
            return ICON_NO_DONE;
        case ES::CANCELED:
            return ICON_NO_CANCELED;
        default:
            return( m_date_begin.is_ordinal() ?
                    ICON_NO_CHAPTER_O : ICON_NO_CHAPTER_D );
    }
}
#endif

Ustring
Chapter::get_list_str() const
{
#ifndef LIFEO_WINDOZE
    static const Ustring tpl[] = { "<b>%2</b>", "<b>%1 -  %2</b>",
                                   "<b><s>%2</s></b>", "<b><s>%1 -  %2</s></b>" };
    int i{ m_date_begin.is_hidden() ? 0 : 1 };
    if( m_status & ES::CANCELED ) i += 2;


    return Glib::ustring::compose( tpl[ i ], m_date_begin.format_string(),
                                   Glib::Markup::escape_text( m_name ) );
#else // FIXME
    if( m_date_begin.is_hidden() )
        return m_name;
    return STR::compose( m_date_begin.format_string(), " -  ", m_name );
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

ChartPoints*
Chapter::create_chart_data() const
{
    if( empty() )
        return nullptr;

    ChartPoints* cp{ new ChartPoints( m_chart_type ) };
    Date d_last{ Date::NOT_SET };

    for( auto iter = rbegin(); iter != rend(); ++iter )
        cp->add_plain( d_last, ( *iter )->get_date() );

    Diary::d->fill_up_chart_points( cp );

    return cp;
}

// CHAPTER CATEGORY ================================================================================
CategoryChapters::CategoryChapters( Diary* const d, const Ustring& name )
:   DiaryElement( d, name ),
    std::map< Date::date_t, Chapter*, FuncCompareDates >( compare_dates )
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
CategoryChapters::create_chapter( const Ustring& name, const Date::date_t date )
{
    Chapter* chapter = new Chapter( m_ptr2diary, name, date );

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
    assert( find( date ) == end() );

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

Date::date_t
CategoryChapters::get_free_order_ordinal() const
{
    assert( m_date_min );   // only for ordinal chapter categories

    if( empty() )
        return m_date_min;

    Date d( begin()->first );
    d.forward_ordinal_order();
    return d.m_date;

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

const Color Theme::s_color_todo( "#FF0000" );
const Color Theme::s_color_progressed( "#FF8811" );
const Color Theme::s_color_done( "#66BB00" );
const Color Theme::s_color_done1( "#77CC11" );
const Color Theme::s_color_done2( "#409000" );
const Color Theme::s_color_canceled( "#AA8855" );

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
                                   "white", "black",
                                   "blue", "#F066FC",
                                   "#FFF955" );
        delete tv;
#else
        s_theme = new ThemeSystem( "sans 11",
                                   "#FFFFFFFFFFFF", "#000000000000",
                                   "#00000000FFFF", "#F0F06666FCFC",
                                   "#FFFFF9F95555" );

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
ElementView< Filter >* Filter::shower( nullptr );

Filter::Filter( Diary* const d, const Ustring& name )
:   DiaryElement( d, name, ES::FILTER_RESET )
{
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
