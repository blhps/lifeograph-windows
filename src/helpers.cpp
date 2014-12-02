/***********************************************************************************

    Copyright (C) 2007-2011 Ahmet Öztürk (aoz_2@yahoo.com)

    Parts of this file are loosely based on an example gcrypt program
    on http://punkroy.drque.net/

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

#include <iomanip>
#include <sstream>
#include <cmath>
#include <unistd.h>

#include "helpers.hpp"


namespace HELPERS
{

/*
void
DONTCALLDIRECTLY::print_internal( std::stringstream& str )
{ }

template< typename Arg1, typename... Args >
void
print_internal( std::stringstream& str, Arg1 arg1, Args... args )
{
    str << arg1;
    print_internal( str, args... );
}*/

Error::Error( const Ustring& error_message )
{
    print_error( error_message );
}

// DATE ============================================================================================
std::string Date::s_date_format_order = "YMD";
char        Date::s_date_format_separator = '.';

Date::Date( const Ustring& str_date )
:   m_date( 0 )
{
    if( parse_string( &m_date, str_date ) != OK )
        m_date = NOT_SET;
}

// TODO: later...
//Date::Date( time_t t )
//{
//    struct tm* timeinfo = localtime( &t );
//    m_date = make_date( timeinfo->tm_year + 1900,
//                        timeinfo->tm_mon +1,
//                        timeinfo->tm_mday );
//    timeinfo->tm_hour = 0;
//    timeinfo->tm_min = 0;
//    timeinfo->tm_sec = 0;
//}

#ifndef LIFEO_WINDOZE
Glib::Date::Month
Date::get_month_glib() const
{
    switch( ( m_date & MONTH_FILTER ) >> 15 )
    {
        case 1:
            return Glib::Date::JANUARY;
        case 2:
            return Glib::Date::FEBRUARY;
        case 3:
            return Glib::Date::MARCH;
        case 4:
            return Glib::Date::APRIL;
        case 5:
            return Glib::Date::MAY;
        case 6:
            return Glib::Date::JUNE;
        case 7:
            return Glib::Date::JULY;
        case 8:
            return Glib::Date::AUGUST;
        case 9:
            return Glib::Date::SEPTEMBER;
        case 10:
            return Glib::Date::OCTOBER;
        case 11:
            return Glib::Date::NOVEMBER;
        case 12:
            return Glib::Date::DECEMBER;
        default:
            return Glib::Date::BAD_MONTH;
    }
}
#endif

Result
Date::parse_string( Date::date_t* date, const Ustring& str_date )
{
    char c_cur;
    unsigned int num[ 4 ] = { 0, 0, 0, 0 };  // fourth int is for trailing spaces
    int i( 0 );

    for( unsigned j = 0; j < str_date.size(); j++ )
    {
        c_cur = str_date[ j ];
        switch( c_cur )
        {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                if( i > 2 )
                    return INVALID;
                num[ i ] *= 10;
                num[ i ] += ( c_cur - '0' );
                break;
            case ' ':
                if( num[ i ] > 0 )
                    i++;
                break;
            case '.':
            case '-':
            case '/':
                if( num[ i ] == 0 || i == 2 )
                    return INVALID;
                else
                    i++;
                break;
            default:
                return INVALID;
        }
    }

    if( num[ 2 ] ) // temporal
    {
        unsigned int year( 0 );
        unsigned int month( 0 );
        unsigned int day( 0 );

        if( num[ 0 ] > 31 && num[ 1 ] <= 12 && num[ 2 ] <= 31 ) // YMD
        {
            year = num[ 0 ];
            month = num[ 1 ];
            day = num[ 2 ];
        }
        else
        {
            if( num[ 0 ] <= 12 && num[ 1 ] <= 12 ) // both DMY and MDY possible
            {
                if( s_date_format_order[ 0 ] == 'M' )
                {
                    month = num[ 0 ];
                    day = num[ 1 ];
                }
                else
                {
                    day = num[ 0 ];
                    month = num[ 1 ];
                }
            }
            else if( num[ 0 ] <= 31 && num[ 1 ] <= 12 ) // DMY
            {
                month = num[ 1 ];
                day = num[ 0 ];
            }
            else if( num[ 0 ] <= 12 && num[ 1 ] <= 31 ) // MDY
            {
                month = num[ 1 ];
                day = num[ 0 ];
            }
            else
                return INVALID;

            year = num[ 2 ];

            if( year < 100 )
                year += ( year < 30 ? 2000 : 1900 );
        }

        if( year < YEAR_MIN || year > YEAR_MAX )
            return OUT_OF_RANGE;

        Date date_tmp( year, month, day );
        if( ! date_tmp.is_valid() ) // checks days in month
            return INVALID;

        if( date )  // pass NULL when just used for checking
            *date = date_tmp.m_date;

    }
    else if( num[ 1 ] )   // ordinal
    {
        if( num[ 0 ] > CHAPTER_MAX || num[ 1 ] > ORDER_MAX )
            return OUT_OF_RANGE;

        if( date )  // pass NULL when just used for checking
            *date = make_date( num[ 0 ], num[ 1 ] );
    }
    else
        return INVALID;

    return OK;
}

Ustring
Date::format_string( const date_t d, const std::string& format, const char separator )
{
    if( d & ORDINAL_FLAG )
    {
        return( get_order( d ) ? STR::compose( get_ordinal_order( d ) + 1, ".", get_order( d ) ) :
                STR::compose( get_ordinal_order( d ) + 1 ) );
    }
    else
    {
        std::stringstream result;
        
        result << std::setfill( '0' ) << std::setw( 2 ) <<
                  get_YMD( d, format[ 0 ] ) << separator <<
                  std::setfill( '0' ) << std::setw( 2 ) <<
                  get_YMD( d, format[ 1 ] ) << separator <<
                  std::setfill( '0' ) << std::setw( 2 ) <<
                  get_YMD( d, format[ 2 ] );
                  
        return result.str();
    }
}

Ustring
Date::format_string_dt( const time_t time )
{
    struct tm* timeinfo = localtime( &time );
    std::stringstream result;
    
    // TODO: does not respect field order preference
    result << 1900 + timeinfo->tm_year << s_date_format_separator <<
              std::setfill( '0' ) << std::setw( 2 ) <<
              timeinfo->tm_mon + 1 << s_date_format_separator <<
              timeinfo->tm_mday << ", " <<
              timeinfo->tm_hour << ":" << timeinfo->tm_min;
           
    return result.str();
}

// date only
Ustring
Date::format_string_d( const time_t time )
{
    struct tm* timeinfo = localtime( &time );
    std::stringstream result;
    
    // TODO: does not respect field order preference
    result << 1900 + timeinfo->tm_year << s_date_format_separator <<
              std::setfill( '0' ) << std::setw( 2 ) <<
              timeinfo->tm_mon + 1 << s_date_format_separator << timeinfo->tm_mday;
}

Ustring
Date::get_year_str() const
{
    return STR::compose( get_year() );
}

#ifndef LIFEO_WINDOZE
Ustring
Date::get_month_str() const
{
    return Glib::Date( 20, get_month_glib(), 2000 ).format_string( "%b" );
}
#endif

Ustring
Date::get_weekday_str() const
{
    // from wikipedia: http://en.wikipedia.org/wiki/Calculating_the_day_of_the_week
    const unsigned int year = get_year();
    const unsigned int century = ( year - ( year % 100 ) ) / 100;
    int c = 2 * ( 3 - ( century % 4 ) );
    int y = year % 100;
    y = y + floor( y / 4 );

    static const int t_m[] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
    struct tm ti;

    int m = get_month() - 1;
    int d = ( c + y + t_m[ m ] + get_day() );

    if( !( get_year() % 4 ) && m < 2 )  // leap year!
        d += 6;

    ti.tm_wday = ( d % 7 );
    char buf[ 32 ];
    strftime( buf, 32, "%A", &ti );
    return Ustring( buf );
}

unsigned int
Date::get_days_in_month() const
{
    switch( get_month() )
    {
        case 4: case 6: case 9: case 11:
            return 30;
        case 2:
            return is_leap_year() ? 29 : 28;
    }

    return 31;
}

time_t
Date::get_ctime( const Date::date_t d )
{
    time_t t;
    time( &t );
    struct tm* timeinfo = localtime( &t );
    timeinfo->tm_year = get_year( d ) - 1900;
    timeinfo->tm_mon = get_month( d ) - 1;
    timeinfo->tm_mday = get_day( d );
    timeinfo->tm_hour = 0;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;

    return( mktime( timeinfo ) );
}

void
Date::forward_months( int months )
{
    months += ( ( m_date & MONTH_FILTER ) >> 15 ); // get month
    m_date &= YEAR_FILTER;   // isolate year
    const int mod_months = months % 12;
    if( mod_months == 0 )
    {
        m_date += make_year( ( months / 12 ) - 1);
        m_date |= 0x60000;  // make month 12
    }
    else
    {
        m_date += make_year( months / 12 );
        m_date |= make_month( mod_months );
    }
}

void
Date::forward_month()
{
    int months = get_month() + 1;
    m_date &= YEAR_FILTER;   // isolate year
    const int mod_months = months % 12;
    if( mod_months == 0 )
    {
        m_date += make_year( ( months / 12 ) - 1);
        m_date |= 0x60000;  // make month 12
    }
    else
    {
        m_date += make_year( months / 12 );
        m_date |= make_month( mod_months );
    }
}

void
Date::forward_day()
{
    int day = get_day();
    if( day >= get_days_in_month() )
    {
        set_day( 1 );
        forward_month();
    }
    else
        set_day( day + 1 );
}

unsigned int
Date::calculate_days_between( const Date& date2 ) const
{
#ifndef LIFEO_WINDOZE
    // TODO: create own implementation
    Glib::Date gdate_begin( get_glib() );
    Glib::Date gdate_end( date2.get_glib() );
    return( gdate_begin.days_between( gdate_end ) );
#else
    return 0; // STUB
#endif
}

unsigned int
Date::calculate_months_between( Date::date_t date2 ) const
{
    return calculate_months_between( m_date, date2 );
}

unsigned int
Date::calculate_months_between( Date::date_t date1, Date::date_t date2 )
{
    int dist( 12 * ( get_year( date2 ) - get_year( date1 ) - 1 ) );
    dist += ( 12 - get_month( date1 ) + get_month( date2 ) );

    return( dist < 0 ? -dist : dist );
}

// CONSOLE MESSAGES ================================================================================
void
print_error( const Ustring& description )
{
    std::cerr << "ERROR: " << description << std::endl;
}

void
print_info( const Ustring& description )
{
    std::cout << "INFO: " << description << std::endl;
}

// STRING PROCESSING
long
convert_string( const std::string& str )
{
    //TODO: add negative number support
    long result( 0 );   // result
    for( unsigned int i = 0;
         i < str.size() && i < 10 && int ( str[ i ] ) >= '0' && int ( str[ i ] ) <= '9';
         i++ )
    {
        result = ( result * 10 ) + int ( str[ i ] ) - '0';
    }
    return( result );
}

bool
str_ends_with ( const std::string& str, const std::string& end )
{
    if( str.length() > end.length() )
    {
        return( str.compare( str.length() - end.length(), end.length(), end ) == 0 );
    }
    else
    {
        return false;
    }
}

// COLOR PROCESSING
#ifndef LIFEO_WINDOZE
Color
contrast2( const Gdk::RGBA& bg, const Gdk::RGBA& c1, const Gdk::RGBA& c2 )
{
    double dist1 = ( fabs( bg.get_red() - c1.get_red() ) +
                     fabs( bg.get_green() - c1.get_green() ) +
                     fabs( bg.get_blue() - c1.get_blue() ) );

    double dist2 = ( fabs( bg.get_red() - c2.get_red() ) +
                     fabs( bg.get_green() - c2.get_green() ) +
                     fabs( bg.get_blue() - c2.get_blue() ) );

    if( dist1 > dist2 )
        return c1;
    else
        return c2;
}

Color
midtone( const Gdk::RGBA& c1, const Gdk::RGBA& c2 )
{
    Gdk::RGBA midtone;
    midtone.set_red_u( ( c1.get_red_u() + c2.get_red_u() ) / 2.0 );
    midtone.set_green_u( ( c1.get_green_u() + c2.get_green_u() ) / 2.0 );
    midtone.set_blue_u( ( c1.get_blue_u() + c2.get_blue_u() ) / 2.0 );
    midtone.set_alpha( 1.0 );
    return midtone;
}

Color
midtone( const Gdk::RGBA& c1, const Gdk::RGBA& c2, float ratio )
{
    Gdk::RGBA midtone;
    midtone.set_red_u( ( c1.get_red_u() * ratio ) + ( c2.get_red_u() * ( 1.0 - ratio ) ) );
    midtone.set_green_u( ( c1.get_green_u() * ratio ) + ( c2.get_green_u() * ( 1.0 - ratio ) ) );
    midtone.set_blue_u( ( c1.get_blue_u() * ratio ) + ( c2.get_blue_u() * ( 1.0 - ratio ) ) );
    midtone.set_alpha( 1.0 );
    return midtone;
}
#endif

// FILE FUNCTIONS
std::ios::pos_type
get_file_size( std::ifstream& file )
{
   std::ios::pos_type size;
   const std::ios::pos_type currentPosition = file.tellg();

   file.seekg( 0, std::ios_base::end );
   size = file.tellg();
   file.seekg( currentPosition );

   return size;
}

// FILE I/O
bool
copy_file_suffix( const std::string& source_path, const std::string& suffix1, int suffix2 )
{
#ifndef LIFEO_WINDOZE
    try
    {
        Glib::RefPtr< Gio::File > file_src = Gio::File::create_for_path( source_path );
        
        if( file_src )
#endif
        {
            std::stringstream ss;
            ss << source_path << suffix1;
            if( suffix2 >= 0 )
                ss << suffix2;
            int index( 0 );

            while( access( ss.str().c_str(), F_OK ) == 0 )
            {
                ss.str( std::string() );
                ss.clear();
                ss << source_path << suffix1;
                if( suffix2 >= 0 )
                    ss << suffix2;
                ss << " (" << ++index << ")";
            }

#ifndef LIFEO_WINDOZE
            Glib::RefPtr< Gio::File > file_dest = Gio::File::create_for_path( ss.str() );
            file_src->copy( file_dest );
#else
            CopyFileA( source_path.c_str(), ss.str().c_str(), true );
#endif
        }
#ifndef LIFEO_WINDOZE
    }
    catch( ... )
    {
        return false;
    }
#endif
    return true;
}

#ifndef LIFEO_WINDOZE
// ENVIRONMENT VARIABLES
std::string
get_env_lang()
{
    static std::string s_lang_env( "" );
    if( s_lang_env.empty() )
    {
        std::string lang = Glib::getenv( "LANG" );
        if( lang.empty() || lang == "C" || lang == "c" )
            s_lang_env = "en";
        else
            s_lang_env = lang;
    }
    return s_lang_env;
}

gunichar
char_lower( gunichar c )
{
    return Glib::Unicode::tolower( c );
}

bool
is_char_alpha( gunichar c )
{
    return Glib::Unicode::isalpha( c );
}

#else
wchar_t*
convert_utf8_to_16( const Ustring& str8 )
{
    wchar_t* str16 = new wchar_t[ str8.size() + 1 ];
    MultiByteToWideChar( CP_UTF8, 0, str8.c_str(), str8.size() + 1, str16, str8.size() + 1 );
    
    return str16;
}

char*
convert_utf16_to_8( const wchar_t* str16 )
{
    char* str8;
    int size = WideCharToMultiByte( CP_UTF8, 0, str16, -1, str8, 0, NULL, NULL );
    str8 = new char[ size ];
    WideCharToMultiByte( CP_UTF8, 0, str16, -1, str8, size, NULL, NULL );

    return str8;
}

wchar_t
char_lower( wchar_t c )
{
    return LOWORD( CharLower( ( LPWSTR ) MAKELONG( c, 0 ) ) );
}

bool
is_char_alpha( wchar_t c )
{
    return IsCharAlpha( c );
}
#endif

// ENCRYPTION ======================================================================================
bool
Cipher::init()
{
    // http://www.gnupg.org/documentation/manuals/gcrypt/Initializing-the-library.html

    // initialize subsystems:
    if( ! gcry_check_version( NULL ) )  // TODO: check version
    {
        print_error( "Libgcrypt version mismatch" );
        return false;
    }

    // disable secure memory
    gcry_control( GCRYCTL_DISABLE_SECMEM, 0 );

    // MAYBE LATER:
    /*
    // suppress warnings
    gcry_control( GCRYCTL_SUSPEND_SECMEM_WARN );

    // allocate a pool of 16k secure memory. this makes the secure memory...
    // ...available and also drops privileges where needed
    gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );
     
    // resume warnings
    gcry_control( GCRYCTL_RESUME_SECMEM_WARN );
    */

    // tell Libgcrypt that initialization has completed
    gcry_control( GCRYCTL_INITIALIZATION_FINISHED, 0 );

    return true;
}

void
Cipher::create_iv( unsigned char** iv )
{
    // (Allocate memory for and fill with strong random data)
    *iv = ( unsigned char* ) gcry_random_bytes( Cipher::cIV_SIZE, GCRY_STRONG_RANDOM );

    if( ! *iv )
        throw Error( "Unable to create IV" );
}

void
Cipher::expand_key( const char* passphrase,
                    const unsigned char* salt,
                    unsigned char** key )
{
    gcry_md_hd_t hash;
    gcry_error_t error = 0;
    int hashdigestsize;
    unsigned char* hashresult;

    // OPEN MESSAGE DIGEST ALGORITHM
    error = gcry_md_open( &hash, cHASH_ALGORITHM, 0 );
    if( error )
        throw Error( "Unable to open message digest algorithm: %s" ); //, gpg_strerror( Error ) );

    // RETRIVE DIGEST SIZE
    hashdigestsize = gcry_md_get_algo_dlen( cHASH_ALGORITHM );

    // ADD SALT TO HASH
    gcry_md_write( hash, salt, cSALT_SIZE );

    // ADD PASSPHRASE TO HASH
    gcry_md_write( hash , passphrase , strlen( passphrase ) );

    // FETCH DIGEST (THE EXPANDED KEY)
    hashresult = gcry_md_read( hash , cHASH_ALGORITHM );

    if( ! hashresult )
    {
        gcry_md_close( hash );
        throw Error( "Unable to finalize key" );
    }

    // ALLOCATE MEMORY FOR KEY
    // can't use the 'HashResult' because those resources are freed after the
    // hash is closed
    *key = new unsigned char[ cKEY_SIZE ];
    if( ! key )
    {
        gcry_md_close( hash );
        throw Error( "Unable to allocate memory for key" );
    }

    // DIGEST SIZE SMALLER THEN KEY SIZE?
    if( hashdigestsize < cKEY_SIZE )
    {
        // PAD KEY WITH '0' AT THE END
        memset( *key , 0 , cKEY_SIZE );

        // COPY EVERYTHING WE HAVE
        memcpy( *key , hashresult , hashdigestsize );
    }
    else
        // COPY ALL THE BYTES WE'RE USING
        memcpy( *key , hashresult , hashdigestsize );

    // FINISHED WITH HASH
    gcry_md_close( hash );
}

// create new expanded key
void
Cipher::create_new_key( char const * passphrase,
                        unsigned char** salt,
                        unsigned char** key )
{
    // ALLOCATE MEMORY FOR AND FILL WITH STRONG RANDOM DATA
    *salt = ( unsigned char* ) gcry_random_bytes( cSALT_SIZE, GCRY_STRONG_RANDOM );

    if( ! *salt )
        throw Error( "Unable to create salt value" );

    expand_key( passphrase, *salt, key );
}

void
Cipher::encrypt_buffer ( unsigned char* buffer,
                         size_t& size,
                         const unsigned char* key,
                         const unsigned char* iv )
{
    gcry_cipher_hd_t    cipher;
    gcry_error_t        error = 0;

    error = gcry_cipher_open( &cipher, cCIPHER_ALGORITHM, cCIPHER_MODE, 0 );

    if( error )
        throw Error( "unable to initialize cipher: " ); // + gpg_strerror( Error ) );

    // GET KEY LENGTH
    int cipherKeyLength = gcry_cipher_get_algo_keylen( cCIPHER_ALGORITHM );
    if( ! cipherKeyLength )
        throw Error( "gcry_cipher_get_algo_keylen failed" );

    // SET KEY
    error = gcry_cipher_setkey( cipher, key, cipherKeyLength );
    if( error )
    {
        gcry_cipher_close( cipher );
        throw Error( "Cipher key setup failed: %s" ); //, gpg_strerror( Error ) );
    }

    // SET INITILIZING VECTOR (IV)
    error = gcry_cipher_setiv( cipher, iv, cIV_SIZE );
    if( error )
    {
        gcry_cipher_close( cipher );
        throw Error( "Unable to setup cipher IV: %s" );// , gpg_strerror( Error ) );
    }

    // ENCRYPT BUFFER TO SELF
    error = gcry_cipher_encrypt( cipher, buffer, size, NULL, 0 );

    if( error )
    {
        gcry_cipher_close( cipher );
        throw Error( "Encrption failed: %s" ); // , gpg_strerror( Error ) );
    }

    gcry_cipher_close( cipher );
}

void
Cipher::decrypt_buffer ( unsigned char* buffer,
                         size_t size,
                         const unsigned char* key,
                         const unsigned char* iv )
{
    gcry_cipher_hd_t cipher;
    gcry_error_t error = 0;

    error = gcry_cipher_open( &cipher, cCIPHER_ALGORITHM, cCIPHER_MODE, 0 );

    if( error )
        throw Error( "Unable to initialize cipher: " ); // + gpg_strerror( Error ) );

    // GET KEY LENGTH
    int cipherKeyLength = gcry_cipher_get_algo_keylen( cCIPHER_ALGORITHM );
    if( ! cipherKeyLength )
        throw Error( "gcry_cipher_get_algo_keylen failed" );

    // SET KEY
    error = gcry_cipher_setkey( cipher, key, cipherKeyLength );
    if( error )
    {
        gcry_cipher_close( cipher );
        throw Error( "Cipher key setup failed: %s" ); //, gpg_strerror( Error ) );
    }

    // SET IV
    error = gcry_cipher_setiv( cipher, iv, cIV_SIZE );
    if( error )
    {
        gcry_cipher_close( cipher );
        throw Error( "Unable to setup cipher IV: %s" );// , gpg_strerror( Error ) );
    }

    // DECRYPT BUFFER TO SELF
    error = gcry_cipher_decrypt( cipher, buffer, size, NULL, 0 );

    if( error )
    {
        gcry_cipher_close( cipher );
        throw Error( "Encryption failed: %s" ); // , gpg_strerror( Error ) );
    }

    gcry_cipher_close( cipher );
}

#ifndef LIFEO_WINDOZE

// MARKED UP MENU ITEM
Gtk::MenuItem*
create_menuitem_markup( const Glib::ustring& str_markup,
                        const Glib::SignalProxy0< void >::SlotType& handler )
{
    // thanks to GNote for showing the way
    Gtk::MenuItem* menuitem = Gtk::manage( new Gtk::MenuItem( str_markup ) );
    Gtk::Label* label = dynamic_cast< Gtk::Label* >( menuitem->get_child() );
    if( label )
        label->set_use_markup( true );
    menuitem->signal_activate().connect( handler );
    return menuitem;
}

// ENTRYCLEAR ======================================================================================
EntryClear::EntryClear( BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& )
    :   Gtk::Entry( cobject ), m_pressed( false )
{
    signal_icon_press().connect( sigc::mem_fun( this, &EntryClear::handle_icon_press ) );
    signal_icon_release().connect( sigc::mem_fun( this, &EntryClear::handle_icon_release ) );
}

void
EntryClear::handle_icon_press( Gtk::EntryIconPosition pos, const GdkEventButton* event )
{
    if( pos == Gtk::ENTRY_ICON_SECONDARY && event->button == 1 )
        m_pressed = true;
}

void
EntryClear::handle_icon_release( Gtk::EntryIconPosition pos, const GdkEventButton* event )
{
    if( pos == Gtk::ENTRY_ICON_SECONDARY && event->button == 1 )
    {
        if( m_pressed )
            set_text( "" );
        m_pressed = false;
    }
}

void
EntryClear::on_changed()
{
    if( get_text().empty() )
    {
        // Fall-back to C API as gtkmm has problems with this:
        unset_icon( Gtk::ENTRY_ICON_SECONDARY );
        set_icon_activatable( false, Gtk::ENTRY_ICON_SECONDARY );
    }
    else
    {
        try
        {
            set_icon_from_pixbuf(
                    Gtk::IconTheme::get_default()->load_icon( "edit-clear-symbolic", 16 ),
                    Gtk::ENTRY_ICON_SECONDARY );
        }
        catch( ... )
        {
            set_icon_from_stock( Gtk::Stock::CLEAR, Gtk::ENTRY_ICON_SECONDARY );
        }
        set_icon_activatable( true, Gtk::ENTRY_ICON_SECONDARY );
    }

    Gtk::Entry::on_changed();
}

bool
EntryClear::on_key_release_event( GdkEventKey* event )
{
    if( event->keyval == GDK_KEY_Escape )
        set_text( "" );

    return Gtk::Entry::on_key_release_event( event );
}

// MENUBUTTON ======================================================================================
#if( GTKMM_MAJOR_VERSION < 3 )

Glib::ustring       Menubutton::s_builder_name;

Menubutton::Menubutton( const Gtk::StockID& stockid,
                        const Glib::ustring& label,
                        Gtk::ReliefStyle style,
                        Gtk::IconSize iconsize,
                        Menu2* menu )
    :   Gtk::ToggleButton(), m_menu( menu )
{
    set_relief( style );

    if( menu == NULL )
        m_menu = new Menu2;
    Gtk::Box* hbox = Gtk::manage( new Gtk::Box );
    Gtk::Image* icon = Gtk::manage( new Gtk::Image( stockid, iconsize ) );
    m_label = Gtk::manage( new Gtk::Label( label ) );
    Gtk::Arrow* arrow = Gtk::manage(
            new Gtk::Arrow( Gtk::ARROW_DOWN, Gtk::SHADOW_IN ) );

    hbox->pack_start( *icon, Gtk::PACK_SHRINK );
    hbox->pack_start( *m_label, Gtk::PACK_EXPAND_WIDGET );
    hbox->pack_start( *arrow, Gtk::PACK_SHRINK );
    add( *hbox );
    m_menu->attach_to_widget( *this );
    m_connection = m_menu->signal_deactivate().connect(
            sigc::mem_fun( *this, &Menubutton::release ) );
    add_events( Gdk::BUTTON_PRESS_MASK );
}

//FIXME: code duplication
Menubutton::Menubutton( const Glib::RefPtr< Gdk::Pixbuf >* pixbuf,
                        const Glib::ustring& label,
                        Gtk::ReliefStyle style,
                        Menu2* menu )
:   Gtk::ToggleButton(), m_menu( menu )
{
    set_relief( style );
    Gtk::Arrow* arrow( NULL );
    Gtk::Image* icon( NULL );
    Gtk::Box* hbox( NULL );
    try
    {
        if( menu == NULL )
            m_menu = new Menu2;
        hbox = Gtk::manage( new Gtk::Box );
        if( pixbuf )
            icon = Gtk::manage( new Gtk::Image( *pixbuf ) );
        m_label = Gtk::manage( new Gtk::Label( label ) );
        arrow = Gtk::manage( new Gtk::Arrow( Gtk::ARROW_DOWN, Gtk::SHADOW_IN ) );
    }
    catch( ... )
    {
        throw Error( "Menubutton creation failed" );
    }

    if( pixbuf )
        hbox->pack_start( *icon, Gtk::PACK_SHRINK );
    hbox->pack_start( *m_label, Gtk::PACK_EXPAND_WIDGET );
    hbox->pack_start( *arrow, Gtk::PACK_SHRINK );
    add( *hbox );
    m_menu->attach_to_widget( *this );
    m_connection = m_menu->signal_deactivate().connect(
            sigc::mem_fun( *this, &Menubutton::release ) );
    add_events( Gdk::BUTTON_PRESS_MASK );
}

Menubutton::Menubutton( BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder )
:   Gtk::ToggleButton( cobject ), m_menu( NULL )
{
    if( !s_builder_name.empty() )
    {
        try
        {
            builder->get_widget_derived( s_builder_name + "_menu", m_menu );
            //builder->get_widget( s_builder_name + "_icon", m_icon );
            //builder->get_widget( s_builder_name + "_label", m_label );
            s_builder_name = "";
        }
        catch( ... )
        {
            throw Error( "Menubutton creation failed" );
        }

        m_menu->attach_to_widget( *this );
        m_connection = m_menu->signal_deactivate().connect(
                sigc::mem_fun( this, &Menubutton::release ) );
    }

    add_events( Gdk::BUTTON_PRESS_MASK );
}

Menubutton::~Menubutton()
{
    delete m_menu;
}

Menu2*
Menubutton::get_menu() const
{
    return m_menu;
}

void
Menubutton::set_menu( Menu2* menu )
{
    if( m_menu == menu )
        return;

    if( m_menu )
    {
        m_menu->detach();
        m_connection.disconnect();
    }

    m_menu = menu;

    set_sensitive( menu != NULL );

    if( menu )
    {
        m_menu->attach_to_widget( *this );
        m_connection = m_menu->signal_deactivate().connect(
                sigc::mem_fun( this, &Menubutton::release ) );
    }
}

bool
Menubutton::clear_menu()
{
    if( m_menu->get_children().size() > 0 )
    {
        m_menu->detach();
        delete m_menu;
        m_menu = new Menu2;
        m_menu->attach_to_widget( *this );
        m_menu->signal_deactivate().connect( sigc::mem_fun( *this, &Menubutton::release ) );

        return true;
    }
    else
        return false;
}

void
Menubutton::release()
{
    set_active( false );
}

void
Menubutton::set_label( const Glib::ustring& string )
{
    m_label->set_label( string );
}

void
Menubutton::get_menu_position( int& x, int& y, bool& push_in )
{
    get_window()->get_origin( x, y );
    x += get_allocation().get_x();
    y += get_allocation().get_y() + get_allocation().get_height();
    push_in = true;
}

bool
Menubutton::on_button_press_event( GdkEventButton* l_event )
{
    m_menu->popup( sigc::mem_fun(
            *this, &Menubutton::get_menu_position), l_event->button, l_event->time);

    set_active( true );

    return true;
}

#endif

// MENUITEMRECENT ==================================================================================
#if( GTKMM_MAJOR_VERSION < 3 ) // to be ported to gtkmm3 if needed
MenuitemRecent::MenuitemRecent( const std::string& path )
:   Gtk::MenuItem(), m_icon_remove( Gtk::Stock::DELETE, Gtk::ICON_SIZE_MENU ),
    m_path( path ), m_flag_deletepressed( false )
{
    Gtk::Label      *label = Gtk::manage( new Gtk::Label(
                                            Glib::filename_display_basename( path ) ) );
    Gtk::Box        *hbox = Gtk::manage( new Gtk::Box );

    label->set_justify( Gtk::JUSTIFY_LEFT );
    label->set_alignment( Gtk::ALIGN_START );
    label->set_ellipsize( Pango::ELLIPSIZE_START );
    label->set_tooltip_text( path );
    m_icon_remove.set_tooltip_text( _( "Remove from the list" ) );
    hbox->set_spacing( 5 );

    hbox->pack_start( *label );
    hbox->pack_start( m_icon_remove, Gtk::PACK_SHRINK );
    this->add( *hbox );

    hbox->show();
    label->show();
}

bool
MenuitemRecent::on_motion_notify_event( GdkEventMotion* event )
{
    if( !( event->state &
            ( Gdk::BUTTON1_MASK | Gdk::BUTTON2_MASK | Gdk::BUTTON3_MASK ) ) )
        m_icon_remove.show();

    return Gtk::MenuItem::on_motion_notify_event( event );
}

bool
MenuitemRecent::on_leave_notify_event( GdkEventCrossing* event )
{
    m_icon_remove.hide();

    return Gtk::MenuItem::on_leave_notify_event( event );
}

bool
MenuitemRecent::on_button_press_event( GdkEventButton* event )
{
    if( event->x >= m_icon_remove.get_allocation().get_x() &&
        event->x < m_icon_remove.get_allocation().get_x() +
                    m_icon_remove.get_allocation().get_width() )
        m_flag_deletepressed = true;
    else
        m_flag_deletepressed = false;

    return Gtk::MenuItem::on_button_press_event( event );
}

bool
MenuitemRecent::on_button_release_event( GdkEventButton* event )
{
    if( m_flag_deletepressed )
    {
        if( event->x >= m_icon_remove.get_allocation().get_x() &&
            event->x < m_icon_remove.get_allocation().get_x() +
                        m_icon_remove.get_allocation().get_width() )
        {
            m_signal_removerecent.emit( m_path );
            return true;
        }
    }

    return Gtk::MenuItem::on_button_release_event( event );
}

// FILEBUTTONRECENT ================================================================================
Glib::ustring   FilebuttonRecent::fallback_label = _( "Select or Create a Diary" );

/*
FilebuttonRecent::FilebuttonRecent( const Glib::RefPtr< Gdk::Pixbuf >& pixbuf,
                                    ListPaths* list_recent,
                                    const Glib::ustring& label_fallback )
:   Menubutton( &pixbuf, label_fallback, Gtk::RELIEF_NORMAL ),
    m_icon_new( Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU ),
    m_icon_browse( Gtk::Stock::OPEN, Gtk::ICON_SIZE_MENU ),
    m_list_recent( list_recent )
{
    // TODO: m_hbox->set_spacing( 4 );

    m_label->set_ellipsize( Pango::ELLIPSIZE_START );
    m_label->set_alignment( Gtk::ALIGN_LEFT );

    m_menu->items().push_back( Gtk::Menu_Helpers::SeparatorElem() );

    m_menu->items().push_back(
            Gtk::Menu_Helpers::ImageMenuElem( _("_Browse For a Diary..."), m_icon_browse,
                    sigc::mem_fun( *this,
                            &FilebuttonRecent::show_filechooser) ) );
    m_menu->items().push_back(
            Gtk::Menu_Helpers::ImageMenuElem( _("_Create A New Diary..."), m_icon_new,
                    sigc::mem_fun( *this,
                            &FilebuttonRecent::on_create_file) ) );

    m_menu->show_all_children();

    // ACCEPT DROPPED FILES
    //~ std::list< Gtk::TargetEntry > list_targets;
    //~ list_targets.push_back( Gtk::TargetEntry( "STRING" ) );
    //~ list_targets.push_back( Gtk::TargetEntry( "text/plain" ) );
    //~ this->drag_dest_set( list_targets );
    // TODO: i could not figure out how this is supposed to work and replicated
    // almost blindly bmpx' approach.
    drag_dest_set( Gtk::DEST_DEFAULT_ALL, ( Gdk::ACTION_COPY | Gdk::ACTION_MOVE ) );
    drag_dest_add_uri_targets();
}*/

FilebuttonRecent::FilebuttonRecent( BaseObjectType* cobj, const Glib::RefPtr<Gtk::Builder>& bui )
:   Menubutton( cobj, bui ),
    m_icon_new( Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU ),
    m_icon_browse( Gtk::Stock::OPEN, Gtk::ICON_SIZE_MENU ),
    m_list_recent( NULL )
{
    m_label->set_ellipsize( Pango::ELLIPSIZE_START );
    m_label->set_alignment( Gtk::ALIGN_START );
    m_label->set_text( fallback_label );

    m_menu->items().push_back( Gtk::Menu_Helpers::SeparatorElem() );

    m_menu->items().push_back(
            Gtk::Menu_Helpers::ImageMenuElem( _("_Browse For a Diary..."), m_icon_browse,
                    sigc::mem_fun( *this,
                            &FilebuttonRecent::show_filechooser) ) );
    m_menu->items().push_back(
            Gtk::Menu_Helpers::ImageMenuElem( _("_Create A New Diary..."), m_icon_new,
                    sigc::mem_fun( *this,
                            &FilebuttonRecent::on_create_file) ) );

    m_menu->show_all_children();

    // ACCEPT DROPPED FILES
    drag_dest_set( Gtk::DEST_DEFAULT_ALL, ( Gdk::ACTION_COPY | Gdk::ACTION_MOVE ) );
    drag_dest_add_uri_targets();
}

void
FilebuttonRecent::set( ListPaths* list_recent )
{
    m_list_recent = list_recent;
}

std::string
FilebuttonRecent::get_filename() const
{
    return( * m_list_recent->begin() );
}

void
FilebuttonRecent::set_filename( const std::string& path )
{
    if( ! m_list_recent->empty() )
        if( path == * m_list_recent->begin() )
            return;

    m_label->set_label( Glib::filename_display_basename( path ) );
    set_tooltip_text( path );
    add_recent( path );
    m_signal_selectionchanged.emit();
}

void
FilebuttonRecent::update_filenames()
{
    if( ! m_list_recent->empty() )
    {
        for( ListPaths::reverse_iterator iter = m_list_recent->rbegin();
             iter != m_list_recent->rend(); ++iter )
        {
            MenuitemRecent* menuitem = Gtk::manage( new MenuitemRecent( *iter ) );

            menuitem->signal_activate().connect(
                    sigc::bind( sigc::mem_fun( this, &FilebuttonRecent::set_filename ), *iter ) );
            menuitem->signal_removerecent().connect(
                    sigc::mem_fun( this, &FilebuttonRecent::remove_recent ) );

            m_menu->items().push_front( Gtk::Menu_Helpers::Element( *menuitem ) );

            menuitem->show();
        }

        m_label->set_label( Glib::filename_display_basename( * m_list_recent->begin() ) );
        set_tooltip_text( * m_list_recent->begin() );
    }
}

bool
FilebuttonRecent::add_recent( const std::string& path )
{
    // do not add a file twice:
    Gtk::Menu_Helpers::MenuList::iterator iter_menu = m_menu->items().begin();
    ListPaths::iterator iter_recent = m_list_recent->begin();
    for( ; iter_recent != m_list_recent->end(); ++iter_recent )
    {
        if( ( *iter_recent ) == path )
        {
            m_menu->items().erase( iter_menu );
            m_list_recent->erase( iter_recent );
            break;
        }
        else
            ++iter_menu;
    }

    MenuitemRecent* menuitem = Gtk::manage( new MenuitemRecent( path ) );

    menuitem->signal_activate().connect(
            sigc::bind( sigc::mem_fun( *this, &FilebuttonRecent::set_filename ),
                        path ) );
    menuitem->signal_removerecent().connect(
            sigc::mem_fun( *this, &FilebuttonRecent::remove_recent ) );

    m_menu->items().push_front( Gtk::Menu_Helpers::Element( *menuitem ) );

    menuitem->show();

    m_list_recent->push_front( path );

    if( m_list_recent->size() > MAX_RECENT_FILE_COUNT )
    {
        m_menu->items().erase( --iter_menu );
        m_list_recent->erase( --iter_recent );
    }

    return true;    // reserved
}

void
FilebuttonRecent::remove_recent( const std::string& path )
{
    Gtk::Menu_Helpers::MenuList::iterator   iter_menu = m_menu->items().begin();
    std::list< std::string >::iterator      iter_recent = m_list_recent->begin();
    bool flag_update = false;
    for( ; iter_recent != m_list_recent->end(); ++iter_recent )
    {
        if( ( *iter_recent ) == path )
        {
            if( iter_recent == m_list_recent->begin() )
                flag_update = true;
            m_menu->items().erase( iter_menu );
            m_list_recent->erase( iter_recent );
            break;
        }
        else
            ++iter_menu;
    }
    // update current path if first item is deleted
    if( flag_update )
    {
        if( m_list_recent->empty() )
        {
            m_label->set_label( fallback_label );
            set_tooltip_text( "" );
        }
        else
        {
            m_label->set_label( Glib::filename_display_basename( m_list_recent->front() ) );
            set_tooltip_text( m_list_recent->front() );
        }
        m_signal_selectionchanged.emit();
    }
}

void
FilebuttonRecent::show_filechooser()
{
    m_filechooserdialog = new Gtk::FileChooserDialog(
            _( "Select a Diary" ), Gtk::FILE_CHOOSER_ACTION_OPEN );

    if( m_list_recent->size() > 0 )
        m_filechooserdialog->set_filename( * m_list_recent->begin() );
    else
        m_filechooserdialog->set_current_folder( Glib::get_home_dir() );

    m_filechooserdialog->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
    m_filechooserdialog->add_button( Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT );
    FilefilterAny       filter_any;
    FilefilterDiary     filter_diary;
    m_filechooserdialog->add_filter( filter_any );
    m_filechooserdialog->add_filter( filter_diary );
    m_filechooserdialog->set_filter( filter_diary );
    if( m_filechooserdialog->run() == Gtk::RESPONSE_ACCEPT )
    {
        set_filename( m_filechooserdialog->get_filename() );
    }

    delete m_filechooserdialog;
    m_filechooserdialog= NULL;
}

void
FilebuttonRecent::on_create_file()
{
    m_signal_createfile.emit();
}

void
FilebuttonRecent::on_drag_data_received( const Glib::RefPtr<Gdk::DragContext>& context,
                                         int, int,
                                         const Gtk::SelectionData& seldata,
                                         guint info,
                                         guint time )
{
    if( seldata.get_length() < 0 )
        return;

    Glib::ustring uri = seldata.get_data_as_string();
    std::string filename = uri.substr( 0, uri.find('\n') - 1 );
    filename = Glib::filename_from_uri( filename );

    if( Glib::file_test( filename, Glib::FILE_TEST_IS_DIR ) )
        return;

    set_filename( filename );
    PRINT_DEBUG( Glib::ustring::compose( "dropped: %1", filename ) );

    context->drag_finish( true, false, time );
}

void
FilebuttonRecent::on_size_allocate( Gtk::Allocation& allocation )
{
    Menubutton::on_size_allocate( allocation );
    m_menu->set_size_request( allocation.get_width(), -1 );
}

SignalVoid
FilebuttonRecent::signal_selection_changed()
{
    return m_signal_selectionchanged;
}

SignalVoid
FilebuttonRecent::signal_create_file()
{
    return m_signal_createfile;
}

#endif
// DIALOGEVENT =====================================================================================
// the rest of the DialogEvent needs to be defined within the user application
void
DialogEvent::handle_logout()
{
    hide();
}

// FRAME (for printing) ============================================================================
Gtk::Frame*
create_frame( const Glib::ustring& str_label, Gtk::Widget& content )
{
    Gtk::Frame* frame = Gtk::manage( new Gtk::Frame );
    Gtk::Label* label = Gtk::manage( new Gtk::Label );

    Gtk::Alignment* alignment = Gtk::manage( new Gtk::Alignment );

    label->set_markup( Glib::ustring::compose( "<b>%1</b>", str_label ) );
    frame->set_shadow_type( Gtk::SHADOW_NONE );
    frame->set_label_widget( *label );
    alignment->set_padding( 0, 0, 12, 0 );
    alignment->add( content );
    frame->add( *alignment );

    return frame;
}

// TREEVIEW ========================================================================================
bool is_treepath_less( const Gtk::TreePath& p, const Gtk::TreePath& pb )
{
    for( unsigned int i = 0; i < p.size(); i++ )
    {
        if( i >= pb.size() )
            return false;

        if( p[i] < pb[ i ] )
            return true;

        if( p[i] > pb[ i ] )
            return false;
    }

    return false; // more or equal
}

bool is_treepath_more( const Gtk::TreePath& p, const Gtk::TreePath& pe )
{
    for( unsigned int i = 0; i < p.size(); i++ )
    {
        if( i >= pe.size() )
            return true;

        if( p[i] > pe[ i ] )
            return true;

        if( p[i] < pe[ i ] )
            return false;
    }

    return false; // less or equal
}

#endif

} // end of name space
