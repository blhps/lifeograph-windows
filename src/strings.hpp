/***********************************************************************************

    Copyright (C) 2007-2014 Ahmet Öztürk (aoz_2@yahoo.com)

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


#ifndef LIFEOGRAPH_STRINGS_HEADER
#define LIFEOGRAPH_STRINGS_HEADER

#ifndef LIFEO_WINDOZE
#include <libintl.h>
#endif
// DEFINITIONS FOR LIBGETTEXT
#ifndef LIFEO_WINDOZE
#define _(String)               gettext(String)
#else
#define _(String)               String
#endif
#define gettext_noop(String)    String
#define N_(String)              gettext_noop(String)
// END OF LIBGETTEXT DEFINITIONS


namespace LIFEO
{

namespace STRING
{

static const char SLOGAN[] =
        N_( "Personal, digital diary" );

static const char CANNOT_WRITE[] =
        N_( "Changes could not be written to diary!" );

static const char CANNOT_WRITE_SUB[] =
        N_( "Check if you have write permissions on "
            "the file and containing folder" );

static const char OFF[] =
        N_( "Off" );

static const char EMPTY_ENTRY_TITLE[] =
// TRANSLATORS: title of an empty diary entry
        N_( "<empty entry>" );

static const char ENTER_PASSWORD[] =
        N_( "Please enter password for selected diary..." );

static const char ENTER_PASSWORD_TIMEOUT[] =
        N_( "Program logged out to protect your privacy. "
            "Please re-enter password..." );

static const char INCOMPATIBLE_DIARY_OLD[] =
        N_( "Selected diary is in an older format which is "
            "not compatible with this version of Lifeograph. "
            "Please select another file..." );

static const char INCOMPATIBLE_DIARY_NEW[] =
        N_( "Selected diary is in a newer format which is "
            "not compatible with this version of Lifeograph. "
            "Please consider upgrading your program." );

static const char CORRUPT_DIARY[] =
        N_( "Selected file is not a valid "
            "Lifeograph diary. "
            "Please select another file..." );

static const char DIARY_NOT_FOUND[] =
        N_( "Selected file is not found. "
            "Please select another file..." );

static const char DIARY_NOT_READABLE[] =
        N_( "Selected file is not readable. "
            "Please check file permissions or select another file..." );

static const char DIARY_LOCKED[] =
        N_( "Selected file is locked which means either it is "
            "being edited by another instance of Lifeograph or "
            "the last session with it did not finish correctly. "
            "This file cannot be opened as long as the lock file (%1) "
            "is there." );

static const char UPGRADE_DIARY_CONFIRM[] =
        N_( "You are about to open an old diary that will be upgraded to the new format.\n\n"
            "Lifeograph will create a backup copy of the current diary for you "
            "in the same directory. Please keep that copy until you are sure that "
            "everything is all right." );

static const char FAILED_TO_OPEN_DIARY[] =
        N_( "Failed to open diary. "
            "Please select another file..." );

static const char DIARY_IS_NOT_ENCRYPTED[] =
        N_( "Press Open to continue" );

static const char COLHEAD_BY_LAST_ACCESS[] =
        N_( "Last Read" );

static const char COLHEAD_BY_LAST_SAVE[] =
        N_( "Last Saved" );

static const char DROP_TAG_TO_FILTER[] =
        N_( "(Drop a tag here to filter)" );

static const char CHANGE_PASSWORD[] =
        N_( "Change Password..." );

static const char ELEM_WITH_ENTRIES[] =
//  TRANSLATORS: e.g. Tag with 5 entries
        N_( "%1 with %2 entrie(s)" );

static const char NEW_CHAPTER_NAME[] =
        N_( "New chapter" );

static const char NEW_CATEGORY_NAME[] =
        N_( "New category" );
        
static const char DEFAULT_CHAPTER_CTG_NAME[] =
        N_( "Default" );

} // namespace STRING

} // namespace LIFO

#endif
