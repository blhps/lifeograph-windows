/***********************************************************************************

    Copyright (C) 2010 Ahmet Öztürk (aoz_2@yahoo.com)

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


#ifndef LIFEOGRAPH_SETTINGS_HEADER
#define LIFEOGRAPH_SETTINGS_HEADER


#include <list>
#include <string>

#include "helpers.hpp"


namespace LIFEO
{

using namespace HELPERS;

static const int    IDLETIME_MIN            = 30;
static const int    IDLETIME_MAX            = 3600;
static const int    IDLETIME_DEFAULT        = 90;
static const int    WIDTH_DEFAULT           = 550;
static const int    HEIGHT_DEFAULT          = 300;
static const int    POSITION_NOTSET         = -0xFFFF;
static const int    PANEPOS_DEFAULT         = 300;
static const int    PANEPOS_TAGS_DEFAULT    = 100;
static const char   EXTENSION_DEFAULT[]     = ".diary";

class Settings
{
    public:
                                    Settings();

        // FUNCTIONS
        bool                        read();
        bool                        write();

        void                        reset();

        void                        update_date_format();

        // VARIABLES FOR OPTIONS
        ListPaths                   recentfiles;

        bool                        autologout;
        int                         idletime;

        bool                        show_formatting_toolbar;

        int                         date_format_order;
        int                         date_format_separator;

        std::string                 diary_extension;

        bool                        flag_maiden_voyage; // whether or not the first run
        int                         width;
        int                         height;
        bool                        state_maximized;
        int                         position_x;
        int                         position_y;
        int                         position_pane;
        int                         position_pane_tags;

        bool                        small_lists;

    protected:
        static const std::string    s_date_format_orders[3];
        static const std::string    s_date_format_separators;

        std::string                 m_path;
};

} // END OF NAMESPACE LIFEO

#endif

