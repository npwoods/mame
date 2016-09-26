//============================================================
//
//  opcntrl.h - Code for handling option resolutions in
//  Win32 controls
//
//============================================================

#ifndef OPCNTRL_H
#define OPCNTRL_H

#include <windows.h>
#include "opresolv.h"

BOOL win_prepare_option_control(HWND control, const util::option_guide *guide,
	const char *optspec);
BOOL win_check_option_control(HWND control);
BOOL win_adjust_option_control(HWND control, int delta);
util::option_resolution::error win_add_resolution_parameter(HWND control, util::option_resolution *resolution);

#endif // OPCNTRL_H
