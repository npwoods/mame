// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

	ui/state.cpp

	Menus for saving and loading state

***************************************************************************/

#include "emu.h"
#include "ui/state.h"


namespace ui {

/***************************************************************************
	BASE CLASS FOR LOAD AND SAVE
***************************************************************************/

int menu_load_save_state_base::s_last_slot_selected;

//-------------------------------------------------
//  ctor
//-------------------------------------------------

menu_load_save_state_base::menu_load_save_state_base(mame_ui_manager &mui, render_container *container, bool disable_not_found_items)
	: menu(mui, container), m_disable_not_found_items(disable_not_found_items), m_enabled_mask(0)
{
}


//-------------------------------------------------
//  dtor
//-------------------------------------------------

menu_load_save_state_base::~menu_load_save_state_base()
{
}


//-------------------------------------------------
//  itemref_from_slot_number
//-------------------------------------------------

void *menu_load_save_state_base::itemref_from_slot_number(int slot)
{
	return (void *)(size_t)slot;
}


//-------------------------------------------------
//  populate
//-------------------------------------------------

void menu_load_save_state_base::populate()
{
	m_enabled_mask = 0;

	// populate all slots
	for (int i = 0; i < SLOT_COUNT; i++)
	{
		// name the save state
		std::string name = string_format("%d", i);

		// compose the filename
		std::string file_name = machine().compose_saveload_filename(name.c_str());

		// get the last modified time
		std::time_t last_modified = machine().get_save_state_modified_time(file_name.c_str());

		// is the file present?
		bool file_present = last_modified != (std::time_t) -1;

		// get the time as a local time string
		char time_string[128];
		if (file_present)
			std::strftime(time_string, sizeof(time_string), "%A %c", std::localtime(&last_modified));
		else
			snprintf(time_string, ARRAY_LENGTH(time_string), "---");

		// create the menu text and flags
		std::string text = string_format("%s: %s", name.c_str(), time_string);

		// is this item enabled?
		bool enabled = file_present || !m_disable_not_found_items;

		// if enabled, set the bit field
		if (enabled)
			m_enabled_mask |= 1 << i;

		// append the menu item
		item_append(
			text,
			std::string(),
			enabled ? 0 : FLAG_DISABLE,
			itemref_from_slot_number(i));
	}

	// select the last slot
	if (s_last_slot_selected >= 0
		&& s_last_slot_selected < SLOT_COUNT
		&& ((m_enabled_mask & (1 << s_last_slot_selected)) != 0))
	{
		auto itemref = itemref_from_slot_number(s_last_slot_selected);
		set_selection(itemref);
	}
}


//-------------------------------------------------
//  handle
//-------------------------------------------------

void menu_load_save_state_base::handle()
{
	// process the menu
	const event *event = process(0);

	// process the event
	if ((event != nullptr) && (event->iptkey == IPT_UI_SELECT))
	{
		// user selected one of the entries
		slot_selected((int)(size_t)event->itemref);
	}
	else if ((event != nullptr) && (event->iptkey == IPT_SPECIAL)
		&& (event->unichar >= '0') && (event->unichar < '0' + SLOT_COUNT))
	{
		// user pressed a shortcut key
		int slot = event->unichar - '0';
		if (m_enabled_mask && 1 << slot)
			slot_selected(slot);
	}
}


//-------------------------------------------------
//  slot_selected
//-------------------------------------------------

void menu_load_save_state_base::slot_selected(int slot)
{
	// name the save state
	std::string name = string_format("%d", slot);

	// handle it
	process_file(name);

	// record the last slot touched
	s_last_slot_selected = slot;

	// no matter what, pop out
	menu::stack_pop(machine());
}


/***************************************************************************
	LOAD STATE
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

menu_load_state::menu_load_state(mame_ui_manager &mui, render_container *container)
	: menu_load_save_state_base(mui, container, true)
{
}


//-------------------------------------------------
//  process_file
//-------------------------------------------------

void menu_load_state::process_file(const std::string &file_name)
{
	machine().schedule_load(file_name.c_str());
}


/***************************************************************************
	SAVE STATE
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

menu_save_state::menu_save_state(mame_ui_manager &mui, render_container *container)
	: menu_load_save_state_base(mui, container, false)
{
}


//-------------------------------------------------
//  process_file
//-------------------------------------------------

void menu_save_state::process_file(const std::string &file_name)
{
	machine().schedule_save(file_name.c_str());
}


} // namespace ui
