// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    ui/bbcontrl.cpp

    MESS's "bit banger" control

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "emu.h"
#include "ui/menu.h"
#include "ui/bbcontrl.h"

namespace ui {

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

#define BITBANGERCMD_SELECT         ((void *) 0x0000)
#define BITBANGERCMD_MODE           ((void *) 0x0001)
#define BITBANGERCMD_BAUD           ((void *) 0x0002)
#define BITBANGERCMD_TUNE           ((void *) 0x0003)


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

ui_menu_bitbanger_control::ui_menu_bitbanger_control(mame_ui_manager &mui, render_container &container, bitbanger_device *device)
	: menu_device_control<bitbanger_device>(mui, container, device)
{
}


//-------------------------------------------------
//  dtor
//-------------------------------------------------

ui_menu_bitbanger_control::~ui_menu_bitbanger_control()
{
}


//-------------------------------------------------
//  populate - populates the main bitbanger control menu
//-------------------------------------------------

void ui_menu_bitbanger_control::populate()
{
	uint32_t flags = 0, mode_flags = 0, baud_flags = 0, tune_flags = 0;

	if( count() > 0 )
	{
		int index = current_index();

		if( index == (count()-1) )
			flags |= FLAG_LEFT_ARROW;
		else
			flags |= FLAG_RIGHT_ARROW;
	}

	if ((current_device() != NULL) && (current_device()->exists()))
	{
		if (current_device()->inc_mode(true))
			mode_flags |= FLAG_RIGHT_ARROW;

		if (current_device()->dec_mode(true))
			mode_flags |= FLAG_LEFT_ARROW;

		if (current_device()->inc_baud(true))
			baud_flags |= FLAG_RIGHT_ARROW;

		if (current_device()->dec_baud(true))
			baud_flags |= FLAG_LEFT_ARROW;

		if (current_device()->inc_tune(true))
			tune_flags |= FLAG_RIGHT_ARROW;

		if (current_device()->dec_tune(true))
			tune_flags |= FLAG_LEFT_ARROW;

		// name of bitbanger file
		item_append(current_device()->device().name(), current_device()->filename(), flags, BITBANGERCMD_SELECT);
		item_append("Device Mode:", current_device()->mode_string(), mode_flags, BITBANGERCMD_MODE);
		item_append("Baud:", current_device()->baud_string(), baud_flags, BITBANGERCMD_BAUD);
		item_append("Baud Tune:", current_device()->tune_string(), tune_flags, BITBANGERCMD_TUNE);
		item_append("Protocol:", "8-1-N", 0, NULL);
	}
	else
	{
		// no bitbanger loaded
		item_append("No Bitbanger Image loaded", NULL, flags, NULL);
	}
}


//-------------------------------------------------
//  handle
//-------------------------------------------------

void ui_menu_bitbanger_control::handle()
{
	// rebuild the menu
	reset(reset_options::REMEMBER_POSITION);
	populate();

	// process the menu
	const event *event = process(PROCESS_LR_REPEAT);

	if (event)
	{
		switch(event->iptkey)
		{
			case IPT_UI_LEFT:
				if (event->itemref==BITBANGERCMD_SELECT)
					previous();
				else if (event->itemref==BITBANGERCMD_MODE)
					current_device()->dec_mode(false);
				else if (event->itemref==BITBANGERCMD_BAUD)
					current_device()->dec_baud(false);
				else if (event->itemref==BITBANGERCMD_TUNE)
					current_device()->dec_tune(false);
				break;

			case IPT_UI_RIGHT:
				if (event->itemref==BITBANGERCMD_SELECT)
					next();
				else if (event->itemref==BITBANGERCMD_MODE)
					current_device()->inc_mode(false);
				else if (event->itemref==BITBANGERCMD_BAUD)
					current_device()->inc_baud(false);
				else if (event->itemref==BITBANGERCMD_TUNE)
					current_device()->inc_tune(false);
				break;
		}
	}
}

} // namespace ui
