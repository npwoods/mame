// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    mame_menubar.c

    Internal MAME menu bar for the user interface.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "emu.h"
#include "ui/mame_menubar.h"
#include "ui/selgame.h"
#include "ui/miscmenu.h"
#include "ui/filesel.h"
#include "ui/tapectrl.h"
#include "ui/bbcontrl.h"
#include "ui/swlist.h"
#include "ui/viewgfx.h"
#include "ui/barcode.h"
#include "ui/menu.h"
#include "ui/cheatopt.h"
#include "ui/inputmap.h"
#include "ui/sliders.h"
#include "ui/slotopt.h"
#include "ui/info.h"
#include "ui/filemngr.h"
#include "softlist.h"
#include "cheat.h"
#include "mame.h"


namespace ui {

//**************************************************************************
//  CONSTANTS
//**************************************************************************

#ifdef MAME_PROFILER
#define HAS_PROFILER			1
#else // !MAME_PROFILER
#define HAS_PROFILER			0
#endif // MAME_PROFILER


//**************************************************************************
//  MENUBAR IMPLEMENTATION
//**************************************************************************

std::string mame_menubar::s_softlist_result;
device_image_interface *mame_menubar::s_softlist_image;


//-------------------------------------------------
//  ctor
//-------------------------------------------------

mame_menubar::mame_menubar(mame_ui_manager &mui)
	: menubar(mui)
{
}


//-------------------------------------------------
//  handle
//-------------------------------------------------

void mame_menubar::handle(render_container *container)
{
	// check to see if we have a softlist selection
	if (s_softlist_result.length() > 0)
	{
		// do the load
		s_softlist_image->load(s_softlist_result.c_str());

		// clear out state
		s_softlist_image = NULL;
		s_softlist_result.clear();
	}

	// call inherited method
	menubar::handle(container);
}


//-------------------------------------------------
//  start_menu
//-------------------------------------------------

void mame_menubar::start_menu(std::unique_ptr<menu> &&menu)
{
	ui().set_handler<mame_ui_manager&>(UI_CALLBACK_TYPE_MENU, ui::menu::ui_handler, ui());
	menu::stack_push(std::move(menu));
}


//-------------------------------------------------
//  start_menu
//-------------------------------------------------

template<class T, typename... Params>
void mame_menubar::start_menu(Params &&... args)
{
	std::unique_ptr<menu> ptr(global_alloc_clear<T>(ui(), container(), std::forward<Params>(args)...));
	start_menu(std::move(ptr));
}


//-------------------------------------------------
//  menubar_draw_ui_elements
//-------------------------------------------------

void mame_menubar::menubar_draw_ui_elements()
{
	// first draw the FPS counter 
	if (ui().show_fps_counter())
	{
		std::string tempstring = machine().video().speed_text();
		ui().draw_text_full(container(), tempstring.c_str(), 0.0f, 0.0f, 1.0f,
					JUSTIFY_RIGHT, WRAP_WORD, DRAW_OPAQUE, rgb_t::white, rgb_t::black, NULL, NULL);
	}

	// draw the profiler if visible 
	if (ui().show_profiler())
	{
		const char *text = g_profiler.text(machine());
		ui().draw_text_full(container(), text, 0.0f, 0.0f, 1.0f, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, rgb_t::white, rgb_t::black, NULL, NULL);
	}

	// check for fast forward 
	if (machine().ioport().type_pressed(IPT_UI_FAST_FORWARD))
	{
		machine().video().set_fastforward(true);
		ui().show_fps_temp(0.5);
	}
	else
		machine().video().set_fastforward(false);

}


//-------------------------------------------------
//  menubar_build_menus
//-------------------------------------------------

void mame_menubar::menubar_build_menus()
{
	// build normal menus
	build_file_menu();
	if (has_images())
		build_images_menu();
	build_options_menu();
	build_settings_menu();
	build_help_menu();

	// and customize them
	// TODO - machine().osd().customize_menubar(*this);
}


//-------------------------------------------------
//  build_file_menu
//-------------------------------------------------

void mame_menubar::build_file_menu()
{
	std::string menu_text;
	menu_item &file_menu = root_menu().append(_("File"));

	// show gfx
	if (ui_gfx_is_relevant(machine()))
		file_menu.append(_("Show Graphics/Palette..."), &mame_menubar::view_gfx, *this, IPT_UI_SHOW_GFX);

	// save screen snapshot
	file_menu.append(_("Save Screen Snapshot(s)"), &video_manager::save_active_screen_snapshots, machine().video(), IPT_UI_SNAPSHOT);

	// record movie
	menu_item &record_movie_menu = file_menu.append(_("Record Movie"), &video_manager::toggle_record_movie, machine().video(), IPT_UI_RECORD_MOVIE);
	record_movie_menu.set_checked(machine().video().is_recording());

	// save state
	file_menu.append(_("Save State..."), &mame_menubar::load_or_save, *this, (UINT32) LOADSAVE_SAVE, IPT_UI_SAVE_STATE);

	// load state
	file_menu.append(_("Load State..."), &mame_menubar::load_or_save, *this, (UINT32) LOADSAVE_LOAD, IPT_UI_LOAD_STATE);

	// separator
	file_menu.append_separator();

	// paste
	if (machine().ioport().has_keyboard() && machine().ioport().natkeyboard().can_post())
	{
		menu_item &paste_menu = file_menu.append(_("Paste"), &mame_ui_manager::paste, ui(), IPT_UI_PASTE);
		paste_menu.set_enabled(ui().can_paste());
	}

	// pause
	menu_item &pause_menu = file_menu.append(_("Pause"), &running_machine::toggle_pause, machine(), IPT_UI_PAUSE);
	pause_menu.set_checked(machine().paused());

	// reset
	menu_item &reset_menu = file_menu.append(_("Reset"));
	reset_menu.append(_("Hard"), &running_machine::schedule_hard_reset, machine(), IPT_UI_RESET_MACHINE);
	reset_menu.append(_("Soft"), &running_machine::schedule_soft_reset, machine(), IPT_UI_SOFT_RESET);

	// separator
	file_menu.append_separator();

	// select new game
	file_menu.append(_("Select New Machine..."), &mame_menubar::select_new_game, *this);

	// exit
	file_menu.append(_("Exit"), &mame_ui_manager::request_quit, ui(), IPT_UI_CANCEL);
}


//-------------------------------------------------
//  build_images_menu
//-------------------------------------------------

void mame_menubar::build_images_menu()
{
	// add the root "Images" menu
	menu_item &images_menu = root_menu().append(_("Images"));

	// loop through all devices
	for (device_image_interface &image : image_interface_iterator(machine().root_device()))
	{
		bool is_loaded = image.basename() != NULL;

		std::string buffer = string_format("%s (%s): \t%s",
			image.device().name(),
			image.brief_instance_name(),
			is_loaded ? image.basename() : "[empty]");

		// append the menu item for this device
		menu_item &menu = images_menu.append(buffer.c_str());

		// software list
		if (image.image_interface() != NULL)
		{
			if (build_software_list_menus(menu, &image))
				menu.append_separator();
		}

		// load
		menu.append(_("Load..."), &mame_menubar::load, *this, &image);

		// unload
		menu_item &unload_menu = menu.append(_("Unload"), &device_image_interface::unload, image);
		unload_menu.set_enabled(is_loaded);

		// tape control
		cassette_image_device *cassette = dynamic_cast<cassette_image_device *>(&image);
		if (cassette != NULL)
		{
			menu_item &control_menu = menu.append(_("Tape Control..."), &mame_menubar::tape_control, *this, cassette);
			control_menu.set_enabled(is_loaded);
		}

		// bitbanger control
		bitbanger_device *bitbanger = dynamic_cast<bitbanger_device *>(&image);
		if (bitbanger != NULL)
		{
			menu_item &control_menu = menu.append(_("Bitbanger Control..."), &mame_menubar::bitbanger_control, *this, bitbanger);
			control_menu.set_enabled(is_loaded);
		}
	}
}


//-------------------------------------------------
//  build_software_list_menus
//-------------------------------------------------

bool mame_menubar::build_software_list_menus(menu_item &menu, device_image_interface *image)
{
	int item_count = 0;
	menu_item *last_menu_item = NULL;
	std::string description;
	softlist_type types[] = { SOFTWARE_LIST_ORIGINAL_SYSTEM, SOFTWARE_LIST_COMPATIBLE_SYSTEM };
	software_list_device_iterator softlist_iter(machine().config().root_device());

	// first do "original system" softlists, then do compatible ones
	for (int typenum = 0; typenum < ARRAY_LENGTH(types); typenum++)
	{
		for (software_list_device &swlist : software_list_device_iterator(machine().config().root_device()))
		{
			if ((swlist.list_type() == types[typenum]) && is_softlist_relevant(&swlist, image->image_interface(), description))
			{
				// we've found a softlist; append the menu item
				last_menu_item = &menu.append(description.c_str(), &mame_menubar::select_from_software_list, *this, image, &swlist);
				item_count++;
			}
		}
	}

	// if we only had one list, lets use a generic name
	if (last_menu_item != NULL && (item_count == 1))
		last_menu_item->set_text(_("Software list..."));

	return item_count > 0;
}


//-------------------------------------------------
//  build_options_menu
//-------------------------------------------------

void mame_menubar::build_options_menu()
{
	std::string menu_text;
	menu_item &options_menu = root_menu().append(_("Options"));

	// throttle
	static const float throttle_rates[] = { 10.0f, 5.0f, 2.0f, 1.0f, 0.5f, 0.2f, 0.1f };
	menu_item &throttle_menu = options_menu.append(_("Throttle"));
	for (int i = 0; i < ARRAY_LENGTH(throttle_rates); i++)
	{
		menu_text = string_format("%d%%", (int) (throttle_rates[i] * 100));
		menu_item &menu = throttle_menu.append(menu_text.c_str(), &mame_menubar::set_throttle_rate, *this, throttle_rates[i]);
		menu.set_checked(machine().video().throttle_rate() == throttle_rates[i]);
	}
	throttle_menu.append_separator();
	throttle_menu.append("Warp Mode", &mame_menubar::set_warp_mode, &mame_menubar::warp_mode, *this, IPT_UI_THROTTLE);

	// frame skip
	menu_item &frameskip_menu = options_menu.append(_("Frame Skip"));
	for (int i = -1; i <= MAX_FRAMESKIP; i++)
	{
		const char *item = _("Auto");
		if (i >= 0)
		{
			menu_text = string_format("%d", i);
			item = menu_text.c_str();
		}

		menu_item &menu = frameskip_menu.append(item, &video_manager::set_frameskip, machine().video(), i);
		menu.set_checked(machine().video().frameskip() == i);
	}

	// separator
	frameskip_menu.append_separator();

	// increase
	frameskip_menu.append(_("Increase"), &mame_ui_manager::increase_frameskip, ui(), IPT_UI_FRAMESKIP_INC);

	// decrease
	frameskip_menu.append(_("Decrease"), &mame_ui_manager::decrease_frameskip, ui(), IPT_UI_FRAMESKIP_DEC);

	// show fps
	options_menu.append(_("Show Frames Per Second"), &mame_ui_manager::set_show_fps, &mame_ui_manager::show_fps, ui(), IPT_UI_SHOW_FPS);

	// show profiler
	if (HAS_PROFILER)
		options_menu.append(_("Show Profiler"), &mame_ui_manager::set_show_profiler, &mame_ui_manager::show_profiler, ui(), IPT_UI_SHOW_PROFILER);

	// video
	// do different things if we actually have multiple render targets
	menu_item &video_menu = options_menu.append(_("Video"));
	if (machine().render().target_by_index(1) != NULL)
	{
		// multiple targets
		int targetnum = 0;
		render_target *target;
		while((target = machine().render().target_by_index(targetnum)) != NULL)
		{
			std::string buffer;
			buffer = string_format(_("Screen #%d"), targetnum++);
			menu_item &target_menu = options_menu.append(buffer.c_str());
			build_video_target_menu(target_menu, *target);
		}
	}
	else
	{
		// single target
		build_video_target_menu(video_menu, *machine().render().first_target());
	}

	// separator
	options_menu.append_separator();

	// slot devices
	slot_interface_iterator slotiter(machine().root_device());
	if (slotiter.first() != NULL)
		options_menu.append<mame_menubar>(_("Slot Devices..."), &mame_menubar::start_menu<menu_slot_devices>, *this);

	// barcode reader
	barcode_reader_device_iterator bcriter(machine().root_device());
	if (bcriter.first() != NULL)
		options_menu.append<mame_menubar>(_("Barcode Reader..."), &mame_menubar::barcode_reader_control, *this);
		
	// network devices
	network_interface_iterator netiter(machine().root_device());
	if (netiter.first() != NULL)
		options_menu.append<mame_menubar>(_("Network Devices..."), &mame_menubar::start_menu<menu_network_devices>, *this);

	// keyboard
	if (machine().ioport().has_keyboard() && machine().ioport().natkeyboard().can_post())
	{
		menu_item &keyboard_menu = options_menu.append(_("Keyboard"));
		keyboard_menu.append<mame_ui_manager, bool>(_("Emulated"), &mame_ui_manager::set_use_natural_keyboard, &mame_ui_manager::use_natural_keyboard, ui(), false);
		keyboard_menu.append<mame_ui_manager, bool>(_("Natural"),  &mame_ui_manager::set_use_natural_keyboard, &mame_ui_manager::use_natural_keyboard, ui(), true);
	}

	// crosshair options
	if (machine().crosshair().get_usage())
		options_menu.append<mame_menubar>(_("Crosshair Options..."), &mame_menubar::start_menu<menu_crosshair>, *this);

	// cheat
	if (machine().options().cheat() && mame_machine_manager::instance()->cheat().entries().size() > 0)
	{
		options_menu.append_separator();
		options_menu.append(_("Cheats enabled"), &cheat_manager::set_enable, &cheat_manager::enabled, mame_machine_manager::instance()->cheat(), IPT_UI_TOGGLE_CHEAT);
		options_menu.append<mame_menubar>(_("Cheat..."), &mame_menubar::start_menu<menu_cheat>, *this);
	}
}


//-------------------------------------------------
//  build_video_target_menu
//-------------------------------------------------

void mame_menubar::build_video_target_menu(menu_item &target_menu, render_target &target)
{
	std::string tempstring;
	const char *view_name;

	// add the menu items for each view
	for(int viewnum = 0; (view_name = target.view_name(viewnum)) != NULL; viewnum++)
	{
		// replace spaces with underscores
		std::string tempstring(view_name);
		strreplace(tempstring, "_", " ");

		// append the menu
		target_menu.append<render_target, int>(tempstring.c_str(), &render_target::set_view, &render_target::view, target, viewnum);
	}

	// separator
	target_menu.append_separator();

	// rotation
	menu_item &rotation_menu = target_menu.append(_("Rotation"));
	rotation_menu.append(_("None"),					&render_target::set_orientation, &render_target::orientation, target, ROT0);
	rotation_menu.append(_("Clockwise 90"),			&render_target::set_orientation, &render_target::orientation, target, ROT90);
	rotation_menu.append(_("180"),					&render_target::set_orientation, &render_target::orientation, target, ROT180);
	rotation_menu.append(_("Counterclockwise 90"),	&render_target::set_orientation, &render_target::orientation, target, ROT270);

	// show backdrops
	target_menu.append(_("Show Backdrops"), &render_target::set_backdrops_enabled, &render_target::backdrops_enabled, target);

	// show overlay
	target_menu.append(_("Show Overlays"), &render_target::set_overlays_enabled, &render_target::overlays_enabled, target);

	// show bezel
	target_menu.append(_("Show Bezels"), &render_target::set_bezels_enabled, &render_target::bezels_enabled, target);

	// show cpanel
	target_menu.append(_("Show CPanels"), &render_target::set_cpanels_enabled, &render_target::cpanels_enabled, target);

	// show marquee
	target_menu.append(_("Show Marquees"), &render_target::set_marquees_enabled, &render_target::marquees_enabled, target);

	// view
	menu_item &view_menu = target_menu.append(_("View"));
	view_menu.append(_("Cropped"), &render_target::set_zoom_to_screen, &render_target::zoom_to_screen, target, true);
	view_menu.append(_("Full"), &render_target::set_zoom_to_screen, &render_target::zoom_to_screen, target, false);
}


//-------------------------------------------------
//  build_settings_menu
//-------------------------------------------------

void mame_menubar::build_settings_menu()
{
	std::string menu_text;
	menu_item &settings_menu = root_menu().append(_("Settings"));

	// general input
	// TODO - BREAK THIS APART?
	settings_menu.append<mame_menubar>(_("General Input..."), &mame_menubar::start_menu<menu_input_groups>, *this);

	// game input
	settings_menu.append<mame_menubar>(_("Machine Input"), &mame_menubar::start_menu<menu_input_specific>, *this);

	// analog controls
	if (machine().ioport().has_analog())
		settings_menu.append<mame_menubar>(_("Analog Controls..."), &mame_menubar::start_menu<menu_analog>, *this);

	// dip switches
	if (machine().ioport().has_dips())
		settings_menu.append<mame_menubar>(_("Dip Switches..."), &mame_menubar::start_menu<menu_settings_dip_switches>, *this);

	// driver configuration
	if (machine().ioport().has_configs())
	{
		settings_menu.append<mame_menubar>(_("Machine Configuration..."), &mame_menubar::start_menu<menu_settings_driver_config>, *this);
	}

	// bios selection
	if (machine().ioport().has_bioses())
		settings_menu.append<mame_menubar>(_("Bios Selection..."), &mame_menubar::start_menu<menu_bios_selection>, *this);

	// sliders
	settings_menu.append<mame_menubar>(_("Sliders..."), &mame_menubar::start_menu<menu_sliders>, *this, IPT_UI_ON_SCREEN_DISPLAY);
}


//-------------------------------------------------
//  build_help_menu
//-------------------------------------------------

void mame_menubar::build_help_menu()
{
	std::string menu_text;
	menu_item &help_menu = root_menu().append(_("Help"));

	// bookkeeping info
	help_menu.append<mame_menubar>(_("Bookkeeping info..."), &mame_menubar::start_menu<menu_bookkeeping>, *this);

	// game info
	help_menu.append<mame_menubar>(_("Machine Information..."), &mame_menubar::start_menu<menu_game_info>, *this);

	// image information
	image_interface_iterator imgiter(machine().root_device());
	if (imgiter.first() != NULL)
		help_menu.append<mame_menubar>(_("Image Information..."), &mame_menubar::start_menu<menu_image_info>, *this);
}


//-------------------------------------------------
//  is_softlist_relevant
//-------------------------------------------------

bool mame_menubar::is_softlist_relevant(software_list_device *swlist, const char *interface, std::string &list_description)
{
	bool result = false;

	for (software_info &swinfo : swlist->get_info())
	{
		software_part *part = swinfo.find_part(NULL, NULL);
		if (part->matches_interface(interface))
		{
			list_description = string_format(_("%s..."), swlist->description());
			result = true;
			break;
		}
	}

	return result;
}


//-------------------------------------------------
//  set_ui_handler
//-------------------------------------------------

template<typename... Params>
void mame_menubar::set_ui_handler(Params... args)
{
	// first pause
	machine().pause();

	// and transfer control
	ui().set_handler(args...);
}


//-------------------------------------------------
//  select_new_game
//-------------------------------------------------

void mame_menubar::select_new_game()
{
	start_menu<menu_select_game>(machine().system().name);
}


//-------------------------------------------------
//  select_from_software_list
//-------------------------------------------------

void mame_menubar::select_from_software_list(device_image_interface *image, software_list_device *swlist)
{
	s_softlist_image = image;
	start_menu<menu_software_list>(swlist, image->image_interface(), s_softlist_result);
}


//-------------------------------------------------
//  tape_control
//-------------------------------------------------

void mame_menubar::tape_control(cassette_image_device *image)
{
	start_menu<menu_tape_control>(image);
}


//-------------------------------------------------
//  bitbanger_control
//-------------------------------------------------

void mame_menubar::bitbanger_control(bitbanger_device *image)
{
	start_menu<ui_menu_bitbanger_control>(image);
}


//-------------------------------------------------
//  barcode_reader_control
//-------------------------------------------------

void mame_menubar::barcode_reader_control()
{
	start_menu<menu_barcode_reader>(nullptr);
}


//-------------------------------------------------
//  load
//-------------------------------------------------

void mame_menubar::load(device_image_interface *image)
{
	start_menu(menu_file_manager::create_device_menu(ui(), container(), image));
}


//-------------------------------------------------
//  has_images
//-------------------------------------------------

bool mame_menubar::has_images()
{
	image_interface_iterator iter(machine().root_device());
	return iter.first() != NULL;
}


//-------------------------------------------------
//  show_fps_temp
//-------------------------------------------------

void mame_menubar::show_fps_temp()
{
	ui().show_fps_temp(2.0);
}


//-------------------------------------------------
//  set_throttle_rate
//-------------------------------------------------

void mame_menubar::set_throttle_rate(float throttle_rate)
{
	show_fps_temp();
	machine().video().set_throttle_rate(throttle_rate);
}


//-------------------------------------------------
//  set_warp_mode
//-------------------------------------------------

void mame_menubar::set_warp_mode(bool warp_mode)
{
	// "warp mode" is the opposite of throttle
	show_fps_temp();
	machine().video().set_throttled(!warp_mode);
}


//-------------------------------------------------
//  warp_mode
//-------------------------------------------------

bool mame_menubar::warp_mode() const
{
	// "warp mode" is the opposite of throttle
	return !machine().video().throttled();
}


//-------------------------------------------------
//  view_gfx
//-------------------------------------------------

void mame_menubar::view_gfx()
{
	// first pause
	machine().pause();

	// and transfer control
	ui().set_handler<mame_ui_manager&, bool>(UI_CALLBACK_TYPE_GENERAL, ui_gfx_ui_handler, ui(), machine().paused());
}


//-------------------------------------------------
//  load_or_save
//-------------------------------------------------

void mame_menubar::load_or_save(UINT32 loadsave)
{
	// first pause
	machine().pause();

	// and transfer control
	ui().set_handler(UI_CALLBACK_TYPE_GENERAL, &mame_ui_manager::handler_load_save, loadsave);
}

} // namespace ui
