//============================================================
//
//  fileview.h - A Win32 file viewer dialog
//
//============================================================
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>

#include "fileview.h"
#include "wimgres.h"
#include "hexview.h"
#include "winutf8.h"
#include "winutils.h"


namespace
{
	class file_viewer
	{
	public:
		file_viewer(std::string &&filename, const void *ptr, size_t size)
			: m_filename(std::move(filename))
			, m_ptr(ptr)
			, m_size(size)
			, m_font(nullptr)
		{
		}

		static INT_PTR CALLBACK dialog_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam)
		{
			INT_PTR rc = 0;
			switch (message)
			{
			case WM_INITDIALOG:
				((file_viewer *)lparam)->handle_init_dialog(dialog);
				break;

			case WM_DESTROY:
				get_file_viewer(dialog).handle_destroy();
				break;

			case WM_COMMAND:
				get_file_viewer(dialog).handle_command(dialog, HIWORD(wparam), LOWORD(wparam));
				break;
			}
			return rc;
		}

	private:
		void handle_init_dialog(HWND dialog)
		{
			m_font = win_create_monospace_font();
			SetWindowLongPtr(dialog, GWLP_USERDATA, (LONG_PTR) this);

			// set the hex view
			HWND hexview = GetDlgItem(dialog, IDC_HEXVIEW);
			SendMessage(hexview, WM_SETFONT, (WPARAM)m_font, (LPARAM)TRUE);
			hexview_setdata(hexview, m_ptr, m_size);
		}

		void handle_destroy()
		{
			if (m_font)
			{
				DeleteObject(m_font);
				m_font = nullptr;
			}
		}

		void handle_command(HWND dialog, WORD notification, WORD id)
		{
			switch (id)
			{
			case IDOK:
			case IDCANCEL:
				EndDialog(dialog, 0);
				break;
			}
		}

		static file_viewer &get_file_viewer(HWND dialog)
		{
			return *((file_viewer *)GetWindowLongPtr(dialog, GWLP_USERDATA));
		}

		std::string		m_filename;
		const void *	m_ptr;
		size_t			m_size;
		HFONT			m_font;
	};
}

void win_fileview_dialog(HWND parent, std::string &&filename, const void *ptr, size_t size)
{
	file_viewer fv(std::move(filename), ptr, size);
	DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_FILEVIEW), parent,
		file_viewer::dialog_proc, (LPARAM)&fv);
}
