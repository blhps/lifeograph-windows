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
                                    Settings() {}
        // FUNCTIONS
        bool                        read();
        bool                        write();

        void                        reset();

        void                        update_date_format();

        // VARIABLES FOR OPTIONS
        ListPaths                   recentfiles;

        bool                        autologout{ true };
        int                         idletime{ IDLETIME_DEFAULT };

        bool                        show_formatting_toolbar{ true };

        int                         date_format_order{ 0 };
        int                         date_format_separator{ 0 };

        std::string                 diary_extension{ EXTENSION_DEFAULT };

        bool                        flag_maiden_voyage{ false }; // whether or not the first run
        int                         width{ WIDTH_DEFAULT };
        int                         height{ HEIGHT_DEFAULT };
        bool                        state_maximized{ false };
        int                         position_x{ POSITION_NOTSET };
        int                         position_y{ POSITION_NOTSET };
        int                         position_pane{ PANEPOS_DEFAULT };
        int                         position_pane_tags{ PANEPOS_TAGS_DEFAULT };

        std::string                 icon_theme;
        bool                        small_lists{ true };

    protected:
        static const std::string    s_date_format_orders[ 3 ];
        static const std::string    s_date_format_separators;

        std::string                 m_path;
};

} // END OF NAMESPACE LIFEO

#endif

