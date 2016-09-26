//============================================================
//
//  wimgtool.h - Win32 GUI Imgtool
//
//============================================================

#ifndef WIMGTOOL_H
#define WIMGTOOL_H

#include <windows.h>
#include "../imgtool.h"

extern const TCHAR wimgtool_class[];
extern const char wimgtool_producttext[];

extern imgtool::library *library;

BOOL wimgtool_registerclass(void);

imgtoolerr_t wimgtool_open_image(HWND window, const imgtool_module *module,
	const std::string &filename, int read_or_write);
void wimgtool_report_error(HWND window, imgtoolerr_t err, const char *imagename, const char *filename);


#endif // WIMGTOOL_H
