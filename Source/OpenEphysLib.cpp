/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2013 Open Ephys

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <PluginInfo.h>
#include "RecordEngine/HDF5Recording.h"
#include "FileSource/KwikFileSource.h"
#include <string>
#ifdef WIN32
#include <Windows.h>
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif


using namespace Plugin;
#define NUM_PLUGINS 2

extern "C" EXPORT void getLibInfo(Plugin::LibraryInfo* info)
{
	info->apiVersion = PLUGIN_API_VER;
	info->name = "Kwik Format";
	info->libVersion = 1;
	info->numPlugins = NUM_PLUGINS;
}

extern "C" EXPORT int getPluginInfo(int index, Plugin::PluginInfo* info)
{
	switch (index)
	{
	case 0:
		info->type = Plugin::PLUGIN_TYPE_RECORD_ENGINE;
		info->recordEngine.name = "Kwik";
		info->recordEngine.creator = &(Plugin::createRecordEngine<HDF5Recording>);
		break;
	case 1:
		info->type = Plugin::PLUGIN_TYPE_FILE_SOURCE;
		info->fileSource.name = "Kwd file";
		info->fileSource.extensions = "kwd";
		info->fileSource.creator = &(Plugin::createFileSource<KWIKFileSource>);
		break;
	default:
		return -1;
	}

	return 0;
}

#ifdef WIN32
BOOL WINAPI DllMain(IN HINSTANCE hDllHandle,
	IN DWORD     nReason,
	IN LPVOID    Reserved)
{
	return TRUE;
}

#endif
