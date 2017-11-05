/***************************************************************************

    mame_menubar.h

    Internal MAME menu bar for the user interface.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef MAME_FRONTEND_UI_EMENUBAR
#define MAME_FRONTEND_UI_EMENUBAR

#include "ui/menubar.h"

namespace ui
{
	menubar::ptr make_mame_menubar(::mame_ui_manager &mui);
};

#endif // MAME_FRONTEND_UI_EMENUBAR
