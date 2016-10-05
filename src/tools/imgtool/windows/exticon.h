// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

	exticon.h

	Facility for providing Win32 icons for appropriate file extensions

***************************************************************************/

#pragma once

#ifndef WIMGTOOL_EXTICON_H
#define WIMGTOOL_EXTICON_H

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
#include <algorithm>
#include <map>

// imgtool headers
#include "imgtool.h"
#include "strconv.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

namespace imgtool
{
	namespace windows
	{
		class extension_icon_provider
		{
		public:
			extension_icon_provider();
			~extension_icon_provider();

			// accessors
			HIMAGELIST normal_icon_list() const { return m_normal.handle(); }
			HIMAGELIST small_icon_list() const { return m_small.handle(); }
			int directory_icon_index() const { return m_directory_icon_index; }

			int provide_icon_index(std::string &&extension);
			int provide_icon_index(const imgtool_iconinfo &icon);

		private:
			template<int size>
			class icon_list
			{
			public:
				icon_list();
				~icon_list();

				HIMAGELIST handle() const { return m_handle; }

			private:
				HIMAGELIST m_handle;
			};

			icon_list<32> m_normal;
			icon_list<16> m_small;
			std::map<std::string, int> m_map;
			int m_directory_icon_index;

			int append_associated_icon(const char *extension);
			template<int size> HICON create_icon(const UINT32 icon_data[size][size]);

			static osd::text::tstring win_get_temp_path();
		};
	};
};

#endif // WIMGTOOL_EXTICON_H
