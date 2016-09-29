//============================================================
//
//  wmain.cpp - Win32 GUI Imgtool main code
//
//============================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include "wimgtool.h"
#include "wimgres.h"
#include "hexview.h"
#include "../modules.h"
#include "winutf8.h"
#include "strconv.h"


//-------------------------------------------------
//  get_command_line_arguments
//-------------------------------------------------

static std::vector<std::string> get_command_line_arguments()
{
	int argc;
	LPWSTR *w_argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	std::vector<std::string> result(argc);
	for (int i = 0; i < argc; i++)
		result[i] = utf8_from_wstring(w_argv[i]);

	LocalFree(w_argv);
	return result;
}


//-------------------------------------------------
//  WinMain
//-------------------------------------------------

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE prev_instance,
	LPSTR command_line, int cmd_show)
{
	MSG msg;
	HWND window;
	BOOL b;
	int rc = -1;
	imgtoolerr_t err;
	HACCEL accel = nullptr;

	std::vector<std::string> argv = get_command_line_arguments();

	// initialize Windows classes
	InitCommonControls();
	if (!wimgtool_registerclass())
		goto done;
	if (!hexview_registerclass())
		goto done;

	// initialize the Imgtool library
	imgtool_init(TRUE, win_output_debug_string_utf8);

	// create the window
	window = CreateWindow(wimgtool_class, nullptr, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, nullptr, nullptr);
	if (!window)
		goto done;

#ifdef MAME_DEBUG
	// run validity checks and if appropriate, warn the user
	if (imgtool_validitychecks())
	{
		win_message_box_utf8(window,
			"Imgtool has failed its consistency checks; this build has problems",
			wimgtool_producttext, MB_OK);
	}
#endif

	// load image specified at the command line
	if (argv.size() >= 2)
	{
		err = wimgtool_open_image(window, nullptr, argv[1], OSD_FOPEN_RW);
		if (err)
			wimgtool_report_error(window, err, argv[1].c_str(), nullptr);
	}

	accel = LoadAccelerators(nullptr, MAKEINTRESOURCE(IDA_WIMGTOOL_MENU));

	// pump messages until the window is gone
	while(IsWindow(window))
	{
		b = GetMessage(&msg, nullptr, 0, 0);
		if (b <= 0)
		{
			window = nullptr;
		}
		else if (!TranslateAccelerator(window, accel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	rc = 0;

done:
	imgtool_exit();
	if (accel)
		DestroyAcceleratorTable(accel);
	return rc;
}
