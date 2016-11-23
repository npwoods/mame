// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    ui/filecreate.h

    MESS's clunky built-in file manager

***************************************************************************/

#ifndef MAME_FRONTEND_UI_FILECREATE_H
#define MAME_FRONTEND_UI_FILECREATE_H

#pragma once

#include "ui/menu.h"

class floppy_image_format_t;

namespace ui {
// ======================> menu_confirm_save_as

class menu_confirm_save_as : public menu
{
public:
	menu_confirm_save_as(mame_ui_manager &mui, render_container &container, bool *yes);
	virtual ~menu_confirm_save_as() override;

private:
	virtual void populate(float &customtop, float &custombottom) override;
	virtual void handle() override;

	bool *m_yes;
};


// ======================> menu_file_create

class menu_file_create : public menu
{
public:
	menu_file_create(mame_ui_manager &mui, render_container &container, device_image_interface &image,
		std::string &current_directory, std::string &current_file,
		const image_device_format *&output_format, std::unique_ptr<util::option_resolution> &option_resolution,
		bool &ok);
	virtual ~menu_file_create() override;

protected:
	virtual void custom_render(void *selectedref, float top, float bottom, float x, float y, float x2, float y2) override;
	virtual void selection_changed() override;

private:
	virtual void populate(float &customtop, float &custombottom) override;
	virtual void handle() override;

	// these are passed in
	device_image_interface &								m_image;
	std::string &											m_current_directory;
	std::string &											m_current_file;
	const image_device_format *&							m_output_format;
	std::unique_ptr<util::option_resolution> &				m_option_resolution;
	bool &													m_ok;

	// these are "runtime" variables
	device_image_interface::formatlist_type::const_iterator	m_selected_format;
	std::string												m_filename;
	void *													m_new_value_itemref;
	std::string												m_new_value;

	void do_select();
	
	// formats
	bool has_formats() const;
	void previous_format();
	void next_format();
	bool has_previous_format() const;
	bool has_next_format() const;
	void format_changed();

	// parameters
	void append_option_item(const util::option_resolution::entry &entry);
	static void *itemref_from_option_guide_parameter(int parameter);
	static int option_guide_parameter_from_itemref(void *itemref);
	void bump_parameter_lower(int parameter);
	void bump_parameter_higher(int parameter);
	void flush_entered_value();
};


// ======================> menu_select_format

class menu_select_format : public menu
{
public:
	menu_select_format(mame_ui_manager &mui, render_container &container,
		floppy_image_format_t **formats, int ext_match, int total_usable, int *result);
	virtual ~menu_select_format() override;

private:
	virtual void populate(float &customtop, float &custombottom) override;
	virtual void handle() override;

	// internal state
	floppy_image_format_t **    m_formats;
	int                         m_ext_match;
	int                         m_total_usable;
	int *                       m_result;
};


} // namespace ui

#endif // MAME_FRONTEND_UI_FILECREATE_H
