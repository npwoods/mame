//============================================================
//
//  fileview.h - A Win32 file viewer dialog
//
//============================================================

#ifndef FILEVIEW_H
#define FILEVIEW_H

#include <windows.h>
#include "../imgtool.h"

void win_fileview_dialog(HWND parent, std::string &&filename, const void *ptr, size_t size);

#endif // FILEVIEW_H
