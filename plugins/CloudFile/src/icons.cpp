#include "stdafx.h"

static IconItem iconList[] =
{
	{	LPGEN("Upload file(s)"),	"upload",	IDI_UPLOAD	},
	{	LPGEN("Dropbox"),			"dropbox",	IDI_DROPBOX	},
	{	LPGEN("GDrive"),			"gdrive",	IDI_GDRIVE	}
};

void InitializeIcons()
{
	Icon_Register(hInstance, "Protocols/" MODULE, iconList, _countof(iconList), MODULE);
}

HANDLE GetIconHandle(int iconId)
{
	for (int i = 0; i < _countof(iconList); i++)
		if (iconList[i].defIconID == iconId)
			return iconList[i].hIcolib;

	return NULL;
}

HANDLE GetIconHandle(const char *name)
{
	for (size_t i = 0; i < _countof(iconList); i++)
		if (mir_strcmpi(iconList[i].szName, name) == 0)
			return iconList[i].hIcolib;

	return NULL;
}

HICON LoadIconEx(int iconId, bool big)
{
	for (int i = 0; i < _countof(iconList); i++)
		if (iconList[i].defIconID == iconId)
			return IcoLib_GetIconByHandle(iconList[i].hIcolib, big);

	return NULL;
}