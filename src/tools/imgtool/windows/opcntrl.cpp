//============================================================
//
//  opcntrl.cpp - Code for handling option resolutions in
//  Win32 controls
//
//  This code was factored out of menu.c when Windows Imgtool
//  started to have dynamic controls
//
//============================================================

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>

#include "opcntrl.h"
#include "strconv.h"
#include "winutf8.h"
#include "strformat.h"


static const TCHAR resolution_entry_prop[] = TEXT("opcntrl_resentry");
static const TCHAR value_prop[] = TEXT("opcntrl_value");


static int get_option_count(const util::option_resolution::entry::rangelist &ranges)
{
	int count = 0;
	for (const auto &range : ranges)
		count += range.max - range.min + 1;

	return count;
}

static BOOL prepare_combobox(HWND control, const util::option_resolution::entry *entry)
{
	int default_index, current_index, option_count;
	int i, j;
	TCHAR buf1[256];
	TCHAR buf2[256];

	SendMessage(control, CB_GETLBTEXT, SendMessage(control, CB_GETCURSEL, 0, 0), (LPARAM) buf1);
	SendMessage(control, CB_RESETCONTENT, 0, 0);

	bool has_option = entry != nullptr;
	if (has_option)
	{
		if ((entry->option_type() != util::option_guide::entry::option_type::INT) && (entry->option_type() != util::option_guide::entry::option_type::ENUM_BEGIN))
			goto unexpected;

		const util::option_resolution::entry::rangelist &ranges = entry->ranges();
		int default_value = atoi(entry->default_value().c_str());

		option_count = 0;
		default_index = -1;
		current_index = -1;

		for (i = 0; ranges[i].min >= 0; i++)
		{
			for (j = ranges[i].min; j <= ranges[i].max; j++)
			{
				if (entry->option_type() == util::option_guide::entry::option_type::INT)
				{
					_sntprintf(buf2, ARRAY_LENGTH(buf2), TEXT("%d"), j);
					SendMessage(control, CB_ADDSTRING, 0, (LPARAM) buf2);
				}
				else if (entry->option_type() == util::option_guide::entry::option_type::ENUM_BEGIN)
				{
					auto iter = std::find_if(
						entry->enum_value_begin(),
						entry->enum_value_end(),
						[j](const auto &this_entry) { return this_entry.parameter() == j; });
					if (iter == entry->enum_value_end())
						goto unexpected;

					tstring tempstr = tstring_from_utf8(iter->display_name());
					SendMessage(control, CB_ADDSTRING, 0, (LPARAM) tempstr.c_str());
				}
				else
					goto unexpected;

				SendMessage(control, CB_SETITEMDATA, option_count, j);

				if (j == default_value)
					default_index = option_count;
				if (!_tcscmp(buf1, buf2))
					current_index = option_count;
				option_count++;
			}
		}

		// if there is only one option, it is effectively disabled
		if (option_count <= 1)
			has_option = false;

		if (current_index >= 0)
			SendMessage(control, CB_SETCURSEL, current_index, 0);
		else if (default_index >= 0)
			SendMessage(control, CB_SETCURSEL, default_index, 0);
	}
	else
	{
		// this item is non applicable
		SendMessage(control, CB_ADDSTRING, 0, (LPARAM) TEXT("N/A"));
		SendMessage(control, CB_SETCURSEL, 0, 0);
	}
	EnableWindow(control, has_option);
	return true;

unexpected:
	throw false;
}



static BOOL check_combobox(HWND control)
{
	return TRUE;
}



static BOOL prepare_editbox(HWND control, const util::option_resolution::entry *entry)
{
	util::option_resolution::error err = util::option_resolution::error::SUCCESS;
	std::string buf;
	int option_count;

	bool has_option = entry != nullptr;
	if (has_option)
	{
		switch(entry->option_type())
		{
		case util::option_guide::entry::option_type::STRING:
			break;

		case util::option_guide::entry::option_type::INT:
			buf = entry->default_value();
			break;

		default:
			err = util::option_resolution::error::INTERNAL;
			goto done;
		}
	}

	if (has_option)
	{
		option_count = get_option_count(entry->ranges());
		if (option_count <= 1)
			has_option = false;
	}

done:
	assert(err != util::option_resolution::error::INTERNAL);
	win_set_window_text_utf8(control, buf.c_str());
	EnableWindow(control, (err == util::option_resolution::error::SUCCESS) && has_option);
	return err == util::option_resolution::error::SUCCESS;
}



static BOOL check_editbox(HWND control)
{
	HANDLE h;
	void *val;
	UINT64 i_val;
	bool is_valid;

	const util::option_resolution::entry &entry = *((const util::option_resolution::entry *) GetProp(control, resolution_entry_prop));

	std::string buf = win_get_window_text_utf8(control);

	switch(entry.option_type())
	{
	case util::option_guide::entry::option_type::INT:
		i_val = atoi(buf.c_str());
		is_valid = std::find_if(
			entry.ranges().begin(),
			entry.ranges().end(),
			[i_val](const auto &r) { return r.min <= i_val && i_val <= r.max; }) != entry.ranges().end();

		if (!is_valid)
		{
			h = GetProp(control, value_prop);
			val = (void*) h;
			buf = util::string_format("%d", val);
			win_set_window_text_utf8(control, buf.c_str());
		}
		else
		{
			SetProp(control, value_prop, (HANDLE) i_val);
		}
		break;

	default:
		throw false;
	}

	return TRUE;
}



BOOL win_prepare_option_control(HWND control, util::option_resolution::entry &entry)
{
	BOOL rc = FALSE;
	TCHAR class_name[32];

	SetProp(control, resolution_entry_prop, (HANDLE)&entry);
	GetClassName(control, class_name, ARRAY_LENGTH(class_name));

	if (!_tcsicmp(class_name, TEXT("ComboBox")))
		rc = prepare_combobox(control, &entry);
	else if (!_tcsicmp(class_name, TEXT("Edit")))
		rc = prepare_editbox(control, &entry);

	return rc;
}



BOOL win_check_option_control(HWND control)
{
	BOOL rc = FALSE;
	TCHAR class_name[32];

	GetClassName(control, class_name, sizeof(class_name)
		/ sizeof(class_name[0]));

	if (!_tcsicmp(class_name, TEXT("ComboBox")))
		rc = check_combobox(control);
	else if (!_tcsicmp(class_name, TEXT("Edit")))
		rc = check_editbox(control);

	return rc;
}



BOOL win_adjust_option_control(HWND control, int delta)
{
	int val, original_val, i;
	BOOL changed = FALSE;

	auto entry = (const util::option_resolution::entry *) GetProp(control, resolution_entry_prop);

	assert(entry->option_type() == util::option_guide::entry::option_type::INT);

	if (delta == 0)
		return TRUE;

	const util::option_resolution::entry::rangelist &ranges = entry->ranges();

	std::string buf = win_get_window_text_utf8(control);

	original_val = atoi(buf.c_str());
	val = original_val + delta;

	for (i = 0; i < ranges.size(); i++)
	{
		if (ranges[i].min > val)
		{
			if ((delta < 0) && (i > 0))
				val = ranges[i-1].max;
			else
				val = ranges[i].min;
			changed = TRUE;
			break;
		}
	}
	if (!changed && (i > 0) && (ranges[i-1].max < val))
		val = ranges[i-1].max;

	if (val != original_val)
	{
		std::string buf = util::string_format("%d", val);
		win_set_window_text_utf8(control, buf.c_str());
	}
	return TRUE;
}



util::option_resolution::error win_add_resolution_parameter(HWND control, util::option_resolution *resolution)
{
	std::string text = win_get_window_text_utf8(control);
	if (text.empty())
		return util::option_resolution::error::INTERNAL;

	auto entry = (util::option_resolution::entry *) GetProp(control, resolution_entry_prop);
	if (!entry)
		return util::option_resolution::error::INTERNAL;

	if (entry->option_type() == util::option_guide::entry::option_type::ENUM_BEGIN)
	{
		// need to convert display name to identifier
		std::string old_text = text;
		text.clear();

		for (auto iter = entry->enum_value_begin(); iter != entry->enum_value_end(); iter++)
		{
			if (iter->display_name() == old_text)
			{
				text = iter->identifier();
				break;
			}
		}
	}

	if (!text.empty())
		entry->set_value(text);

	return util::option_resolution::error::SUCCESS;
}



