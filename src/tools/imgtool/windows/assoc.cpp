#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <ctype.h>
#include <shlwapi.h>
#include "osdcomm.h"
#include "assoc.h"


static LONG reg_query_string(HKEY key, TCHAR *buf, DWORD buflen)
{
	LONG rc;
	DWORD type;

	buflen *= sizeof(*buf);
	rc = RegQueryValueEx(key, nullptr, nullptr, &type, (LPBYTE) buf, &buflen);
	if (rc != ERROR_SUCCESS)
		return rc;
	if (type != REG_SZ)
		return -1;
	return 0;
}



static void get_open_command(const struct win_association_info *assoc,
	TCHAR *buf, size_t buflen)
{
	int i;

	GetModuleFileName(GetModuleHandle(nullptr), buf, buflen);

	for (i = 0; buf[i]; i++)
		buf[i] = toupper(buf[i]);
	buf[i++] = ' ';
	_tcscpy(&buf[i], assoc->open_args);
}



BOOL win_association_exists(const struct win_association_info *assoc)
{
	BOOL rc = FALSE;
	TCHAR buf[1024];
	TCHAR expected[1024];
	HKEY key1 = nullptr;
	HKEY key2 = nullptr;

	// first check to see if the extension is there at all
	if (RegOpenKey(HKEY_CLASSES_ROOT, assoc->file_class, &key1))
		goto done;

	if (RegOpenKey(key1, TEXT("shell\\open\\command"), &key2))
		goto done;

	if (reg_query_string(key2, buf, ARRAY_LENGTH(buf)))
		goto done;

	get_open_command(assoc, expected, ARRAY_LENGTH(expected));
	rc = !_tcscmp(expected, buf);

done:
	if (key2)
		RegCloseKey(key2);
	if (key1)
		RegCloseKey(key1);
	return rc;
}



BOOL win_is_extension_associated(const struct win_association_info *assoc,
	LPCTSTR extension)
{
	HKEY key = nullptr;
	TCHAR buf[256];
	BOOL rc = FALSE;

	// first check to see if the extension is there at all
	if (!win_association_exists(assoc))
		goto done;

	if (RegOpenKey(HKEY_CLASSES_ROOT, extension, &key))
		goto done;

	if (reg_query_string(key, buf, ARRAY_LENGTH(buf)))
		goto done;

	rc = !_tcscmp(buf, assoc->file_class);

done:
	if (key)
		RegCloseKey(key);
	return rc;
}



BOOL win_associate_extension(const struct win_association_info *assoc,
	LPCTSTR extension, BOOL is_set)
{
	HKEY key1 = nullptr;
	HKEY key2 = nullptr;
	HKEY key3 = nullptr;
	HKEY key4 = nullptr;
	HKEY key5 = nullptr;
	DWORD disposition;
	TCHAR buf[1024];
	BOOL rc = FALSE;

	if (!is_set)
	{
		if (win_is_extension_associated(assoc, extension))
		{
			SHDeleteKey(HKEY_CLASSES_ROOT, extension);
		}
	}
	else
	{
		if (!win_association_exists(assoc))
		{
			if (RegCreateKeyEx(HKEY_CLASSES_ROOT, assoc->file_class, 0, nullptr, 0,
					KEY_ALL_ACCESS, nullptr, &key1, &disposition))
				goto done;
			if (RegCreateKeyEx(key1, TEXT("shell"), 0, nullptr, 0,
					KEY_ALL_ACCESS, nullptr, &key2, &disposition))
				goto done;
			if (RegCreateKeyEx(key2, TEXT("open"), 0, nullptr, 0,
					KEY_ALL_ACCESS, nullptr, &key3, &disposition))
				goto done;
			if (RegCreateKeyEx(key3, TEXT("command"), 0, nullptr, 0,
					KEY_ALL_ACCESS, nullptr, &key4, &disposition))
				goto done;

			get_open_command(assoc, buf, ARRAY_LENGTH(buf));
			if (RegSetValue(key4, nullptr, REG_SZ, buf, sizeof(buf)))
				goto done;
		}

		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, extension, 0, nullptr, 0,
				KEY_ALL_ACCESS, nullptr, &key5, &disposition))
			goto done;
		if (RegSetValue(key5, nullptr, REG_SZ, assoc->file_class,
				(_tcslen(assoc->file_class) + 1) * sizeof(TCHAR)))
			goto done;
	}

	rc = TRUE;

done:
	if (key5)
		RegCloseKey(key5);
	if (key4)
		RegCloseKey(key4);
	if (key3)
		RegCloseKey(key3);
	if (key2)
		RegCloseKey(key2);
	if (key1)
		RegCloseKey(key1);
	return rc;
}
