/***************************************************************************

    emenubar.h

    Internal MAME menu bar for the user interface.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef MAME_FRONTEND_UI_EMENUBAR
#define MAME_FRONTEND_UI_EMENUBAR

#include "ui/menubar.h"
#include "imagedev/cassette.h"
#include "imagedev/bitbngr.h"

namespace ui {

class menu;

class emu_menubar : public menubar
{
public:
	emu_menubar(::mame_ui_manager &mui);

	virtual void handle(render_container *container) override;

protected:
	virtual void menubar_build_menus() override;
	virtual void menubar_draw_ui_elements() override;

private:
	// variables
	static device_image_interface *s_softlist_image;
	static std::string s_softlist_result;

	// menubar building
	void build_file_menu();
	void build_images_menu();
	bool build_software_list_menus(menu_item &menu, device_image_interface *image);
	void build_options_menu();
	void build_video_target_menu(menu_item &target_menu, render_target &target);
	void build_settings_menu();
	void build_help_menu();

	// miscellaneous
	bool is_softlist_relevant(software_list_device *swlist, const char *interface, std::string &list_description);
	void set_ui_handler(ui_callback callback, UINT32 param);
	void select_new_game();
	void select_from_software_list(device_image_interface *image, software_list_device *swlist);
	void tape_control(cassette_image_device *image);
	void bitbanger_control(bitbanger_device *image);
	void barcode_reader_control();
	void load(device_image_interface *image);
	bool has_images();
	void show_fps_temp();
	void set_throttle_rate(float throttle_rate);
	void set_warp_mode(bool warp_mode);
	bool warp_mode() const;
	void start_menu(menu *menu);

	// template methods
	template<class _Menu>
	void start_menu();
};

} // namespace ui

#endif // MAME_FRONTEND_UI_EMENUBAR
