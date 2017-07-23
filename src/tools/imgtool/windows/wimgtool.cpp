//============================================================
//
//  wimgtool.cpp - Win32 GUI Imgtool
//
//============================================================
// standard windows headers
#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0501
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wingdi.h>

// standard C headers
#include <stdio.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include <dlgs.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <tchar.h>
#include <map>

// Wimgtool headers
#include "wimgtool.h"
#include "wimgres.h"
#include "pool.h"
#include "strconv.h"
#include "attrdlg.h"
#include "exticon.h"
#include "fileview.h"
#include "secview.h"
#include "winfile.h"
#include "winutf8.h"
#include "winutil.h"
#include "winutils.h"

const TCHAR wimgtool_class[] = TEXT("wimgtool_class");
const char wimgtool_producttext[] = "MAME Image Tool";

extern void win_association_dialog(HWND parent);

class wimgtool_info
{
public:
	HWND listview;					// handle to list view
	HWND statusbar;					// handle to the status bar
	imgtool::image::ptr image;		// the currently loaded image
	imgtool::partition::ptr partition;	// the currently loaded partition

	std::string filename;
	int open_mode;
	std::string current_directory;

	imgtool::windows::extension_icon_provider icon_provider;

	HICON readonly_icon;

	HIMAGELIST dragimage;
	POINT dragpt;

	wimgtool_info()
		: listview(nullptr), statusbar(nullptr), image(nullptr), partition(nullptr)
		, open_mode(0), readonly_icon(nullptr), dragimage(nullptr)
	{
	}
};

static void tstring_rtrim(TCHAR *buf)
{
	size_t buflen;
	TCHAR *s;

	buflen = _tcslen(buf);
	if (buflen)
	{
		for (s = &buf[buflen-1]; s >= buf && isspace(*s); s--)
			*s = '\0';
	}
}

static wimgtool_info *get_wimgtool_info(HWND window)
{
	wimgtool_info *info;
	LONG_PTR l;
	l = GetWindowLongPtr(window, GWLP_USERDATA);
	info = (wimgtool_info *) l;
	return info;
}

static DWORD win_get_file_attributes_utf8(const char *filename)
{
	auto t_filename = osd::text::to_tstring(filename);
	return GetFileAttributes(t_filename.c_str());
}

struct foreach_entry
{
	imgtool_dirent dirent;
	struct foreach_entry *next;
};

static imgtoolerr_t foreach_selected_item(HWND window,
	imgtoolerr_t (*proc)(HWND, const imgtool_dirent *, void *), void *param)
{
	wimgtool_info *info;
	imgtoolerr_t err;
	int selected_item;
	int selected_index = -1;
	char *s;
	LVITEM item;
	struct foreach_entry *first_entry = nullptr;
	struct foreach_entry *last_entry = nullptr;
	struct foreach_entry *entry;
	HRESULT res;
	info = get_wimgtool_info(window);

	if (info->image)
	{
		do
		{
			do
			{
				selected_index = ListView_GetNextItem(info->listview, selected_index, LVIS_SELECTED);
				if (selected_index < 0)
				{
					selected_item = -1;
				}
				else
				{
					item.mask = LVIF_PARAM;
					item.iItem = selected_index;
					res = ListView_GetItem(info->listview, &item); res++;
					selected_item = item.lParam;
				}
			}
			while((selected_index >= 0) && (selected_item < 0));

			if (selected_item >= 0)
			{
				entry = (foreach_entry*)alloca(sizeof(*entry));
				entry->next = nullptr;

				// retrieve the directory entry
				err = info->partition->get_directory_entry(info->current_directory.c_str(), selected_item, entry->dirent);
				if (err)
					return err;

				// if we have a path, prepend the path
				if (!info->current_directory.empty())
				{
					char path_separator = (char)info->partition->get_info_int(IMGTOOLINFO_INT_PATH_SEPARATOR);

					// create a copy of entry->dirent.filename
					s = (char *) alloca(strlen(entry->dirent.filename) + 1);
					strcpy(s, entry->dirent.filename);

					// copy the full path back in
					snprintf(entry->dirent.filename, ARRAY_LENGTH(entry->dirent.filename),
						"%s%c%s", info->current_directory.c_str(), path_separator, s);
				}

				// append to list
				if (last_entry)
					last_entry->next = entry;
				else
					first_entry = entry;
				last_entry = entry;
			}
		}
		while(selected_item >= 0);
	}

	// now that we have the list, call the callbacks
	for (entry = first_entry; entry; entry = entry->next)
	{
		err = proc(window, &entry->dirent, param);
		if (err)
			return err;
	}
	return IMGTOOLERR_SUCCESS;
}



void wimgtool_report_error(HWND window, imgtoolerr_t err, const char *imagename, const char *filename)
{
	const char *error_text;
	const char *source;
	std::string imagename_basename;
	std::string buffer;
	const char *message;

	error_text = imgtool_error(err);

	switch(ERRORSOURCE(err))
	{
		case IMGTOOLERR_SRC_IMAGEFILE:
			imagename_basename = core_filename_extract_base(imagename);
			source = imagename_basename.c_str();
			break;
		case IMGTOOLERR_SRC_FILEONIMAGE:
			source = filename;
			break;
		default:
			source = nullptr;
			break;
	}

	if (source)
	{
		buffer = string_format("%s: %s", source, error_text);
		message = buffer.c_str();
	}
	else
	{
		message = error_text;
	}
	win_message_box_utf8(window, message, wimgtool_producttext, MB_OK);
}



static imgtoolerr_t append_dirent(HWND window, int index, const imgtool_dirent *entry)
{
	LVITEM lvi;
	int new_index, column_index;
	wimgtool_info *info;
	TCHAR buffer[32];
	int icon_index = -1;
	imgtool_partition_features features;
	struct tm *local_time;

	info = get_wimgtool_info(window);
	features = info->partition->get_features();

	/* try to get a custom icon */
	if (features.supports_geticoninfo)
	{
		imgtool_iconinfo iconinfo;
		std::string buf = string_format("%s%s", info->current_directory, entry->filename);
		info->partition->get_icon_info(buf.c_str(), &iconinfo);

		icon_index = info->icon_provider.provide_icon_index(iconinfo);
	}

	if (icon_index < 0)
	{
		if (entry->directory)
		{
			icon_index = info->icon_provider.directory_icon_index();
		}
		else
		{
			// identify and normalize the file extension
			std::string extension = core_filename_extract_extension(entry->filename);
			icon_index = info->icon_provider.provide_icon_index(std::move(extension));
		}
	}

	osd::text::tstring t_entry_filename = osd::text::to_tstring(entry->filename);

	memset(&lvi, 0, sizeof(lvi));
	lvi.iItem = ListView_GetItemCount(info->listview);
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.pszText = (LPTSTR) t_entry_filename.c_str();
	lvi.lParam = index;

	// if we have an icon, use it
	if (icon_index >= 0)
	{
		lvi.mask |= LVIF_IMAGE;
		lvi.iImage = icon_index;
	}

	new_index = ListView_InsertItem(info->listview, &lvi);

	if (entry->directory)
	{
		_sntprintf(buffer, ARRAY_LENGTH(buffer), TEXT("<DIR>"));
	}
	else
	{
		// set the file size
		_sntprintf(buffer, ARRAY_LENGTH(buffer), TEXT("%I64u"), entry->filesize);
	}
	column_index = 1;
	ListView_SetItemText(info->listview, new_index, column_index++, buffer);

	// set creation time, if supported
	if (features.supports_creation_time)
	{
		if (entry->creation_time != 0)
		{
			local_time = localtime(&entry->creation_time);
			_sntprintf(buffer, ARRAY_LENGTH(buffer), _tasctime(local_time));
			tstring_rtrim(buffer);
			ListView_SetItemText(info->listview, new_index, column_index, buffer);
		}
		column_index++;
	}

	// set last modified time, if supported
	if (features.supports_lastmodified_time)
	{
		if (entry->lastmodified_time != 0)
		{
			local_time = localtime(&entry->lastmodified_time);
			_sntprintf(buffer, ARRAY_LENGTH(buffer), _tasctime(local_time));
			tstring_rtrim(buffer);
			ListView_SetItemText(info->listview, new_index, column_index, buffer);
		}
		column_index++;
	}

	// set attributes and corruption notice
	if (entry->attr)
	{
		osd::text::tstring t_tempstr = osd::text::to_tstring(entry->attr);
		ListView_SetItemText(info->listview, new_index, column_index++, (LPTSTR) t_tempstr.c_str());
	}
	if (entry->corrupt)
	{
		ListView_SetItemText(info->listview, new_index, column_index++, (LPTSTR) TEXT("Corrupt"));
	}
	return (imgtoolerr_t)0;
}



static imgtoolerr_t refresh_image(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	wimgtool_info *info;
	imgtool::directory::ptr imageenum;
	char size_buf[32];
	imgtool_dirent entry;
	UINT64 filesize;
	int i;
	BOOL is_root_directory;
	imgtool_partition_features features;
	char path_separator;
	HRESULT res;
	osd::text::tstring tempstr;

	info = get_wimgtool_info(window);
	size_buf[0] = '\0';

	res = ListView_DeleteAllItems(info->listview); res++;

	if (info->image)
	{
		features = info->partition->get_features();

		is_root_directory = TRUE;
		if (!info->current_directory.empty())
		{
			for (i = 0; info->current_directory[i]; i++)
			{
				path_separator = (char) info->partition->get_info_int(IMGTOOLINFO_INT_PATH_SEPARATOR);
				if (info->current_directory[i] != path_separator)
				{
					is_root_directory = FALSE;
					break;
				}
			}
		}

		memset(&entry, 0, sizeof(entry));

		// add ".." to non-root directory entries
		if (!is_root_directory)
		{
			strcpy(entry.filename, "..");
			entry.directory = 1;
			entry.attr[0] = '\0';
			err = append_dirent(window, -1, &entry);
			if (err)
				return err;
		}

		err = imgtool::directory::open(*info->partition, info->current_directory, imageenum);
		if (err)
			return err;

		i = 0;
		do
		{
			err = imageenum->get_next(entry);
			if (err)
				return err;

			if (entry.filename[0])
			{
				err = append_dirent(window, i++, &entry);
				if (err)
					return err;
			}
		}
		while(!entry.eof);

		if (features.supports_freespace)
		{
			err = info->partition->get_free_space(filesize);
			if (err)
				return err;
			snprintf(size_buf, ARRAY_LENGTH(size_buf), "%u bytes free", (unsigned) filesize);
		}

	}

	tempstr = osd::text::to_tstring(size_buf);
	SendMessage(info->statusbar, SB_SETTEXT, 2, (LPARAM) tempstr.c_str());

	return IMGTOOLERR_SUCCESS;
}



static imgtoolerr_t full_refresh_image(HWND window)
{
	wimgtool_info *info;
	LVCOLUMN col;
	int column_index = 0;
	int i;
	std::string buf;
	std::string imageinfo;
	TCHAR file_title_buf[MAX_PATH];
	const char *statusbar_text[2];
	imgtool_partition_features features;
	std::string basename;
	extern const char build_version[];

	info = get_wimgtool_info(window);

	// get the modules and features
	if (info->partition)
		features = info->partition->get_features();
	else
		memset(&features, 0, sizeof(features));

	if (!info->filename.empty())
	{
		// get file title from Windows
		osd::text::tstring t_filename = osd::text::to_tstring(info->filename);
		GetFileTitle(t_filename.c_str(), file_title_buf, ARRAY_LENGTH(file_title_buf));
		std::string utf8_file_title = osd::text::from_tstring(file_title_buf);

		// get info from image
		if (info->image)
			imageinfo = info->image->info();

		// combine all of this into a title bar
		if (!info->current_directory.empty())
		{
			// has a current directory
			if (!imageinfo.empty())
			{
				buf = string_format("%s (\"%s\") - %s", utf8_file_title, imageinfo, info->current_directory);
			}
			else
			{
				buf = string_format("%s - %s", utf8_file_title, info->current_directory);
			}
		}
		else
		{
			// no current directory
			buf = string_format(
				!imageinfo.empty() ? "%s (\"%s\")" : "%s",
				utf8_file_title, imageinfo);
		}

		basename = core_filename_extract_base(info->filename);
		statusbar_text[0] = basename.c_str();
		statusbar_text[1] = info->image->module().description;
	}
	else
	{
		buf = string_format("%s %s", wimgtool_producttext, build_version);
		statusbar_text[0] = nullptr;
		statusbar_text[1] = nullptr;
	}

	win_set_window_text_utf8(window, buf.c_str());

	for (i = 0; i < ARRAY_LENGTH(statusbar_text); i++)
	{
		auto t_tempstr = statusbar_text[i]
			? std::make_unique<osd::text::tstring>(osd::text::to_tstring(statusbar_text[i]))
			: std::unique_ptr<osd::text::tstring>();

		SendMessage(info->statusbar, SB_SETTEXT, i, (LPARAM)(t_tempstr.get() ? t_tempstr->c_str() : nullptr));
	}

	// set the icon
	if (info->image && (info->open_mode == OSD_FOPEN_READ))
		SendMessage(info->statusbar, SB_SETICON, 0, (LPARAM) info->readonly_icon);
	else
		SendMessage(info->statusbar, SB_SETICON, 0, (LPARAM) 0);

	DragAcceptFiles(window, !info->filename.empty());

	// create the listview columns
	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 200;
	col.pszText = (LPTSTR) TEXT("Filename");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Size");
	col.fmt = LVCFMT_RIGHT;
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	if (features.supports_creation_time)
	{
		col.mask = LVCF_TEXT | LVCF_WIDTH;
		col.cx = 160;
		col.pszText = (LPTSTR) TEXT("Creation time");
		if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
			return IMGTOOLERR_OUTOFMEMORY;
	}

	if (features.supports_lastmodified_time)
	{
		col.mask = LVCF_TEXT | LVCF_WIDTH;
		col.cx = 160;
		col.pszText = (LPTSTR) TEXT("Last modified time");
		if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
			return IMGTOOLERR_OUTOFMEMORY;
	}

	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Attributes");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Notes");
	if (ListView_InsertColumn(info->listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	// delete extraneous columns
	while(ListView_DeleteColumn(info->listview, column_index))
		;
	return refresh_image(window);
}



static imgtoolerr_t setup_openfilename_struct(win_open_file_name *ofn, HWND window, bool creating_file)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const imgtool_module *default_module = nullptr;
	char *initial_dir = nullptr;
	char *dir_char;
	imgtool_module_features features;
	DWORD filter_index = 0, current_index = 0;
	const wimgtool_info *info;
	int i;

	info = get_wimgtool_info(window);
	if (info->image)
		default_module = &info->image->module();

	std::stringstream filter;

	// if we're opening (not creating), then "Autodetect" is pertinent
	if (!creating_file)
	{
		current_index++;
		filter << "Autodetect (*.*)|*.*|";
	}

	// write out library modules
	for (const auto &module : imgtool_get_modules())
	{
		// check to see if we have the right features
		features = imgtool_get_module_features(module.get());
		if (creating_file ? features.supports_create : features.supports_open)
		{
			// is this the filter we are asking for?
			current_index++;
			if (module.get() == default_module)
				filter_index = current_index;

			filter << module->description << " (";

			for (i = 0; module->extensions[i]; i++)
			{
				if (module->extensions[i] == ',')
					filter << ';';
				if ((i == 0) || (module->extensions[i] == ','))
					filter << "*.";
				if (module->extensions[i] != ',')
					filter << module->extensions[i];
			}
			filter << ")|";

			for (i = 0; module->extensions[i]; i++)
			{
				if (module->extensions[i] == ',')
					filter << ';';
				if ((i == 0) || (module->extensions[i] == ','))
					filter << "*.";
				if (module->extensions[i] != ',')
					filter << module->extensions[i];
			}

			filter << '|';
		}
	}

	// populate the actual structure
	memset(ofn, 0, sizeof(*ofn));
	ofn->flags = OFN_EXPLORER;
	ofn->owner = window;
	ofn->filter = filter.str();
	ofn->filter_index = filter_index;

	// can we specify an initial directory?
	if (!info->filename.empty())
	{
		// copy the filename into the filename structure
		ofn->filename = info->filename;

		// specify an initial directory
		initial_dir = (char*)alloca((strlen(info->filename.c_str()) + 1) * sizeof(*info->filename.c_str()));
		strcpy(initial_dir, info->filename.c_str());
		dir_char = strrchr(initial_dir, '\\');
		if (dir_char)
			dir_char[1] = '\0';
		else
			initial_dir = nullptr;
		ofn->initial_directory = initial_dir;
	}

	return err;
}



const imgtool_module *find_filter_module(int filter_index,
	BOOL creating_file)
{
	imgtool_module_features features;

	if (filter_index-- == 0)
		return nullptr;
	if (!creating_file && (filter_index-- == 0))
		return nullptr;

	for (const auto &module : imgtool_get_modules())
	{
		features = imgtool_get_module_features(module.get());
		if (creating_file ? features.supports_create : features.supports_open)
		{
			if (filter_index-- == 0)
				return module.get();
		}
	}
	return nullptr;
}

//============================================================
//  win_mkdir
//============================================================

osd_file::error win_mkdir(const char *dir)
{
	osd::text::tstring tempstr = osd::text::to_tstring(dir);

	return !CreateDirectory(tempstr.c_str(), nullptr)
		? win_error_to_file_error(GetLastError())
		: osd_file::error::NONE;
}


static imgtoolerr_t get_recursive_directory(imgtool::partition &partition, const char *path, LPCSTR local_path)
{
	imgtoolerr_t err;
	imgtool::directory::ptr imageenum;
	imgtool_dirent entry;
	const char *subpath;
	char local_subpath[MAX_PATH];

	if (win_mkdir(local_path) != osd_file::error::NONE)
		return IMGTOOLERR_UNEXPECTED;

	err = imgtool::directory::open(partition, path, imageenum);
	if (err)
		return err;

	do
	{
		err = imageenum->get_next(entry);
		if (err)
			return err;

		if (!entry.eof)
		{
			snprintf(local_subpath, ARRAY_LENGTH(local_subpath),
				"%s\\%s", local_path, entry.filename);
			subpath = partition.path_concatenate(path, entry.filename);

			if (entry.directory)
				err = get_recursive_directory(partition, subpath, local_subpath);
			else
				err = partition.get_file(subpath, nullptr, local_subpath, nullptr);
			if (err)
				return err;
		}
	}
	while(!entry.eof);

	return IMGTOOLERR_SUCCESS;
}




static imgtoolerr_t put_recursive_directory(imgtool::partition &partition, LPCTSTR local_path, const char *path)
{
	imgtoolerr_t err;
	HANDLE h = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA wfd;
	const char *subpath;
	TCHAR local_subpath[MAX_PATH];

	err = partition.create_directory(path);
	if (err)
		goto done;

	_sntprintf(local_subpath, ARRAY_LENGTH(local_subpath), TEXT("%s\\*.*"), local_path);

	h = FindFirstFile(local_subpath, &wfd);
	if (h && (h != INVALID_HANDLE_VALUE))
	{
		do
		{
			if (_tcscmp(wfd.cFileName, TEXT(".")) && _tcscmp(wfd.cFileName, TEXT("..")))
			{
				_sntprintf(local_subpath, ARRAY_LENGTH(local_subpath), TEXT("%s\\%s"), local_path, wfd.cFileName);
				std::string filename = osd::text::from_tstring(wfd.cFileName);
				subpath = partition.path_concatenate(path, filename.c_str());

				if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					err = put_recursive_directory(partition, local_subpath, subpath);
				}
				else
				{
					std::string tempstr = osd::text::from_tstring(local_subpath);
					err = partition.put_file(subpath, nullptr, tempstr.c_str(), nullptr, nullptr);
				}
				if (err)
					goto done;
			}
		}
		while(FindNextFile(h, &wfd));
	}

done:
	if (h && (h != INVALID_HANDLE_VALUE))
		FindClose(h);
	return err;
}



imgtoolerr_t wimgtool_open_image(HWND window, const imgtool_module *module,
	const std::string &filename, int read_or_write)
{
	imgtoolerr_t err;
	imgtool::image::ptr image;
	imgtool::partition::ptr partition;
	imgtool_module *identified_module;
	wimgtool_info *info;
	int partition_index = 0;
	const char *root_path;
	imgtool_module_features features = { 0, };

	info = get_wimgtool_info(window);

	// if the module is not specified, auto detect the format
	if (!module)
	{
		err = imgtool::image::identify_file(filename.c_str(), &identified_module, 1);
		if (err)
			goto done;
		module = identified_module;
	}

	// check to see if this module actually supports writing
	if (read_or_write != OSD_FOPEN_READ)
	{
		features = imgtool_get_module_features(module);
		if (features.is_read_only)
			read_or_write = OSD_FOPEN_READ;
	}

	info->filename = filename;

	// try to open the image
	err = imgtool::image::open(module, filename, read_or_write, image);
	if ((ERRORCODE(err) == IMGTOOLERR_READONLY) && read_or_write)
	{
		// if we failed when open a read/write image, try again
		read_or_write = OSD_FOPEN_READ;
		err = imgtool::image::open(module, filename, read_or_write, image);
	}
	if (err)
		goto done;

	// try to open the partition
	err = imgtool::partition::open(*image, partition_index, partition);
	if (err)
		goto done;

	// unload current image and partition
	info->image = std::move(image);
	info->partition = std::move(partition);
	info->open_mode = read_or_write;
	info->current_directory.clear();

	// do we support directories?
	if (info->partition->get_features().supports_directories)
	{
		root_path = partition->get_root_path();
		info->current_directory.assign(root_path);
	}

	// refresh the window
	full_refresh_image(window);

done:
	return err;
}



static void menu_new(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	win_open_file_name ofn;
	const imgtool_module *module;
	std::unique_ptr<util::option_resolution> resolution;

	err = setup_openfilename_struct(&ofn, window, true);
	if (err)
		goto done;
	ofn.flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
	ofn.instance = GetModuleHandle(nullptr);
	ofn.template_name = MAKEINTRESOURCE(IDD_FILETEMPLATE);
	ofn.hook = win_new_dialog_hook;
	ofn.custom_data = (LPARAM) &resolution;
	ofn.type = WIN_FILE_DIALOG_SAVE;
	if (!win_get_file_name_dialog(&ofn))
		goto done;

	module = find_filter_module(ofn.filter_index, TRUE);

	err = imgtool::image::create(module, ofn.filename.c_str(), resolution.get());
	if (err)
		goto done;

	err = wimgtool_open_image(window, module, ofn.filename, OSD_FOPEN_RW);
	if (err)
		goto done;

done:
	if (err)
		wimgtool_report_error(window, err, ofn.filename.c_str(), nullptr);
}



static void menu_open(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	win_open_file_name ofn;
	const imgtool_module *module;
	int read_or_write;

	err = setup_openfilename_struct(&ofn, window, false);
	if (err)
		goto done;
	ofn.flags |= OFN_FILEMUSTEXIST;
	ofn.type = WIN_FILE_DIALOG_OPEN;
	if (!win_get_file_name_dialog(&ofn))
		goto done;

	module = find_filter_module(ofn.filter_index, FALSE);

	// is this file read only?
	if ((ofn.flags & OFN_READONLY) || (win_get_file_attributes_utf8(ofn.filename.c_str()) & FILE_ATTRIBUTE_READONLY))
		read_or_write = OSD_FOPEN_READ;
	else
		read_or_write = OSD_FOPEN_RW;

	err = wimgtool_open_image(window, module, ofn.filename, read_or_write);
	if (err)
		goto done;

done:
	if (err)
		wimgtool_report_error(window, err, ofn.filename.c_str(), nullptr);
}



static void menu_insert(HWND window)
{
	imgtoolerr_t err;
	char *image_filename = nullptr;
	win_open_file_name ofn;
	wimgtool_info *info;
	std::unique_ptr<util::option_resolution> opts;
	BOOL cancel;
	//const imgtool_module *module;
	const char *fork = nullptr;
	struct transfer_suggestion_info suggestion_info;
	int use_suggestion_info;
	imgtool::stream::ptr stream;
	filter_getinfoproc filter = nullptr;
	const util::option_guide *writefile_optguide;
	const char *writefile_optspec;
	std::string basename;

	info = get_wimgtool_info(window);

	memset(&ofn, 0, sizeof(ofn));
	ofn.type = WIN_FILE_DIALOG_OPEN;
	ofn.owner = window;
	ofn.flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;
	if (!win_get_file_name_dialog(&ofn))
	{
		err = (imgtoolerr_t)0;
		goto done;
	}

	/* we need to open the stream at this point, so that we can suggest the transfer */
	stream = imgtool::stream::ptr(imgtool::stream::open(ofn.filename.c_str(), OSD_FOPEN_READ));
	if (!stream)
	{
		err = IMGTOOLERR_FILENOTFOUND;
		goto done;
	}

	//module = info->image->module();

	/* figure out which filters are appropriate for this file */
	info->partition->suggest_file_filters(nullptr, stream.get(), suggestion_info.suggestions,
		ARRAY_LENGTH(suggestion_info.suggestions));

	/* do we need to show an option dialog? */
	writefile_optguide = (const util::option_guide *) info->partition->get_info_ptr(IMGTOOLINFO_PTR_WRITEFILE_OPTGUIDE);
	writefile_optspec = info->partition->get_info_string(IMGTOOLINFO_STR_WRITEFILE_OPTSPEC);
	if (suggestion_info.suggestions[0].viability || (writefile_optguide && writefile_optspec))
	{
		use_suggestion_info = (suggestion_info.suggestions[0].viability != SUGGESTION_END);
		err = win_show_option_dialog(window, use_suggestion_info ? &suggestion_info : nullptr,
			writefile_optguide, writefile_optspec, opts, &cancel);
		if (err || cancel)
			goto done;

		if (use_suggestion_info)
		{
			fork = suggestion_info.suggestions[suggestion_info.selected].fork;
			filter = suggestion_info.suggestions[suggestion_info.selected].filter;
		}
	}

	// figure out the image filename
	basename = core_filename_extract_base(ofn.filename);
	image_filename = (char *) alloca((basename.size() + 1) * sizeof(image_filename[0]));
	strcpy(image_filename, basename.c_str());

	/* append the current directory, if appropriate */
	if (!info->current_directory.empty())
	{
		image_filename = (char *)info->partition->path_concatenate(info->current_directory.c_str(), image_filename);
	}

	err = info->partition->write_file(image_filename, fork, *stream, opts.get(), filter);
	if (err)
		goto done;

	err = refresh_image(window);
	if (err)
		goto done;

done:
	if (err)
		wimgtool_report_error(window, err, image_filename, ofn.filename.c_str());
}



static UINT_PTR CALLBACK extract_dialog_hook(HWND dlgwnd, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	UINT_PTR rc = 0;
	int i;
	HWND filter_combo;
	struct transfer_suggestion_info *info;
	OPENFILENAME *ofi;
	LONG_PTR l;

	filter_combo = GetDlgItem(dlgwnd, IDC_FILTERCOMBO);

	switch(message)
	{
		case WM_INITDIALOG:
			ofi = (OPENFILENAME *) lparam;
			info = (struct transfer_suggestion_info *) ofi->lCustData;
			SetWindowLongPtr(dlgwnd, GWLP_USERDATA, (LONG_PTR) info);

			for (i = 0; info->suggestions[i].viability; i++)
			{
				SendMessage(filter_combo, CB_ADDSTRING, 0, (LPARAM) info->suggestions[i].description);
			}
			SendMessage(filter_combo, CB_SETCURSEL, info->selected, 0);

			rc = TRUE;
			break;

		case WM_COMMAND:
			switch(HIWORD(wparam))
			{
				case CBN_SELCHANGE:
					if (LOWORD(wparam) == IDC_FILTERCOMBO)
					{
						l = GetWindowLongPtr(dlgwnd, GWLP_USERDATA);
						info = (struct transfer_suggestion_info *) l;
						info->selected = SendMessage(filter_combo, CB_GETCURSEL, 0, 0);
					}
					break;
			}
			break;
	}
	return rc;
}



static imgtoolerr_t menu_extract_proc(HWND window, const imgtool_dirent *entry, void *param)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	win_open_file_name ofn;
	wimgtool_info *info;
	const char *filename;
	const char *image_basename;
	const char *fork;
	struct transfer_suggestion_info suggestion_info;
	int i;
	filter_getinfoproc filter;

	info = get_wimgtool_info(window);

	filename = entry->filename;

	// figure out a suggested host filename
	image_basename = info->partition->get_base_name(entry->filename);

	// try suggesting some filters (only if doing a single file)
	if (!entry->directory)
	{
		info->partition->suggest_file_filters(filename, nullptr, suggestion_info.suggestions,
			ARRAY_LENGTH(suggestion_info.suggestions));

		suggestion_info.selected = 0;
		for (i = 0; i < ARRAY_LENGTH(suggestion_info.suggestions); i++)
		{
			if (suggestion_info.suggestions[i].viability == SUGGESTION_RECOMMENDED)
			{
				suggestion_info.selected = i;
				break;
			}
		}
	}
	else
	{
		memset(&suggestion_info, 0, sizeof(suggestion_info));
		suggestion_info.selected = -1;
	}

	// set up the actual struct
	memset(&ofn, 0, sizeof(ofn));
	ofn.owner = window;
	ofn.flags = OFN_EXPLORER;
	ofn.filter = "All files (*.*)|*.*";
	ofn.filename = image_basename;

	if (suggestion_info.suggestions[0].viability)
	{
		ofn.flags |= OFN_ENABLEHOOK | OFN_ENABLESIZING | OFN_ENABLETEMPLATE;
		ofn.hook = extract_dialog_hook;
		ofn.instance = GetModuleHandle(nullptr);
		ofn.template_name = MAKEINTRESOURCE(IDD_EXTRACTOPTIONS);
		ofn.custom_data = (LPARAM) &suggestion_info;
	}

	ofn.type = WIN_FILE_DIALOG_SAVE;
	if (!win_get_file_name_dialog(&ofn))
		goto done;

	if (entry->directory)
	{
		err = get_recursive_directory(*info->partition, filename, ofn.filename.c_str());
		if (err)
			goto done;
	}
	else
	{
		if (suggestion_info.selected >= 0)
		{
			fork = suggestion_info.suggestions[suggestion_info.selected].fork;
			filter = suggestion_info.suggestions[suggestion_info.selected].filter;
		}
		else
		{
			fork = nullptr;
			filter = nullptr;
		}

		err = info->partition->get_file(filename, fork, ofn.filename.c_str(), filter);
		if (err)
			goto done;
	}

done:
	if (err)
		wimgtool_report_error(window, err, filename, ofn.filename.c_str());
	return err;
}



static void menu_extract(HWND window)
{
	foreach_selected_item(window, menu_extract_proc, nullptr);
}



struct createdir_dialog_info
{
	HWND ok_button;
	HWND edit_box;
	TCHAR buf[256];
};



static INT_PTR CALLBACK createdir_dialog_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam)
{
	struct createdir_dialog_info *info;
	LONG_PTR l;
	INT_PTR rc = 0;
	int id;

	switch(message)
	{
		case WM_INITDIALOG:
			EnableWindow(GetDlgItem(dialog, IDOK), FALSE);
			SetWindowLongPtr(dialog, GWLP_USERDATA, lparam);
			info = (struct createdir_dialog_info *) lparam;

			info->ok_button = GetDlgItem(dialog, IDOK);
			info->edit_box = GetDlgItem(dialog, IDC_EDIT);
			SetFocus(info->edit_box);
			break;

		case WM_COMMAND:
			l = GetWindowLongPtr(dialog, GWLP_USERDATA);
			info = (struct createdir_dialog_info *) l;

			switch(HIWORD(wparam))
			{
				case BN_CLICKED:
					id = LOWORD(wparam);
					if (id == IDCANCEL)
						info->buf[0] = '\0';
					EndDialog(dialog, id);
					break;

				case EN_CHANGE:
					GetWindowText(info->edit_box,
						info->buf, ARRAY_LENGTH(info->buf));
					EnableWindow(info->ok_button, info->buf[0] != '\0');
					break;
			}
			break;
	}
	return rc;
}



static void menu_createdir(HWND window)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	struct createdir_dialog_info cdi;
	wimgtool_info *info;
	char *s;
	std::string utf8_dirname;

	info = get_wimgtool_info(window);

	memset(&cdi, 0, sizeof(cdi));
	DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_CREATEDIR),
		window, createdir_dialog_proc, (LPARAM) &cdi);

	if (cdi.buf[0] == '\0')
		goto done;
	utf8_dirname = osd::text::from_tstring(cdi.buf);

	if (!info->current_directory.empty())
	{
		s = (char *)info->partition->path_concatenate(info->current_directory.c_str(), utf8_dirname.c_str());
		utf8_dirname.assign(s);
	}

	err = info->partition->create_directory(utf8_dirname.c_str());
	if (err)
		goto done;

	err = refresh_image(window);
	if (err)
		goto done;

done:
	if (err)
		wimgtool_report_error(window, err, nullptr, utf8_dirname.c_str());
}



static imgtoolerr_t menu_delete_proc(HWND window, const imgtool_dirent *entry, void *param)
{
	imgtoolerr_t err;
	wimgtool_info *info;

	info = get_wimgtool_info(window);

	if (entry->directory)
		err = info->partition->delete_directory(entry->filename);
	else
		err = info->partition->delete_file(entry->filename);
	if (err)
		goto done;

done:
	if (err)
		wimgtool_report_error(window, err, nullptr, entry->filename);
	return err;
}



static void menu_delete(HWND window)
{
	imgtoolerr_t err;

	foreach_selected_item(window, menu_delete_proc, nullptr);

	err = refresh_image(window);
	if (err)
		wimgtool_report_error(window, err, nullptr, nullptr);
}



static imgtoolerr_t menu_view_proc(HWND window, const imgtool_dirent *entry, void *param)
{
	imgtoolerr_t err;
	wimgtool_info *info = get_wimgtool_info(window);

	// read the file
	imgtool::stream::ptr stream = imgtool::stream::open_mem(nullptr, 0);
	err = info->partition->read_file(entry->filename, nullptr, *stream, nullptr);
	if (err)
	{
		wimgtool_report_error(window, err, nullptr, entry->filename);
		return err;
	}

	win_fileview_dialog(window, entry->filename, stream->getptr(), stream->size());
	return IMGTOOLERR_SUCCESS;
}



static void menu_view(HWND window)
{
	foreach_selected_item(window, menu_view_proc, nullptr);
}



static void menu_sectorview(HWND window)
{
	wimgtool_info *info;
	info = get_wimgtool_info(window);
	win_sectorview_dialog(window, info->image.get());
}



static void set_listview_style(HWND window, DWORD style)
{
	wimgtool_info *info;

	info = get_wimgtool_info(window);
	style &= LVS_TYPEMASK;
	style |= (GetWindowLong(info->listview, GWL_STYLE) & ~LVS_TYPEMASK);
	SetWindowLong(info->listview, GWL_STYLE, style);
}



static LRESULT wimgtool_create(HWND window, CREATESTRUCT *pcs)
{
	wimgtool_info *info;
	static const int status_widths[3] = { 200, 400, -1 };

	info = new wimgtool_info();
	if (!info)
		return -1;

	SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR) info);

	// create the list view
	info->listview = CreateWindow(WC_LISTVIEW, nullptr,
		WS_VISIBLE | WS_CHILD, 0, 0, pcs->cx, pcs->cy, window, nullptr, nullptr, nullptr);
	if (!info->listview)
		return -1;
	set_listview_style(window, LVS_REPORT);

	// create the status bar
	info->statusbar = CreateWindow(STATUSCLASSNAME, nullptr, WS_VISIBLE | WS_CHILD,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, window, nullptr, nullptr, nullptr);
	if (!info->statusbar)
		return -1;
	SendMessage(info->statusbar, SB_SETPARTS, ARRAY_LENGTH(status_widths),
		(LPARAM) status_widths);

	// create imagelists
	(void)ListView_SetImageList(info->listview, info->icon_provider.normal_icon_list(), LVSIL_NORMAL);
	(void)ListView_SetImageList(info->listview, info->icon_provider.small_icon_list(), LVSIL_SMALL);

	// get icons
	info->readonly_icon = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_READONLY), IMAGE_ICON, 16, 16, 0);

	full_refresh_image(window);
	return 0;
}



static void wimgtool_destroy(HWND window)
{
	wimgtool_info *info;

	info = get_wimgtool_info(window);

	if (info)
	{
		DestroyIcon(info->readonly_icon);
		delete info;
	}
}



static void drop_files(HWND window, HDROP drop)
{
	wimgtool_info *info;
	UINT count, i;
	TCHAR buffer[MAX_PATH];
	std::string subpath;
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;

	info = get_wimgtool_info(window);

	count = DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0);
	for (i = 0; i < count; i++)
	{
		DragQueryFile(drop, i, buffer, ARRAY_LENGTH(buffer));
		std::string filename = osd::text::from_tstring(buffer);

		// figure out the file/dir name on the image
		subpath = string_format("%s%s",
			info->current_directory,
			core_filename_extract_base(filename));

		if (GetFileAttributes(buffer) & FILE_ATTRIBUTE_DIRECTORY)
			err = put_recursive_directory(*info->partition, buffer, subpath.c_str());
		else
			err = info->partition->put_file(subpath.c_str(), nullptr, filename.c_str(), nullptr, nullptr);
		if (err)
			goto done;
	}

done:
	refresh_image(window);
	if (err)
		wimgtool_report_error(window, err, nullptr, nullptr);
}



static imgtoolerr_t change_directory(HWND window, const char *dir)
{
	wimgtool_info *info = get_wimgtool_info(window);
	std::string new_current_dir = info->partition->path_concatenate(info->current_directory.c_str(), dir);
	info->current_directory = std::move(new_current_dir);
	return full_refresh_image(window);
}



static imgtoolerr_t double_click(HWND window)
{
	imgtoolerr_t err;
	wimgtool_info *info;
	LVHITTESTINFO htinfo;
	LVITEM item;
	POINTS pt;
	RECT r;
	DWORD pos;
	imgtool_dirent entry;
	int selected_item;
	HRESULT res;

	info = get_wimgtool_info(window);

	memset(&htinfo, 0, sizeof(htinfo));
	pos = GetMessagePos();
	pt = MAKEPOINTS(pos);
	GetWindowRect(info->listview, &r);
	htinfo.pt.x = pt.x - r.left;
	htinfo.pt.y = pt.y - r.top;
	res = ListView_HitTest(info->listview, &htinfo); res++;

	if (htinfo.flags & LVHT_ONITEM)
	{
		memset(&entry, 0, sizeof(entry));

		item.mask = LVIF_PARAM;
		item.iItem = htinfo.iItem;
		res = ListView_GetItem(info->listview, &item);

		selected_item = item.lParam;

		if (selected_item < 0)
		{
			strcpy(entry.filename, "..");
			entry.directory = 1;
		}
		else
		{
			err = info->partition->get_directory_entry(info->current_directory.c_str(), selected_item, entry);
			if (err)
				return err;
		}

		if (entry.directory)
		{
			err = change_directory(window, entry.filename);
			if (err)
				return err;
		}
	}
	return IMGTOOLERR_SUCCESS;
}



static BOOL context_menu(HWND window, LONG x, LONG y)
{
	wimgtool_info *info;
	LVHITTESTINFO hittest;
	BOOL rc = FALSE;
	HMENU menu;
	HRESULT res;

	info = get_wimgtool_info(window);

	memset(&hittest, 0, sizeof(hittest));
	hittest.pt.x = x;
	hittest.pt.y = y;
	ScreenToClient(info->listview, &hittest.pt);
	res = ListView_HitTest(info->listview, &hittest); res++;

	if (hittest.flags & LVHT_ONITEM)
	{
		menu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_FILECONTEXT_MENU));
		TrackPopupMenu(GetSubMenu(menu, 0), TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, window, nullptr);
		DestroyMenu(menu);
		rc = TRUE;
	}
	return rc;
}



struct selection_info
{
	unsigned int has_files : 1;
	unsigned int has_directories : 1;
};



static imgtoolerr_t init_menu_proc(HWND window, const imgtool_dirent *entry, void *param)
{
	struct selection_info *si;
	si = (struct selection_info *) param;
	if (entry->directory)
		si->has_directories = 1;
	else
		si->has_files = 1;
	return IMGTOOLERR_SUCCESS;
}



static void init_menu(HWND window, HMENU menu)
{
	wimgtool_info *info;
	imgtool_module_features module_features;
	imgtool_partition_features partition_features;
	struct selection_info si;
	unsigned int can_read, can_write, can_createdir, can_delete;
	LONG lvstyle;

	memset(&module_features, 0, sizeof(module_features));
	memset(&partition_features, 0, sizeof(partition_features));
	memset(&si, 0, sizeof(si));

	info = get_wimgtool_info(window);

	if (info->image)
	{
		module_features = imgtool_get_module_features(&info->image->module());
		partition_features = info->partition->get_features();
		foreach_selected_item(window, init_menu_proc, &si);
	}

	can_read      = partition_features.supports_reading && (si.has_files || si.has_directories);
	can_write     = partition_features.supports_writing && (info->open_mode != OSD_FOPEN_READ);
	can_createdir = partition_features.supports_createdir && (info->open_mode != OSD_FOPEN_READ);
	can_delete    = (si.has_files || si.has_directories)
						&& (!si.has_files || partition_features.supports_deletefile)
						&& (!si.has_directories || partition_features.supports_deletedir)
						&& (info->open_mode != OSD_FOPEN_READ);

	EnableMenuItem(menu, ID_IMAGE_INSERT,
		MF_BYCOMMAND | (can_write ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(menu, ID_IMAGE_EXTRACT,
		MF_BYCOMMAND | (can_read ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(menu, ID_IMAGE_CREATEDIR,
		MF_BYCOMMAND | (can_createdir ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(menu, ID_IMAGE_DELETE,
		MF_BYCOMMAND | (can_delete ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(menu, ID_IMAGE_VIEW,
		MF_BYCOMMAND | (can_read ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(menu, ID_IMAGE_SECTORVIEW,
		MF_BYCOMMAND | (module_features.supports_readsector ? MF_ENABLED : MF_GRAYED));

	lvstyle = GetWindowLong(info->listview, GWL_STYLE) & LVS_TYPEMASK;
	CheckMenuItem(menu, ID_VIEW_ICONS,
		MF_BYCOMMAND | (lvstyle == LVS_ICON) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_VIEW_LIST,
		MF_BYCOMMAND | (lvstyle == LVS_LIST) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_VIEW_DETAILS,
		MF_BYCOMMAND | (lvstyle == LVS_REPORT) ? MF_CHECKED : MF_UNCHECKED);
}



static LRESULT CALLBACK wimgtool_wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	wimgtool_info *info;
	RECT window_rect;
	RECT status_rect;
	int window_width;
	int window_height;
	int status_height;
	NMHDR *notify;
	POINT pt;
	LRESULT lres;
	HWND target_window;
	DWORD style;

	info = get_wimgtool_info(window);

	switch(message)
	{
		case WM_CREATE:
			if (wimgtool_create(window, (CREATESTRUCT *) lparam))
				return -1;
			break;

		case WM_DESTROY:
			wimgtool_destroy(window);
			break;

		case WM_SIZE:
			GetClientRect(window, &window_rect);
			GetClientRect(info->statusbar, &status_rect);

			window_width = window_rect.right - window_rect.left;
			window_height = window_rect.bottom - window_rect.top;
			status_height = status_rect.bottom - status_rect.top;

			SetWindowPos(info->listview, nullptr, 0, 0, window_width,
				window_height - status_height, SWP_NOMOVE | SWP_NOZORDER);
			SetWindowPos(info->statusbar, nullptr, 0, window_height - status_height,
				window_width, status_height, SWP_NOMOVE | SWP_NOZORDER);
			break;

		case WM_INITMENU:
			init_menu(window, (HMENU) wparam);
			break;

		case WM_DROPFILES:
			drop_files(window, (HDROP) wparam);
			break;

		case WM_COMMAND:
			switch(LOWORD(wparam))
			{
				case ID_FILE_NEW:
					menu_new(window);
					break;

				case ID_FILE_OPEN:
					menu_open(window);
					break;

				case ID_FILE_CLOSE:
					PostMessage(window, WM_CLOSE, 0, 0);
					break;

				case ID_IMAGE_INSERT:
					menu_insert(window);
					break;

				case ID_IMAGE_EXTRACT:
					menu_extract(window);
					break;

				case ID_IMAGE_CREATEDIR:
					menu_createdir(window);
					break;

				case ID_IMAGE_DELETE:
					menu_delete(window);
					break;

				case ID_IMAGE_VIEW:
					menu_view(window);
					break;

				case ID_IMAGE_SECTORVIEW:
					menu_sectorview(window);
					break;

				case ID_VIEW_ICONS:
					set_listview_style(window, LVS_ICON);
					break;

				case ID_VIEW_LIST:
					set_listview_style(window, LVS_LIST);
					break;

				case ID_VIEW_DETAILS:
					set_listview_style(window, LVS_REPORT);
					break;

				case ID_VIEW_ASSOCIATIONS:
					win_association_dialog(window);
					break;
			}
			break;

		case WM_NOTIFY:
			notify = (NMHDR *) lparam;
			switch(notify->code)
			{
				case NM_DBLCLK:
					double_click(window);
					break;

				case LVN_BEGINDRAG:
					pt.x = 8;
					pt.y = 8;

					lres = SendMessage(info->listview, LVM_CREATEDRAGIMAGE,
						(WPARAM) ((NM_LISTVIEW *) lparam)->iItem, (LPARAM) &pt);
					info->dragimage = (HIMAGELIST) lres;

					pt = ((NM_LISTVIEW *) notify)->ptAction;
					ClientToScreen(info->listview, &pt);

					ImageList_BeginDrag(info->dragimage, 0, 0, 0);
					ImageList_DragEnter(GetDesktopWindow(), pt.x, pt.y);
					SetCapture(window);
					info->dragpt = pt;
					break;
			}
			break;

		case WM_MOUSEMOVE:
			if (info->dragimage)
			{
				pt.x = LOWORD(lparam);
				pt.y = HIWORD(lparam);
				ClientToScreen(window, &pt);
				info->dragpt = pt;

				ImageList_DragMove(pt.x, pt.y);
			}
			break;

		case WM_LBUTTONUP:
			if (info->dragimage)
			{
				target_window = WindowFromPoint(info->dragpt);

				ImageList_DragLeave(info->listview);
				ImageList_EndDrag();
				ImageList_Destroy(info->dragimage);
				ReleaseCapture();
				info->dragimage = nullptr;
				info->dragpt.x = 0;
				info->dragpt.y = 0;

				style = GetWindowLong(target_window, GWL_EXSTYLE);
				if (style & WS_EX_ACCEPTFILES)
				{
				}
			}
			break;

		case WM_CONTEXTMENU:
			context_menu(window, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
			break;
	}

	return DefWindowProc(window, message, wparam, lparam);
}



BOOL wimgtool_registerclass(void)
{
	WNDCLASS wimgtool_wndclass;

	memset(&wimgtool_wndclass, 0, sizeof(wimgtool_wndclass));
	wimgtool_wndclass.lpfnWndProc = wimgtool_wndproc;
	wimgtool_wndclass.lpszClassName = wimgtool_class;
	wimgtool_wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_WIMGTOOL_MENU);
	wimgtool_wndclass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_IMGTOOL));
	return RegisterClass(&wimgtool_wndclass);
}
