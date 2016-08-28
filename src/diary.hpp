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


#ifndef LIFEOGRAPH_DIARY_HEADER
#define LIFEOGRAPH_DIARY_HEADER


#include "helpers.hpp"	// i18n headers
#include "diarydata.hpp"
#include "entry.hpp"


namespace LIFEO
{

const char                      DB_FILE_HEADER[] = "LIFEOGRAPHDB";
const int                       DB_FILE_VERSION_INT{ 1040 };
const int                       DB_FILE_VERSION_INT_MIN( 110 );
const std::string::size_type    PASSPHRASE_MIN_SIZE( 4 );
const char                      LOCK_SUFFIX[] = ".~LOCK~";


class Diary : public DiaryElement, public DiaryElementChart
{
    public:
        static ElementView< Diary >* shower;

        enum SetPathType { SPT_NORMAL, SPT_READ_ONLY, SPT_NEW };

                                Diary();
                                ~Diary();

        static Diary*           d;

        bool	                is_old()
        { return( m_read_version < DB_FILE_VERSION_INT ); }

        Result                  init_new( const std::string& );

        // DISK I/O
        LIFEO::Result           read_body();
        LIFEO::Result           read_header();

        LIFEO::Result           write();
        LIFEO::Result           write( const std::string& );
        LIFEO::Result           write_copy( const std::string&, const std::string&, bool );
        LIFEO::Result           write_txt( const std::string&, bool );

        virtual void            clear();

        int	                    get_size() const override
        { return m_entries.size(); }
        Type                    get_type() const override
        { return ET_DIARY; }

#ifndef LIFEO_WINDOZE
        const Icon&             get_icon() const override;
        const Icon&             get_icon32() const override;
#else
        int                     get_icon() const;
#endif

        Ustring                 get_list_str() const override
#ifndef LIFEO_WINDOZE
        { return Glib::ustring::compose( "<b>%1</b>", Glib::Markup::escape_text( m_name ) ); }
#else
        { return m_name; }
#endif

        LIFEO::Result           set_path( const std::string&, SetPathType );
        const std::string&	    get_path() const;
        bool                    is_path_set() const;

        bool                    set_passphrase( const std::string& );
        void                    clear_passphrase();
        std::string	            get_passphrase() const;
        bool                    compare_passphrase( const std::string& ) const;
        bool                    is_passphrase_set() const;

        bool                    is_read_only() const
        { return m_flag_read_only; }

        DiaryElement*           get_element( DEID ) const;
        DiaryElement*           get_startup_elem() const;
        void                    set_startup_elem( const DEID );
        DiaryElement*           get_most_current_elem() const;
        DiaryElement*           get_prev_session_elem() const;
        void                    set_last_elem( const DiaryElement* );

        bool                    is_first( const Entry* const ) const;
        bool                    is_last( const Entry* const ) const;
        SortingCriteria	        get_sorting_criteria() const
        { return m_option_sorting_criteria; }
        void	                set_sorting_criteria( SortingCriteria sc )
        { m_option_sorting_criteria = sc; }

        std::string             get_lang() const { return m_language; }
        void                    set_lang( const std::string& lang ) { m_language = lang; }

        DEID                    create_new_id( DiaryElement* element )
        {
            DEID retval;
            if( m_force_id == DEID_UNSET )
                retval = m_current_id;
            else
            {
                retval = m_force_id;
                m_force_id = DEID_UNSET;
            }
            m_ids[ retval ] = element;

            while( m_ids.find( m_current_id ) != m_ids.end() )
                m_current_id++;

            return retval;
        }
        void                    erase_id( DEID id ) { m_ids.erase( id ); }
        bool                    set_force_id( DEID id )
        {
            if( m_ids.find( id ) != m_ids.end() || id <= DEID_DIARY )
                return false;
            m_force_id = id;
            return true;
        }

        int                     get_time_span() const;
        void                    fill_up_chart_points( ChartPoints* ) const;

        // FILTERING
        void                    set_search_text( const Ustring& );
        Ustring	                get_search_text() const
        { return m_search_text; }
        bool                    is_search_active() const
        { return( m_search_text.empty() == false ); }
        int                     replace_text( const Ustring& );

        Filter*                 get_filter()
        { return m_filter_active; }
        Filter*                 get_filter_default()
        { return m_filter_default; }
        void                    set_filter_tag( const Tag* tag )
        { m_filter_active->set_tag( tag ); }
        const Tag*              get_filter_tag() const
        { return m_filter_active->get_tag(); }
        Date::date_t            get_filter_date_begin()
        { return m_filter_active->get_date_begin(); }
        Date::date_t            get_filter_date_end()
        { return m_filter_active->get_date_end(); }
        void                    set_filter_date_begin( Date::date_t d )
        { m_filter_active->set_date_begin( d ); }
        void                    set_filter_date_end( Date::date_t d )
        { m_filter_active->set_date_end( d ); }

        ElemStatus              get_filter_status() const
        { return m_filter_active->get_status(); }

        // ENTRIES
        Entry*                  get_entry( const Date::date_t, bool=false );
        EntryVector*            get_entries( Date::date_t );
        PoolEntries&            get_entries()
        { return m_entries; }
        bool                    get_day_has_multiple_entries( const Date& );
        Entry*                  get_entry_today();
        Entry*                  get_entry_next_in_day( const Date& );
        Entry*                  get_entry_first();
        Entry*                  get_entry_latest() const; // get last temporal entry
        void                    set_entry_date( Entry*, const Date& );
        Entry*                  create_entry( Date::date_t, const Ustring& = "",
                                              bool = false );
        // adds a new entry to today even if there is already one or more:
        Entry*                  add_today();
        virtual bool            dismiss_entry( Entry* );
        bool                    make_free_entry_order( Date& ) const;

        // TAGS
        PoolTags*               get_tags()
        { return &m_tags; }
        PoolCategoriesTags*     get_tag_categories()
        { return &m_tag_categories; }
        CategoryTags*           create_tag_ctg();
        CategoryTags*           create_tag_ctg( const Ustring& );
        void                    dismiss_tag_ctg( CategoryTags*, bool = false );
        Tag*                    create_tag( const Ustring&, CategoryTags*,
                                            int = ChartPoints::MONTHLY|ChartPoints::BOOLEAN );
        void                    dismiss_tag( Tag*, bool = false );
        Untagged*               get_untagged()
        { return &m_untagged; }

        // CHAPTERS
        CategoryChapters*       get_current_chapter_ctg()
        { return m_ptr2chapter_ctg_cur; }
        void                    set_current_chapter_ctg( CategoryChapters* ctg )
        {
            m_ptr2chapter_ctg_cur = ctg;
            update_entries_in_chapters();
        }
        CategoryChapters*       create_chapter_ctg();
        CategoryChapters*       create_chapter_ctg( const Ustring& );
        void                    dismiss_chapter_ctg( CategoryChapters* );
        bool                    rename_chapter_ctg( CategoryChapters*, const Ustring& );
        void                    dismiss_chapter( Chapter*, bool = false );
        CategoryChapters*       get_topics()
        { return m_topics; }
        CategoryChapters*       get_groups()
        { return m_groups; }
        void                    update_entries_in_chapters();
        void                    add_entry_to_related_chapter( Entry* );
        void                    remove_entry_from_chapters( Entry* );
        void                    set_topic_order( Chapter*, Date::date_t );
        Chapter*                get_orphans()
        { return &m_orphans; }

        //Date                    get_free_chapter_order_temporal();

        bool                    is_encrypted() const
        { return( ! m_passphrase.empty() ); }

        ChartPoints*            create_chart_data() const;
        void                    show() override;

        bool                    import_tag( Tag* );
        bool                    import_entries( const Diary&, bool, const Ustring& );
        bool                    import_chapters( const Diary& );

        static bool             s_flag_ignore_locks;

    protected:
        // IDS (must be first)
        PoolDEIDs               m_ids;
        DEID                    m_current_id = DEID_FIRST;
        DEID                    m_force_id = DEID_UNSET;
        DEID                    m_startup_elem_id = HOME_CURRENT_ELEM;
        DEID                    m_last_elem_id = DEID_DIARY;
        // PROPERTIES
        std::string             m_path;
        std::string             m_passphrase;
        std::string             m_language;
        // CONTENT
        PoolEntries             m_entries;
        Untagged                m_untagged; // pseudo tag "untagged"
        PoolTags                m_tags;
        PoolCategoriesTags      m_tag_categories;
        PoolCategoriesChapters	m_chapter_categories;
        CategoryChapters*       m_ptr2chapter_ctg_cur{ nullptr };
        CategoryChapters*       m_topics; // ordinal chapters
        CategoryChapters*       m_groups; // chapters with invisible order
        // entries that remain out of defined chapters:
        Chapter                 m_orphans{ nullptr, "", Date::DATE_MAX };
        // OPTIONS & FLAGS
        SortingCriteria	        m_option_sorting_criteria{ SC_DATE};
        int                     m_read_version{ 0 };
        bool                    m_flag_only_save_filtered{ false };
        bool                    m_flag_read_only{ false };
        // FILTERING
        Ustring                 m_search_text;
        Filter*                 m_filter_active;
        Filter*                 m_filter_default;
        // TODO FilterVector            m_filters;  // user defined filters

        LIFEO::Result           parse_db_body_text( std::istream& );
        LIFEO::Result           parse_db_body_text_1040( std::istream& );
        LIFEO::Result           parse_db_body_text_1010( std::istream& );
        LIFEO::Result           parse_db_body_text_110( std::istream& );

        void                    upgrade_entries();

        void                    do_standard_checks_after_parse();

        bool                    create_db_body_text( std::stringstream& );
        bool                    create_db_header_text( std::stringstream&, bool );
        void                    create_db_tag_text( char, const Tag*, std::stringstream& );

        void                    close_file();
        LIFEO::Result           read_plain();
        LIFEO::Result           read_encrypted();
        LIFEO::Result           write_plain( const std::string&, bool = false );
        LIFEO::Result           write_encrypted( const std::string& );

        bool                    remove_lock();

    private:
        std::ifstream*          m_ifstream{ nullptr };

    friend class EntryView;
    friend class DiaryView;
    friend class TagView;
    friend class CategoryTagsView;
    friend class ThemeView;
    friend class PanelDiary;
    friend class PanelExtra;
    friend class WidgetPanelExp;
    friend class DialogImport;
};

} // end of namespace LIFEO

#endif

