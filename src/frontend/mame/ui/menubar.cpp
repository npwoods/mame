/***************************************************************************

    menubar.cpp

    Internal MAME menu bar for the user interface.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "emu.h"
#include "natkeyboard.h"
#include "ui/menubar.h"
#include "ui/ui.h"
#include "uiinput.h"
#include "ui/menu.h"
#include "rendfont.h"


namespace ui {

//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define SEPARATOR_HEIGHT		0.25


//**************************************************************************
//  CLASSES
//**************************************************************************

class tabbed_text_iterator
{
public:
	tabbed_text_iterator(const char *text);
	tabbed_text_iterator(const std::string &text);

	bool next();
	int index() const { return m_index; }
	const char *current() { return m_current; }

private:
	std::string		m_buffer;
	const char *	m_text;
	const char *	m_current;
	int				m_position;
	int				m_index;
};


//**************************************************************************
//  MENUBAR IMPLEMENTATION
//**************************************************************************

//-------------------------------------------------
//  ctor
//-------------------------------------------------

menubar::menubar(mame_ui_manager &mui)
	: m_ui(mui), m_menus(*this)
{
	m_container = nullptr;
	m_shortcuted_menu_items = nullptr;
	m_selected_item = nullptr;
	m_active_item = nullptr;
	m_dragged = false;
	m_checkmark_width = -1;
	m_mouse_x = -1;
	m_mouse_y = -1;
	m_mouse_button = false;
	m_last_mouse_move = 0;
	m_first_time = true;
	m_has_been_invoked = false;
}


//-------------------------------------------------
//  dtor
//-------------------------------------------------

menubar::~menubar()
{
}


//-------------------------------------------------
//	reset
//-------------------------------------------------

void menubar::reset()
{
}


//-------------------------------------------------
//  shortcut_key_pressed
//-------------------------------------------------

bool menubar::shortcut_key_pressed(int key)
{
	return (key != IPT_INVALID) && machine().ui_input().pressed_repeat(key, 0);
}


//-------------------------------------------------
//  handle
//-------------------------------------------------

void menubar::handle(render_container &current_container)
{
	m_container = &current_container;

	// do we need to initialize the menus?
	if (m_menus.is_empty())
		menubar_build_menus();

	// measure standard string widths (if necessary)
	if (m_checkmark_width <= 0)
	{
		m_checkmark.assign("_> ");
		convert_command_glyph(m_checkmark);
		m_checkmark_width = ui().get_string_width(m_checkmark.c_str());
	}

	// reset screen locations of all menu items
	m_menus.clear_area_recursive();

	// calculate visibility of the menubar
	m_menubar_visibility = get_menubar_visibility();

	// draw conventional UI elements (e.g. - frameskip)
	menubar_draw_ui_elements();

	float text_height = ui().get_line_height();
	float spacing = text_height / 10;
	float x = spacing;
	float y = spacing;

	for(menu_item *mi = m_menus.child(); mi != nullptr; mi = mi->next())
	{
		float width = ui().get_string_width(mi->text().c_str());

		ui().draw_outlined_box(
			container(),
			x,
			y,
			x + width + (spacing * 2),
			y + text_height + (spacing * 2),
			adjust_color(ui().colors().border_color()),
			adjust_color(ui().colors().background_color()));

		draw_menu_item_text(
			mi,
			x + spacing,
			y + spacing,
			x + spacing + width,
			y + spacing + text_height,
			false);

		// child menu open?
		if (is_child_menu_visible(mi))
			draw_child_menu(mi, x, y + text_height + (spacing * 3));

		x += width + spacing * 4;
	}

	// is the natural keyboard enabled?
	if (machine().ioport().natkeyboard().in_use() && (ui().machine().phase() == machine_phase::RUNNING))
		 ui().process_natural_keyboard();

	// loop while we have interesting events
	while(!event_loop())
	{
	}
}


//-------------------------------------------------
//  event_loop
//-------------------------------------------------

bool menubar::event_loop()
{
	bool done = false;

	ui_event local_menu_event;
	if (machine().ui_input().pop_event(&local_menu_event))
	{
		// find the menu item we're pointing at
		find_mouse(m_mouse_x, m_mouse_y, m_mouse_button);
		menu_item *mi = m_menus.find_point(m_mouse_x, m_mouse_y);

		switch (local_menu_event.event_type)
		{
			case ui_event::type::MOUSE_DOWN:
				if (mi != nullptr)
				{
					m_selected_item = mi;
					m_active_item = mi;
					m_dragged = false;
				}
				break;

			case ui_event::type::MOUSE_MOVE:
				// record the move
				m_last_mouse_move = osd_ticks();

				// moving is only interesting if we have an active menu selection
				if (m_mouse_button && m_active_item != nullptr)
				{
					// are we changing the active menu item?
					if (m_active_item != mi)
					{
						if (mi != nullptr)
							m_active_item = mi->has_children() ? mi->child() : mi;
						m_dragged = true;
						done = true;
					}

					// are we changing the selection?
					if (m_selected_item != mi)
					{
						m_selected_item = mi;
						done = true;
					}
				}
				break;

			case ui_event::type::MOUSE_UP:
				m_active_item = nullptr;
				if (m_selected_item && m_selected_item == mi)
				{
					// looks like we did a mouse up on the current selection; we
					// should invoke or expand the current selection (whichever
					// may be appropriate)
					if (m_selected_item->is_invokable())
						invoke(m_selected_item);
					else if (m_selected_item->has_children())
						m_active_item = m_selected_item->child();
				}
				else if (m_dragged)
				{
					m_selected_item = nullptr;					
				}
				done = true;
				break;

			default:
				break;
		}
	}

	if (!done)
	{
		bool navigation_input_pressed = poll_navigation_keys();
		poll_shortcut_keys(navigation_input_pressed || m_first_time);
		m_first_time = false;
		done = true;
	}
	return done;
}


//-------------------------------------------------
//  poll_navigation_keys
//-------------------------------------------------

bool menubar::poll_navigation_keys()
{
	int code_previous_menu = IPT_INVALID;
	int code_next_menu = IPT_INVALID;
	int code_child_menu1 = IPT_INVALID;
	int code_child_menu2 = IPT_INVALID;
	int code_parent_menu = IPT_INVALID;
	int code_previous_sub_menu = IPT_INVALID;
	int code_next_sub_menu = IPT_INVALID;
	int code_selected = (m_selected_item && m_selected_item->is_invokable())
		? IPT_UI_SELECT
		: IPT_INVALID;

	// are we navigating the menu?
	if (m_selected_item != nullptr)
	{
		// if so, are we in a pull down menu?
		if (!m_selected_item->is_sub_menu())
		{
			// no pull down menu selected
			code_previous_menu = IPT_UI_LEFT;
			code_next_menu = IPT_UI_RIGHT;
			code_child_menu1 = IPT_UI_SELECT;
			code_child_menu2 = IPT_UI_DOWN;
		}
		else
		{
			// pull down menu selected
			code_previous_menu = IPT_UI_UP;
			code_next_menu = IPT_UI_DOWN;
			if (m_selected_item->child())
			{
				code_child_menu1 = IPT_UI_SELECT;
				code_child_menu2 = IPT_UI_RIGHT;
			}
			code_previous_sub_menu = IPT_UI_LEFT;
			code_next_sub_menu = IPT_UI_RIGHT;
			if (m_selected_item->parent()->is_sub_menu())
				code_parent_menu = IPT_UI_LEFT;
		}
	}

	bool result = true;
	if (shortcut_key_pressed(code_previous_menu))
		result = walk_selection_previous();
	else if (shortcut_key_pressed(code_next_menu))
		result = walk_selection_next();
	else if (shortcut_key_pressed(code_child_menu1) || shortcut_key_pressed(code_child_menu2))
		result = walk_selection_child();
	else if (shortcut_key_pressed(IPT_UI_CANCEL))
		result = walk_selection_escape();
	else if (shortcut_key_pressed(code_parent_menu))
		result = walk_selection_parent();
	else if (shortcut_key_pressed(code_previous_sub_menu))
		result = walk_selection_previous_sub_menu();
	else if (shortcut_key_pressed(code_next_sub_menu))
		result = walk_selection_next_sub_menu();
	else if (shortcut_key_pressed(IPT_UI_CONFIGURE))
		toggle_selection();
	else if (shortcut_key_pressed(code_selected))
		invoke(m_selected_item);
	else
		result = false;	// didn't do anything

	// if we changed something, set the active item accordingly
	if (result)
		m_active_item = m_selected_item;

	return result;
}


//-------------------------------------------------
//  poll_shortcut_keys
//-------------------------------------------------

bool menubar::poll_shortcut_keys(bool swallow)
{
	// loop through all shortcut items
	for (menu_item *item = m_shortcuted_menu_items; item != nullptr; item = item->next_with_shortcut())
	{
		assert(item->is_invokable());
		
		// did we press this shortcut?
		if (shortcut_key_pressed(item->shortcut()) && !swallow)
		{
			// this shortcut was pressed and we're not swallowing them; invoke it
			invoke(item);
			return true;
		}
	}
	return false;
}


//-------------------------------------------------
//  toggle_selection
//-------------------------------------------------

void menubar::toggle_selection()
{
	m_selected_item = m_selected_item != nullptr
		? nullptr
		: m_menus.child();
}


//-------------------------------------------------
//  invoke
//-------------------------------------------------

void menubar::invoke(menu_item *menu)
{
	// first, we're ending the menu; pop us off first
	m_has_been_invoked = true;

	// and invoke the selection
	menu->invoke();
}


//-------------------------------------------------
//  walk_selection_previous
//-------------------------------------------------

bool menubar::walk_selection_previous()
{
	if (m_selected_item)
	{
		do
		{
			m_selected_item = m_selected_item->previous()
				? m_selected_item->previous()
				: m_selected_item->parent()->last_child();
		}
		while(!m_selected_item->is_enabled());
	}
	else
	{
		m_selected_item = m_menus.last_child();
	}
	return true;
}


//-------------------------------------------------
//  walk_selection_next
//-------------------------------------------------

bool menubar::walk_selection_next()
{
	if (m_selected_item)
	{
		do
		{
			m_selected_item = m_selected_item->next()
				? m_selected_item->next()
				: m_selected_item->parent()->child();
		}
		while(!m_selected_item->is_enabled());
	}
	else
	{
		m_selected_item = m_menus.child();
	}
	return true;
}


//-------------------------------------------------
//  walk_selection_child
//-------------------------------------------------

bool menubar::walk_selection_child()
{
	bool result = false;
	if (m_selected_item && m_selected_item->child())
	{
		m_selected_item = m_selected_item->child();
		result = true;
	}
	return result;
}


//-------------------------------------------------
//  walk_selection_parent
//-------------------------------------------------

bool menubar::walk_selection_parent()
{
	bool result = false;
	if (m_selected_item && m_selected_item->parent() && m_selected_item->parent() != &m_menus)
	{
		m_selected_item = m_selected_item->parent();
		result = true;
	}
	return result;
}


//-------------------------------------------------
//  walk_selection_escape
//-------------------------------------------------

bool menubar::walk_selection_escape()
{
	bool result = walk_selection_parent();

	if (!result && m_selected_item != nullptr)
	{
		m_selected_item = nullptr;
		result = true;
	}

	return result;
}


//-------------------------------------------------
//  walk_selection_previous_sub_menu
//-------------------------------------------------

bool menubar::walk_selection_previous_sub_menu()
{
	while(walk_selection_parent())
		;
	bool result = walk_selection_previous();
	if (result)
		walk_selection_child();
	return result;
}


//-------------------------------------------------
//  walk_selection_next_sub_menu
//-------------------------------------------------

bool menubar::walk_selection_next_sub_menu()
{
	while(walk_selection_parent())
		;
	bool result = walk_selection_next();
	if (result)
		walk_selection_child();
	return result;
}


//-------------------------------------------------
//  draw_child_menu
//-------------------------------------------------

void menubar::draw_child_menu(menu_item *menu, float x, float y)
{
	float text_height = ui().get_line_height();
	float separator_height = text_height * SEPARATOR_HEIGHT;
	float spacing = text_height / 10;

	// calculate the maximum width and menu item count
	float max_widths[4] = {0, };
	float total_height = (spacing * 2);
	float max_shortcuts_width = 0;
	for(menu_item *mi = menu->child(); mi != nullptr; mi = mi->next())
	{
		// aggregate the maximum width for each column
		tabbed_text_iterator iter(mi->text());
		while(iter.next())
		{
			float width = ui().get_string_width(iter.current());
			assert(iter.index() < ARRAY_LENGTH(max_widths));
			max_widths[iter.index()] = std::max(max_widths[iter.index()], width);
		}

		// measure the shortcut
		float shortcut_width = mi->shortcut_text_width();
		if (shortcut_width > 0)
			max_shortcuts_width = std::max(max_shortcuts_width, shortcut_width + spacing);

		// increase the height
		total_height += (mi->is_separator() ? separator_height : text_height);
	}

	// get the aggregate maximum widths across all columns
	float max_width = m_checkmark_width * 2 + max_shortcuts_width;
	for (int i = 0; i < ARRAY_LENGTH(max_widths); i++)
		max_width += max_widths[i];

	// are we going to go over the right of the screen?
	float right = x + max_width + (spacing * 3);
	if (right > 1.0f)
		x = std::max(0.0f, x - (right - 1.0f));

	// draw the menu outline
	ui().draw_outlined_box(
		container(),
		x,
		y,
		x + max_width + (spacing * 2),
		y + total_height,
		adjust_color(ui().colors().background_color()));

	// draw the individual items
	float my = y;
	for(menu_item *mi = menu->child(); mi != nullptr; mi = mi->next())
	{
		if (mi->is_separator())
		{
			// draw separator
			container().add_line(
				x,
				my + spacing + separator_height / 2,
				x + max_width + (spacing * 2),
				my + spacing + separator_height / 2,
				separator_height / 8,
				adjust_color(ui().colors().border_color()),
				0);
		}
		else
		{
			// draw normal text
			draw_menu_item_text(
				mi,
				x + spacing,
				my + spacing,
				x + spacing + max_width,
				my + spacing + text_height,
				true,
				max_widths);
		}

		// move down...
		my += (mi->is_separator() ? separator_height : text_height);
	}

	// draw child menus
	my = y;
	for(menu_item *mi = menu->child(); mi != nullptr; mi = mi->next())
	{
		// child menu open?
		if (!mi->is_separator() && is_child_menu_visible(mi))
		{
			draw_child_menu(
				mi,
				x + max_width + (spacing * 2),
				my);
		}

		// move down...
		my += (mi->is_separator() ? separator_height : text_height);
	}
}


//-------------------------------------------------
//  is_child_menu_visible
//-------------------------------------------------

bool menubar::is_child_menu_visible(menu_item *menu) const
{
	menu_item *current_menu = m_active_item ? m_active_item : m_selected_item;
	return current_menu && current_menu->is_child_of(menu);
}


//-------------------------------------------------
//  draw_menu_item_text
//-------------------------------------------------

void menubar::draw_menu_item_text(menu_item *mi, float x0, float y0, float x1, float y1, bool decorations, const float *column_widths)
{
	// set our area
	mi->set_area(x0, y0, x1, y1);

	// choose the color
	rgb_t fgcolor, bgcolor;
	if (!mi->is_enabled())
	{
		// disabled
		fgcolor = ui().colors().unavailable_color();
		bgcolor = ui().colors().text_bg_color();
	}
	else if (mi == m_selected_item)
	{
		// selected
		fgcolor = ui().colors().selected_color();
		bgcolor = ui().colors().selected_bg_color();
	}
	else if ((m_mouse_x >= x0) && (m_mouse_x < x1) && (m_mouse_y >= y0) && (m_mouse_y < y1))
	{
		// hover
		fgcolor = ui().colors().mouseover_color();
		bgcolor = ui().colors().mouseover_bg_color();
	}
	else
	{
		// normal
		fgcolor = ui().colors().text_color();
		bgcolor = ui().colors().text_bg_color();
	}

	// highlight?
	if (bgcolor != ui().colors().text_bg_color())
		highlight(x0, y0, x1, y1, adjust_color(bgcolor));

	// do we have to draw additional decorations?
	if (decorations)
	{
		// account for the checkbox
		if (mi->is_checked())
			ui().draw_text_full(container(), m_checkmark.c_str(), x0, y0, 1.0f - x0, ui::text_layout::LEFT, ui::text_layout::WORD, mame_ui_manager::NORMAL, adjust_color(fgcolor), adjust_color(bgcolor));
		x0 += m_checkmark_width;

		// expanders?
		if (mi->child())
		{
			float lr_arrow_width = 0.4f * (y1 - y0) * machine().render().ui_aspect();
			draw_arrow(
				x1 - lr_arrow_width,
				y0 + (0.1f * (y1 - y0)),
				x1,
				y0 + (0.9f * (y1 - y0)),
				adjust_color(fgcolor),
				ROT90);
		}

		// shortcut?
		ui().draw_text_full(
			container(),
			mi->shortcut_text(),
			x0,
			y0,
			x1 - x0,
			ui::text_layout::RIGHT,
			ui::text_layout::WORD,
			mame_ui_manager::NORMAL,
			adjust_color(fgcolor),
			adjust_color(bgcolor));
	}

	tabbed_text_iterator iter(mi->text());
	while(iter.next())
	{
		ui().draw_text_full(container(), iter.current(), x0, y0, 1.0f - x0, ui::text_layout::LEFT, ui::text_layout::WORD, mame_ui_manager::NORMAL, adjust_color(fgcolor), adjust_color(bgcolor));
		if (column_widths != nullptr)
			x0 += column_widths[iter.index()];
	}
}


//-------------------------------------------------
//  find_mouse
//-------------------------------------------------

bool menubar::find_mouse(float &mouse_x, float &mouse_y, bool &mouse_button)
{
	bool result = false;
	mouse_x = -1;
	mouse_y = -1;

	int32_t mouse_target_x, mouse_target_y;
	render_target *mouse_target = machine().ui_input().find_mouse(&mouse_target_x, &mouse_target_y, &mouse_button);
	if (mouse_target != nullptr)
	{
		if (mouse_target->map_point_container(mouse_target_x, mouse_target_y, container(), mouse_x, mouse_y))
			result = true;
	}

	return result;
}


//-------------------------------------------------
//  get_menubar_visibility
//-------------------------------------------------

menubar::menubar_visibility_t menubar::get_menubar_visibility()
{
	menubar_visibility_t result;

	// is the mouse in the menu bar?
	bool in_menu_bar = (m_mouse_y >= 0) && (m_mouse_y <= ui().get_line_height());

	// did we recently move the mouse?
	bool recently_moved = (m_last_mouse_move != 0)
		&& ((osd_ticks() - m_last_mouse_move) * 5 / osd_ticks_per_second() < 1);

	// make the choice
	if ((m_selected_item != nullptr) || (m_active_item != nullptr))
		result = MENUBAR_VISIBILITY_VISIBLE;
	else if (in_menu_bar || recently_moved)
		result = MENUBAR_VISIBILITY_TRANSLUCENT;
	else
		result = MENUBAR_VISIBILITY_INVISIBLE;

	return result;
}


//-------------------------------------------------
//  adjust_color
//-------------------------------------------------

rgb_t menubar::adjust_color(rgb_t color)
{
	switch(m_menubar_visibility)
	{
		case MENUBAR_VISIBILITY_INVISIBLE:
			color = rgb_t::transparent();
			break;

		case MENUBAR_VISIBILITY_TRANSLUCENT:
			color = rgb_t(
				color.a() / 4,
				color.r(),
				color.g(),
				color.b());
			break;
		
		case MENUBAR_VISIBILITY_VISIBLE:
		default:
			// do nothing
			break;
	}
	return color;
}


//-------------------------------------------------
//  highlight
//-------------------------------------------------

void menubar::highlight(float x0, float y0, float x1, float y1, rgb_t bgcolor)
{
	// hack to get the widgets
	widgets_manager &widgets(ui::menu::get_widgets_manager(machine()));

	// do the highlight
	container().add_quad(x0, y0, x1, y1, bgcolor, widgets.hilight_texture(), PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXWRAP(true) | PRIMFLAG_PACKABLE);
}


//-------------------------------------------------
//  draw_arrow
//-------------------------------------------------

void menubar::draw_arrow(float x0, float y0, float x1, float y1, rgb_t fgcolor, uint32_t orientation)
{
	// hack to get the widgets
	widgets_manager &widgets(ui::menu::get_widgets_manager(machine()));

	// do the draw_arrow
	container().add_quad(x0, y0, x1, y1, fgcolor, widgets.arrow_texture(), PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXORIENT(orientation) | PRIMFLAG_PACKABLE);
}


//**************************************************************************
//  MENU ITEMS
//**************************************************************************

//-------------------------------------------------
//  menu_item::ctor
//-------------------------------------------------

menubar::menu_item::menu_item(menubar &menubar, std::string &&text, menubar::menu_item *parent, std::function<void()> &&func, int shortcut)
	: m_menubar(menubar)
	, m_text(std::move(text))
	, m_func(std::move(func))
	, m_shortcut(shortcut)
	, m_shortcut_text_width(-1)
	, m_is_checked(false)
	, m_is_enabled(true)
	, m_is_separator(false)
	, m_parent(parent)
	, m_first_child(nullptr)
	, m_last_child(nullptr)
	, m_previous(nullptr)
	, m_next(nullptr)
{
	// should be the same check in uiinput.cpp
	assert(shortcut == IPT_INVALID || (shortcut >= IPT_UI_CONFIGURE && shortcut <= IPT_OSD_16));
	clear_area();
}


//-------------------------------------------------
//  menu_item::dtor
//-------------------------------------------------

menubar::menu_item::~menu_item()
{
	menu_item *mi = m_first_child;
	while (mi)
	{
		menu_item *next = mi->m_next;
		delete mi;
		mi = next;
	}
}


//-------------------------------------------------
//  menu_item::set_area
//-------------------------------------------------

void menubar::menu_item::set_area(float x0, float y0, float x1, float y1)
{
	m_x0 = x0;
	m_y0 = y0;
	m_x1 = x1;
	m_y1 = y1;
}


//-------------------------------------------------
//  menu_item::clear_area_recursive
//-------------------------------------------------

void menubar::menu_item::clear_area_recursive()
{
	clear_area();
	if (m_first_child)
		m_first_child->clear_area_recursive();
	if (m_next)
		m_next->clear_area_recursive();
}


//-------------------------------------------------
//  menu_item::initialize
//-------------------------------------------------

void menubar::menu_item::initialize(menubar::menu_item &child)
{
	// link this back to the previous item
	child.m_previous = m_last_child;

	// link the end of the chain to this new item
	if (m_last_child)
		m_last_child->m_next = &child;
	else
		m_first_child = &child;

	// this new child is now last in the chain
	m_last_child = &child;

	// link this up into the shortcut list, if appropriate
	if (child.shortcut() != 0)
	{
		child.set_next_with_shortcut(m_menubar.m_shortcuted_menu_items);
		m_menubar.m_shortcuted_menu_items = &child;
	}
}


//-------------------------------------------------
//  menu_item::append
//-------------------------------------------------

menubar::menu_item &menubar::menu_item::append(std::string &&text, std::function<void()> &&func, int shortcut)
{
	menu_item *child = new menu_item(m_menubar, std::move(text), this, std::move(func), shortcut);
	initialize(*child);
	return *child;
}


//-------------------------------------------------
//  menu_item::append_separator
//-------------------------------------------------

void menubar::menu_item::append_separator()
{
	// only append a separator if the last item is not a separator
	if (m_last_child != nullptr && !m_last_child->m_is_separator)
	{
		menu_item &separator = append("-");
		separator.set_enabled(false);
		separator.m_is_separator = true;
	}
}


//-------------------------------------------------
//  menu_item::find_point
//-------------------------------------------------

menubar::menu_item *menubar::menu_item::find_point(float x, float y)
{
	menu_item *result = nullptr;

	if (m_is_enabled && (x >= m_x0) && (y >= m_y0) && (x <= m_x1) && (y <= m_y1))
		result = this;

	if (!result && m_first_child)
		result = m_first_child->find_point(x, y);
	if (!result && m_next)
		result = m_next->find_point(x, y);
	return result;
}


//-------------------------------------------------
//  menu_item::find_child
//-------------------------------------------------

menubar::menu_item &menubar::menu_item::find_child(const char *target)
{
	menu_item *item = find_child_internal(target);
	assert(item != nullptr);
	return *item;
}


//-------------------------------------------------
//  menu_item::find_child_internal
//-------------------------------------------------

menubar::menu_item *menubar::menu_item::find_child_internal(const char *target)
{
	if (!strcmp(target, text().c_str()))
		return this;

	for(menu_item *item = child(); item != nullptr; item = item->next())
	{
		menu_item *result = item->find_child_internal(target);
		if (result != nullptr)
			return result;
	}
	return nullptr;
}


//-------------------------------------------------
//  menu_item::shortcut_text
//-------------------------------------------------

const char *menubar::menu_item::shortcut_text()
{	
	// do we have to calculate this stuff?
	if (m_shortcut_text_width < 0)
	{
		// clear the text
		m_shortcut_text.assign("");

		// first, do we even have a shortcut?
		if (shortcut() != 0)
		{
			// iterate over the input ports and add menu items
			for (input_type_entry &entry : m_menubar.machine().ioport().types())
			{
				// add if we match the group and we have a valid name
				if (entry.group() == IPG_UI && entry.name() != nullptr && entry.name()[0] != 0 && entry.type() == shortcut())
				{
					const input_seq &seq = m_menubar.machine().ioport().type_seq(entry.type(), entry.player(), SEQ_TYPE_STANDARD);
					sensible_seq_name(m_shortcut_text, seq);
					break;
				}
			}
		}

		// finally calculate the text width
		m_shortcut_text_width = m_menubar.ui().get_string_width(m_shortcut_text.c_str());
	}

	// return the text
	return m_shortcut_text.c_str();
}


//-------------------------------------------------
//  menu_item::shortcut_text_width
//-------------------------------------------------

float menubar::menu_item::shortcut_text_width()
{
	// force a calculation, if we didn't have one
	shortcut_text();

	// and return the text width
	return m_shortcut_text_width;
}


//-------------------------------------------------
//  menu_item::sensible_seq_name
//-------------------------------------------------

void menubar::menu_item::sensible_seq_name(std::string &text, const input_seq &seq)
{
	// special case; we don't want 'None'
	if (seq[0] == input_seq::end_code)
		m_shortcut_text.assign("");
	else
		m_shortcut_text = m_menubar.machine().input().seq_name(seq);
}


//-------------------------------------------------
//  menu_item::is_child_of
//-------------------------------------------------

bool menubar::menu_item::is_child_of(menubar::menu_item *that) const
{
	for(menu_item *mi = m_parent; mi != nullptr; mi = mi->m_parent)
	{
		if (mi == that)
			return true;
	}
	return false;
}


//**************************************************************************
//  TABBED TEXT ITERATOR
//**************************************************************************

//-------------------------------------------------
//  tabbed_text_iterator::ctor
//-------------------------------------------------

tabbed_text_iterator::tabbed_text_iterator(const char *text)
{
	m_text = text;
	m_current = nullptr;
	m_position = 0;
	m_index = -1;
}


tabbed_text_iterator::tabbed_text_iterator(const std::string &text)
	: tabbed_text_iterator(text.c_str())
{
}


//-------------------------------------------------
//  tabbed_text_iterator::next
//-------------------------------------------------

bool tabbed_text_iterator::next()
{
	const char *current_text = &m_text[m_position];
	const char *tabpos = strchr(current_text, '\t');

	if (tabpos != nullptr)
	{
		int count = tabpos - current_text;
		m_buffer.assign(current_text, count);
		m_position += count + 1;
		m_current = m_buffer.c_str();
		m_index++;
	}
	else if (*current_text != '\0')
	{
		m_current = current_text;
		m_position += strlen(m_current);
		m_index++;
	}
	else
	{
		m_current = nullptr;
	}
	return m_current != nullptr;
}

} // namespace ui
