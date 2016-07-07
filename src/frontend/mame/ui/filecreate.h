// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

	ui/filecreate.h

	MESS's clunky built-in file manager

***************************************************************************/

#pragma once

#ifndef MAME_FRONTEND_UI_FILECREATE_H
#define MAME_FRONTEND_UI_FILECREATE_H

#include "ui/menu.h"

class floppy_image_format_t;

namespace ui {
// ======================> menu_confirm_save_as

class menu_confirm_save_as : public menu
{
public:
	menu_confirm_save_as(mame_ui_manager &mui, render_container *container, bool *yes);
	virtual ~menu_confirm_save_as() override;
	virtual void populate() override;
	virtual void handle() override;

private:
	bool *m_yes;
};


// ======================> menu_file_create

class menu_file_create : public menu
{
public:
	menu_file_create(mame_ui_manager &mui, render_container *container, device_image_interface *image, std::string &current_directory, std::string &current_file, bool &ok);
	virtual ~menu_file_create() override;
	virtual void populate() override;
	virtual void handle() override;
	virtual void custom_render(void *selectedref, float top, float bottom, float x, float y, float x2, float y2) override;

private:
	device_image_interface *								m_image;
	std::string &											m_current_directory;
	std::string &											m_current_file;
	device_image_interface::formatlist_type::const_iterator	m_current_format;
	std::string												m_filename;
	bool &													m_ok;

	void do_select();
	bool has_formats() const;
	void previous_format();
	void next_format();
	bool has_previous_format() const;
	bool has_next_format() const;
	void format_changed();
};


// ======================> menu_select_format

class menu_select_format : public menu
{
public:
	menu_select_format(mame_ui_manager &mui, render_container *container,
		floppy_image_format_t **formats, int ext_match, int total_usable, int *result);
	virtual ~menu_select_format() override;
	virtual void populate() override;
	virtual void handle() override;

private:
	// internal state
	floppy_image_format_t **    m_formats;
	int                         m_ext_match;
	int                         m_total_usable;
	int *                       m_result;
};


} // namespace ui

#endif // MAME_FRONTEND_UI_FILECREATE_H
