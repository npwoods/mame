//============================================================
//
//  secview.cpp - A Win32 sector editor dialog
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

#include "secview.h"
#include "wimgres.h"
#include "hexview.h"
#include "winutf8.h"
#include "winutils.h"
#include "anchor.h"

struct sectorview_info
{
	imgtool::image *image;
	HFONT font;
	UINT32 track, head, sector;
	LONG old_width;
	LONG old_height;
	anchor_manager anchor;

	sectorview_info(imgtool::image *i)
		: image(i)
		, font(nullptr)
		, track(0)
		, head(0)
		, sector(0)
		, old_width(0)
		, old_height(0)
		, anchor({
			{ IDC_HEXVIEW,		anchor_manager::anchor::LEFT | anchor_manager::anchor::TOP | anchor_manager::anchor::RIGHT | anchor_manager::anchor::BOTTOM },
			{ IDOK,				anchor_manager::anchor::RIGHT | anchor_manager::anchor::BOTTOM },
			{ IDC_TRACKEDIT,	anchor_manager::anchor::LEFT | anchor_manager::anchor::BOTTOM },
			{ IDC_TRACKLABEL,	anchor_manager::anchor::LEFT | anchor_manager::anchor::BOTTOM },
			{ IDC_TRACKSPIN,	anchor_manager::anchor::LEFT | anchor_manager::anchor::BOTTOM },
			{ IDC_HEADEDIT,		anchor_manager::anchor::LEFT | anchor_manager::anchor::BOTTOM },
			{ IDC_HEADLABEL,	anchor_manager::anchor::LEFT | anchor_manager::anchor::BOTTOM },
			{ IDC_HEADSPIN,		anchor_manager::anchor::LEFT | anchor_manager::anchor::BOTTOM },
			{ IDC_SECTOREDIT,	anchor_manager::anchor::LEFT | anchor_manager::anchor::BOTTOM },
			{ IDC_SECTORLABEL,	anchor_manager::anchor::LEFT | anchor_manager::anchor::BOTTOM },
			{ IDC_SECTORSPIN,	anchor_manager::anchor::LEFT | anchor_manager::anchor::BOTTOM } } )
	{
	}
};



static struct sectorview_info *get_sectorview_info(HWND dialog)
{
	LONG_PTR l;
	l = GetWindowLongPtr(dialog, GWLP_USERDATA);
	return (struct sectorview_info *) l;
}


// reads sector data from the disk image into the view
static imgtoolerr_t read_sector_data(HWND dialog, UINT32 track, UINT32 head, UINT32 sector)
{
	imgtoolerr_t err;
	struct sectorview_info *info = get_sectorview_info(dialog);

	std::vector<UINT8> data;
	err = info->image->read_sector(track, head, sector, data);
	if (err)
		goto done;

	if (!hexview_setdata(GetDlgItem(dialog, IDC_HEXVIEW), data.data(), data.size()))
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	info->track = track;
	info->head = head;
	info->sector = sector;

done:
	return err;
}



// sets the sector text
static void set_sector_text(HWND dialog)
{
	struct sectorview_info *info;
	CHAR buf[32];

	info = get_sectorview_info(dialog);

	if (info->track != ~0)
		snprintf(buf, ARRAY_LENGTH(buf), "%u", (unsigned int) info->track);
	else
		buf[0] = '\0';
	win_set_window_text_utf8(GetDlgItem(dialog, IDC_TRACKEDIT), buf);

	if (info->head != ~0)
		snprintf(buf, ARRAY_LENGTH(buf), "%u", (unsigned int) info->head);
	else
		buf[0] = '\0';
	win_set_window_text_utf8(GetDlgItem(dialog, IDC_HEADEDIT), buf);

	if (info->sector != ~0)
		snprintf(buf, ARRAY_LENGTH(buf), "%u", (unsigned int) info->sector);
	else
		buf[0] = '\0';
	win_set_window_text_utf8(GetDlgItem(dialog, IDC_SECTOREDIT), buf);
}



static void change_sector(HWND dialog)
{
	struct sectorview_info *info;
	//imgtoolerr_t err;
	TCHAR buf[32];
	UINT32 new_track, new_head, new_sector;

	info = get_sectorview_info(dialog);

	GetWindowText(GetDlgItem(dialog, IDC_TRACKEDIT), buf, ARRAY_LENGTH(buf));
	new_track = (UINT32) _ttoi(buf);
	GetWindowText(GetDlgItem(dialog, IDC_HEADEDIT), buf, ARRAY_LENGTH(buf));
	new_head = (UINT32) _ttoi(buf);
	GetWindowText(GetDlgItem(dialog, IDC_SECTOREDIT), buf, ARRAY_LENGTH(buf));
	new_sector = (UINT32) _ttoi(buf);

	if ((info->track != new_track) || (info->head != new_head) || (info->sector != new_sector))
	{
		//err =
		read_sector_data(dialog, new_track, new_head, new_sector);
		// TODO: this causes a stack overflow
		//if (err)
		//  set_sector_text(dialog);
	}
}



static void setup_spin_control(HWND dialog, int spin_item, int edit_item, int range_start, int range_end)
{
	HWND spin_control, edit_control;
	spin_control = GetDlgItem(dialog, spin_item);
	edit_control = GetDlgItem(dialog, edit_item);
	SendMessage(spin_control, UDM_SETBUDDY, (WPARAM) edit_control, 0);
	SendMessage(spin_control, UDM_SETRANGE, range_start, MAKELONG(range_end, 0));
}



static INT_PTR CALLBACK win_sectorview_dialog_proc(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	imgtoolerr_t err;
	INT_PTR rc = 0;
	struct sectorview_info *info;
	RECT dialog_rect;

	switch(message)
	{
		case WM_INITDIALOG:
		{
			int tracks = 32768;
			int heads = 32768;
			int sectors = 32768;

			// TODO: get actual tracks, heads, sectors from image

			info = (struct sectorview_info *) lparam;
			info->font = win_create_monospace_font();

			GetWindowRect(dialog, &dialog_rect);
			info->old_width = dialog_rect.right - dialog_rect.left;
			info->old_height = dialog_rect.bottom - dialog_rect.top;
			info->track = ~0;
			info->head = ~0;
			info->sector = ~0;

			SendMessage(GetDlgItem(dialog, IDC_HEXVIEW), WM_SETFONT, (WPARAM) info->font, (LPARAM) TRUE);
			SetWindowLongPtr(dialog, GWLP_USERDATA, lparam);

			setup_spin_control(dialog, IDC_TRACKSPIN, IDC_TRACKEDIT, 0, tracks-1);
			setup_spin_control(dialog, IDC_HEADSPIN, IDC_HEADEDIT, 0, heads-1);
			setup_spin_control(dialog, IDC_SECTORSPIN, IDC_SECTOREDIT, 0, sectors-1);

			info->anchor.setup(dialog);

			// TODO: get first sector id instead of try and error
			err = read_sector_data(dialog, 0, 0, 0);
			if (err == IMGTOOLERR_SEEKERROR)
				err = read_sector_data(dialog, 0, 0, 1);
			if (!err)
				set_sector_text(dialog);
			break;
		}

		case WM_DESTROY:
			info = get_sectorview_info(dialog);
			if (info->font)
			{
				DeleteObject(info->font);
				info->font = nullptr;
			}
			break;

		case WM_SYSCOMMAND:
			if (wparam == SC_CLOSE)
				EndDialog(dialog, 0);
			break;

		case WM_COMMAND:
			switch(HIWORD(wparam))
			{
				case EN_CHANGE:
					change_sector(dialog);
					break;

				default:
					switch(LOWORD(wparam))
					{
						case IDOK:
						case IDCANCEL:
							EndDialog(dialog, 0);
							break;
					}
					break;
			}
			break;

		case WM_SIZE:
			info = get_sectorview_info(dialog);
			info->anchor.resize();
			break;

		case WM_MOUSEWHEEL:
			if (HIWORD(wparam) & 0x8000)
				SendMessage(GetDlgItem(dialog, IDC_HEXVIEW), WM_VSCROLL, SB_LINEDOWN, 0);
			else
				SendMessage(GetDlgItem(dialog, IDC_HEXVIEW), WM_VSCROLL, SB_LINEUP, 0);
			break;
	}

	return rc;
}



void win_sectorview_dialog(HWND parent, imgtool::image *image)
{
	struct sectorview_info info(image);
	DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_SECTORVIEW), parent,
		win_sectorview_dialog_proc, (LPARAM) &info);
}
