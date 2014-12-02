#include <cstdlib>
#include <cstring>
#include <iostream>


#include "ext.h"

namespace {
	Ext *extension;
};

#ifdef __GNUC__
	#include <dlfcn.h>
	#include <Poco/UnicodeConverter.h>
	//#include <stdio.h>
	// Code for GNU C compiler
	static void __attribute__((constructor))
	extension_init(void)
	{
		Dl_info dl_info;
		dladdr((void*)extension_init, &dl_info);

		std::string path(dl_info.dli_fname);
		std::wstring wpath;
		Poco::UnicodeConverter::toUTF16(path, wpath);
		extension = (new Ext(wpath));
	}

	static void __attribute__((destructor))
	extension_destroy(void)
	{
		extension->stop();
	}

	extern "C"
	{
		void RVExtension(char *output, int outputSize, const char *function); 
	};

	void RVExtension(char *output, int outputSize, const char *function)
	{
		outputSize -= 1;
		extension->callExtenion(output, outputSize, function);
	};

#elif _MSC_VER
	// Code for MSVC compiler
	#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
	#include <windows.h>

	BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
	{
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
			WCHAR path[MAX_PATH+1];
			GetModuleFileNameW (NULL, path, (MAX_PATH + 1));

			extension = new Ext(std::wstring(path));
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			//extension->stop();
			break;
		}
		return TRUE;
	}

	extern "C"
	{
		__declspec(dllexport) void __stdcall RVExtension(char *output, int outputSize, const char *function); 
	};

	void __stdcall RVExtension(char *output, int outputSize, const char *function)
	{
		outputSize -= 1;
		extension->callExtenion(output,outputSize,function);
	};
#endif
