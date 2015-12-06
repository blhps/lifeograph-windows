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


#ifndef LIFEOGRAPH_DIARYDATA_HEADER
#define LIFEOGRAPH_DIARYDATA_HEADER


#include "helpers.hpp"	// i18n headers

#ifndef LIFEO_WINDOZE
#include <gtkmm/treemodel.h>
#endif
#include <set>
#include <map>
#include <vector>

namespace LIFEO
{

using namespace HELPERS;

typedef char SortingCriteria;
static const SortingCriteria SC_DATE    = 'd';
static const SortingCriteria SC_SIZE_C  = 's';  // size (char count)
static const SortingCriteria SC_CHANGE  = 'c';  // last change date

typedef unsigned long DEID; // unique diary element id
static const DEID DEID_DIARY = 10000;       // reserved for Diary
static const DEID DEID_FIRST = 10001;       // first sub item in the diary
static const DEID DEID_UNSET = 404;         // :)
static const DEID HOME_CURRENT_ELEM = 1;    // element shown at startup
static const DEID HOME_LAST_ELEM = 2;       // element shown at startup
static const DEID HOME_FIXED_ELEM = 3;      // element shown at startup (defines boundary)

typedef unsigned long ElemStatus;
namespace ES
{
    static const ElemStatus _VOID_           = 0x0;
    static const ElemStatus EXPANDED         = 0x40;

    static const ElemStatus NOT_FAVORED      = 0x100;
    static const ElemStatus FAVORED          = 0x200;
    static const ElemStatus FILTER_FAVORED   = NOT_FAVORED|FAVORED;
    static const ElemStatus NOT_TRASHED      = 0x400;
    static const ElemStatus TRASHED          = 0x800;
    static const ElemStatus FILTER_TRASHED   = NOT_TRASHED|TRASHED;
    static const ElemStatus NOT_TODO         = 0x1000;
    static const ElemStatus TODO             = 0x2000;
    static const ElemStatus PROGRESSED       = 0x4000;
    static const ElemStatus DONE             = 0x8000;
    static const ElemStatus CANCELED         = 0x10000;
    static const ElemStatus FILTER_TODO      = NOT_TODO|TODO|PROGRESSED|DONE|CANCELED;

    static const ElemStatus ENTRY_DEFAULT       = NOT_FAVORED|NOT_TRASHED|NOT_TODO;
    static const ElemStatus ENTRY_DEFAULT_FAV   = FAVORED|NOT_TRASHED|NOT_TODO;
    static const ElemStatus CHAPTER_DEFAULT     = EXPANDED|NOT_TODO;

    // FILTER RELATED CONSTANTS AND ALIASES
    static const ElemStatus SHOW_NOT_FAVORED    = NOT_FAVORED;
    static const ElemStatus SHOW_FAVORED        = FAVORED;
    static const ElemStatus SHOW_NOT_TRASHED    = NOT_TRASHED;
    static const ElemStatus SHOW_TRASHED        = TRASHED;
    static const ElemStatus SHOW_NOT_TODO       = NOT_TODO;
    static const ElemStatus SHOW_TODO           = TODO;
    static const ElemStatus SHOW_PROGRESSED     = PROGRESSED;
    static const ElemStatus SHOW_DONE           = DONE;
    static const ElemStatus SHOW_CANCELED       = CANCELED;

    static const ElemStatus FILTER_TAG          = 0x100000;
    static const ElemStatus FILTER_DATE_BEGIN   = 0x200000;
    static const ElemStatus FILTER_DATE_END     = 0x400000;
    static const ElemStatus FILTER_INDIVIDUAL   = 0x800000;
    static const ElemStatus FILTER_OUTSTANDING  = 0x20000000;

    static const ElemStatus FILTERED_OUT        = 0x40000000;

    static const ElemStatus FILTER_RESET        = FILTER_FAVORED|SHOW_NOT_TRASHED|SHOW_NOT_TODO|
                                                  SHOW_TODO|SHOW_PROGRESSED|FILTER_OUTSTANDING;
    static const ElemStatus FILTER_MAX          = 0x7FFFFFFF;
    // to go parallel with Java version 0x7FFFFFFF is the max
}

class DiaryElement;     // forward declaration
class Untagged;         // forward declaration
class Entry;            // forward declaration
class Filter;           // forward declaration
class Diary;            // forward declaration
#ifndef LIFEO_WINDOZE
class ListData;	        // forward declaration
#else
#include <commctrl.h>
#endif

typedef std::vector< Entry* >               EntryVector;
typedef std::vector< Entry* >::iterator     EntryVectorIter;

typedef std::set< Entry* >                  EntrySet;
typedef std::set< Entry* >::iterator        EntrySetIter;

typedef std::map< DEID, DiaryElement* >     PoolDEIDs;

// DIARYELEMENT ====================================================================================
class NamedElement
{
    public:
                                NamedElement() : m_name( "" ) {}
                                NamedElement( const Ustring& name ) : m_name( name ) {}
        virtual                 ~NamedElement() {}  //needed because of virtual methods

        Ustring                 get_name() const
        { return m_name; }
        std::string             get_name_std() const  // std::string version
        { return m_name; }
        void                    set_name( const Ustring& name )
        { m_name = name; }

    protected:
        Ustring                 m_name;
};

class DiaryElement : public NamedElement
{
    public:
        static bool             FLAG_ALLOCATE_GUI_FOR_DIARY;

        static const Icon       s_pixbuf_null;
        static const Ustring    s_type_names[];

        enum Type
        {   // CAUTION: order is significant and shouldn't be changed!
            // type_names above should follow the same order to work
            ET_NONE, ET_TAG, ET_UNTAGGED, ET_TAG_CTG, ET_CHAPTER_CTG, ET_FILTER,
            // entry list elements:
            ET_DIARY, ET_CHAPTER, ET_TOPIC, ET_GROUP, ET_ENTRY, ET_MULTIPLE_ENTRIES, ET_DATE,
            // special types for pseudo elements:
            ET_HEADER
        };
                                DiaryElement(); // only for pseudo elements
                                DiaryElement( Diary* const, const Ustring&,
                                              ElemStatus = ES::_VOID_ );
                                DiaryElement( Diary* const, DEID, ElemStatus = ES::_VOID_ );
        virtual                 ~DiaryElement();

        virtual Type            get_type() const = 0;
        Ustring                 get_type_name()
        { return s_type_names[ get_type() ]; }
        virtual int             get_size() const = 0;
#ifndef LIFEO_WINDOZE
        virtual const Icon&     get_icon() const
        { return( s_pixbuf_null ); }
        virtual const Icon&     get_icon32() const
        { return( s_pixbuf_null ); }

#else
#define ICON_NO_DIARY          0
#define ICON_NO_ENTRY          1
#define ICON_NO_TODO           2
#define ICON_NO_PROGRESSED     3
#define ICON_NO_DONE           4
#define ICON_NO_CANCELED       5
#define ICON_NO_CHAPTER_D      6
#define ICON_NO_CHAPTER_O      7
#define ICON_NO_TAG            8
#define ICON_NO_TAG_THEME      9
#define ICON_NO_UNTAGGED       10
#define ICON_NO_TAG_CTG        11
        virtual int             get_icon() const { return 0; }
#endif

        virtual Date            get_date() const
        { return Date( Date::NOT_APPLICABLE ); }
        Date::date_t            get_date_t() const
        { return get_date().m_date; }

        virtual Ustring         get_list_str() const
        { return ""; }

        DEID                    get_id() const
        { return m_id; }

        virtual void            show() = 0;
        virtual void            prepare_for_hiding() {}
        virtual bool            get_filtered_out()
        { return false; }

        ElemStatus              get_status() const
        { return m_status; }
        void                    set_status( ElemStatus status )
        { m_status = status; }

        void                    set_status_flag( ElemStatus flag, bool add )
        {
            if( add )
                m_status |= flag;
            else if( m_status & flag )
                m_status -= flag;
        }

        // only for entries and chapters:
        ElemStatus              get_todo_status() const
        { return( m_status & ES::FILTER_TODO ); }
        void                    set_todo_status( ElemStatus );

        virtual bool            is_favored() const
        { return false; }

#ifndef LIFEO_WINDOZE
        ListData*               m_list_data;
#else
        HTREEITEM               m_list_data;
#endif

    protected:
        Diary* const            m_ptr2diary;

        const DEID              m_id;
        ElemStatus              m_status;
};

#ifndef LIFEO_WINDOZE
class ListData
{
    public:
        class Colrec : public Gtk::TreeModel::ColumnRecord
        {
            public:
                Colrec()
                {
                    add( ptr );
                    add( icon );
                    add( info );
                    add( icon2 );
                }
                // HIDDEN COLUMNS
                Gtk::TreeModelColumn< DiaryElement* >                   ptr;
                // COLUMNS
                Gtk::TreeModelColumn< Glib::RefPtr< Gdk::Pixbuf > >     icon;
                Gtk::TreeModelColumn< Glib::ustring >                   info;
                Gtk::TreeModelColumn< Glib::RefPtr< Gdk::Pixbuf > >     icon2;
        };
        static Colrec*          colrec;

        Gtk::TreePath           treepath;
};
#endif

bool compare_listitems( DiaryElement*, DiaryElement* );
bool compare_listitems_by_name( DiaryElement*, DiaryElement* );
bool compare_names( const Ustring&, const Ustring& );

typedef bool( *FuncCompareStrings )( const Ustring&, const Ustring& ) ;
typedef bool( *FuncCompareDiaryElem )( DiaryElement*, DiaryElement* ) ;

template< class T >
Ustring
create_unique_name_for_map( const std::map< Ustring, T*, FuncCompareStrings > &map,
                            const Ustring &name0 )
{
    Ustring name = name0;
    for( int i = 1; map.find( name ) != map.end(); i++ )
    {
#ifndef LIFEO_WINDOZE
        name = Glib::ustring::compose( "%1 %2", name0, i );
#else
        name = STR::compose( name0, " ", i );
#endif
    }

    return name;
}

// ElementShower class is for communication between Diary and...
// ...GUI classes. we may resort to signals if needed:
class ElementShowerProto
{
    public:
#ifndef LIFEO_WINDOZE
        ElementShowerProto() : m_grid_popover( NULL ) {}
#endif
        virtual             ~ElementShowerProto() {}

        virtual DiaryElement*   get_element() = 0;

#ifndef LIFEO_WINDOZE
        Gtk::Widget*            get_widget()
        { return m_vbox; }
        virtual Gtk::Widget*    get_popover()
        { return m_grid_popover; }
        virtual Gtk::Widget*    get_extra_tools()
        { return NULL; }
#endif

        virtual Ustring         get_title_str() const
        { return ""; }
        virtual Ustring         get_info_str() const
        { return ""; }

        virtual bool            is_title_editable() const
        { return false; }
        virtual Ustring         get_title_edit_str() const
        { return get_title_str(); }
        virtual bool            check_title_applicable( const Ustring& ) const
        { return false; }
        virtual bool            apply_title( const Ustring& )
        { return false; }

    protected:
#ifndef LIFEO_WINDOZE
        Gtk::Box*               m_vbox;
        Gtk::Grid*              m_grid_popover;
#endif
};

template< class T >
class ElementShower : public ElementShowerProto
{
    public:
        ElementShower() : m_ptr2elem( NULL ) {}
        virtual                 ~ElementShower() {}
        virtual void            show( T& ) = 0;
        virtual void            prepare_for_hiding( T& ) {}

        T*                      get_element()
        { return m_ptr2elem; }

    protected:
        T*                      m_ptr2elem;
};

template< typename T >
class DiaryElementReferrer : public DiaryElement, public std::set< T*, FuncCompareDiaryElem >
{
// NOTE: this class and its derivatives are not responsible for handling of...
// ...its children's allocation and deletion
    public:
        typedef std::set< T*, FuncCompareDiaryElem > Set;

        DiaryElementReferrer( Diary* const d )
        :   DiaryElement( d, "" ),
            std::set< T*, FuncCompareDiaryElem >( compare_listitems ) {}
        DiaryElementReferrer( Diary* const d, const Ustring& name, ElemStatus es,
                               FuncCompareDiaryElem f = compare_listitems )
        :   DiaryElement( d, name, es ),
            std::set< T*, FuncCompareDiaryElem >( f ) {}

        virtual int             get_size() const
        { return std::set< T*, FuncCompareDiaryElem >::size(); }
};

// THEME ===========================================================================================
class Theme
{
    public:
                                    Theme();
                                    Theme( const Ustring&,
                                           const std::string&,
                                           const std::string&,
                                           const std::string&,
                                           const std::string&,
                                           const std::string& );
        // duplicates an existing theme, works like a copy constructor
                                    Theme( const Theme* );
        virtual                     ~Theme() {}


        virtual bool                is_system() const
        { return false; }
#ifndef LIFEO_WINDOZE
        Pango::FontDescription      font;
#else
        std::string                 font;
#endif
        Color                       color_base;
        Color                       color_text;
        Color                       color_heading;
        Color                       color_subheading;
        Color                       color_highlight;

        // CONSTANT COLORS
        static const Color          s_color_match1;
        static const Color          s_color_match2;
        static const Color          s_color_link1;
        static const Color          s_color_link2;
        static const Color          s_color_broken1;
        static const Color          s_color_broken2;

        static const Color          s_color_todo;
        static const Color          s_color_progressed;
        static const Color          s_color_done;
        static const Color          s_color_done1;
        static const Color          s_color_done2;
        static const Color          s_color_canceled;
};

class ThemeSystem : public Theme
{
    public:
        static ThemeSystem*         get();
        bool                        is_system() const
        { return true; }

    protected:
                                    ThemeSystem( const Ustring&,
                                                 const std::string&,
                                                 const std::string&,
                                                 const std::string&,
                                                 const std::string&,
                                                 const std::string& );
};

// TAG =============================================================================================
class Tag;	// forward declaration

// TAG CATEGORY
class CategoryTags : public DiaryElementReferrer< Tag >
{
    public:
        static ElementShower< CategoryTags >* shower;

                                CategoryTags( Diary* const, const Ustring& );

        void                    show();

        Type                    get_type() const
        { return ET_TAG_CTG; }

#ifndef LIFEO_WINDOZE
        const Icon&             get_icon() const;
        const Icon&             get_icon32() const;
#else
        int                     get_icon() const;
#endif
        Ustring                 get_list_str() const
#ifndef LIFEO_WINDOZE
        { return Glib::ustring::compose( "<b>%1</b>", Glib::Markup::escape_text( m_name ) ); }
#else
        { return m_name; }
#endif

        bool                    get_expanded() const
        { return( m_status & ES::EXPANDED ); }
        void                    set_expanded( bool flag_expanded )
        { set_status_flag( ES::EXPANDED, flag_expanded ); }

    protected:

    friend class CategoryTagsView;
    friend class PoolCategoriesTags;
    friend class PanelExtra;
};

// POOL OF DEFINED TAG CATEGORIES
class PoolCategoriesTags : public std::map< Ustring, CategoryTags*, FuncCompareStrings >
{
    public:
                                PoolCategoriesTags();
                                ~PoolCategoriesTags();

        bool                    rename_category( CategoryTags*, const Ustring& );
        void                    clear();
};

// TAG
class Tag : public DiaryElementReferrer< Entry >
{
    public:
        static ElementShower< Tag >* shower;

                                Tag( Diary* const d )
                                :   DiaryElementReferrer( d ),
                                    m_ptr2category( NULL ), m_theme( NULL ) {}
        explicit                Tag( Diary* const, const Ustring&, CategoryTags* );
        virtual                 ~Tag();

        void                    show();

        CategoryTags*           get_category()
        { return m_ptr2category; }
        void                    set_category( CategoryTags* );

        Theme*                  get_theme() const;
        bool                    get_has_own_theme() const
        { return( m_theme != NULL ); }
        Theme*                  get_own_theme();
        Theme*                  create_own_theme_duplicating( const Theme* );
        void                    reset_theme();

        void                    set_name( const Ustring& );

        //Date                  get_date() const; // if earliest entry's date is needed
        virtual Type            get_type() const
        { return ET_TAG; }

#ifndef LIFEO_WINDOZE
        virtual const Icon&     get_icon() const;
        virtual const Icon&     get_icon32() const;
#else
        virtual int             get_icon() const;
#endif

        virtual Ustring         get_list_str() const
#ifndef LIFEO_WINDOZE
        { return Glib::Markup::escape_text( m_name ); }
#else
        { return m_name; }
#endif

    protected:
        CategoryTags*           m_ptr2category;
        Theme*                  m_theme;

    friend class PoolTags;
    friend class Tagset;
};

class Untagged : public Tag
{
    public:
                                Untagged();

        Type                    get_type() const
        { return ET_UNTAGGED; }
#ifndef LIFEO_WINDOZE
        const Icon&             get_icon() const;
        const Icon&             get_icon32() const;
#else
        int                     get_icon() const;
#endif

        Ustring                 get_list_str() const;

        void                    reset()
        {
            clear();
            reset_theme();
        }
};

// POOL OF DEFINED TAGS
class PoolTags : public std::map< Ustring, Tag*, FuncCompareStrings >
{
    public:
                                PoolTags()
        :   std::map< Ustring, Tag*, FuncCompareStrings >( compare_names ) {}
                                ~PoolTags();

        bool                    handle_tag_changed( );
        bool                    rename( Tag*, const Ustring& );
        Tag*                    get_tag( unsigned int );
        Tag*                    get_tag( const Ustring& );
        void                    clear();
};

// TAGS OF AN ENTRY
// not responsible for memory handling
class Tagset : public std::set< Tag*, FuncCompareDiaryElem >
{
    public:
                                Tagset()
        :   std::set< Tag*, FuncCompareDiaryElem >( compare_listitems_by_name ) {}

                                ~Tagset();
        bool                    add( Tag* );
        bool                    checkfor_member( const Tag* ) const;
        const Tag*              get_tag( unsigned int ) const;
        //bool                  remove_tag( const Glib::ustring& );

    protected:

};

// CHAPTER =========================================================================================
class Chapter : public DiaryElementReferrer< Entry >
{
    public:
        static ElementShower< Chapter >* shower;

                                Chapter( Diary* const, const Ustring&, Date::date_t );
        // this class should have a virtual dtor for some reason because
        // if it does not, its derived classes cannot be deleted
        virtual                 ~Chapter() {}

        void                    show();

        void                    set_name( const Ustring& );

        Ustring                 get_date_str() const;

        void                    set_date( Date::date_t date )
        { m_date_begin.m_date = date; update_type(); }

        Type                    get_type() const
        { return m_type; }

#ifndef LIFEO_WINDOZE
        const Icon&             get_icon() const;
        const Icon&             get_icon32() const;
#else
        int                     get_icon() const;
#endif

        Date                    get_date() const;

        Ustring                 get_list_str() const;

        Date                    get_free_order() const;

        bool                    get_expanded() const
        { return( m_status & ES::EXPANDED ); }
        void                    set_expanded( bool flag_expanded )
        { set_status_flag( ES::EXPANDED, flag_expanded ); }

        bool                    is_ordinal() const
        { return m_date_begin.is_ordinal(); }

        void                    recalculate_span( const Chapter* );
        int                     get_time_span() const
        { return m_time_span; }
        void                    set_time_span( int s )
        { m_time_span = s; }

        void                    update_type();

    protected:
        Date                    m_date_begin;
        int                     m_time_span;
        Type                    m_type;

    friend class CategoryChapters;
    friend class ChapterView;
};

class CategoryChapters :
        public DiaryElement, public std::map< Date::date_t, Chapter*, FuncCompareDates >
{
    public:
        explicit                CategoryChapters( Diary* const, const Ustring& );
        // for topics and groups which do not have names:
        explicit                CategoryChapters( Diary* const, Date::date_t );
        virtual                 ~CategoryChapters();

        Type                    get_type() const
        { return ET_CHAPTER_CTG; }
        virtual int             get_size() const
        { return size(); }

        void                    show() {} // not used

        Chapter*                get_chapter( const Date::date_t ) const;

        Chapter*                create_chapter( const Ustring&, const Date::date_t );
        Chapter*                create_chapter_ordinal( const Ustring& );
        bool                    set_chapter_date( Chapter*, Date::date_t );

        void                    clear();

        Date::date_t            get_free_order_ordinal() const;

    protected:
        const Date::date_t      m_date_min;

        bool                    add( Chapter* );
};

class PoolCategoriesChapters :
        public std::map< Ustring, CategoryChapters*, FuncCompareStrings >
{
    public:
                                    PoolCategoriesChapters()
        :   std::map< Ustring, CategoryChapters*, FuncCompareStrings >(
                compare_names ) {}
                                    ~PoolCategoriesChapters();
        void                        clear();
};

// FILTERS =========================================================================================
class Filter : public DiaryElement
{
    public:
        static ElementShower< Filter >* shower;

                                    Filter( Diary* const, const Ustring& );

        bool                        operator==( const Filter& f )
        {
            if( m_status != f.m_status )
                return false;
            if( m_date_begin != f.m_date_begin )
                return false;

            if( m_date_end != f.m_date_end )
                return false;

            if( m_tag != f.m_tag )
                return false;

            if( m_entries != f.m_entries )
                return false;

            return true;
        }

        void                        show();

        DiaryElement::Type          get_type() const
        { return ET_FILTER; }
        int                         get_size() const
        { return 0; }   // redundant
#ifndef LIFEO_WINDOZE
        const Icon&                 get_icon() const;
        const Icon&                 get_icon32() const;
#endif

        Ustring                     get_list_str() const
#ifndef LIFEO_WINDOZE
        { return Glib::Markup::escape_text( m_name ); }
#else
        { return m_name; }
#endif

        void                        reset();
        void                        set( const Filter* );

        const Tag*                  get_tag() const
        {
            return m_tag;
        }
        void                        set_tag( const Tag* );

        void                        set_favorites( bool, bool );
        void                        set_trash( bool, bool );
        void                        set_todo( bool, bool, bool, bool, bool );

        Date::date_t                get_date_begin() const
        { return m_date_begin; }
        Date::date_t                get_date_end() const
        { return m_date_end; }
        void                        set_date_begin( Date::date_t d );
        void                        set_date_end( Date::date_t d );
        void                        clear_dates();

        void                        set_status_applied()
        { if( m_status & ES::FILTER_OUTSTANDING ) m_status -= ES::FILTER_OUTSTANDING; }

        void                        set_status_outstanding()
        { m_status |= ES::FILTER_OUTSTANDING; }

        const EntrySet*             get_entries() const
        {
            return &m_entries;
        }
        void                        add_entry( Entry* );
        void                        remove_entry( Entry* );
        void                        clear_entries();
        bool                        is_entry_filtered( Entry* ) const;

    protected:
        const Tag*                  m_tag;
        Date::date_t                m_date_begin;
        Date::date_t                m_date_end;
        EntrySet                    m_entries;
};

} // end of namespace LIFEO

#endif

