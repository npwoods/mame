#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>


#include "attrdlg.h"
#include "wimgres.h"
#include "opcntrl.h"
#include "winutf8.h"

#define CONTROL_FILTER	10000
#define CONTROL_START	10001

static const TCHAR owner_prop[] = TEXT("wimgtool_owned");

extern const imgtool_module *find_filter_module(int filter_index,
	BOOL creating_file);

static HFONT get_window_font(HWND window)
{
	LRESULT rc;
	rc = SendMessage(window, WM_GETFONT, 0, 0);
	return (HFONT) rc;
}



static int create_option_controls(HWND dialog, HFONT font, int margin, int *y,
	struct transfer_suggestion_info *suggestion_info, util::option_resolution *resolution)
{
	int label_width = 100;
	int control_height = 20;
	int control_count = 0;
	RECT dialog_rect;
	HWND control, aux_control;
	DWORD style;
	int i, x, width, selected;

	GetWindowRect(dialog, &dialog_rect);

	if (suggestion_info)
	{
		// set up label control
		control = CreateWindow(TEXT("STATIC"), TEXT("Mode:"), WS_CHILD | WS_VISIBLE,
			margin, *y + 2, label_width, control_height, dialog, nullptr, nullptr, nullptr);
		SendMessage(control, WM_SETFONT, (WPARAM) font, 0);
		SetProp(control, owner_prop, (HANDLE) 1);

		// set up the active control
		x = margin + label_width;
		width = dialog_rect.right - dialog_rect.left - x - margin;

		style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST;
		control = CreateWindow(TEXT("ComboBox"), nullptr, style,
				x, *y, width, control_height * 8, dialog, nullptr, nullptr, nullptr);

		SetWindowLong(control, GWL_ID, CONTROL_FILTER);
		SendMessage(control, WM_SETFONT, (WPARAM) font, 0);
		SetProp(control, owner_prop, (HANDLE) 1);

		selected = 0;
		for (i = 0; suggestion_info->suggestions[i].viability; i++)
		{
			SendMessage(control, CB_ADDSTRING, 0, (LPARAM) suggestion_info->suggestions[i].description);
			if (suggestion_info->suggestions[i].viability == SUGGESTION_RECOMMENDED)
				selected = i;
		}
		SendMessage(control, CB_SETCURSEL, selected, 0);

		*y += control_height;
	}

	if (resolution != nullptr)
	{
		for (auto iter = resolution->entries_begin(); iter != resolution->entries_end(); iter++)
		{
			const auto &entry = *iter;

			// set up label control
			std::string buf = string_format("%s:", entry.display_name());
			control = win_create_window_ex_utf8(0, "STATIC", buf.c_str(), WS_CHILD | WS_VISIBLE,
				margin, *y + 2, label_width, control_height, dialog, nullptr, nullptr, nullptr);
			SendMessage(control, WM_SETFONT, (WPARAM) font, 0);
			SetProp(control, owner_prop, (HANDLE) 1);

			// set up the active control
			x = margin + label_width;
			width = dialog_rect.right - dialog_rect.left - x - margin;

			aux_control = nullptr;
			switch(entry.option_type())
			{
			case util::option_guide::entry::option_type::STRING:
				style = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP;
				control = CreateWindow(TEXT("Edit"), nullptr, style,
						x, *y, width, control_height, dialog, nullptr, nullptr, nullptr);
				break;

			case util::option_guide::entry::option_type::INT:
				style = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER;
				control = CreateWindow(TEXT("Edit"), nullptr, style,
					x, *y, width - 16, control_height, dialog, nullptr, nullptr, nullptr);
				style = WS_CHILD | WS_VISIBLE | UDS_AUTOBUDDY;
				aux_control = CreateWindow(TEXT("msctls_updown32"), nullptr, style,
					x + width - 16, *y, 16, control_height, dialog, nullptr, nullptr, nullptr);
#if (_WIN32_IE >= 0x0400)
				SendMessage(aux_control, UDM_SETRANGE32, 0x80000000, 0x7FFFFFFF);
#else // !(_WIN32_IE >= 0x0400)
				SendMessage(aux_control, UDM_SETRANGE, 0, MAKELONG(UD_MAXVAL, UD_MINVAL));
#endif // (_WIN32_IE >= 0x0400)
				break;

			case util::option_guide::entry::option_type::ENUM_BEGIN:
				style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST;
				control = CreateWindow(TEXT("ComboBox"), nullptr, style,
						x, *y, width, control_height * 8, dialog, nullptr, nullptr, nullptr);
				break;

			default:
				control = nullptr;
				break;
			}

			if (!control)
				return -1;

			SetWindowLong(control, GWL_ID, CONTROL_START + control_count++);
			SendMessage(control, WM_SETFONT, (WPARAM) font, 0);
			SetProp(control, owner_prop, (HANDLE) 1);
			win_prepare_option_control(control, *iter);

			if (aux_control)
				SetProp(aux_control, owner_prop, (HANDLE) 1);

			if (entry.option_type() == util::option_guide::entry::option_type::ENUM_BEGIN)
			{
				while((iter + 1)->option_type() == util::option_guide::entry::option_type::ENUM_VALUE)
					iter++;
			}

			*y += control_height;
		}
	}
	return control_count;
}



static void apply_option_controls(HWND dialog, struct transfer_suggestion_info *suggestion_info,
	util::option_resolution *resolution, int control_count)
{
	int i;
	HWND control;

	if (suggestion_info)
	{
		control = GetDlgItem(dialog, CONTROL_FILTER);
		if (control)
			suggestion_info->selected = SendMessage(control, CB_GETCURSEL, 0, 0);
	}

	for (i = 0; i < control_count; i++)
	{
		control = GetDlgItem(dialog, CONTROL_START + i);
		if (control)
			win_add_resolution_parameter(control, resolution);
	}
}



struct new_dialog_info
{
	HWND parent;
	const imgtool_module *module;
	int control_count;
	int margin;
	unsigned int expanded : 1;
};



static void adjust_dialog_height(HWND dlgwnd)
{
	struct new_dialog_info *info;
	HWND more_button;
	HWND control;
	RECT r1, r2, r3;
	int control_id;
	LONG_PTR l;

	l = GetWindowLongPtr(dlgwnd, GWLP_USERDATA);
	info = (struct new_dialog_info *) l;
	more_button = GetDlgItem(dlgwnd, IDC_MORE);

	if (!info->expanded || (info->control_count == 0))
		control_id = IDC_MORE;
	else
		control_id = CONTROL_START + info->control_count - 1;
	control = GetDlgItem(dlgwnd, control_id);
	assert(control);

	GetWindowRect(control, &r1);
	GetWindowRect(dlgwnd, &r2);
	GetWindowRect(info->parent, &r3);

	SetWindowPos(dlgwnd, nullptr, 0, 0, r2.right - r2.left,
		r1.bottom + info->margin - r2.top, SWP_NOMOVE | SWP_NOZORDER);
	SetWindowPos(info->parent, nullptr, 0, 0, r3.right - r3.left,
		r1.bottom + info->margin - r3.top, SWP_NOMOVE | SWP_NOZORDER);

	SetWindowText(more_button, info->expanded ? TEXT("Less <<") : TEXT("More >>"));
}



static UINT_PTR new_dialog_typechange(HWND dlgwnd, int filter_index)
{
	struct new_dialog_info *info;
	int y;
	const util::option_guide *guide;
	LONG_PTR l;
	HWND more_button;
	HWND control, next_control;
	RECT r1, r2;
	HFONT font;

	l = GetWindowLongPtr(dlgwnd, GWLP_USERDATA);
	info = (struct new_dialog_info *) l;
	more_button = GetDlgItem(dlgwnd, IDC_MORE);

	info->module = find_filter_module(filter_index, TRUE);

	// clean out existing control windows
	info->control_count = 0;
	control = nullptr;
	while((control = FindWindowEx(dlgwnd, control, nullptr, nullptr)) != nullptr)
	{
		while(control && GetProp(control, owner_prop))
		{
			next_control = FindWindowEx(dlgwnd, control, nullptr, nullptr);
			DestroyWindow(control);
			control = next_control;
		}
	}

	guide = info->module->createimage_optguide;
	if (guide)
	{
		font = get_window_font(more_button);
		GetWindowRect(more_button, &r1);
		GetWindowRect(dlgwnd, &r2);
		y = r1.bottom + info->margin - r2.top;

		auto resolution = std::make_unique<util::option_resolution>(*guide);
		resolution->set_specification(info->module->createimage_optspec);

		info->control_count = create_option_controls(dlgwnd, font,
			r1.left - r2.left, &y, nullptr, resolution.get());
	}
	adjust_dialog_height(dlgwnd);
	return 0;
}



UINT_PTR CALLBACK win_new_dialog_hook(HWND dlgwnd, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	UINT_PTR rc = 0;
	const NMHDR *notify;
	const OFNOTIFY *ofn_notify;
	const NM_UPDOWN *ud_notify;
	struct new_dialog_info *info;
	const imgtool_module *module;
	HWND control;
	RECT r1, r2;
	LONG_PTR l;
	int id;
	util::option_resolution *resolution;
	HWND more_button;
	LRESULT lres;

	l = GetWindowLongPtr(dlgwnd, GWLP_USERDATA);
	info = (struct new_dialog_info *) l;

	more_button = GetDlgItem(dlgwnd, IDC_MORE);

	switch(message)
	{
		case WM_INITDIALOG:
			info = (struct new_dialog_info *) malloc(sizeof(*info));
			if (!info)
				return -1;
			memset(info, 0, sizeof(*info));
			SetWindowLongPtr(dlgwnd, GWLP_USERDATA, (LONG_PTR) info);

			info->parent = GetParent(dlgwnd);

			// compute lower margin
			GetWindowRect(more_button, &r1);
			GetWindowRect(dlgwnd, &r2);
			info->margin = r2.bottom - r1.bottom;

			// change dlgwnd height
			GetWindowRect(info->parent, &r1);
			SetWindowPos(dlgwnd, nullptr, 0, 0, r1.right - r1.left, r2.bottom - r2.top, SWP_NOMOVE | SWP_NOZORDER);
			break;

		case WM_DESTROY:
			if (info)
				free(info);
			break;

		case WM_COMMAND:
			switch(HIWORD(wparam))
			{
				case BN_CLICKED:
					if (LOWORD(wparam) == IDC_MORE)
					{
						info->expanded = !info->expanded;
						adjust_dialog_height(dlgwnd);
					}
					break;

				case EN_KILLFOCUS:
					control = (HWND) lparam;
					id = GetWindowLong(control, GWL_ID);
					if ((id >= CONTROL_START) && (id < CONTROL_START + info->control_count))
						win_check_option_control(control);
					break;
			}
			break;

		case WM_NOTIFY:
			notify = (const NMHDR *) lparam;
			switch(notify->code)
			{
				case UDN_DELTAPOS:
					ud_notify = (const NM_UPDOWN *) notify;
					lres = SendMessage(notify->hwndFrom, UDM_GETBUDDY, 0, 0);
					control = (HWND) lres;
					win_adjust_option_control(control, ud_notify->iDelta);
					break;

				case CDN_INITDONE:
					ofn_notify = (const OFNOTIFY *) notify;
					rc = new_dialog_typechange(dlgwnd, ofn_notify->lpOFN->nFilterIndex);
					break;

				case CDN_FILEOK:
					ofn_notify = (const OFNOTIFY *) notify;
					module = info->module;
					resolution = nullptr;

					if (module->createimage_optguide && module->createimage_optspec)
					{
						resolution = new util::option_resolution(*module->createimage_optguide);
						resolution->set_specification(module->createimage_optspec);
						apply_option_controls(dlgwnd, nullptr, resolution, info->control_count);
					}
					*((util::option_resolution **) ofn_notify->lpOFN->lCustData) = resolution;
					break;

				case CDN_TYPECHANGE:
					ofn_notify = (const OFNOTIFY *) notify;
					rc = new_dialog_typechange(dlgwnd, ofn_notify->lpOFN->nFilterIndex);
					break;
			}
			break;
	}

	return rc;
}



struct putfileopt_info
{
	struct transfer_suggestion_info *suggestion_info;
	util::option_resolution *resolution;
	int control_count;
};

static INT_PTR CALLBACK putfileopt_dialogproc(HWND dialog, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	struct putfileopt_info *pfo_info;
	HFONT font;
	int xmargin, ymargin, y, control_count, rc;
	HWND ok_button, cancel_button;
	RECT r1, r2;
	LONG_PTR l;

	switch(message)
	{
		case WM_INITDIALOG:
			pfo_info = (struct putfileopt_info *) lparam;
			SetWindowLongPtr(dialog, GWLP_USERDATA, lparam);

			ok_button = GetDlgItem(dialog, IDOK);
			cancel_button = GetDlgItem(dialog, IDCANCEL);
			font = get_window_font(ok_button);

			GetWindowRect(cancel_button, &r1);
			GetWindowRect(dialog, &r2);
			xmargin = r1.left - r2.left;
			ymargin = y = r1.top - r2.top - 20;

			control_count = create_option_controls(dialog, font, xmargin, &y, pfo_info->suggestion_info, pfo_info->resolution);
			if (control_count < 0)
				return -1;
			pfo_info->control_count = control_count;

			y += ymargin;
			SetWindowPos(cancel_button, nullptr, xmargin, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			GetWindowRect(ok_button, &r1);
			SetWindowPos(ok_button, nullptr, r1.left - r2.left, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			SetWindowPos(dialog, nullptr, 0, 0, r2.right - r2.left, y + r1.bottom - r2.top, SWP_NOZORDER | SWP_NOMOVE);
			break;

		case WM_COMMAND:
			rc = LOWORD(wparam);
			if (rc == IDOK || rc == IDCANCEL)
			{
				if (rc == IDOK)
				{
					l = GetWindowLongPtr(dialog, GWLP_USERDATA);
					pfo_info = (struct putfileopt_info *) l;
					apply_option_controls(dialog, pfo_info->suggestion_info, pfo_info->resolution, pfo_info->control_count);
				}

				EndDialog(dialog, LOWORD(wparam));
			}
			break;
	}
	return 0;
}



imgtoolerr_t win_show_option_dialog(HWND parent, struct transfer_suggestion_info *suggestion_info,
	const util::option_guide *guide, const char *optspec,
	std::unique_ptr<util::option_resolution> &result, BOOL *cancel)
{
	std::unique_ptr<util::option_resolution> res;
	struct putfileopt_info pfo_info;
	int rc;

	*cancel = FALSE;

	if (guide)
	{
		res = std::make_unique<util::option_resolution>(*guide);
		res->set_specification(optspec);
	}

	pfo_info.resolution = res.get();
	pfo_info.suggestion_info = suggestion_info;
	rc = DialogBoxParam(nullptr, MAKEINTRESOURCE(IDD_FILEOPTIONS), parent,
		putfileopt_dialogproc, (LPARAM) &pfo_info);
	*cancel = (rc == IDCANCEL);

	result = std::move(res);
	return IMGTOOLERR_SUCCESS;
}



