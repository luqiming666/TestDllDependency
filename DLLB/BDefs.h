#pragma once

#ifdef  DLL_B_EXPORTS
#define DLL_B_API __declspec(dllexport)
#else
#define DLL_B_API __declspec(dllimport)
#endif
