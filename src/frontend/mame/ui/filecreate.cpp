// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    ui/filecreate.cpp

    MAME's clunky built-in file manager

    TODO
        - Support image creation arguments

***************************************************************************/

#include "emu.h"

#include "ui/filecreate.h"
#include "ui/ui.h"

#include "imagedev/floppy.h"

#include "zippath.h"

#include <cstring>


namespace ui {
/***************************************************************************
	CONSTANTS
***************************************************************************/

// conditional compilation to enable chosing of image formats - this is not
// yet fully implemented
#define ENABLE_FORMATS          1

// itemrefs for key menu items
#define ITEMREF_NEW_IMAGE_NAME		((void *) 0x0001)
#define ITEMREF_FORMAT				((void *) 0x0003)
#define ITEMREF_NO					((void *) 0x0004)
#define ITEMREF_YES					((void *) 0x0005)
#define ITEMREF_PARAMETER_BEGIN		((void *) 0x1000)
#define ITEMREF_PARAMETER_END		((void *) 0x1100)


/***************************************************************************
MENU HELPERS
***************************************************************************/

//-------------------------------------------------
//  input_character - inputs a typed character
//  into a buffer
//-------------------------------------------------

template <typename F>
static void input_character(std::string &buffer, unicode_char unichar, F &&filter)
{
	auto buflen = buffer.size();

	if ((unichar == 8) || (unichar == 0x7f))
	{
		// backspace
		if (0 < buflen)
		{
			auto buffer_oldend = buffer.c_str() + buflen;
			auto buffer_newend = utf8_previous_char(buffer_oldend);
			buffer.resize(buffer_newend - buffer.c_str());
		}
	}
	else if ((unichar >= ' ') && (!filter || filter(unichar)))
	{
		// append this character
		buffer += utf8_from_uchar(unichar);
	}
}


/***************************************************************************
CONFIRM SAVE AS MENU
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

menu_confirm_save_as::menu_confirm_save_as(mame_ui_manager &mui, render_container &container, bool *yes)
	: menu(mui, container)
{
	m_yes = yes;
	*m_yes = false;
}


//-------------------------------------------------
//  dtor
//-------------------------------------------------

menu_confirm_save_as::~menu_confirm_save_as()
{
}


//-------------------------------------------------
//  populate
//-------------------------------------------------

void menu_confirm_save_as::populate()
{
	item_append(_("File Already Exists - Override?"), "", FLAG_DISABLE, nullptr);
	item_append(menu_item_type::SEPARATOR);
	item_append(_("No"), "", 0, ITEMREF_NO);
	item_append(_("Yes"), "", 0, ITEMREF_YES);
}

//-------------------------------------------------
//  handle - confirm save as menu
//-------------------------------------------------

void menu_confirm_save_as::handle()
{
	// process the menu
	const event *event = process(0);

	// process the event
	if ((event != nullptr) && (event->iptkey == IPT_UI_SELECT))
	{
		if (event->itemref == ITEMREF_YES)
			*m_yes = true;

		// no matter what, pop out
		stack_pop();
	}
}



/***************************************************************************
FILE CREATE MENU
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

menu_file_create::menu_file_create(mame_ui_manager &mui, render_container &container, device_image_interface *image, std::string &current_directory, std::string &current_file, bool &ok)
	: menu(mui, container)
	, m_ok(ok)
	, m_current_directory(current_directory)
	, m_current_file(current_file)
{
	m_image = image;
	m_ok = true;

	// set up initial filename
	m_filename.reserve(1024);
	m_filename = core_filename_extract_base(current_file);

	// chose a format
	if (has_formats())
	{
		// do we have options?  if so, set them up
		const auto &option_guide = m_image->create_option_guide();
		if (option_guide.entries().size() > 0)
		{
			// set up the option resolution
			m_option_resolution = std::make_unique<util::option_resolution>(option_guide);
		}

		// get the first format for now
		m_current_format = m_image->formatlist().begin();

		// set the specification to the current format
		if (m_option_resolution.get() != nullptr && m_current_format != m_image->formatlist().end())
			m_option_resolution->set_specification((*m_current_format)->optspec());

		// if we have an initial file format, lets try naming this file
		if (m_filename.empty())
		{
			const std::string &extensions = (*m_current_format)->extensions()[0];
			m_filename = string_format("%s.%s", m_image->brief_instance_name(), extensions);
		}
	}
}


//-------------------------------------------------
//  dtor
//-------------------------------------------------

menu_file_create::~menu_file_create()
{
}


//-------------------------------------------------
//  has_formats
//-------------------------------------------------

bool menu_file_create::has_formats() const
{
	return ENABLE_FORMATS && m_image->formatlist().size() > 0;
}


//-------------------------------------------------
//  custom_render - perform our special rendering
//-------------------------------------------------

void menu_file_create::custom_render(void *selectedref, float top, float bottom, float origx1, float origy1, float origx2, float origy2)
{
	extra_text_render(top, bottom, origx1, origy1, origx2, origy2,
		m_current_directory.c_str(),
		nullptr);
}


//-------------------------------------------------
//  populate - populates the file creator menu
//-------------------------------------------------

void menu_file_create::populate()
{
	std::string buffer;
	const std::string *new_image_name;

	// append the "New Image Name" item
	if (get_selection_ref() == ITEMREF_NEW_IMAGE_NAME)
	{
		buffer = m_filename + "_";
		new_image_name = &buffer;
	}
	else
	{
		new_image_name = &m_filename;
	}
	item_append(_("New Image Name:"), *new_image_name, 0, ITEMREF_NEW_IMAGE_NAME);
	item_append(menu_item_type::SEPARATOR);

	// do we support multiple formats?
	if (has_formats())
	{
		// display the format
		UINT32 flags =	(has_previous_format() ? FLAG_LEFT_ARROW : 0)
					|	(has_next_format() ? FLAG_RIGHT_ARROW : 0);
		item_append(_("Format:"), (*m_current_format)->description(), flags, ITEMREF_FORMAT);

		// do we have options?
		if (m_option_resolution.get() != nullptr)
		{
			for (auto iter = m_option_resolution->entries_begin(); iter != m_option_resolution->entries_end(); iter++)
			{
				auto &entry(*iter);

				// get the name and value of this entry
				auto name = string_format("%s:", _(entry.display_name()));
				auto value = entry.is_pertinent()
					? entry.value()
					: _("N/A");

				// append the entry
				UINT32 flags = entry.is_pertinent() ? 0 : FLAG_DISABLE;
				flags |= entry.can_bump_lower() ? FLAG_LEFT_ARROW : 0;
				flags |= entry.can_bump_higher() ? FLAG_RIGHT_ARROW : 0;
				auto itemref = itemref_from_option_guide_parameter(entry.parameter());
				item_append(name, value, flags, itemref);
			}
		}
		item_append(menu_item_type::SEPARATOR);
	}

	// finish up
	customtop = ui().get_line_height() + 3.0f * UI_BOX_TB_BORDER;
}


//-------------------------------------------------
//  itemref_from_option_guide_parameter
//-------------------------------------------------

void *menu_file_create::itemref_from_option_guide_parameter(int parameter)
{
	auto itemref_int = parameter + (int)(unsigned int)(FPTR)ITEMREF_PARAMETER_BEGIN;
	return (void *)(FPTR)(unsigned int)itemref_int;
}


//-------------------------------------------------
//  handle - file creator menu
//-------------------------------------------------

void menu_file_create::handle()
{
	// do we have an option selected?
	int opt_parameter = (get_selection_ref() >= ITEMREF_PARAMETER_BEGIN) && (get_selection_ref() < ITEMREF_PARAMETER_END)
		? ((int)(unsigned int)(FPTR)get_selection_ref()) - (int)(unsigned int)(FPTR)ITEMREF_PARAMETER_BEGIN
		: 0;

	// process the menu
	const event *event = process(0);

	// process the event
	if (event)
	{
		// handle selections
		switch (event->iptkey)
		{
		case IPT_UI_SELECT:
			do_select();
			break;

		case IPT_SPECIAL:
			if (get_selection_ref() == ITEMREF_NEW_IMAGE_NAME)
			{
				input_character(m_filename, event->unichar, &osd_is_valid_filepath_char);
				reset(reset_options::REMEMBER_POSITION);
			}
			break;

		case IPT_UI_LEFT:
			if (get_selection_ref() == ITEMREF_FORMAT)
				previous_format();
			else if (opt_parameter != 0)
				bump_parameter_lower(opt_parameter);
			break;

		case IPT_UI_RIGHT:
			if (get_selection_ref() == ITEMREF_FORMAT)
				next_format();
			else if (opt_parameter != 0)
				bump_parameter_higher(opt_parameter);
			break;

		case IPT_UI_CANCEL:
			m_ok = false;
			break;
		}
	}
}


//-------------------------------------------------
//  do_select
//-------------------------------------------------

void menu_file_create::do_select()
{
	// was this an absolute path?
	if (osd_is_absolute_path(m_filename))
	{
		// if so, did we move to a directory?
		if (osd_stat(m_filename)->type == osd::directory::entry::entry_type::DIR)
		{
			m_current_directory = std::move(m_filename);
			m_filename.clear();
			return;
		}

		// check the parent
		auto parent = util::zippath_parent(m_filename.c_str());
		if (osd_stat(m_filename)->type != osd::directory::entry::entry_type::DIR)
		{
			ui().popup_time(1, "%s", _("Invalid directory"));
			return;
		}
	}

	// check for a file extension
	auto period_position = m_filename.find(".");
	if (period_position == std::string::npos || period_position >= m_filename.length() - 1)
	{
		ui().popup_time(1, "%s", _("Please enter a file extension too"));
		return;
	}

	m_current_file = std::move(m_filename);
	menu::stack_pop(machine());
}


//-------------------------------------------------
//  previous_format
//-------------------------------------------------

void menu_file_create::previous_format()
{
	if (has_previous_format())
	{
		m_current_format--;
		format_changed();
	}
}


//-------------------------------------------------
//  next_format
//-------------------------------------------------

void menu_file_create::next_format()
{
	if (has_next_format())
	{
		m_current_format++;
		format_changed();
	}
}


//-------------------------------------------------
//  has_previous_format
//-------------------------------------------------

bool menu_file_create::has_previous_format() const
{
	return m_current_format != m_image->formatlist().begin();
}


//-------------------------------------------------
//  has_next_format
//-------------------------------------------------

bool menu_file_create::has_next_format() const
{
	return (m_current_format + 1) != m_image->formatlist().end();
}


//-------------------------------------------------
//  format_changed
//-------------------------------------------------

void menu_file_create::format_changed()
{
	// we need to repopulate the menu
	reset(reset_options::REMEMBER_REF);

	// rename the file appropriately
	const std::string &extensions = (*m_current_format)->extensions()[0];

	auto period_pos = m_filename.rfind('.');
	if (period_pos != std::string::npos)
		m_filename.resize(period_pos);
	if (!extensions.empty())
		m_filename += "." + extensions;

	// set the new specification
	m_option_resolution->set_specification((*m_current_format)->optspec());
}


//-------------------------------------------------
//  bump_parameter_lower
//-------------------------------------------------

void menu_file_create::bump_parameter_lower(int parameter)
{
	// look up the entry - this is expected to succeed
	auto entry = m_option_resolution->find(parameter);
	assert(entry != nullptr);

	// we need to make sure we can bump first
	if (entry->can_bump_lower() && entry->bump_lower())
	{
		// we need to repopulate the menu
		reset(reset_options::REMEMBER_REF);
	}
}


//-------------------------------------------------
//  bump_parameter_higher
//-------------------------------------------------

void menu_file_create::bump_parameter_higher(int parameter)
{
	// look up the entry - this is expected to succeed
	auto entry = m_option_resolution->find(parameter);
	assert(entry != nullptr);

	// we need to make sure we can bump first
	if (entry->can_bump_higher() && entry->bump_higher())
	{
		// we need to repopulate the menu
		reset(reset_options::REMEMBER_REF);
	}
}


/***************************************************************************
SELECT FORMAT MENU
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

menu_select_format::menu_select_format(mame_ui_manager &mui, render_container &container, floppy_image_format_t **formats, int ext_match, int total_usable, int *result)
	: menu(mui, container)
{
	m_formats = formats;
	m_ext_match = ext_match;
	m_total_usable = total_usable;
	m_result = result;
}


//-------------------------------------------------
//  dtor
//-------------------------------------------------

menu_select_format::~menu_select_format()
{
}


//-------------------------------------------------
//  populate
//-------------------------------------------------

void menu_select_format::populate()
{
	item_append(_("Select image format"), "", FLAG_DISABLE, nullptr);
	for (int i = 0; i < m_total_usable; i++)
	{
		const floppy_image_format_t *fmt = m_formats[i];

		if (i && i == m_ext_match)
			item_append(menu_item_type::SEPARATOR);
		item_append(fmt->description(), fmt->name(), 0, (void *)(FPTR)i);
	}
}


//-------------------------------------------------
//  handle
//-------------------------------------------------

void menu_select_format::handle()
{
	// process the menu
	const event *event = process(0);
	if (event != nullptr && event->iptkey == IPT_UI_SELECT)
	{
		*m_result = int(FPTR(event->itemref));
		stack_pop();
	}
}


} // namespace ui
