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

#ifdef LIFEO_WINDOZE
#include <sstream>
#include <cmath>
#include <unistd.h>
#include <sys/stat.h>
#endif

#include "helpers.hpp"


namespace HELPERS
{

Error::Error( const Ustring& error_message )
{
    print_error( error_message );
}

// DATE ============================================================================================
std::string Date::s_date_format_order = "YMD";
char        Date::s_date_format_separator = '.';

Date::Date( const std::string& str_date )
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
    std::stringstream result;

    if( d & ORDINAL_FLAG )
    {
        result << ( get_ordinal_order( d ) + 1 );
        if( get_order( d ) )
            result << "." << get_order( d );
    }
    else
    {
        auto get_YMD = [ &d ]( char c ) -> unsigned int
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
            };

        for( unsigned int i = 0; i < format.size(); i++ )
        {
            result << std::setfill( '0' ) << std::setw( 2 ) << get_YMD( format[ i ] );
            if( i != format.size() - 1 )
                result << separator;
        }
    }
    return result.str();
}

Ustring
Date::format_string_dt( const time_t time )
{
    struct tm* timeinfo = localtime( &time );
    std::stringstream result;

    result << format_string( make_date_from_ctime( timeinfo ) )
           << ", "
           << std::setfill( '0' ) << std::setw( 2 ) << timeinfo->tm_hour << ":"
           << std::setfill( '0' ) << std::setw( 2 ) << timeinfo->tm_min;

    return result.str();
}

// date only
Ustring
Date::format_string_d( const time_t time )
{
    struct tm* timeinfo = localtime( &time );
    return format_string( make_date_from_ctime( timeinfo ) );
}

std::string
Date::get_year_str() const
{
    std::stringstream result;
    result << get_year();
    return result.str();
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
    unsigned int day = get_day();
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
    int dist{ 12 * int( get_year( date2 ) - get_year( date1 ) ) };
    dist += ( get_month( date2 ) - get_month( date1 ) );

    return( dist < 0 ? -dist : dist );
}

int
Date::calculate_months_between_neg( Date::date_t date1, Date::date_t date2 )
{
    int dist{ 12 * int( get_year( date2 ) - get_year( date1 ) ) };
    dist += ( get_month( date2 ) - get_month( date1 ) );

    return dist;
}

// COLOR OPERATIONS ================================================================================
#ifndef LIFEO_WINDOZE
Gdk::RGBA
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

Gdk::RGBA
midtone( const Gdk::RGBA& c1, const Gdk::RGBA& c2 )
{
    Gdk::RGBA midtone;
    midtone.set_red_u( ( c1.get_red_u() + c2.get_red_u() ) / 2.0 );
    midtone.set_green_u( ( c1.get_green_u() + c2.get_green_u() ) / 2.0 );
    midtone.set_blue_u( ( c1.get_blue_u() + c2.get_blue_u() ) / 2.0 );
    midtone.set_alpha( 1.0 );
    return midtone;
}

Gdk::RGBA
midtone( const Gdk::RGBA& c1, const Gdk::RGBA& c2, float ratio )
{
    Gdk::RGBA midtone;
    midtone.set_red_u( ( c1.get_red_u() * ratio ) + ( c2.get_red_u() * ( 1.0 - ratio ) ) );
    midtone.set_green_u( ( c1.get_green_u() * ratio ) + ( c2.get_green_u() * ( 1.0 - ratio ) ) );
    midtone.set_blue_u( ( c1.get_blue_u() * ratio ) + ( c2.get_blue_u() * ( 1.0 - ratio ) ) );
    midtone.set_alpha( 1.0 );
    return midtone;
}

#else

COLORREF
contrast2( COLORREF bg, COLORREF c1, COLORREF c2 )
{
    double dist1 = ( fabs( GetRValue( bg ) - GetRValue( c1 ) ) +
                     fabs( GetGValue( bg ) - GetGValue( c1 ) ) +
                     fabs( GetBValue( bg ) - GetBValue( c1 ) ) );

    double dist2 = ( fabs( GetRValue( bg ) - GetRValue( c2 ) ) +
                     fabs( GetGValue( bg ) - GetGValue( c2 ) ) +
                     fabs( GetBValue( bg ) - GetBValue( c2 ) ) );

    if( dist1 > dist2 )
        return c1;
    else
        return c2;
}

COLORREF
midtone( COLORREF c1, COLORREF c2, float ratio )
{
    return RGB(
            ( int ) ( ( GetRValue( c1 ) * ratio ) + ( GetRValue( c2 ) * ( 1.0 - ratio ) ) ),
            ( int ) ( ( GetGValue( c1 ) * ratio ) + ( GetGValue( c2 ) * ( 1.0 - ratio ) ) ),
            ( int ) ( ( GetBValue( c1 ) * ratio ) + ( GetBValue( c2 ) * ( 1.0 - ratio ) ) ) );
}
    
#endif

// FILE OPERATIONS =================================================================================
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

bool
is_dir( const char* path )
{
    struct stat s;

    if( stat( path, &s ) == 0 )
    {
        if( s.st_mode & S_IFDIR )
            return true;
        else if( s.st_mode & S_IFREG ) // file
            return false;
        else // something else
            return false;
    }
    else
        throw Error( "Stat failed" );
}

// TEXT OPERATIONS =================================================================================
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

#ifndef LIFEO_WINDOZE
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

// code taken from: http://alter.org.ua/docs/win/args/
PCHAR*
command_line_to_argvA( PCHAR CmdLine, int* _argc )
{
    PCHAR* argv;
    PCHAR  _argv;
    ULONG  len;
    ULONG  argc;
    CHAR   a;
    ULONG  i, j;

    BOOLEAN  in_QM;
    BOOLEAN  in_TEXT;
    BOOLEAN  in_SPACE;

    len = strlen(CmdLine);
    i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

    argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
        i + (len+2)*sizeof(CHAR));

    _argv = (PCHAR)(((PUCHAR)argv)+i);

    argc = 0;
    argv[argc] = _argv;
    in_QM = FALSE;
    in_TEXT = FALSE;
    in_SPACE = TRUE;
    i = 0;
    j = 0;

    while( a = CmdLine[i] ) {
        if(in_QM) {
            if(a == '\"') {
                in_QM = FALSE;
            } else {
                _argv[j] = a;
                j++;
            }
        } else {
            switch(a) {
            case '\"':
                in_QM = TRUE;
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                in_SPACE = FALSE;
                break;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                if(in_TEXT) {
                    _argv[j] = '\0';
                    j++;
                }
                in_TEXT = FALSE;
                in_SPACE = TRUE;
                break;
            default:
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                _argv[j] = a;
                j++;
                in_SPACE = FALSE;
                break;
            }
        }
        i++;
    }
    _argv[j] = '\0';
    argv[argc] = NULL;

    (*_argc) = argc;
    return argv;
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
        try // may not work!
        {
            set_icon_from_icon_name( "edit-clear-symbolic", Gtk::ENTRY_ICON_SECONDARY );
        }
        catch( ... )
        {
            set_icon_from_icon_name( "edit-clear", Gtk::ENTRY_ICON_SECONDARY );
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
