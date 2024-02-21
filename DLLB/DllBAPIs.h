#pragma once

#include "BDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

	DLL_B_API int dllbapi_add(int a, int b);

	namespace MySpace {
		DLL_B_API int dllbapi_minus(int a, int b);
	}

#ifdef __cplusplus
};
#endif


namespace MySpace {
	DLL_B_API int dllbapi_power(int a, int b);
}