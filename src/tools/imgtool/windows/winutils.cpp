//============================================================
//
//  winutils.cpp - Generic Win32 utility code
//
//============================================================

// Needed for better file dialog
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif // _WIN32_WINNT
#define _WIN32_WINNT 0x500

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <tchar.h>
#include <stdlib.h>
#include <algorithm>

// MAME headers
#include "winutils.h"
#include "strconv.h"

// stupid hack; not sure why this is needed
#ifdef const
#undef const
#endif // const



//============================================================
//  win_get_file_name_dialog - sanitize all of the ugliness
//  in invoking GetOpenFileName() and GetSaveFileName()
//============================================================

BOOL win_get_file_name_dialog(win_open_file_name *ofn)
{
	BOOL dialog_result;
	OSVERSIONINFO vers;
	OPENFILENAME os_ofn;
	DWORD os_ofn_size;
	osd::text::tstring t_filter;
	LPTSTR t_file = nullptr;
	std::unique_ptr<osd::text::tstring> t_initial_directory;
	DWORD t_file_size = 0;

	// determine the version of Windows
	memset(&vers, 0, sizeof(vers));
	vers.dwOSVersionInfoSize = sizeof(vers);
	GetVersionEx(&vers);

	// based on the version of Windows, determine the correct struct size
	if (vers.dwMajorVersion >= 5)
	{
		os_ofn_size = sizeof(os_ofn);
	}
	else
	{
#ifdef __GNUC__
		os_ofn_size = 76;	// MinGW does not define OPENFILENAME_NT4
#else
		os_ofn_size = sizeof(OPENFILENAME_NT4);
#endif
	}

	// do we have to translate the filter?
	if (!ofn->filter.empty())
	{
		t_filter = osd::text::to_tstring(ofn->filter);

		// convert a pipe-char delimited string into a NUL delimited string
		for (auto iter = t_filter.begin(); iter != t_filter.end(); iter++)
			*iter = (*iter != '|') ? *iter : '\0';
		
		// we need another NUL
		t_filter += (TCHAR)'\0';
	}

	// do we need to translate the file parameter?
	{
		auto buffer = osd::text::to_tstring(ofn->filename);
		t_file_size = std::max(buffer.length() + 1, (size_t)MAX_PATH);
		t_file = (LPTSTR)alloca(t_file_size * sizeof(*t_file));
		_tcscpy(t_file, buffer.c_str());
	}

	// do we need to translate the initial directory?
	if (ofn->initial_directory != nullptr)
		t_initial_directory = std::make_unique<osd::text::tstring>(osd::text::to_tstring(ofn->initial_directory));

	// translate our custom structure to a native Win32 structure
	memset(&os_ofn, 0, sizeof(os_ofn));
	os_ofn.lStructSize = os_ofn_size;
	os_ofn.hwndOwner = ofn->owner;
	os_ofn.hInstance = ofn->instance;
	os_ofn.lpstrFilter = !t_filter.empty() ? t_filter.c_str() : nullptr;
	os_ofn.nFilterIndex = ofn->filter_index;
	os_ofn.lpstrFile = t_file;
	os_ofn.nMaxFile = t_file_size;
	os_ofn.lpstrInitialDir = t_initial_directory.get() != nullptr ? t_initial_directory->c_str() : nullptr;
	os_ofn.Flags = ofn->flags;
	os_ofn.lCustData = ofn->custom_data;
	os_ofn.lpfnHook = ofn->hook;
	os_ofn.lpTemplateName = ofn->template_name;

	// invoke the correct Win32 call
	switch(ofn->type)
	{
		case WIN_FILE_DIALOG_OPEN:
			dialog_result = GetOpenFileName(&os_ofn);
			break;

		case WIN_FILE_DIALOG_SAVE:
			dialog_result = GetSaveFileName(&os_ofn);
			break;

		default:
			// should not reach here
			dialog_result = FALSE;
			break;
	}

	// copy data out
	ofn->filter_index = os_ofn.nFilterIndex;
	ofn->flags = os_ofn.Flags;

	// copy file back out into passed structure
	ofn->filename = t_file != nullptr
		? osd::text::from_tstring(t_file)
		: "";

	// we've completed the process
	return dialog_result;
}



//============================================================
//  win_scroll_window
//============================================================

void win_scroll_window(HWND window, WPARAM wparam, int scroll_bar, int scroll_delta_line)
{
	SCROLLINFO si;
	int scroll_pos;

	// retrieve vital info about the scroll bar
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	GetScrollInfo(window, scroll_bar, &si);

	scroll_pos = si.nPos;

	// change scroll_pos in accordance with this message
	switch(LOWORD(wparam))
	{
		case SB_BOTTOM:
			scroll_pos = si.nMax;
			break;
		case SB_LINEDOWN:
			scroll_pos += scroll_delta_line;
			break;
		case SB_LINEUP:
			scroll_pos -= scroll_delta_line;
			break;
		case SB_PAGEDOWN:
			scroll_pos += scroll_delta_line;
			break;
		case SB_PAGEUP:
			scroll_pos -= scroll_delta_line;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			scroll_pos = HIWORD(wparam);
			break;
		case SB_TOP:
			scroll_pos = si.nMin;
			break;
	}

	// max out the scroll bar value
	if (scroll_pos < si.nMin)
		scroll_pos = si.nMin;
	else if (scroll_pos > (si.nMax - si.nPage))
		scroll_pos = (si.nMax - si.nPage);

	// if the value changed, set the scroll position
	if (scroll_pos != si.nPos)
	{
		SetScrollPos(window, scroll_bar, scroll_pos, TRUE);
		ScrollWindowEx(window, 0, si.nPos - scroll_pos, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
	}
}



//============================================================
//  win_append_menu_utf8
//============================================================

BOOL win_append_menu_utf8(HMENU menu, UINT flags, UINT_PTR id, const char *item)
{
	const TCHAR *t_item = (const TCHAR*)item;
	osd::text::tstring t_str;

	// only convert string when it's not a bitmap
	if (!(flags & MF_BITMAP) && item)
	{
		t_str = osd::text::to_tstring(item);
		t_item = t_str.c_str();
	}

	return AppendMenu(menu, flags, id, t_item);
}


//============================================================
//  win_create_monospace_font
//============================================================

HFONT win_create_monospace_font()
{
	HDC temp_dc;
	HFONT font;

	temp_dc = GetDC(nullptr);

	font = CreateFont(-MulDiv(8, GetDeviceCaps(temp_dc, LOGPIXELSY), 72), 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, TEXT("Lucida Console"));
	if (temp_dc)
		ReleaseDC(nullptr, temp_dc);
	return font;
}



