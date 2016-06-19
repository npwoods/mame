// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  winutil.h - Win32 OSD core utility functions
//
//============================================================

#ifndef __WINUTIL__
#define __WINUTIL__

#include "osdcore.h"
#include <string>
#include <vector>

// Shared code
osd_dir_entry_type win_attributes_to_entry_type(DWORD attributes);
BOOL win_is_gui_application(void);
HMODULE WINAPI GetModuleHandleUni();

#endif // __WINUTIL__
