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
extern const char build_version[];

namespace
{
	class wimgtool_instance
	{
	public:
		wimgtool_instance(HWND window)
			: m_window(window), m_listview(nullptr), m_statusbar(nullptr), m_image(nullptr), m_partition(nullptr)
			, m_open_mode(0), m_readonly_icon(nullptr), m_dragimage(nullptr)
		{
		}

		~wimgtool_instance()
		{
			if (m_readonly_icon)
			{
				DestroyIcon(m_readonly_icon);
				m_readonly_icon = nullptr;
			}			
		}

		imgtoolerr_t open_image(const imgtool_module *module, const std::string &filename, int read_or_write);
		void report_error(imgtoolerr_t err, const char *imagename, const char *filename);
		static wimgtool_instance *get_instance(HWND hwnd);
		static LRESULT CALLBACK wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

	private:
		imgtoolerr_t foreach_selected_item(const std::function<imgtoolerr_t(const imgtool_dirent &)> &proc);
		imgtoolerr_t foreach_selected_item(imgtoolerr_t(wimgtool_instance::*proc)(const imgtool_dirent &));
		imgtoolerr_t append_dirent(int index, const imgtool_dirent &entry);
		std::string full_path_from_dirent(const imgtool_dirent &dirent);
		imgtoolerr_t refresh_image();
		imgtoolerr_t full_refresh_image();
		win_open_file_name setup_openfilename_struct(bool creating_file);
		void menu_new();
		void menu_open();
		void menu_insert();
		imgtoolerr_t menu_extract_proc(const imgtool_dirent &entry);
		void menu_extract();
		void menu_createdir();
		imgtoolerr_t menu_delete_proc(const imgtool_dirent &entry);
		void menu_delete();
		imgtoolerr_t menu_view_proc(const imgtool_dirent &entry);
		void menu_view();
		void menu_sectorview();
		void set_listview_style(DWORD style);
		LRESULT handle_create(const CREATESTRUCT &pcs);
		void drop_files(HDROP drop);
		imgtoolerr_t change_directory(const char *dir);
		imgtoolerr_t double_click();
		BOOL context_menu(LONG x, LONG y);
		void init_menu(HMENU menu);
		LRESULT wndproc(UINT message, WPARAM wparam, LPARAM lparam);

		HWND										m_window;
		HWND										m_listview;		// handle to list view
		HWND										m_statusbar;	// handle to the status bar
		imgtool::image::ptr							m_image;		// the currently loaded image
		imgtool::partition::ptr						m_partition;	// the currently loaded partition

		std::string									m_filename;
		int											m_open_mode;
		std::string									m_current_directory;

		imgtool::windows::extension_icon_provider	m_icon_provider;

		HICON										m_readonly_icon;

		HIMAGELIST									m_dragimage;
		POINT										m_dragpt;
	};
}


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


//-------------------------------------------------
//  get_instance
//-------------------------------------------------

wimgtool_instance *wimgtool_instance::get_instance(HWND window)
{
	return ((wimgtool_instance *)GetWindowLongPtr(window, GWLP_USERDATA));
}


//-------------------------------------------------
//  win_get_file_attributes_utf8
//-------------------------------------------------

static DWORD win_get_file_attributes_utf8(const char *filename)
{
	auto t_filename = osd::text::to_tstring(filename);
	return GetFileAttributes(t_filename.c_str());
}


//-------------------------------------------------
//  foreach_selected_item
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::foreach_selected_item(const std::function<imgtoolerr_t (const imgtool_dirent &)> &proc)
{
	imgtoolerr_t err;
	int selected_item;
	int selected_index = -1;
	LVITEM item;
	std::vector<imgtool_dirent> entries;
	HRESULT res;

	if (m_image)
	{
		do
		{
			do
			{
				selected_index = ListView_GetNextItem(m_listview, selected_index, LVIS_SELECTED);
				if (selected_index < 0)
				{
					selected_item = -1;
				}
				else
				{
					item.mask = LVIF_PARAM;
					item.iItem = selected_index;
					res = ListView_GetItem(m_listview, &item); res++;
					selected_item = item.lParam;
				}
			}
			while((selected_index >= 0) && (selected_item < 0));

			if (selected_item >= 0)
			{
				// retrieve the directory entry
				imgtool_dirent dirent;
				err = m_partition->get_directory_entry(m_current_directory.c_str(), selected_item, dirent);
				if (err)
					return err;

				// if we have a path, prepend the path
				if (!m_current_directory.empty())
				{
					// copy the full path back in
					std::string s = full_path_from_dirent(dirent);
					snprintf(dirent.filename, ARRAY_LENGTH(dirent.filename), "%s", s.c_str());
				}

				// append to list
				entries.push_back(std::move(dirent));
			}
		}
		while(selected_item >= 0);
	}

	// now that we have the list, call the callbacks
	for (auto &dirent : entries)
	{
		err = proc(dirent);
		if (err)
			return err;
	}
	return IMGTOOLERR_SUCCESS;
}


//-------------------------------------------------
//  foreach_selected_item
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::foreach_selected_item(imgtoolerr_t(wimgtool_instance::*proc)(const imgtool_dirent &))
{
	return foreach_selected_item([this, &proc](const imgtool_dirent &dirent)
	{
		return ((*this).*(proc))(dirent);
	});
}


//-------------------------------------------------
//  report_error
//-------------------------------------------------

void wimgtool_instance::report_error(imgtoolerr_t err, const char *imagename, const char *filename)
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
	win_message_box_utf8(m_window, message, wimgtool_producttext, MB_OK);
}


//-------------------------------------------------
//  append_dirent
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::append_dirent(int index, const imgtool_dirent &entry)
{
	LVITEM lvi;
	int new_index, column_index;
	TCHAR buffer[32];
	int icon_index = -1;

	imgtool_partition_features features = m_partition->get_features();

	// try to get a custom icon
	if (features.supports_geticoninfo)
	{
		imgtool_iconinfo iconinfo;
		std::string buf = full_path_from_dirent(entry);
		m_partition->get_icon_info(buf.c_str(), &iconinfo);

		icon_index = m_icon_provider.provide_icon_index(iconinfo);
	}

	if (icon_index < 0)
	{
		if (entry.directory)
		{
			icon_index = m_icon_provider.directory_icon_index();
		}
		else
		{
			// identify and normalize the file extension
			std::string extension = core_filename_extract_extension(entry.filename);
			icon_index = m_icon_provider.provide_icon_index(std::move(extension));
		}
	}

	osd::text::tstring t_entry_filename = osd::text::to_tstring(entry.filename);

	memset(&lvi, 0, sizeof(lvi));
	lvi.iItem = ListView_GetItemCount(m_listview);
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.pszText = (LPTSTR) t_entry_filename.c_str();
	lvi.lParam = index;

	// if we have an icon, use it
	if (icon_index >= 0)
	{
		lvi.mask |= LVIF_IMAGE;
		lvi.iImage = icon_index;
	}

	new_index = ListView_InsertItem(m_listview, &lvi);

	if (entry.directory)
	{
		_sntprintf(buffer, ARRAY_LENGTH(buffer), TEXT("<DIR>"));
	}
	else
	{
		// set the file size
		_sntprintf(buffer, ARRAY_LENGTH(buffer), TEXT("%I64u"), entry.filesize);
	}
	column_index = 1;
	ListView_SetItemText(m_listview, new_index, column_index++, buffer);

	// set creation time, if supported
	if (features.supports_creation_time)
	{
		if (entry.creation_time)
		{
			std::tm local_time = entry.creation_time.localtime();
			_sntprintf(buffer, ARRAY_LENGTH(buffer), _tasctime(&local_time));
			tstring_rtrim(buffer);
			ListView_SetItemText(m_listview, new_index, column_index, buffer);
		}
		column_index++;
	}

	// set last modified time, if supported
	if (features.supports_lastmodified_time)
	{
		if (entry.lastmodified_time != 0)
		{
			std::tm local_time = entry.lastmodified_time.localtime();
			_sntprintf(buffer, ARRAY_LENGTH(buffer), _tasctime(&local_time));
			tstring_rtrim(buffer);
			ListView_SetItemText(m_listview, new_index, column_index, buffer);
		}
		column_index++;
	}

	// set attributes and corruption notice
	if (entry.attr)
	{
		osd::text::tstring t_tempstr = osd::text::to_tstring(entry.attr);
		ListView_SetItemText(m_listview, new_index, column_index++, (LPTSTR) t_tempstr.c_str());
	}
	if (entry.corrupt)
	{
		ListView_SetItemText(m_listview, new_index, column_index++, (LPTSTR) TEXT("Corrupt"));
	}
	return IMGTOOLERR_SUCCESS;
}


//-------------------------------------------------
//  full_path_from_dirent
//-------------------------------------------------

std::string wimgtool_instance::full_path_from_dirent(const imgtool_dirent &dirent)
{
	char path_separator = (char)m_partition->get_info_int(IMGTOOLINFO_INT_PATH_SEPARATOR);
	return util::string_format("%s%c%s", m_current_directory, path_separator, dirent.filename);
}


//-------------------------------------------------
//  refresh_image
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::refresh_image()
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	imgtool::directory::ptr imageenum;
	std::string size_buf;
	imgtool_dirent entry;
	UINT64 filesize;
	int i;
	BOOL is_root_directory;
	imgtool_partition_features features;
	char path_separator;
	HRESULT res;
	osd::text::tstring tempstr;

	res = ListView_DeleteAllItems(m_listview); res++;

	if (m_image)
	{
		features = m_partition->get_features();

		is_root_directory = TRUE;
		if (!m_current_directory.empty())
		{
			for (i = 0; m_current_directory[i]; i++)
			{
				path_separator = (char) m_partition->get_info_int(IMGTOOLINFO_INT_PATH_SEPARATOR);
				if (m_current_directory[i] != path_separator)
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
			err = append_dirent(-1, entry);
			if (err)
				return err;
		}

		err = imgtool::directory::open(*m_partition, m_current_directory, imageenum);
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
				err = append_dirent(i++, entry);
				if (err)
					return err;
			}
		}
		while(!entry.eof);

		if (features.supports_freespace)
		{
			err = m_partition->get_free_space(filesize);
			if (err)
				return err;
			size_buf = util::string_format("%u bytes free", (unsigned) filesize);
		}
	}

	tempstr = osd::text::to_tstring(size_buf);
	SendMessage(m_statusbar, SB_SETTEXT, 2, (LPARAM) tempstr.c_str());

	return IMGTOOLERR_SUCCESS;
}


//-------------------------------------------------
//  full_refresh_image
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::full_refresh_image()
{
	LVCOLUMN col;
	int column_index = 0;
	int i;
	std::string buf;
	std::string imageinfo;
	TCHAR file_title_buf[MAX_PATH];
	const char *statusbar_text[2];
	imgtool_partition_features features;
	std::string basename;

	// get the modules and features
	if (m_partition)
		features = m_partition->get_features();
	else
		memset(&features, 0, sizeof(features));

	if (!m_filename.empty())
	{
		// get file title from Windows
		osd::text::tstring t_filename = osd::text::to_tstring(m_filename);
		GetFileTitle(t_filename.c_str(), file_title_buf, ARRAY_LENGTH(file_title_buf));
		std::string utf8_file_title = osd::text::from_tstring(file_title_buf);

		// get info from image
		if (m_image)
			imageinfo = m_image->info();

		// combine all of this into a title bar
		if (!m_current_directory.empty())
		{
			// has a current directory
			if (!imageinfo.empty())
			{
				buf = string_format("%s (\"%s\") - %s", utf8_file_title, imageinfo, m_current_directory);
			}
			else
			{
				buf = string_format("%s - %s", utf8_file_title, m_current_directory);
			}
		}
		else
		{
			// no current directory
			buf = util::string_format(
				!imageinfo.empty() ? "%s (\"%s\")" : "%s",
				utf8_file_title, imageinfo);
		}

		basename = core_filename_extract_base(m_filename);
		statusbar_text[0] = basename.c_str();
		statusbar_text[1] = m_image->module().description;
	}
	else
	{
		buf = util::string_format("%s %s", wimgtool_producttext, build_version);
		statusbar_text[0] = nullptr;
		statusbar_text[1] = nullptr;
	}

	win_set_window_text_utf8(m_window, buf.c_str());

	for (i = 0; i < ARRAY_LENGTH(statusbar_text); i++)
	{
		auto t_tempstr = statusbar_text[i]
			? std::make_unique<osd::text::tstring>(osd::text::to_tstring(statusbar_text[i]))
			: std::unique_ptr<osd::text::tstring>();

		SendMessage(m_statusbar, SB_SETTEXT, i, (LPARAM)(t_tempstr.get() ? t_tempstr->c_str() : nullptr));
	}

	// set the icon
	if (m_image && (m_open_mode == OSD_FOPEN_READ))
		SendMessage(m_statusbar, SB_SETICON, 0, (LPARAM) m_readonly_icon);
	else
		SendMessage(m_statusbar, SB_SETICON, 0, (LPARAM) 0);

	DragAcceptFiles(m_window, !m_filename.empty());

	// create the listview columns
	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 200;
	col.pszText = (LPTSTR) TEXT("Filename");
	if (ListView_InsertColumn(m_listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Size");
	col.fmt = LVCFMT_RIGHT;
	if (ListView_InsertColumn(m_listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	if (features.supports_creation_time)
	{
		col.mask = LVCF_TEXT | LVCF_WIDTH;
		col.cx = 160;
		col.pszText = (LPTSTR) TEXT("Creation time");
		if (ListView_InsertColumn(m_listview, column_index++, &col) < 0)
			return IMGTOOLERR_OUTOFMEMORY;
	}

	if (features.supports_lastmodified_time)
	{
		col.mask = LVCF_TEXT | LVCF_WIDTH;
		col.cx = 160;
		col.pszText = (LPTSTR) TEXT("Last modified time");
		if (ListView_InsertColumn(m_listview, column_index++, &col) < 0)
			return IMGTOOLERR_OUTOFMEMORY;
	}

	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Attributes");
	if (ListView_InsertColumn(m_listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	col.mask = LVCF_TEXT | LVCF_WIDTH;
	col.cx = 60;
	col.pszText = (LPTSTR) TEXT("Notes");
	if (ListView_InsertColumn(m_listview, column_index++, &col) < 0)
		return IMGTOOLERR_OUTOFMEMORY;

	// delete extraneous columns
	while(ListView_DeleteColumn(m_listview, column_index))
		;
	return refresh_image();
}


//-------------------------------------------------
//  setup_openfilename_struct
//-------------------------------------------------

win_open_file_name wimgtool_instance::setup_openfilename_struct(bool creating_file)
{
	const imgtool_module *default_module = nullptr;
	char *initial_dir = nullptr;
	char *dir_char;
	imgtool_module_features features;
	DWORD filter_index = 0, current_index = 0;
	int i;

	if (m_image)
		default_module = &m_image->module();

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
	win_open_file_name ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.flags = OFN_EXPLORER;
	ofn.owner = m_window;
	ofn.filter = filter.str();
	ofn.filter_index = filter_index;

	// can we specify an initial directory?
	if (!m_filename.empty())
	{
		// copy the filename into the filename structure
		ofn.filename = m_filename;

		// specify an initial directory
		initial_dir = (char*)alloca((strlen(m_filename.c_str()) + 1) * sizeof(*m_filename.c_str()));
		strcpy(initial_dir, m_filename.c_str());
		dir_char = strrchr(initial_dir, '\\');
		if (dir_char)
			dir_char[1] = '\0';
		else
			initial_dir = nullptr;
		ofn.initial_directory = initial_dir;
	}

	return ofn;
}


//-------------------------------------------------
//  find_filter_module
//-------------------------------------------------

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


//-------------------------------------------------
//  win_mkdir
//-------------------------------------------------

osd_file::error win_mkdir(const char *dir)
{
	osd::text::tstring tempstr = osd::text::to_tstring(dir);

	return !CreateDirectory(tempstr.c_str(), nullptr)
		? win_error_to_file_error(GetLastError())
		: osd_file::error::NONE;
}


//-------------------------------------------------
//  get_recursive_directory
//-------------------------------------------------

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


//-------------------------------------------------
//  put_recursive_directory
//-------------------------------------------------

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


//-------------------------------------------------
//  open_image
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::open_image(const imgtool_module *module, const std::string &filename, int read_or_write)
{
	imgtoolerr_t err;
	imgtool::image::ptr image;
	imgtool::partition::ptr partition;
	imgtool_module *identified_module;
	int partition_index = 0;
	const char *root_path;
	imgtool_module_features features = { 0, };

	// if the module is not specified, auto detect the format
	if (!module)
	{
		err = imgtool::image::identify_file(filename.c_str(), &identified_module, 1);
		if (err)
			return err;
		module = identified_module;
	}

	// check to see if this module actually supports writing
	if (read_or_write != OSD_FOPEN_READ)
	{
		features = imgtool_get_module_features(module);
		if (features.is_read_only)
			read_or_write = OSD_FOPEN_READ;
	}

	m_filename = filename;

	// try to open the image
	err = imgtool::image::open(module, filename, read_or_write, image);
	if ((ERRORCODE(err) == IMGTOOLERR_READONLY) && read_or_write)
	{
		// if we failed when open a read/write image, try again
		read_or_write = OSD_FOPEN_READ;
		err = imgtool::image::open(module, filename, read_or_write, image);
	}
	if (err)
		return err;

	// try to open the partition
	err = imgtool::partition::open(*image, partition_index, partition);
	if (err)
		return err;

	// unload current image and partition
	m_image = std::move(image);
	m_partition = std::move(partition);
	m_open_mode = read_or_write;
	m_current_directory.clear();

	// do we support directories?
	if (m_partition->get_features().supports_directories)
	{
		root_path = m_partition->get_root_path();
		m_current_directory.assign(root_path);
	}

	// refresh the window
	full_refresh_image();

	return IMGTOOLERR_SUCCESS;
}


//-------------------------------------------------
//  menu_new
//-------------------------------------------------

void wimgtool_instance::menu_new()
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const imgtool_module *module;
	std::unique_ptr<util::option_resolution> resolution;

	win_open_file_name ofn = setup_openfilename_struct(true);
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

	err = open_image(module, ofn.filename, OSD_FOPEN_RW);
	if (err)
		goto done;

done:
	if (err)
		report_error(err, ofn.filename.c_str(), nullptr);
}


//-------------------------------------------------
//  menu_new
//-------------------------------------------------

void wimgtool_instance::menu_open()
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	const imgtool_module *module;
	int read_or_write;

	win_open_file_name ofn = setup_openfilename_struct(false);
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

	err = open_image(module, ofn.filename, read_or_write);
	if (err)
		goto done;

done:
	if (err)
		report_error(err, ofn.filename.c_str(), nullptr);
}


//-------------------------------------------------
//  menu_insert
//-------------------------------------------------

void wimgtool_instance::menu_insert()
{
	imgtoolerr_t err;
	char *image_filename = nullptr;
	win_open_file_name ofn;
	std::unique_ptr<util::option_resolution> opts;
	BOOL cancel;
	const char *fork = nullptr;
	struct transfer_suggestion_info suggestion_info;
	int use_suggestion_info;
	imgtool::stream::ptr stream;
	filter_getinfoproc filter = nullptr;
	const util::option_guide *writefile_optguide;
	const char *writefile_optspec;
	std::string basename;

	memset(&ofn, 0, sizeof(ofn));
	ofn.type = WIN_FILE_DIALOG_OPEN;
	ofn.owner = m_window;
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

	//module = m_image->module();

	/* figure out which filters are appropriate for this file */
	m_partition->suggest_file_filters(nullptr, stream.get(), suggestion_info.suggestions,
		ARRAY_LENGTH(suggestion_info.suggestions));

	/* do we need to show an option dialog? */
	writefile_optguide = (const util::option_guide *) m_partition->get_info_ptr(IMGTOOLINFO_PTR_WRITEFILE_OPTGUIDE);
	writefile_optspec = m_partition->get_info_string(IMGTOOLINFO_STR_WRITEFILE_OPTSPEC);
	if (suggestion_info.suggestions[0].viability || (writefile_optguide && writefile_optspec))
	{
		use_suggestion_info = (suggestion_info.suggestions[0].viability != SUGGESTION_END);
		err = win_show_option_dialog(m_window, use_suggestion_info ? &suggestion_info : nullptr,
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
	if (!m_current_directory.empty())
	{
		image_filename = (char *)m_partition->path_concatenate(m_current_directory.c_str(), image_filename);
	}

	err = m_partition->write_file(image_filename, fork, *stream, opts.get(), filter);
	if (err)
		goto done;

	err = refresh_image();
	if (err)
		goto done;

done:
	if (err)
		report_error(err, image_filename, ofn.filename.c_str());
}


//-------------------------------------------------
//  extract_dialog_hook
//-------------------------------------------------

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


//-------------------------------------------------
//  menu_extract_proc
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::menu_extract_proc(const imgtool_dirent &entry)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	win_open_file_name ofn;
	const char *filename;
	const char *image_basename;
	const char *fork;
	struct transfer_suggestion_info suggestion_info;
	int i;
	filter_getinfoproc filter;

	filename = entry.filename;

	// figure out a suggested host filename
	image_basename = m_partition->get_base_name(entry.filename);

	// try suggesting some filters (only if doing a single file)
	if (!entry.directory)
	{
		m_partition->suggest_file_filters(filename, nullptr, suggestion_info.suggestions,
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
	ofn.owner = m_window;
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

	if (entry.directory)
	{
		err = get_recursive_directory(*m_partition, filename, ofn.filename.c_str());
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

		err = m_partition->get_file(filename, fork, ofn.filename.c_str(), filter);
		if (err)
			goto done;
	}

done:
	if (err)
		report_error(err, filename, ofn.filename.c_str());
	return err;
}


//-------------------------------------------------
//  menu_extract_proc
//-------------------------------------------------

void wimgtool_instance::menu_extract()
{
	foreach_selected_item(&wimgtool_instance::menu_extract_proc);
}


//-------------------------------------------------
//  createdir_dialog_proc
//-------------------------------------------------

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


//-------------------------------------------------
//  menu_createdir
//-------------------------------------------------

void wimgtool_instance::menu_createdir()
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	struct createdir_dialog_info cdi;
	char *s;
	std::string utf8_dirname;

	memset(&cdi, 0, sizeof(cdi));
	DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_CREATEDIR),
		m_window, createdir_dialog_proc, (LPARAM) &cdi);

	if (cdi.buf[0] == '\0')
		goto done;
	utf8_dirname = osd::text::from_tstring(cdi.buf);

	if (!m_current_directory.empty())
	{
		s = (char *)m_partition->path_concatenate(m_current_directory.c_str(), utf8_dirname.c_str());
		utf8_dirname.assign(s);
	}

	err = m_partition->create_directory(utf8_dirname.c_str());
	if (err)
		goto done;

	err = refresh_image();
	if (err)
		goto done;

done:
	if (err)
		report_error(err, nullptr, utf8_dirname.c_str());
}


//-------------------------------------------------
//  menu_delete_proc
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::menu_delete_proc(const imgtool_dirent &entry)
{
	imgtoolerr_t err;

	if (entry.directory)
		err = m_partition->delete_directory(entry.filename);
	else
		err = m_partition->delete_file(entry.filename);
	if (err)
		goto done;

done:
	if (err)
		report_error(err, nullptr, entry.filename);
	return err;
}


//-------------------------------------------------
//  menu_delete
//-------------------------------------------------

void wimgtool_instance::menu_delete()
{
	imgtoolerr_t err;

	foreach_selected_item(&wimgtool_instance::menu_delete_proc);

	err = refresh_image();
	if (err)
		report_error(err, nullptr, nullptr);
}


//-------------------------------------------------
//  menu_view_proc
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::menu_view_proc(const imgtool_dirent &entry)
{
	imgtoolerr_t err;

	// read the file
	imgtool::stream::ptr stream = imgtool::stream::open_mem(nullptr, 0);
	err = m_partition->read_file(entry.filename, nullptr, *stream, nullptr);
	if (err)
	{
		report_error(err, nullptr, entry.filename);
		return err;
	}

	win_fileview_dialog(m_window, entry.filename, stream->getptr(), stream->size());
	return IMGTOOLERR_SUCCESS;
}


//-------------------------------------------------
//  menu_view
//-------------------------------------------------

void wimgtool_instance::menu_view()
{
	foreach_selected_item(&wimgtool_instance::menu_view_proc);
}


//-------------------------------------------------
//  menu_view
//-------------------------------------------------

void wimgtool_instance::menu_sectorview()
{
	win_sectorview_dialog(m_window, m_image.get());
}


//-------------------------------------------------
//  set_listview_style
//-------------------------------------------------

void wimgtool_instance::set_listview_style(DWORD style)
{
	style &= LVS_TYPEMASK;
	style |= (GetWindowLong(m_listview, GWL_STYLE) & ~LVS_TYPEMASK);
	SetWindowLong(m_listview, GWL_STYLE, style);
}


//-------------------------------------------------
//  handle_create
//-------------------------------------------------

LRESULT wimgtool_instance::handle_create(const CREATESTRUCT &pcs)
{
	static const int status_widths[3] = { 200, 400, -1 };

	// create the list view
	m_listview = CreateWindow(WC_LISTVIEW, nullptr,
		WS_VISIBLE | WS_CHILD, 0, 0, pcs.cx, pcs.cy, m_window, nullptr, nullptr, nullptr);
	if (!m_listview)
		return -1;
	set_listview_style(LVS_REPORT);

	// create the status bar
	m_statusbar = CreateWindow(STATUSCLASSNAME, nullptr, WS_VISIBLE | WS_CHILD,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_window, nullptr, nullptr, nullptr);
	if (!m_statusbar)
		return -1;
	SendMessage(m_statusbar, SB_SETPARTS, ARRAY_LENGTH(status_widths),
		(LPARAM) status_widths);

	// create imagelists
	(void)ListView_SetImageList(m_listview, m_icon_provider.normal_icon_list(), LVSIL_NORMAL);
	(void)ListView_SetImageList(m_listview, m_icon_provider.small_icon_list(), LVSIL_SMALL);

	// get icons
	m_readonly_icon = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_READONLY), IMAGE_ICON, 16, 16, 0);

	full_refresh_image();
	return 0;
}


//-------------------------------------------------
//  drop_files
//-------------------------------------------------

void wimgtool_instance::drop_files(HDROP drop)
{
	UINT count, i;
	TCHAR buffer[MAX_PATH];
	std::string subpath;
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;

	count = DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0);
	for (i = 0; i < count; i++)
	{
		DragQueryFile(drop, i, buffer, ARRAY_LENGTH(buffer));
		std::string filename = osd::text::from_tstring(buffer);

		// figure out the file/dir name on the image
		subpath = string_format("%s%s",
			m_current_directory,
			core_filename_extract_base(filename));

		if (GetFileAttributes(buffer) & FILE_ATTRIBUTE_DIRECTORY)
			err = put_recursive_directory(*m_partition, buffer, subpath.c_str());
		else
			err = m_partition->put_file(subpath.c_str(), nullptr, filename.c_str(), nullptr, nullptr);
		if (err)
			goto done;
	}

done:
	refresh_image();
	if (err)
		report_error(err, nullptr, nullptr);
}


//-------------------------------------------------
//  change_directory
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::change_directory(const char *dir)
{
	std::string new_current_dir = m_partition->path_concatenate(m_current_directory.c_str(), dir);
	m_current_directory = std::move(new_current_dir);
	return full_refresh_image();
}


//-------------------------------------------------
//  double_click
//-------------------------------------------------

imgtoolerr_t wimgtool_instance::double_click()
{
	imgtoolerr_t err;
	LVHITTESTINFO htinfo;
	LVITEM item;
	POINTS pt;
	RECT r;
	DWORD pos;
	imgtool_dirent entry;
	int selected_item;
	HRESULT res;

	memset(&htinfo, 0, sizeof(htinfo));
	pos = GetMessagePos();
	pt = MAKEPOINTS(pos);
	GetWindowRect(m_listview, &r);
	htinfo.pt.x = pt.x - r.left;
	htinfo.pt.y = pt.y - r.top;
	res = ListView_HitTest(m_listview, &htinfo); res++;

	if (htinfo.flags & LVHT_ONITEM)
	{
		memset(&entry, 0, sizeof(entry));

		item.mask = LVIF_PARAM;
		item.iItem = htinfo.iItem;
		res = ListView_GetItem(m_listview, &item);

		selected_item = item.lParam;

		if (selected_item < 0)
		{
			strcpy(entry.filename, "..");
			entry.directory = 1;
		}
		else
		{
			err = m_partition->get_directory_entry(m_current_directory.c_str(), selected_item, entry);
			if (err)
				return err;
		}

		if (entry.directory)
		{
			err = change_directory(entry.filename);
			if (err)
				return err;
		}
	}
	return IMGTOOLERR_SUCCESS;
}


//-------------------------------------------------
//  context_menu
//-------------------------------------------------

BOOL wimgtool_instance::context_menu(LONG x, LONG y)
{
	LVHITTESTINFO hittest;
	BOOL rc = FALSE;
	HMENU menu;
	HRESULT res;

	memset(&hittest, 0, sizeof(hittest));
	hittest.pt.x = x;
	hittest.pt.y = y;
	ScreenToClient(m_listview, &hittest.pt);
	res = ListView_HitTest(m_listview, &hittest); res++;

	if (hittest.flags & LVHT_ONITEM)
	{
		menu = LoadMenu(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDR_FILECONTEXT_MENU));
		TrackPopupMenu(GetSubMenu(menu, 0), TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, m_window, nullptr);
		DestroyMenu(menu);
		rc = TRUE;
	}
	return rc;
}


//-------------------------------------------------
//  init_menu
//-------------------------------------------------

void wimgtool_instance::init_menu(HMENU menu)
{
	imgtool_module_features module_features;
	imgtool_partition_features partition_features;
	bool has_files = false;
	bool has_directories = false;
	unsigned int can_read, can_write, can_createdir, can_delete;
	LONG lvstyle;

	memset(&module_features, 0, sizeof(module_features));
	memset(&partition_features, 0, sizeof(partition_features));

	if (m_image)
	{
		module_features = imgtool_get_module_features(&m_image->module());
		partition_features = m_partition->get_features();
		foreach_selected_item([&has_directories, &has_files](const imgtool_dirent &entry)
		{
			if (entry.directory)
				has_directories = true;
			else
				has_files = true;
			return IMGTOOLERR_SUCCESS;
		});
	}

	can_read      = partition_features.supports_reading && (has_files || has_directories);
	can_write     = partition_features.supports_writing && (m_open_mode != OSD_FOPEN_READ);
	can_createdir = partition_features.supports_createdir && (m_open_mode != OSD_FOPEN_READ);
	can_delete    = (has_files || has_directories)
						&& (!has_files || partition_features.supports_deletefile)
						&& (!has_directories || partition_features.supports_deletedir)
						&& (m_open_mode != OSD_FOPEN_READ);

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

	lvstyle = GetWindowLong(m_listview, GWL_STYLE) & LVS_TYPEMASK;
	CheckMenuItem(menu, ID_VIEW_ICONS,
		MF_BYCOMMAND | (lvstyle == LVS_ICON) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_VIEW_LIST,
		MF_BYCOMMAND | (lvstyle == LVS_LIST) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_VIEW_DETAILS,
		MF_BYCOMMAND | (lvstyle == LVS_REPORT) ? MF_CHECKED : MF_UNCHECKED);
}


//-------------------------------------------------
//  wndproc
//-------------------------------------------------

LRESULT wimgtool_instance::wndproc(UINT message, WPARAM wparam, LPARAM lparam)
{
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

	switch(message)
	{
		case WM_CREATE:
			if (handle_create(*((CREATESTRUCT *) lparam)))
				return -1;
			break;

		case WM_DESTROY:
			delete this;
			break;

		case WM_SIZE:
			GetClientRect(m_window, &window_rect);
			GetClientRect(m_statusbar, &status_rect);

			window_width = window_rect.right - window_rect.left;
			window_height = window_rect.bottom - window_rect.top;
			status_height = status_rect.bottom - status_rect.top;

			SetWindowPos(m_listview, nullptr, 0, 0, window_width,
				window_height - status_height, SWP_NOMOVE | SWP_NOZORDER);
			SetWindowPos(m_statusbar, nullptr, 0, window_height - status_height,
				window_width, status_height, SWP_NOMOVE | SWP_NOZORDER);
			break;

		case WM_INITMENU:
			init_menu((HMENU) wparam);
			break;

		case WM_DROPFILES:
			drop_files((HDROP) wparam);
			break;

		case WM_COMMAND:
			switch(LOWORD(wparam))
			{
				case ID_FILE_NEW:
					menu_new();
					break;

				case ID_FILE_OPEN:
					menu_open();
					break;

				case ID_FILE_CLOSE:
					PostMessage(m_window, WM_CLOSE, 0, 0);
					break;

				case ID_IMAGE_INSERT:
					menu_insert();
					break;

				case ID_IMAGE_EXTRACT:
					menu_extract();
					break;

				case ID_IMAGE_CREATEDIR:
					menu_createdir();
					break;

				case ID_IMAGE_DELETE:
					menu_delete();
					break;

				case ID_IMAGE_VIEW:
					menu_view();
					break;

				case ID_IMAGE_SECTORVIEW:
					menu_sectorview();
					break;

				case ID_VIEW_ICONS:
					set_listview_style(LVS_ICON);
					break;

				case ID_VIEW_LIST:
					set_listview_style(LVS_LIST);
					break;

				case ID_VIEW_DETAILS:
					set_listview_style(LVS_REPORT);
					break;

				case ID_VIEW_ASSOCIATIONS:
					win_association_dialog(m_window);
					break;
			}
			break;

		case WM_NOTIFY:
			notify = (NMHDR *) lparam;
			switch(notify->code)
			{
				case NM_DBLCLK:
					double_click();
					break;

				case LVN_BEGINDRAG:
					pt.x = 8;
					pt.y = 8;

					lres = SendMessage(m_listview, LVM_CREATEDRAGIMAGE,
						(WPARAM) ((NM_LISTVIEW *) lparam)->iItem, (LPARAM) &pt);
					m_dragimage = (HIMAGELIST) lres;

					pt = ((NM_LISTVIEW *) notify)->ptAction;
					ClientToScreen(m_listview, &pt);

					ImageList_BeginDrag(m_dragimage, 0, 0, 0);
					ImageList_DragEnter(GetDesktopWindow(), pt.x, pt.y);
					SetCapture(m_window);
					m_dragpt = pt;
					break;
			}
			break;

		case WM_MOUSEMOVE:
			if (m_dragimage)
			{
				pt.x = LOWORD(lparam);
				pt.y = HIWORD(lparam);
				ClientToScreen(m_window, &pt);
				m_dragpt = pt;

				ImageList_DragMove(pt.x, pt.y);
			}
			break;

		case WM_LBUTTONUP:
			if (m_dragimage)
			{
				target_window = WindowFromPoint(m_dragpt);

				ImageList_DragLeave(m_listview);
				ImageList_EndDrag();
				ImageList_Destroy(m_dragimage);
				ReleaseCapture();
				m_dragimage = nullptr;
				m_dragpt.x = 0;
				m_dragpt.y = 0;

				style = GetWindowLong(target_window, GWL_EXSTYLE);
				if (style & WS_EX_ACCEPTFILES)
				{
				}
			}
			break;

		case WM_CONTEXTMENU:
			context_menu(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
			break;
	}

	return DefWindowProc(m_window, message, wparam, lparam);
}


//-------------------------------------------------
//  wndproc
//-------------------------------------------------

LRESULT CALLBACK wimgtool_instance::wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	wimgtool_instance *info = get_instance(window);
	if (!info)
	{
		info = new wimgtool_instance(window);
		SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR)info);
	}
	return info->wndproc(message, wparam, lparam);
}


//-------------------------------------------------
//  wimgtool_registerclass
//-------------------------------------------------

BOOL wimgtool_registerclass(void)
{
	WNDCLASS wimgtool_wndclass;

	memset(&wimgtool_wndclass, 0, sizeof(wimgtool_wndclass));
	wimgtool_wndclass.lpfnWndProc = wimgtool_instance::wndproc;
	wimgtool_wndclass.lpszClassName = wimgtool_class;
	wimgtool_wndclass.lpszMenuName = MAKEINTRESOURCE(IDR_WIMGTOOL_MENU);
	wimgtool_wndclass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_IMGTOOL));
	return RegisterClass(&wimgtool_wndclass);
}


//-------------------------------------------------
//  wimgtool_open_image
//-------------------------------------------------

imgtoolerr_t wimgtool_open_image(HWND window, const imgtool_module *module,
	const std::string &filename, int read_or_write)
{
	return wimgtool_instance::get_instance(window)->open_image(module, filename, read_or_write);
}


//-------------------------------------------------
//  wimgtool_report_error
//-------------------------------------------------

void wimgtool_report_error(HWND window, imgtoolerr_t err, const char *imagename, const char *filename)
{
	wimgtool_instance::get_instance(window)->report_error(err, imagename, filename);
}
