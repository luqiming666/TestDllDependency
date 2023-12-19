#pragma once

#ifdef  WIN32DLLC_EXPORTS
#define Win32DLLC_API __declspec(dllexport)
#else
#define Win32DLLC_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

	Win32DLLC_API void dllcapi_init();
	Win32DLLC_API void dllcapi_release();
	Win32DLLC_API int dllcapi_add_four_numbers(int a1, int a2, int a3, int a4);

#ifdef __cplusplus
};
#endif