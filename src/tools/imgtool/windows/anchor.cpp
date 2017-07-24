//============================================================
//
//  anchor.cpp - Component to implement anchoring
//
//============================================================

#include "anchor.h"
#include <initializer_list>


//-------------------------------------------------
//  ctor
//-------------------------------------------------

anchor_manager::anchor_manager(std::vector<entry> &&entries)
	: m_window(nullptr)
	, m_entries(std::move(entries))
	, m_old_width(-1)
	, m_old_height(-1)
{
}


//-------------------------------------------------
//  setup
//-------------------------------------------------

void anchor_manager::setup(HWND window)
{
	m_window = window;
	get_dimensions(m_old_width, m_old_height);
}


//-------------------------------------------------
//  get_dimensions
//-------------------------------------------------

void anchor_manager::get_dimensions(LONG &width, LONG &height)
{
	RECT rect;
	GetWindowRect(m_window, &rect);
	width = rect.right - rect.left;
	height = rect.bottom - rect.top;
}


//-------------------------------------------------
//  resize
//-------------------------------------------------

void anchor_manager::resize()
{
	LONG new_width, new_height;
	get_dimensions(new_width, new_height);
	LONG width_delta = new_width - m_old_width;
	LONG height_delta = new_height - m_old_height;
	m_old_width = new_width;
	m_old_height = new_height;

	RECT dialog_rect, adjusted_dialog_rect;
	LONG dialog_left, dialog_top;
	LONG left, top;
	UINT64 width, height;
	HANDLE width_prop, height_prop;
	static const TCHAR winprop_negwidth[] = TEXT("winprop_negwidth");
	static const TCHAR winprop_negheight[] = TEXT("winprop_negheight");

	// figure out the dialog client top/left coordinates
	GetWindowRect(m_window, &dialog_rect);
	adjusted_dialog_rect = dialog_rect;
	AdjustWindowRectEx(&adjusted_dialog_rect,
		GetWindowLong(m_window, GWL_STYLE),
		GetMenu(m_window) ? TRUE : FALSE,
		GetWindowLong(m_window, GWL_EXSTYLE));
	dialog_left = dialog_rect.left + (dialog_rect.left - adjusted_dialog_rect.left);
	dialog_top = dialog_rect.top + (dialog_rect.top - adjusted_dialog_rect.top);

	for (auto &entry : m_entries)
	{
		HWND dlgitem = GetDlgItem(m_window, entry.control);
		if (dlgitem)
		{
			RECT dlgitem_rect;
			GetWindowRect(dlgitem, &dlgitem_rect);

			left = dlgitem_rect.left - dialog_left;
			top = dlgitem_rect.top - dialog_top;
			width = dlgitem_rect.right - dlgitem_rect.left;
			height = dlgitem_rect.bottom - dlgitem_rect.top;

			width_prop = GetProp(dlgitem, winprop_negwidth);
			if (width_prop)
				width = (intptr_t)width_prop;
			height_prop = GetProp(dlgitem, winprop_negheight);
			if (height_prop)
				height = (intptr_t)height_prop;

			if ((entry.anchor & (anchor::LEFT | anchor::RIGHT)) == anchor::RIGHT)
				left += width_delta;
			else if ((entry.anchor & (anchor::LEFT | anchor::RIGHT)) == (anchor::LEFT | anchor::RIGHT))
				width += width_delta;

			if ((entry.anchor & (anchor::TOP | anchor::BOTTOM)) == anchor::BOTTOM)
				top += height_delta;
			else if ((entry.anchor & (anchor::TOP | anchor::BOTTOM)) == (anchor::TOP | anchor::BOTTOM))
				height += height_delta;

			// Record the width/height properties if they are negative
			if (width < 0)
			{
				SetProp(dlgitem, winprop_negwidth, (HANDLE)width);
				width = 0;
			}
			else if (width_prop)
				RemoveProp(dlgitem, winprop_negwidth);
			if (height < 0)
			{
				SetProp(dlgitem, winprop_negheight, (HANDLE)height);
				height = 0;
			}
			else if (height_prop)
				RemoveProp(dlgitem, winprop_negheight);

			// Actually move the window
			SetWindowPos(dlgitem, 0, left, top, width, height, SWP_NOZORDER);
			InvalidateRect(dlgitem, nullptr, TRUE);
		}
	}
}

