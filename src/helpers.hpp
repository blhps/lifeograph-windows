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


#ifndef LIFEOGRAPH_HELPERS_HEADER
#define LIFEOGRAPH_HELPERS_HEADER


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

#include <gcrypt.h>

#ifndef LIFEO_WINDOZE
#include <gtkmm.h>
#endif

#ifndef LIFEO_WINDOZE
typedef Glib::ustring Ustring;
typedef Glib::RefPtr< Gdk::Pixbuf > Icon;
typedef Gdk::RGBA Color;
#else
// values are temporary
typedef std::string Ustring;
typedef void* Icon;
typedef std::string Color;
#endif

namespace HELPERS
{

class STR
{
	private:
		static void print_internal( std::stringstream& ) {}
		
		
		template< typename Arg1, typename... Args >
		static void print_internal( std::stringstream& str, Arg1 arg1, Args... args )
		{
		    str << arg1;
		    print_internal( str, args... );
		}

	public:
		template< typename... Args >
		static Ustring compose( Args... args )
		{
		    std::stringstream str;
		    print_internal( str, args... );
		    
		    return str.str();
		}

};

class Error
{
    public:
                        Error( const Ustring& );
};

enum Result
{
    OK,
    ABORTED,
    SUCCESS,
    FAILURE,
    COULD_NOT_START,
    COULD_NOT_FINISH,
    WRONG_PASSWORD,
    //APPARENTLY_ENCRYTED_FILE,
    //APPARENTLY_PLAIN_FILE,
    INCOMPATIBLE_FILE_OLD,
    INCOMPATIBLE_FILE_NEW,
    CORRUPT_FILE,
    // EMPTY_DATABASE, // not used anymore

    // RESULTS USED BY set_path():
    FILE_NOT_FOUND,
    FILE_NOT_READABLE,
    FILE_NOT_WRITABLE,
    FILE_LOCKED,

    // RESULTS USED BY Date
    OUT_OF_RANGE,
    INVALID
};

// RESPONSE IDS
static const int        RESPONSE_CANCEL     = 0;
static const int        RESPONSE_GO         = 1;

// DATE ============================================================================================
// order: 10 bits
// day:    5 bits
// month:  4 bits
// year:  12 bits
// ordinal flag:  1 bit (32nd bit)

class Date
{
    public:
        static const unsigned long  NOT_APPLICABLE      = 0x0;
        static const unsigned long  NOT_SET             = 0xFFFFFFFF;
        static const unsigned long  DATE_MAX            = 0xFFFFFFFF;

        static const unsigned int   YEAR_MIN            = 1900;
        static const unsigned int   YEAR_MAX            = 2199;
        static const unsigned int   CHAPTER_MAX         = 1024;

        static const unsigned int   ORDER_FILTER        =      0x3FF;
        static const unsigned long  DAY_FILTER          =     0x7C00;
        static const unsigned long  MONTH_FILTER        =    0x78000;
        static const unsigned long  YEAR_FILTER         = 0x7FF80000;
        static const unsigned long  ORDER_FILTER_INV    = DATE_MAX ^ ORDER_FILTER;
        static const unsigned long  DAY_FILTER_INV      = DATE_MAX ^ DAY_FILTER;
        static const unsigned long  MONTH_FILTER_INV    = DATE_MAX ^ MONTH_FILTER;
        static const unsigned long  YEAR_FILTER_INV     = DATE_MAX ^ YEAR_FILTER;
        static const unsigned long  YEARMONTH_FILTER    = YEAR_FILTER|MONTH_FILTER;
        static const unsigned int   PURE_FILTER         = DATE_MAX ^ ORDER_FILTER;

        // hidden elements' sequence numbers are not shown
        static const unsigned long  VISIBLE_FLAG        = 0x40000000;  // only for ordinal items

        static const unsigned long  ORDINAL_STEP        =      0x400;
        static const unsigned long  ORDINAL_FLAG        = 0x80000000;
        static const unsigned long  ORDINAL_FILTER      = ORDINAL_STEP * ( CHAPTER_MAX - 1 );
        static const unsigned long  ORDINAL_LAST        = ORDINAL_FILTER;

        static const unsigned long  ORDER_MAX           = ORDER_FILTER;

        static const unsigned long  TOPIC_NO_FLAGS_FILTER   = ORDINAL_FILTER|ORDER_FILTER;

        static const unsigned long  TOPIC_MIN           = VISIBLE_FLAG|ORDINAL_FLAG;
        static const unsigned long  GROUP_MIN           = ORDINAL_FLAG;

        static std::string          s_date_format_order;
        static char                 s_date_format_separator;

        typedef unsigned long date_t;

        void operator=( const Date& date )
        { m_date = date.m_date; }
        void operator=( const Date::date_t& date )
        { m_date = date; }

        bool operator>( const Date& date_r )
        { return( m_date > date_r.m_date ); }
        bool operator>=( const Date& date_r )
        { return( m_date >= date_r.m_date ); }
        bool operator<( const Date& date_r )
        { return( m_date < date_r.m_date ); }

        bool operator==( const Date& date )
        { return( m_date == date.m_date ); }
        bool operator==( const Date::date_t& date )
        { return( m_date == date ); }

        bool operator!=( const Date& date )
        { return( m_date != date.m_date ); }

        explicit                    Date( date_t date )
            :   m_date( date ) {}
                                    Date( unsigned int y, unsigned int m, unsigned int d,
                                          unsigned int o = 0 )
            :   m_date( ( y << 19 ) | ( m << 15 ) | ( d << 10 ) | o ) {}
        explicit                    Date( const std::string& );
        // TODO: later: explicit                    Date( time_t );
        // ORDINAL C'TOR
                                    Date( unsigned int o1, unsigned int o2 )
            :   m_date( VISIBLE_FLAG | ORDINAL_FLAG | ( o1 << 10 ) | o2 ) {}

        static date_t               get_today( int order = 0 )
        {
            time_t t = time( NULL );
            struct tm* ti = localtime( &t );
            return make_date( ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday, order );
        }

        void                        set_now( int order = 0 )
        { m_date = get_today( order ); }

        date_t                      get_pure() const
        { return( m_date & PURE_FILTER ); }
        static date_t               get_pure( const date_t d )
        { return( d & PURE_FILTER ); }

        date_t                      get_yearmonth() const
        { return( m_date & YEARMONTH_FILTER ); }
        static date_t               get_yearmonth( const date_t d )
        { return( d & YEARMONTH_FILTER ); }

        unsigned int                get_year() const
        { return ( ( m_date & YEAR_FILTER ) >> 19 ); }
        static unsigned int         get_year( const date_t d )
        { return ( ( d & YEAR_FILTER ) >> 19 ); }
        
        static int get_YMD( long d, char c )
        {
            switch( c )
            {
                case 'Y':
                    return get_year( d );
                case 'M':
                    return get_month( d );
                case 'D':
                default: // no error checking for now
                    return get_day( d );
            }
        }

        std::string                 get_year_str() const;
        std::string                 get_month_str() const;
        std::string                 get_day_str() const;
        std::string                 get_weekday_str() const;

        unsigned int                get_days_in_month() const;

        bool                        is_leap_year() const
        {
            int year( get_year() );
            if( ( year % 400 ) == 0 )
                return true;
            else if( ( year % 100 ) == 0 )
                return false;
            else if( ( year % 4 ) == 0 )
                return true;

            return false;
        }

        unsigned int                get_month() const
        { return( ( m_date & MONTH_FILTER ) >> 15 ); }
        static unsigned int         get_month( const date_t d )
        { return( ( d & MONTH_FILTER ) >> 15 ); }

#ifndef LIFEO_WINDOZE
        Glib::Date::Month           get_month_glib() const;
#endif

        unsigned int                get_day() const
        { return( ( m_date & DAY_FILTER ) >> 10 ); }
        static unsigned int         get_day( const date_t d )
        { return( ( d & DAY_FILTER ) >> 10 ); }

        unsigned int                get_order() const
        { return( m_date & ORDER_FILTER ); }
        static unsigned int         get_order( const date_t d )
        { return( d & ORDER_FILTER ); }

        unsigned int                get_ordinal_order() const
        { return( ( m_date & ORDINAL_FILTER ) >> 10 ); }
        static unsigned int         get_ordinal_order( const date_t d )
        { return( ( d & ORDINAL_FILTER ) >> 10 ); }

        static Date::date_t         get_order_begin() {
            return ORDINAL_FLAG;
        }

#ifndef LIFEO_WINDOZE
        Glib::Date                  get_glib() const
        { return Glib::Date( get_day(), get_month_glib(), get_year() ); }
#endif
        time_t                      get_ctime() const
        { return get_ctime( m_date ); }
        static time_t               get_ctime( const date_t );

        void                        set_year( unsigned int y )
        {
            if( y >= YEAR_MIN && y <= YEAR_MAX )
            {
                m_date &= YEAR_FILTER_INV;
                m_date |= ( y << 19 );
            }
        }
        void                        set_month( unsigned int m )
        {
            if( m < 13 )
            {
                m_date &= MONTH_FILTER_INV;
                m_date |= ( m << 15 );
            }
        }
        void                        set_day( unsigned int d )
        {
            if( d < 32 )
            {
                m_date &= DAY_FILTER_INV;
                m_date |= ( d << 10 );
            }
        }
        void                        reset_order_0()
        { m_date &= ORDER_FILTER_INV; }
        void                        reset_order_1()
        { m_date &= ORDER_FILTER_INV; m_date |= 0x1; }
        static void                 reset_order_1( date_t& date )
        { date &= ORDER_FILTER_INV; date |= 0x1; }

        static Result               parse_string( Date::date_t*, const std::string& );
        std::string                 format_string() const
        { return format_string( m_date, s_date_format_order, s_date_format_separator ); }
        static std::string          format_string( const date_t date )
        { return format_string( date, s_date_format_order, s_date_format_separator ); }
        static std::string          format_string( const date_t, const std::string&, const char );
        static std::string          format_string_dt( const time_t );
        static std::string          format_string_d( const time_t );

        bool                        is_valid() const
        {
            return( get_day() > 0 && get_day() <= get_days_in_month() );
        }
        bool                        is_set() const
        { return( m_date != NOT_SET ); }

        bool                        is_ordinal() const
        { return( m_date & ORDINAL_FLAG ); }
        static bool                 is_ordinal( const date_t d )
        { return( d & ORDINAL_FLAG ); }

        bool                        is_hidden() const
        { return( is_ordinal() && !( m_date & VISIBLE_FLAG ) ); }
        static bool                 is_hidden( const date_t d )
        { return( is_ordinal( d ) && !( d & VISIBLE_FLAG ) ); }

        void                        forward_months( int months );
        void                        forward_month();
        void                        forward_day();

        void                        backward_ordinal_order()
        { if( get_ordinal_order() > 0 ) m_date -= ORDINAL_STEP; }
        static void                 backward_ordinal_order( Date::date_t& d )
        { if( get_ordinal_order( d ) > 0 ) d -= ORDINAL_STEP; }
        void                        forward_ordinal_order()
        { if( ( m_date & ORDINAL_FILTER ) < ORDINAL_LAST ) m_date += ORDINAL_STEP; }
        static void                 forward_ordinal_order( Date::date_t& d )
        { if( ( d & ORDINAL_FILTER ) != ORDINAL_LAST ) d += ORDINAL_STEP; }

        static date_t               make_year( unsigned int y )
        { return( y << 19 ); }
        static date_t               make_month( unsigned int m )
        { return( m << 15 ); }
        static date_t               make_day( unsigned int m )
        { return( m << 10 ); }
        static date_t               make_date( unsigned int y, unsigned int m, unsigned int d,
                                               unsigned int o = 0 )
        { return( ( y << 19 ) | ( m << 15 ) | ( d << 10 ) | o ); }
        static date_t               make_date( unsigned int c, unsigned int o = 0 )
        { return( TOPIC_MIN | ( ( c - 1 ) * ORDINAL_STEP ) | o ); }

        unsigned int                calculate_days_between( const Date& ) const;
        unsigned int                calculate_months_between( Date::date_t ) const;
        static unsigned int         calculate_months_between( Date::date_t, Date::date_t );

    //protected:
        date_t                      m_date;
};

/*
inline  int
subtract_months( Date d1, Date d2 )
{
    return ( ( isolate_month( d1 ) - isolate_month( d2 ) ) +
             ( ( isolate_year( d1 ) - isolate_year( d2 ) ) * 12 ) );
}
*/

inline bool
compare_dates( const Date::date_t& date_l, const Date::date_t& date_r )
{
    return( date_l > date_r );
}

typedef bool( *FuncCompareDates )( const Date::date_t&, const Date::date_t& ) ;

// GLOBAL FUNCTIONS ================================================================================
void                print_error( const std::string& );
void                print_info( const std::string& );

#if LIFEOGRAPH_DEBUG_BUILD
#define PRINT_DEBUG( msg )  std::cout << "DEBUG: " << msg << std::endl
#define PRINT_DEBUG2( a1, a2 )  std::cout << "DEBUG: " << a1 << a2 << std::endl
#define PRINT_DEBUG3( a1, a2, a3 )  std::cout << "DEBUG: " << a1 << a2 << a3 << std::endl
#define PRINT_DEBUG4( a1, a2, a3, a4 )  std::cout << "DEBUG: " << a1 << a2 << a3 << a4 << std::endl
#else
#define PRINT_DEBUG( msg )  ;
#define PRINT_DEBUG2( a1, a2 )  ;
#define PRINT_DEBUG3( a1, a2, a3 )  ;
#define PRINT_DEBUG4( a1, a2, a3, a4 )  ;
#endif

long                convert_string( const std::string& );
bool                str_ends_with ( const std::string&, const std::string& );

#ifndef LIFEO_WINDOZE
Gdk::RGBA           contrast2( const Gdk::RGBA&, const Gdk::RGBA&, const Gdk::RGBA& );
//Gdk::RGBA           contrast( const Gdk::RGBA&, const Gdk::RGBA& ); DEPRECATED
Gdk::RGBA           midtone( const Gdk::RGBA&, const Gdk::RGBA& );
Gdk::RGBA           midtone( const Gdk::RGBA&, const Gdk::RGBA&, float );

inline Glib::ustring
convert_gdkcolor_to_html( const Gdk::RGBA& gdkcolor )
{
    // this function's source of inspiration is Geany
    char buffer[ 8 ];

    g_snprintf( buffer, 8, "#%02X%02X%02X",
            gdkcolor.get_red_u() >> 8,
            gdkcolor.get_green_u() >> 8,
            gdkcolor.get_blue_u() >> 8 );
    return buffer;
}

inline Glib::ustring
convert_gdkrgba_to_string( const Gdk::RGBA& gdkcolor )
{
    char buffer[ 14 ];
    g_snprintf( buffer, 14, "#%04X%04X%04X",
            int( gdkcolor.get_red() * 0xFFFF ),
            int( gdkcolor.get_green() * 0xFFFF ),
            int( gdkcolor.get_blue() * 0xFFFF ) );
    return buffer;
}
#endif

std::ios::pos_type  get_file_size( std::ifstream& );

bool                copy_file_suffix( const std::string&, const std::string&, int );

std::string         get_env_lang();

#ifndef LIFEO_WINDOZE
Gtk::MenuItem*      create_menuitem_markup( const Glib::ustring&,
                                            const Glib::SignalProxy0< void >::SlotType& );
#endif

// COMMON SIGNALS ==================================================================================
#ifndef LIFEO_WINDOZE
typedef sigc::signal< void >        SignalVoid;
#endif

typedef std::set< std::string >     ListPaths;

// ENCRYPTION ======================================================================================
class Cipher
{
    public:
        static const int    cCIPHER_ALGORITHM   = GCRY_CIPHER_AES256;
        static const int    cCIPHER_MODE        = GCRY_CIPHER_MODE_CFB;
        static const int    cIV_SIZE            = 16; // = 128 bits
        static const int    cSALT_SIZE          = 16; // = 128 bits
        static const int    cKEY_SIZE           = 32; // = 256 bits
        static const int    cHASH_ALGORITHM     = GCRY_MD_SHA256;

        static bool         init();

        static void         create_iv( unsigned char** );
        static void         expand_key( char const*,
                                        const unsigned char*,
                                        unsigned char** );
        static void         create_new_key( char const*,
                                            unsigned char**,
                                            unsigned char** );
        static void         encrypt_buffer( unsigned char*,
                                            size_t&,
                                            const unsigned char*,
                                            const unsigned char* );
        static void         decrypt_buffer( unsigned char*,
                                            size_t,
                                            const unsigned char*,
                                            const unsigned char* );

    protected:

    private:

};

struct CipherBuffers
{
    CipherBuffers()
    :   buffer( NULL ), salt( NULL ), iv( NULL ), key( NULL ) {}

    unsigned char* buffer;
    unsigned char* salt;
    unsigned char* iv;
    unsigned char* key;

    void clear()
    {
        if( buffer ) delete[] buffer;
        if( salt ) delete[] salt;
        if( iv ) delete[] iv;
        if( key ) delete[] key;
    }
};

// ENTRY WIDGET WITH SELF-HANDLED CLEAR ICON =======================================================
#ifndef LIFEO_WINDOZE
class EntryClear : public Gtk::Entry
{
    public:
                            EntryClear( BaseObjectType*, const Glib::RefPtr<Gtk::Builder>& );

    protected:
        void                handle_icon_press( Gtk::EntryIconPosition, const GdkEventButton* );
        void                handle_icon_release( Gtk::EntryIconPosition, const GdkEventButton* );
        virtual void        on_changed();
        virtual bool        on_key_release_event( GdkEventKey* );
        bool                m_pressed;
};
#endif

// MENU 2 ==========================================================================================
#if( GTKMM_MAJOR_VERSION < 3 )
#ifndef LIFEO_WINDOZE

class Menu2 : public Gtk::Menu
{
    public:
        Menu2() { }
        Menu2( BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& )
        :   Gtk::Menu( cobject ) { }

    protected:

    private:

    friend class Menubutton;
};

class Menubutton : public Gtk::ToggleButton
{
    public:
                            Menubutton( const Gtk::StockID&,
                                        const Glib::ustring&,
                                        Gtk::ReliefStyle = Gtk::RELIEF_NONE,
                                        Gtk::IconSize = Gtk::ICON_SIZE_BUTTON,
                                        Menu2* = NULL );
                            Menubutton( const Glib::RefPtr< Gdk::Pixbuf >*,
                                        const Glib::ustring&,
                                        Gtk::ReliefStyle = Gtk::RELIEF_NONE,
                                        Menu2* = NULL );
                            Menubutton( BaseObjectType*,
                                        const Glib::RefPtr< Gtk::Builder >& );
        virtual             ~Menubutton();

        Menu2*              get_menu() const;
        void                set_menu( Menu2* );
        bool                clear_menu();
        void                release();

        void                set_label( const Glib::ustring& );
        void                get_menu_position( int&, int&, bool& );

        static Glib::ustring
                            s_builder_name;

    protected:
        bool                on_button_press_event( GdkEventButton* );

        Menu2*              m_menu;
        Gtk::Label*         m_label;
        sigc::connection    m_connection;

    private:
};

class MenuitemRecent : public Gtk::MenuItem
{
    public:
        typedef sigc::signal< void, const std::string& >
                                    Signal_void_string;

                                    MenuitemRecent( const std::string& );

        Signal_void_string          signal_removerecent()
        { return m_signal_removerecent; }

    protected:
        virtual bool                on_motion_notify_event( GdkEventMotion* );
        virtual bool                on_leave_notify_event( GdkEventCrossing* );
        virtual bool                on_button_release_event( GdkEventButton* );
        virtual bool                on_button_press_event( GdkEventButton* );

        Gtk::Image                  m_icon_remove;
        std::string                 m_path;
        bool                        m_flag_deletepressed;

        Signal_void_string          m_signal_removerecent;
};

class FilebuttonRecent : public Menubutton
{
    public:
        const unsigned int  MAX_RECENT_FILE_COUNT( 8 );

        typedef std::list< std::string > ListPaths;
//                                    FilebuttonRecent( const Glib::RefPtr< Gdk::Pixbuf >&,
//                                                      ListPaths*, const Glib::ustring& );
                                    FilebuttonRecent( BaseObjectType*,
                                                      const Glib::RefPtr< Gtk::Builder >& );

        void                        set( ListPaths* );

        ListPaths*                  get_recentlist()
        {
            return m_list_recent;
        }
        std::string                 get_filename() const;
        std::string                 get_filename_recent( int ) const;
        void                        set_filename( const std::string& );
        void                        update_filenames();

        bool                        add_recent( const std::string& );
        void                        remove_recent( const std::string& );

        void                        show_filechooser();

        SignalVoid                  signal_selection_changed();
        SignalVoid                  signal_create_file();

        bool                        handle_motion_notify( GdkEventMotion* );

        static Glib::ustring        fallback_label;

    protected:
        virtual void                on_create_file();
        virtual void                on_size_allocate( Gtk::Allocation& );
        virtual void                on_drag_data_received(
                                            const Glib::RefPtr<Gdk::DragContext>&,
                                            int, int,
                                            const Gtk::SelectionData&, guint, guint );

    private:
        Gtk::FileChooserDialog*     m_filechooserdialog;
        Gtk::Image                  m_icon_new;
        Gtk::Image                  m_icon_browse;
        ListPaths*                  m_list_recent;
        SignalVoid                  m_signal_selectionchanged;
        SignalVoid                  m_signal_createfile;
};

#endif
#endif

// FILE FILTERS ====================================================================================
//class FilefilterAny : public Gtk::FileFilter
//{
//  public:
//      FilefilterAny()
//      {
//          set_name( _("All Files") );
//          add_pattern( "*" );
//      }
//};
//
//class FilefilterDiary : public Gtk::FileFilter
//{
//  public:
//      FilefilterDiary()
//      {
//          set_name( _("Diary Files (*.diary)") );
//          add_mime_type( "application/x-lifeographdiary" );
//      }
//};

// DIALOGEVENT =====================================================================================
#ifndef LIFEO_WINDOZE
class DialogEvent : public Gtk::Dialog
{
    public:
                            DialogEvent( const Glib::ustring& );
                            DialogEvent( BaseObjectType*,
                                         const Glib::RefPtr< Gtk::Builder >& );

    protected:
        bool                on_event( GdkEvent* );
        void                handle_logout();

};

// FRAME FOR PRINTING ==============================================================================
Gtk::Frame* create_frame( const Glib::ustring&, Gtk::Widget& );

// TREEVIEW ========================================================================================
// scroll_to_row( Path& ) does not work for some reason. So, implemented our own
bool is_treepath_less( const Gtk::TreePath&, const Gtk::TreePath& );
bool is_treepath_more( const Gtk::TreePath&, const Gtk::TreePath& );

#endif

} // end of namespace HELPERS

#endif

