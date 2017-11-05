// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    menubar.h

    Internal MAME menu bar for the user interface.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef MAME_FRONTEND_UI_MENUBAR
#define MAME_FRONTEND_UI_MENUBAR

#include <functional>
#include <utility>
#include "render.h"
#include "ui/ui.h"

class mame_ui_manager;

namespace ui {

//**************************************************************************
//  MENU BAR
//**************************************************************************

class menubar
{
public:
	menubar(::mame_ui_manager &mui);
	virtual ~menubar();

	typedef std::unique_ptr<menubar> ptr;

	// menu item
	class menu_item
	{
	public:
		menu_item(menubar &menubar, std::string &&text = "", menu_item *parent = nullptr, std::function<void()> &&func = nullptr, int shortcut = 0);
		~menu_item();

		// methods
		menu_item &append(std::string &&text, std::function<void()> &&func = nullptr, int shortcut = 0);
		void append_separator();
		bool is_child_of(menu_item *that) const;
		void invoke() { m_func(); }
		void clear_area_recursive();
		menu_item *find_point(float x, float y);
		menu_item &find_child(const char *target);
		const char *shortcut_text();
		float shortcut_text_width();
		void sensible_seq_name(std::string &text, const input_seq &seq);

		// template methods; look I tried to use delegate.h but I got humbled...
		template<class _Target> menu_item &append(std::string &&text, void (_Target::*callback)(), _Target &obj, int shortcut = 0)
		{
			return append(
				std::move(text),
				[&obj, callback] { ((obj).*(callback))(); },
				shortcut);
		}
		template<class _Target, typename _Arg> menu_item &append(std::string &&text, void (_Target::*callback)(_Arg), _Target &obj, _Arg arg, int shortcut = 0)
		{
			return append(
				std::move(text),
				[&obj, callback, arg] { ((obj).*(callback))(arg); },
				shortcut);
		}
		template<class _Target, typename _Arg1, typename _Arg2> menu_item &append(std::string &&text, void (_Target::*callback)(_Arg1, _Arg2), _Target &obj, _Arg1 arg1, _Arg2 arg2, int shortcut = 0)
		{
			return append(
				std::move(text),
				[&obj, callback, arg1, arg2] { ((obj).*(callback))(arg1, arg2); },
				shortcut);
		}
		template<class _Target> menu_item &append(std::string &&text, void (_Target::*set_callback)(bool), bool (_Target::*get_callback)() const, _Target &obj, int shortcut = 0)
		{
			// tailored for a toggle
			bool current_value = ((obj).*(get_callback))();
			menu_item &menu = append(std::move(text), set_callback, obj, !current_value, shortcut);
			menu.set_checked(current_value);
			return menu;
		}
		template<class _Target, typename _Arg> menu_item &append(std::string &&text, void (_Target::*set_callback)(_Arg), _Arg (_Target::*get_callback)() const, _Target &obj, _Arg arg, int shortcut = 0)
		{
			// tailored for a set operation
			_Arg current_value = ((obj).*(get_callback))();
			menu_item &menu = append(std::move(text), set_callback, obj, arg, shortcut);
			menu.set_checked(current_value == arg);
			return menu;
		}

		// getters
		bool is_empty() const { return !m_first_child; }
		bool is_invokable() const { return !!m_func; }
		bool is_checked() const { return m_is_checked; }
		bool is_enabled() const { return m_is_enabled; }
		bool is_separator() const { return m_is_separator; }
		bool has_children() const { return m_first_child ? true : false; }
		int shortcut() const { return m_shortcut; }
		const std::string &text() const { return m_text; }
		menu_item *parent() { return m_parent; }
		menu_item *child() { return m_first_child; }
		menu_item *last_child() { return m_last_child; }
		menu_item *previous() { return m_previous; }
		menu_item *next() { return m_next; }
		menu_item *next_with_shortcut() { return m_next_with_shortcut; }
		bool is_sub_menu() const { return m_parent && m_parent->m_parent; }

		// setters
		void set_area(float x0, float y0, float x1, float y1);
		void clear_area() { set_area(-1, -1, -1, -1); }
		void set_checked(bool checked) { m_is_checked = checked; }
		void set_enabled(bool enabled) { m_is_enabled = enabled; }
		void set_text(const char *text) { m_text.assign(text); }
		void set_next_with_shortcut(menu_item *item) { m_next_with_shortcut = item; }

	private:
		// private variables
		menubar &				m_menubar;
		std::string				m_text;
		std::function<void()>	m_func;
		int						m_shortcut;
		std::string				m_shortcut_text;
		float					m_shortcut_text_width;
		bool					m_is_checked;
		bool					m_is_enabled;
		bool					m_is_separator;
		menu_item *				m_parent;
		menu_item *				m_first_child;
		menu_item *				m_last_child;
		menu_item *				m_previous;
		menu_item *				m_next;
		menu_item *				m_next_with_shortcut;
		float					m_x0;
		float					m_y0;
		float					m_x1;
		float					m_y1;

		// private methods
		void initialize(menu_item &child);
		menu_item *find_child_internal(const char *target);
	};

	// methods
	virtual void reset();
	virtual void handle(render_container &container);

	// getters
	bool has_focus() const { return m_menubar_visibility == MENUBAR_VISIBILITY_VISIBLE; }
	bool has_selection() const { return m_selected_item != nullptr; }
	menu_item &root_menu() { return m_menus; }
	bool has_been_invoked() const { return m_has_been_invoked; }

protected:
	// implemented by child classes
	virtual void menubar_build_menus() = 0;
	virtual void menubar_draw_ui_elements() = 0;

	// accessors
	mame_ui_manager &ui() const { return m_ui; }
	running_machine &machine() const { return ui().machine(); }
	render_container &container() { return *m_container; }

private:
	// menubar visibility
	enum menubar_visibility_t
	{
		MENUBAR_VISIBILITY_INVISIBLE,
		MENUBAR_VISIBILITY_TRANSLUCENT,
		MENUBAR_VISIBILITY_VISIBLE
	};

	// instance variables
	mame_ui_manager &		m_ui;						// UI we are attached to
	render_container *		m_container;
	menu_item				m_menus;					// the root menu item
	menu_item *				m_shortcuted_menu_items;	// list of menu items with shortcuts
	menu_item *				m_selected_item;			// current selection
	menu_item *				m_active_item;				// active menu item
	bool					m_dragged;					// have we dragged over at least one item?
	float					m_mouse_x, m_mouse_y;
	bool					m_mouse_button;
	std::string				m_checkmark;				// checkmark text
	float					m_checkmark_width;			// checkmark string width
	osd_ticks_t				m_last_mouse_move;
	menubar_visibility_t	m_menubar_visibility;
	bool					m_first_time;
	bool					m_has_been_invoked;

	// selection walking
	bool walk_selection_previous();
	bool walk_selection_next();
	bool walk_selection_child();
	bool walk_selection_parent();
	bool walk_selection_escape();
	bool walk_selection_previous_sub_menu();
	bool walk_selection_next_sub_menu();

	// miscellaneous
	void draw_child_menu(menu_item *menu, float x, float y);
	bool is_child_menu_visible(menu_item *menu) const;
	void draw_menu_item_text(menu_item *mi, float x0, float y0, float x1, float y1, bool decorations, const float *column_widths = NULL);
	bool event_loop();
	bool poll_navigation_keys();
	bool poll_shortcut_keys(bool swallow);
	bool shortcut_key_pressed(int key);
	void toggle_selection();
	void invoke(menu_item *menu);
	bool find_mouse(float &mouse_x, float &mouse_y, bool &mouse_button);
	menubar_visibility_t get_menubar_visibility();
	rgb_t adjust_color(rgb_t color);
	void highlight(float x0, float y0, float x1, float y1, rgb_t bgcolor);
	void draw_arrow(float x0, float y0, float x1, float y1, rgb_t fgcolor, uint32_t orientation);
};

} // namespace ui

#endif /* MAME_FRONTEND_UI_MENUBAR */
