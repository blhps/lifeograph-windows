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


#ifndef LIFEOGRAPH_WIN_WIDGETTAGS_HEADER
#define LIFEOGRAPH_WIN_WIDGETTAGS_HEADER


#include <winsock2.h>   // just to silence the compiler
#include <windows.h>
#include <list>

#include "helpers.hpp"
#include "entry.hpp"


namespace LIFEO
{

// TAG WIDGET FOR LISTING TAGS OF AN ENTRY
class WidgetTagList
{
    public:
        WidgetTagList();

        void                        set_entry( Entry* );
        void                        update();
        void                        update_full();

        bool                        handle_draw();
        void                        handle_resize( int, int );
        void                        handle_mouse_move( int, int );
        void                        handle_click( int, int );
        
        int                         get_height();

        void                        set_editable( bool flag_editable )
        { m_flag_editable = flag_editable; }
        
        //HWND                        m_hwnd;

    protected:
        // CONSTANTS
        static constexpr float      MARGIN = 18.0;
        static constexpr float      HSPACING = 10.0;
        static constexpr float      VSPACING = 14.0;
        static constexpr float      LABEL_OFFSET = 2.0;
        static constexpr float      TEXT_HEIGHT = 12.0;
        static constexpr float      ITEM_BORDER = 4.0;  // must be < MARGIN
        static constexpr int        ICON_SIZE = 16;
        static constexpr float      ITEM_HEIGHT =  24.0; // ITEM_BORDER + ICON_SIZE + ITEM_BORDER
        static constexpr float      HALF_HEIGHT = 12.0; // ITEM_HEIGHT / 2

        struct TagItem
        {
            const Tag* tag;
            float xl, xr, yl, yr;
        };
        typedef std::list< TagItem > TagItemList;

        class AddTagItem : public Tag
        {
            public:
                AddTagItem() : Tag( NULL ) {}
        };

        void                        add_item( const HDC,
                                              const Wstring&,
                                              const Tag* );

        int                         calculate_height();

        // DATA
        Entry*                      m_ptr2entry;

        // GEOMETRY
        int                         m_width;
        int                         m_height;
        float                       m_pos_x;
        float                       m_pos_y;
        int                         m_y_offset;

        TagItemList                 m_items;
        const Tag*                  m_hovered_tag;
        AddTagItem                  m_add_tag_item;


        bool                        m_flag_editable;    // not red-only
};

} // end of namespace LIFEO

#endif
