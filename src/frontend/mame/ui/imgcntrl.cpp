// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    ui/imgcntrl.cpp

    MAME's clunky built-in file manager

***************************************************************************/

#include "emu.h"

#include "ui/imgcntrl.h"

#include "ui/ui.h"
#include "ui/filesel.h"
#include "ui/filecreate.h"
#include "ui/swlist.h"

#include "audit.h"
#include "drivenum.h"
#include "emuopts.h"
#include "softlist_dev.h"
#include "zippath.h"


namespace ui {

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

menu_control_device_image::menu_control_device_image(mame_ui_manager &mui, render_container &container, device_image_interface &image)
	: menu(mui, container)
	, m_image(image)
	, m_create_ok(false)
	, m_create_confirmed(false)
{
	m_submenu_result.i = -1;

	m_state = START_FILE;

	// if the image exists, set the working directory to the parent directory
	if (m_image.exists())
	{
		m_current_file.assign(m_image.filename());
		util::zippath_parent(m_current_directory, m_current_file);
	}
	else
	{
		m_current_directory = m_image.working_directory();
	}

	// check to see if the path exists; if not clear it
	util::zippath_directory::ptr directory;
	if (util::zippath_directory::open(m_current_directory, directory) != osd_file::error::NONE)
		m_current_directory.clear();
}


//-------------------------------------------------
//  dtor
//-------------------------------------------------

menu_control_device_image::~menu_control_device_image()
{
}


//-------------------------------------------------
//  test_create - creates a new disk image
//-------------------------------------------------

void menu_control_device_image::test_create(bool &can_create, bool &need_confirm)
{
	// assemble the full path
	auto path = util::zippath_combine(m_current_directory, m_current_file);

	// does a file or a directory exist at the path
	auto entry = osd_stat(path.c_str());
	auto file_type = (entry != nullptr) ? entry->type : osd::directory::entry::entry_type::NONE;

	switch(file_type)
	{
		case osd::directory::entry::entry_type::NONE:
			// no file/dir here - always create
			can_create = true;
			need_confirm = false;
			break;

		case osd::directory::entry::entry_type::FILE:
			// a file exists here - ask for permission from the user
			can_create = true;
			need_confirm = true;
			break;

		case osd::directory::entry::entry_type::DIR:
			// a directory exists here - we can't save over it
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

void menu_control_device_image::hook_load(const std::string &name)
{
	m_image.load(name);
	stack_pop();
}


//-------------------------------------------------
//  populate
//-------------------------------------------------

void menu_control_device_image::populate(float &customtop, float &custombottom)
{
}


//-------------------------------------------------
//  handle
//-------------------------------------------------

void menu_control_device_image::handle()
{
	switch(m_state)
	{
	case START_FILE:
		m_submenu_result.filesel = menu_file_selector::result::INVALID;
		menu::stack_push<menu_file_selector>(ui(), container(), &m_image, m_current_directory, m_current_file, true, m_image.image_interface()!=nullptr, m_image.is_creatable(), m_submenu_result.filesel);
		m_state = SELECT_FILE;
		break;


	case SELECT_FILE:
		switch(m_submenu_result.filesel)
		{
		case menu_file_selector::result::EMPTY:
			m_image.unload();
			stack_pop();
			break;

		case menu_file_selector::result::FILE:
			hook_load(m_current_file);
			break;

		case menu_file_selector::result::CREATE:
			menu::stack_push<menu_file_create>(ui(), container(), m_image, m_current_directory, m_current_file, m_create_format, m_option_resolution, m_create_ok);
			m_state = CHECK_CREATE;
			break;

		default: // return to system
			stack_pop();
			break;
		}
		break;

	case CREATE_FILE: {
		bool can_create, need_confirm;
		test_create(can_create, need_confirm);
		if(can_create) {
			if(need_confirm) {
				menu::stack_push<menu_confirm_save_as>(ui(), container(), &m_create_confirmed);
				m_state = CREATE_CONFIRM;
			} else {
				m_state = DO_CREATE;
				handle();
			}
		} else {
			m_state = START_FILE;
			handle();
		}
		break;
	}

	case CREATE_CONFIRM:
		m_state = m_create_confirmed ? DO_CREATE : START_FILE;
		handle();
		break;

	case CHECK_CREATE:
		m_state = m_create_ok ? CREATE_FILE : START_FILE;
		handle();
		break;

	case DO_CREATE: {
		auto path = util::zippath_combine(m_current_directory, m_current_file);
		image_init_result err = m_image.create(path, m_create_format, m_option_resolution.get());
		if (err != image_init_result::PASS)
			machine().popmessage("Error: %s", m_image.error());
		stack_pop();
		break;
	}
	}
}

} // namespace ui
