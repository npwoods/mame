// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    ui/imgcntrl.cpp

    MESS's clunky built-in file manager

***************************************************************************/

#include "emu.h"

#include "ui/imgcntrl.h"

#include "ui/ui.h"
#include "ui/filesel.h"
#include "ui/swlist.h"

#include "audit.h"
#include "drivenum.h"
#include "emuopts.h"
#include "softlist.h"
#include "zippath.h"


/***************************************************************************
SHIMS - temporarily until the core is refactored
***************************************************************************/

static std::string zippath_parent(const char *path)
{
	std::string result;
	util::zippath_parent(result, path);
	return result;
}

static std::string zippath_combine(const std::string &path1, const std::string &path2)
{
	std::string result;
	util::zippath_combine(result, path1.c_str(), path2.c_str());
	return result;
}

namespace ui {

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

menu_control_device_image::menu_control_device_image(mame_ui_manager &mui, render_container *container, device_image_interface *_image)
	: menu(mui, container),
		submenu_result(0),
		create_ok(false),
		create_confirmed(false)
{
	image = _image;

	state = START_FILE;

	// does the image exist?
	if (image->exists())
	{
		// if so, set the current file and directory based on that file
		m_current_file = image->filename();
		m_current_directory = zippath_parent(m_current_file.c_str());
	}
	else
	{
		// if not, just set the current directory to that slot's current working directory
		m_current_directory = image->working_directory();
	}

	// check to see if the path exists; if not clear it
	if (util::zippath_opendir(m_current_directory.c_str(), nullptr) != osd_file::error::NONE)
		m_current_directory.clear();
}


//-------------------------------------------------
//  test_create - creates a new disk image
//-------------------------------------------------

void menu_control_device_image::test_create(bool &can_create, bool &need_confirm)
{
	osd::directory::entry::entry_type file_type;

	// assemble the full path
	auto path = zippath_combine(m_current_directory, m_current_file);

	/* does a file or a directory exist at the path */
	auto entry = osd_stat(path.c_str());
	file_type = (entry != nullptr) ? entry->type : osd::directory::entry::entry_type::NONE;

	switch(file_type)
	{
		case osd::directory::entry::entry_type::NONE:
			/* no file/dir here - always create */
			can_create = true;
			need_confirm = false;
			break;

		case osd::directory::entry::entry_type::FILE:
			/* a file exists here - ask for permission from the user */
			can_create = true;
			need_confirm = true;
			break;

		case osd::directory::entry::entry_type::DIR:
			/* a directory exists here - we can't save over it */
			ui().popup_time(5, "%s", _("Cannot save over directory"));
			can_create = false;
			need_confirm = false;
			break;

		default:
			can_create = false;
			need_confirm = false;
			fatalerror("Unexpected\n");
	}
}


//-------------------------------------------------
//  hook_load
//-------------------------------------------------

void menu_control_device_image::hook_load(std::string name, bool softlist)
{
	if (image->is_reset_on_load()) image->set_init_phase();
	image->load(name.c_str());
	menu::stack_pop(machine());
}


//-------------------------------------------------
//  populate
//-------------------------------------------------

void menu_control_device_image::populate()
{
}


//-------------------------------------------------
//  handle
//-------------------------------------------------

void menu_control_device_image::handle()
{
	switch(state) {
	case START_FILE: {
		submenu_result = -1;
		menu::stack_push<menu_file_selector>(ui(), container, image, m_current_directory, m_current_file, true, image->image_interface()!=nullptr, image->is_creatable(), &submenu_result);
		state = SELECT_FILE;
		break;
	}

		break;

	case SELECT_FILE:
		switch(submenu_result) {
		case menu_file_selector::R_EMPTY:
			image->unload();
			menu::stack_pop(machine());
			break;

		case menu_file_selector::R_FILE:
			hook_load(m_current_file, false);
			break;

		case menu_file_selector::R_CREATE:
			menu::stack_push<menu_file_create>(ui(), container, image, m_current_directory, m_current_file, &create_ok);
			state = CHECK_CREATE;
			break;

		case -1: // return to system
			menu::stack_pop(machine());
			break;
		}
		break;

	case CREATE_FILE: {
		bool can_create, need_confirm;
		test_create(can_create, need_confirm);
		if(can_create) {
			if(need_confirm) {
				menu::stack_push<menu_confirm_save_as>(ui(), container, &create_confirmed);
				state = CREATE_CONFIRM;
			} else {
				state = DO_CREATE;
				handle();
			}
		} else {
			state = START_FILE;
			handle();
		}
		break;
	}

	case CREATE_CONFIRM:
		state = create_confirmed ? DO_CREATE : START_FILE;
		handle();
		break;

	case CHECK_CREATE:
		state = create_ok ? CREATE_FILE : START_FILE;
		handle();
		break;

	case DO_CREATE: {
		auto path = zippath_combine(m_current_directory, m_current_file);
		int err = image->create(path.c_str(), nullptr, nullptr);
		if (err != 0)
			machine().popmessage("Error: %s", image->error());
		menu::stack_pop(machine());
		break;
	}
	}
}

} // namespace ui
