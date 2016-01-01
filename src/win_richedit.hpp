/***********************************************************************************

    Copyright (C) 2014-2015 Ahmet Öztürk (aoz_2@yahoo.com)

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


#ifndef LIFEOGRAPH_WIN_RICHEDIT_HEADER
#define LIFEOGRAPH_WIN_RICHEDIT_HEADER


#include <deque>
#include <richedit.h>

#include "helpers.hpp"
#include "diarydata.hpp"
#include "entry.hpp"
#include "entry_parser.hpp"


namespace LIFEO
{

const int       UNDO_MERGE_TIMEOUT = 3;

enum LinkStatus
{
    LS_OK,
    LS_ENTRY_UNAVAILABLE,
    LS_INVALID,     // separator: to check a valid entry link: linkstatus < LS_INVALID
    LS_CYCLIC,

    LS_FILE_OK,
    LS_FILE_INVALID,
    LS_FILE_UNAVAILABLE,
    LS_FILE_UNKNOWN
};

// LINKS ===========================================================================================
/*class Link
{
    public:
        enum LinkType
        {
            LT_NONE, LT_ID, LT_ENTRY, LT_URI, LT_CHECK, LT_IMAGE
        };

        virtual                     ~Link();
        virtual void                go() = 0;

        // position of link in the buffer:
        Glib::RefPtr< Gtk::TextMark >
                                    m_mark_start;
        Glib::RefPtr< Gtk::TextMark >
                                    m_mark_end;

        const LinkType              type;

    protected:
                                    Link( const Glib::RefPtr< Gtk::TextMark >&,
                                          const Glib::RefPtr< Gtk::TextMark >&,
                                          LinkType );
};

class LinkID : public Link
{
    public:
        typedef sigc::signal< void, Date >
                                    Signal_void_Date;

                                    LinkID( const Glib::RefPtr< Gtk::TextMark >&,
                                            const Glib::RefPtr< Gtk::TextMark >&,
                                            DEID );
        void                        go();

        static Signal_void_Date     signal_activated()
        { return m_signal_activated; }

        DEID                        m_id;

    protected:
        static Signal_void_Date     m_signal_activated;
};

class LinkEntry : public Link
{
    public:
                                    LinkEntry( const Glib::RefPtr< Gtk::TextMark >&,
                                               const Glib::RefPtr< Gtk::TextMark >&,
                                               Date );
        void                        go();

        Date                        m_date;

    protected:
        static Gtk::Menu*           menu_link;
};

class LinkUri : public Link
{
    public:
                                    LinkUri( const Glib::RefPtr< Gtk::TextMark >&,
                                             const Glib::RefPtr< Gtk::TextMark >&,
                                             const std::string& );
        void                        go();

        std::string                 m_url;
        static Gtk::TextBuffer*     m_ptr2buffer;
};

class LinkCheck : public Link
{
    public:
                                    LinkCheck( const Glib::RefPtr< Gtk::TextMark >&,
                                               const Glib::RefPtr< Gtk::TextMark >&,
                                               unsigned int = 0 );
        void                        go();

        static const Glib::ustring  boxes;
        unsigned int                m_state_index;
};

class InlineImage
{
    public:
        InlineImage( const Glib::RefPtr< Gtk::TextMark >& );
        ~InlineImage();

        Glib::RefPtr< Gtk::TextMark > m_mark;
};*/

// TEXTBUFFER ======================================================================================
class RichEdit : public EntryParser
{
    public:
        static const int            LEFT_MARGIN = 10;


        typedef void ( RichEdit::*ParsingApplierFn )();

                                    RichEdit();

        void                        handle_logout();
        void                        handle_login();

//        Link*                       get_link( LONG ) const;
//        Link*                       get_link( LONG ) const;

        void                        set_richtext( Entry* );
        LONG                        get_length();
        Wstring                     get_text();

        void                        set_search_str( const Wstring& );
        bool                        select_searchstr_previous();
        bool                        select_searchstr_next();

        void                        toggle_format(  CHARFORMAT2&, const Wstring& );
        void                        toggle_bold();
        void                        toggle_italic();
        void                        toggle_highlight();
        void                        toggle_strikethrough();

        void                        calculate_para_bounds( LONG&, LONG& );
        bool                        calculate_multiline_selection_bounds( LONG, LONG );
        void                        handle_indent();
        void                        handle_unindent();

        void                        add_bullet();
        void                        add_checkbox();
        void                        add_empty_line_above();
        void                        remove_empty_line_above();
        void                        move_line_up();
        void                        move_line_down();
        void                        insert_link( DiaryElement* );
        void                        insert_time_stamp();

        std::string                 set_language( std::string );

        void                        set_theme( const Theme* );

        void                        reparse();
        
        COLORREF                    midcontrast( const Theme*, const COLORREF );
        
        void                        handle_change();
        bool                        handle_space();
        bool                        handle_new_line();

        HWND                        m_hwnd;

    protected:
        // PARSING
        void                        parse( LONG, LONG );

        void                        apply_heading();
        void                        apply_subheading();
        void                        apply_bold();
        void                        apply_italic();
        void                        apply_strikethrough();
        void                        apply_highlight();
        void                        apply_comment();
        void                        apply_ignore();
/*        void                        apply_link();
        void                        apply_link_date();
        void                        apply_link_id();*/
        void                        apply_check( CHARFORMAT2&, CHARFORMAT2*, int );
        void                        apply_check_ccl();
        void                        apply_check_unf();
        void                        apply_check_prg();
        void                        apply_check_fin();
        void                        apply_match();
//        void                        apply_indent();

        void                        apply_markup( CHARFORMAT2& );
        
        Wchar                       get_char_at( int );

        Wstring::size_type          parser_open_tag_begin;
        Wstring::size_type          parser_open_tag_end;
        bool                        parser_open_tag_is_link;
        Wstring::size_type          parser_offset_insert_prev;

        // LINKS
        void                        clear_links( LONG, LONG );
        void                        clear_links();

        void                        clear_images( LONG, LONG& );
        void                        clear_images();
        //bool                        update_thumbnail_width( int );

        void                        apply_format( CHARFORMAT2&, LONG, LONG );
        void                        remove_all_formats( LONG, LONG );

        // FORMATS
        CHARFORMAT2                 m_format_default;
        CHARFORMAT2                 m_format_heading;
        CHARFORMAT2                 m_format_subheading;
        CHARFORMAT2                 m_format_match;
        CHARFORMAT2                 m_format_markup;
        CHARFORMAT2                 m_format_markup_link;
        CHARFORMAT2                 m_format_hidden;
        CHARFORMAT2                 m_format_bold;
        CHARFORMAT2                 m_format_italic;
        CHARFORMAT2                 m_format_strikethrough;
        CHARFORMAT2                 m_format_highlight;
        CHARFORMAT2                 m_format_comment;
        CHARFORMAT2                 m_format_region;
        CHARFORMAT2                 m_format_link;
        CHARFORMAT2                 m_format_link_broken;
        CHARFORMAT2                 m_format_link_hidden;
        CHARFORMAT2                 m_format_checkbox;
        CHARFORMAT2                 m_format_image;
        CHARFORMAT2                 m_format_misspelled;
        CHARFORMAT2                 m_format_done;
        CHARFORMAT2                 m_format_checkbox_todo;
        CHARFORMAT2                 m_format_checkbox_progressed;
        CHARFORMAT2                 m_format_checkbox_done;
        CHARFORMAT2                 m_format_checkbox_canceled;

        // OTHER VARIABLES
        bool                        m_flag_settextoperation;
        int                         m_flag_ongoing_operation;   // Incremental to allow nesting
        bool                        m_flag_parsing;
        Entry*                      m_ptr2entry;

    private:
        //bool                        m_flag_size_allocated;
        //bool                        m_flag_set_text_queued;
};

} // end of namespace LIFEO

#endif
