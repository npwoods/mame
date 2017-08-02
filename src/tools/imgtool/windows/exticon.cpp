// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

	exticon.cpp

	Facility for providing Win32 icons for appropriate file extensions

***************************************************************************/

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0501
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wingdi.h>

// standard C++ headers
#include <string.h>
#include <tchar.h>

// MAME/wimgtool headers
#include "exticon.h"
#include "strconv.h"

namespace imgtool
{
	namespace windows
	{

//**************************************************************************
//  EXTENSION ICON PROVIDER
//**************************************************************************

static const char *FOLDER_ICON = (const char *)(ptrdiff_t)-1;

//-------------------------------------------------
//  ctor
//-------------------------------------------------

extension_icon_provider::extension_icon_provider()
{
	m_directory_icon_index = append_associated_icon(FOLDER_ICON);
}


//-------------------------------------------------
//  dtor
//-------------------------------------------------

extension_icon_provider::~extension_icon_provider()
{
}


//-------------------------------------------------
//  provide_icon_index
//-------------------------------------------------

int extension_icon_provider::provide_icon_index(std::string &&extension)
{
	// identify and normalize the file extension
	if (extension.empty())
		extension = ".bin";
	else
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);


	// try to find this icon
	int icon_index;
	auto iter = m_map.find(extension);
	if (iter != m_map.end())
	{
		// we've found it - use it
		icon_index = iter->second;
	}
	else
	{
		// we have not; create a new one
		icon_index = append_associated_icon(extension.c_str());
		m_map[extension] = icon_index;
	}
	return icon_index;
}


//-------------------------------------------------
//  win_get_temp_path
//-------------------------------------------------

osd::text::tstring extension_icon_provider::win_get_temp_path()
{
	osd::text::tstring buffer;
	const size_t buffer_size = MAX_PATH + 1;

	// reserve buffer space
	buffer.resize(buffer_size - 1);

	// retrieve temporary file path
	size_t length = GetTempPath(buffer_size, &buffer[0]);

	// resize appropriately
	buffer.resize(length);

	return buffer;
}


//-------------------------------------------------
//  append_associated_icon
//-------------------------------------------------

int extension_icon_provider::append_associated_icon(const char *extension)
{
	HANDLE file = INVALID_HANDLE_VALUE;
	WORD icon_index;
	int index = -1;

	// retrieve temporary file path
	osd::text::tstring file_path = win_get_temp_path();

	// if we have the folder icon, we're done - otherwise...
	if (extension != FOLDER_ICON)
	{
		// create bogus temporary file so that we can get the icon
		file_path += TEXT("tmp");
		if (extension)
			file_path += osd::text::to_tstring(extension);

		file = CreateFile(file_path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, 0, nullptr);
	}

	// extract the icon
	HICON icon = ExtractAssociatedIcon(GetModuleHandle(nullptr), (LPTSTR) file_path.c_str(), &icon_index);
	if (icon)
	{
		index = ImageList_AddIcon(m_normal.handle(), icon);
		ImageList_AddIcon(m_small.handle(), icon);
		DestroyIcon(icon);
	}

	// remote temporary file if we created one
	if (file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
		DeleteFile(file_path.c_str());
	}

	return index;
}


//-------------------------------------------------
//  provide_icon_index
//-------------------------------------------------

int extension_icon_provider::provide_icon_index(const imgtool_iconinfo &icon)
{
	int icon_index = -1;
	if (icon.icon16x16_specified || icon.icon32x32_specified)
	{
		HICON icon16x16 = icon.icon16x16_specified
			? create_icon<16>(icon.icon16x16)
			: nullptr;

		HICON icon32x32 = icon.icon32x32_specified
			? create_icon<32>(icon.icon32x32)
			: nullptr;

		icon_index = ImageList_AddIcon(m_normal.handle(), icon32x32 ? icon32x32 : icon16x16);
		ImageList_AddIcon(m_small.handle(), icon16x16 ? icon16x16 : icon32x32);
	}
	return icon_index;
}


//-------------------------------------------------
//  create_icon
//-------------------------------------------------

template<int size>
HICON extension_icon_provider::create_icon(const UINT32 icon_data[size][size])
{
	const int width = size;
	const int height = size;
	HDC dc = nullptr;
	HICON icon = nullptr;
	BYTE *color_bits, *mask_bits;
	HBITMAP color_bitmap = nullptr, mask_bitmap = nullptr;
	ICONINFO iconinfo;
	UINT32 pixel;
	UINT8 mask;
	int x, y;
	BITMAPINFO bmi;

	// we need a device context
	dc = CreateCompatibleDC(nullptr);
	if (!dc)
		goto done;

	// create foreground bitmap
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	color_bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void **)&color_bits, nullptr, 0);
	if (!color_bitmap)
		goto done;

	// create mask bitmap
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 1;
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	mask_bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void **)&mask_bits, nullptr, 0);
	if (!color_bitmap)
		goto done;

	// transfer data from our structure to the bitmaps
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			mask = 1 << (7 - (x % 8));
			pixel = icon_data[y][x];

			// foreground icon
			color_bits[((height - y - 1) * width + x) * 3 + 2] = (pixel >> 16);
			color_bits[((height - y - 1) * width + x) * 3 + 1] = (pixel >> 8);
			color_bits[((height - y - 1) * width + x) * 3 + 0] = (pixel >> 0);

			// mask
			if (pixel & 0x80000000)
				mask_bits[((height - y - 1) * width + x) / 8] &= ~mask;
			else
				mask_bits[((height - y - 1) * width + x) / 8] |= mask;
		}
	}

	// actually create the icon
	memset(&iconinfo, 0, sizeof(iconinfo));
	iconinfo.fIcon = TRUE;
	iconinfo.hbmColor = color_bitmap;
	iconinfo.hbmMask = mask_bitmap;
	icon = CreateIconIndirect(&iconinfo);

done:
	if (color_bitmap)
		DeleteObject(color_bitmap);
	if (mask_bitmap)
		DeleteObject(mask_bitmap);
	if (dc)
		DeleteDC(dc);
	return icon;
}


//-------------------------------------------------
//  icon_list::ctor
//-------------------------------------------------

template<int size>
extension_icon_provider::icon_list<size>::icon_list()
{
	m_handle = ImageList_Create(size, size, ILC_COLORDDB | ILC_MASK, 0, 0);
	if (!m_handle)
		throw std::bad_alloc();
}


//-------------------------------------------------
//  icon_list::dtor
//-------------------------------------------------

template<int size>
extension_icon_provider::icon_list<size>::~icon_list()
{
	ImageList_Destroy(m_handle);
	m_handle = nullptr;
}

};	// namespace windows
};	// namespace imgtool
