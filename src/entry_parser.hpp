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


#ifndef LIFEOGRAPH_ENTRY_PARSER_HEADER
#define LIFEOGRAPH_ENTRY_PARSER_HEADER


#ifndef LIFEO_WINDOZE
#include <gtkmm.h>
#else
#include <deque>
#endif

#include "helpers.hpp"


namespace LIFEO
{

using namespace HELPERS;

enum CharFlag
{
    CF_NOT_SET          = 0,
    CF_NOTHING          = 0x1,
    CF_NEWLINE          = 0x2,
    CF_PUNCTUATION_RAW  = 0x4,
    CF_SPACE            = 0x8,          // space that will come eventually
    CF_TAB              = 0x10,
    CF_IMMEDIATE        = 0x20,         // indicates contiguity of chars

    CF_ASTERISK         = 0x40,         // bold
    CF_UNDERSCORE       = 0x80,         // italic
    CF_EQUALS           = 0x100,        // strikethrough
    CF_HASH             = 0x400,        // highlight
    CF_MARKUP           = CF_ASTERISK|CF_UNDERSCORE|CF_EQUALS|CF_HASH,

    CF_SLASH            = 0x800,
    CF_ALPHA            = 0x1000,
    CF_NUMBER           = 0x2000,
    CF_ALHANUM          = CF_ALPHA|CF_NUMBER,
    CF_AT               = 0x4000,       // email
    CF_SPELLCHECK       = 0x8000,

    CF_DOTYM            = 0x10000,
    CF_DOTMD            = 0x20000,
    CF_DOTDATE          = CF_DOTMD|CF_DOTYM,

    CF_LESS             = 0x80000,      // tagging
    CF_MORE             = 0x100000,
    CF_SBB              = 0x200000,     // square bracket begin: comments
    CF_SBE              = 0x400000,     // square bracket end: comments

    CF_TODO             = 0x1000000,    // ~,+,x

    CF_IGNORE           = 0x40000000,
    CF_EOT              = 0x80000000,   // End of Text

    CF_ANY              = 0xFFFFFFFF,

    CF_PUNCTUATION      = CF_PUNCTUATION_RAW|CF_SLASH|CF_DOTDATE|CF_LESS|CF_MORE|CF_SBB|CF_SBE,
    CF_FORMATCHAR       = CF_ASTERISK|CF_UNDERSCORE|CF_EQUALS|CF_HASH|CF_SBB|CF_SBE,
    CF_NUM_SLSH         = CF_NUMBER|CF_SLASH,
    CF_NUM_CKBX         = CF_NUMBER|CF_TODO,
    CF_NONSPACE         = CF_PUNCTUATION|CF_MARKUP|CF_ALPHA|CF_NUMBER|CF_AT|CF_TODO,
    CF_NONTAB           = CF_NONSPACE|CF_SPACE,

    CF_SEPARATOR        = CF_SPACE|CF_TAB|CF_NEWLINE,
    CF_NOT_SEPARATOR    = CF_ANY^CF_SEPARATOR,

    CF_ALPHASPELL       = CF_ALPHA|CF_SPELLCHECK
};

class EntryParser
{
    public:
        typedef void ( EntryParser::*FPtr_void )();

        struct AbsChar  // abstract char
        {
            AbsChar( unsigned int f, FPtr_void a, bool j = false )
            : flags( f ), applier( a ), junction( j ) {}
            unsigned int    flags;
            FPtr_void       applier;
            bool            junction;
        };

                                    //EntryParser() {}
        virtual                     ~EntryParser() {}

        void                        parse( Wstring::size_type,
                                           Wstring::size_type );

        void                        set_search_str( const Wstring& );

    protected:
        virtual Wchar               get_char_at( int ) = 0;

        void                        process_char( unsigned int, unsigned int,
                                                  unsigned int,
                                                  FPtr_void );

        // TRIGGERERS
        void                        trigger_subheading();
        void                        trigger_markup( unsigned int, FPtr_void );
        void                        trigger_bold();
        void                        trigger_italic();
        void                        trigger_strikethrough();
        void                        trigger_highlight();
        void                        trigger_comment();
        void                        trigger_link();
        void                        trigger_link_at();
        void                        trigger_link_date();
        void                        trigger_list();
        void                        trigger_ignore();
        void                        junction_link_date();
        void                        junction_link_hidden_tab();
        void                        junction_list();
        void                        junction_list2();
        void                        junction_date_dotym();   // dot between year and month
        void                        junction_date_dotmd();   // dot between month and day
        void                        junction_ignore();

        // HELPERS
        void                        handle_number();
        void                        begin_apply()
        {
            apply_regular();
            pos_regular = pos_current + 1;
        }

        // APPLIERS (TO BE OVERRIDEN)
        virtual void                apply_regular() { }
        virtual void                apply_heading() { }
        void                        apply_subheading_0(); // a temporary solution
        virtual void                apply_subheading() { }
        virtual void                apply_bold() { }
        virtual void                apply_italic() { }
        virtual void                apply_strikethrough() { }
        virtual void                apply_highlight() { }
        virtual void                apply_comment() { }
        virtual void                apply_ignore() { }
        virtual void                apply_link() { }
        virtual void                apply_link_date() { }
        virtual void                apply_link_id() { }
        void                        apply_check_ccl_0();  // a temporary solution
        virtual void                apply_check_ccl() { }
        virtual void                apply_check_unf() { }
        virtual void                apply_check_prg() { }
        virtual void                apply_check_fin_0();  // a temporary solution
        virtual void                apply_check_fin() { }

        virtual void                apply_heading_end() { }
        virtual void                apply_subheading_end() { }
        virtual void                apply_check_ccl_end() { }
//        virtual void                apply_check_unf_end() { }   // maybe later..
        virtual void                apply_check_fin_end() { }
//        virtual void                apply_ignore_end() { }

        virtual void                apply_indent() { }
        virtual void                apply_match() { }
        virtual void                handle_word() { } // for spell-checking

        void                        reset( Wstring::size_type, Wstring::size_type );

        Wstring::size_type          m_pos_start;    // position of first char to process
        Wstring::size_type          m_pos_end;      // position of last char (constant)
        Wstring::size_type          pos_current;
        Wstring::size_type          pos_word;
        Wstring::size_type          pos_alpha;
        Wstring::size_type          pos_regular;    // start of current regular char array
        Wstring::size_type          pos_tab;        // position of invisible link tab
        Wstring::size_type          pos_search;

        unsigned int                m_cf_last;
        unsigned int                m_cf_req;       // required previous char
        Wchar                       char_current;
        Wstring                     m_word_last;
        unsigned int                m_word_count{ 0 };
        Wstring                     alpha_last;     // last word consisting purely of letters
        unsigned int                int_last{ 0 };
        Date                        date_last{ 0 };
        unsigned long               id_last;
        bool                        m_flag_hidden_link;
        bool                        m_flag_handle_word;

        Wstring                     m_search_str;

        std::deque< AbsChar >       m_chars_looked_for;
        FPtr_void                   m_applier_nl;

    private:
        Wstring::size_type          i_search;
        Wstring::size_type          i_search_end;
};

}   // end of namespace LIFEO

#endif
