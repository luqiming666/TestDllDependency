#ifndef DLL_CDLLBWrapper_H
#define DLL_CDLLBWrapper_H

// based on LateLoad DLL Wrapper by Jason De Arte
// http://www.codeproject.com/useritems/LateLoad.asp

#include "LateLoad.h"

LATELOAD_BEGIN_CLASS(CDLLBWrapper,	// the name of the class
	DLLB,			// the DLL name
	FALSE,			// FALSE = it will ONLY be loaded when 
				   // any bound function is first used
	TRUE)			// TRUE = FreeLibrary will be called 
				   // in the destructor

//
// Function Declaration, 3 Parameters, returns int 
//
LATELOAD_FUNC_3(-1, int, STDAPIVCALLTYPE, AddThreeNumbers, int, int, int)

LATELOAD_FUNC_2(-1, int, STDAPIVCALLTYPE, dllbapi_add, int, int)
LATELOAD_FUNC_2(-1, int, STDAPIVCALLTYPE, dllbapi_minus, int, int)

LATELOAD_END_CLASS()

#endif // DLL_CDLLBWrapper_H