#ifndef __LATE_LOAD__H__
#define __LATE_LOAD__H__

#if _MSC_VER > 1000
#pragma once
#endif

/*
	Module : LateLoad.h
	Purpose: I was tired of constantly re-writing LoadLibrary & GetProcAddress code, 
	         over & over & over again.  So I created a series of macros that will 
	         define & create a wrapper class to do it all for me.
	         This is NOT an end-all-be-all solution for loading DLLs.  This is just a
	         handy lightweight helper when you would use LoadLibrary/GetProcAddress.
	         For more information on DLLs, check out MSDN.

	         This was inspired by my own need, as well as a nice little MSDN article 
	         titled "DLLs the Dynamic Way" by "MicHael Galkovsky"
	         http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndllpro/html/dlldynamic.asp
	         Unfortunatly, there was no source to go with the article. So, I wrote this 
	         version quite a bit differently.

	Author : Copyright (c) 2004 by Jason De Arte, All Rights Reserved.
	Site   : http://1001010.com/code/	         
	
	License: Use it however you like as a compiled portion of your product(s).  
	         Don't be a jerk by claiming the it as your own.

	Usage  : The following will declare the class "CUser32Wrapper" that will load
	         function pointer(s) from USER32.DLL.  Those function names will become 
	         member function(s) of the newly created class.

	LATELOAD_BEGIN_CLASS(CUser32Wrapper,user32,FALSE)
		LATELOAD_FUNC_0(NULL,HCURSOR,WINAPI,GetCursor)
		LATELOAD_FUNC_1(NULL,HANDLE,WINAPI,NonExistantFunction,BOOL)
	LATELOAD_END_CLASS()

	// The above statements will auto-generate a class that functionaly equivalent to...
	class CUser32Wrapper : public CLateLoadBase
	{
	public:
		BOOL Is_GetCursor();
		HCURSOR WINAPI GetCursor();
		BOOL Is_NonExistantFunction();
		HANDLE WINAPI NonExistantFunction();
	};

	// And usage is even easier
	CUser32Wrapper uw;
	HCURSOR h = uw.GetCursor();

	// And for DLLs missing a function export, you can still use the member function!
	uw.NonExistantFunction(true);
	// It will just return the default non-existant value in the definition
	
	// To determine if the funtion was imported, use the "BOOL Is_<FunctionName>();" member
	Is_NonExistantFunction();
	// If it exists, it will return TRUE.  False if it does not exist

	-------------------------------------------

	Let's look at the various defines that are used...

	LATELOAD_BEGIN_CLASS(CUser32Wrapper,user32,FALSE)
	 CUser32Wrapper - the class that will be created
	 user32 - the library that will be loaded 
	 FALSE - determines if "FreeLibrary()" will be called in the destructor.


	LATELOAD_FUNC_0(NULL,HCURSOR,WINAPI,GetCursor)
	 NULL - if the ProcAddress was not loaded, this value is returned in the call to that func
	 HCURSOR - the return type for the function
	 WINAPI - function calling convention
	 GetCursor - the exported function

	LATELOAD_FUNC_1 (up to _9)
	 Identical to LATELOAD_FUNC_0, except it allows 1-9 parameters to the function
	
	LATELOAD_FUNC_0_ (up to _9)
	 Same as before, but for use with NULL return types

	LATELOAD_END_CLASS()
	 finalizes the class declaration

	-------------------------------------------
	History:
		2004.Feb.29.JED - Created
*/

//
// Used for keeping track
//
enum ImportedProcState
{
	ipsUnknown = 0,  // No attempts to load the Proc have been made yet
	ipsMissing,      // GetProcAddress failed to find the exported function
	ipsAvailable,    // the Proc is Ready & Available for use
	ips_MAX
};

#ifndef NO_VTABLE
# if _MSC_VER > 1000
#  define NO_VTABLE __declspec(novtable)
# else
// 2004.Feb.29.JED - Note to self, find out what is appropriate under non-msvc compilers
//#  define NO_VTABLE
# endif //_MSC_VER
#endif

//
// Declaration for the utility base class, not intended for non-inherited use.
//
class NO_VTABLE CLateLoadBase
{
	//
	// Why no v-table?
	// 1) not needed
	// 2) reduces code size
	// 3) LATELOAD_BEGIN_CLASS calls ZeroMemory to blank out all the derived function 
	//    pointers & ImportedProcState memeber variables - in the process the vtable
	//    will get blanked out.
	// 4) This class is not intended to be instantiated on its own.
	// 5) Makes it so that using this class on its own will cause an Access Violation
	//
protected:
	HMODULE  m_module;          // Handle to the DLL
	DWORD    m_dwLoadLibError;  // If there was an error from LoadLibrary
	BOOL     m_bManaged;        // FreeLibrary in the destructor?
	LPTSTR   m_pszModule;       // The name of the module, handy for first-use loading

	/*****************************************************************************
	* NAME: 
	*  CLateLoadBase::dll_LoadLibrary
	* 
	* DESCRIPTION: 
	*  Loads the dll.  
	* 
	*******************************************************************************/
protected:
	BOOL dll_LoadLibrary(LPCTSTR pszLibrary, BOOL bDoItNow)
	{
		//
		// Make a record of the DLL name
		//
		if( !m_pszModule )
		{
			if( pszLibrary && *pszLibrary)
			{
				int nLen = lstrlen(pszLibrary); 
				m_pszModule = new TCHAR[nLen+2]; 
				if( m_pszModule ) 
				{ 
					ZeroMemory( m_pszModule, sizeof(TCHAR)*(nLen+2) ); 
					//lstrcpy(m_pszModule,pszLibrary); 
					lstrcpyn(m_pszModule, pszLibrary, nLen+2);
					
				} 
				else 
				{ 
					ASSERT(!"CLateLoadBase::dll_LoadLibrary - Unable to allocate memory for a string!"); 
				}
			} 
			else 
			{ 
				ASSERT(!"CLateLoadBase::dll_LoadLibrary - We need a valid pszLibrary string"); 
			} 
		}

		// Nothing to do?
		if( m_module )
			return TRUE;

		// Should we do this later?
		if( !bDoItNow )
			return FALSE;

		// Load the DLL
		m_dwLoadLibError = 0;
		m_module = ::LoadLibrary(pszLibrary);
		if( m_module )
			return TRUE;

		// Unable to load it, find out why
		m_dwLoadLibError = GetLastError();

		// We rely on the last error being not equal to zero on LoadLibrary failure.
		// So, in the off chance it IS zero, we set it to a non-zero value.  
		// Tip: According to MSDN, Bit 29 in Get/SetLastError logic is reserved for Application 
		//      defined error codes.  No system level error codes should have this bit set.
		if( !m_dwLoadLibError )
		{
			ASSERT(!"Wow, that should NOT have happened!  Could you tell JED how you did it?");
			m_dwLoadLibError = 0x20000000;
		}

		return FALSE;
	}

	/*****************************************************************************
	* NAME: 
	*  CLateLoadBase::dll_GetProcAddress
	* 
	* DESCRIPTION: 
	*  Loads the function pointer from the DLL
	* 
	*******************************************************************************/
protected:
	FARPROC dll_GetProcAddress(LPCSTR pszFunctionName, ImportedProcState& ips)
	{
		FARPROC pfn = NULL;
		ips = ipsUnknown;
		
		// Does the DLL still need to be loaded?
		if( !m_module && m_pszModule && *m_pszModule && 
		    0 == m_dwLoadLibError   // There is no reason to attempt to load the DLL more than once
		  )
		{
			dll_LoadLibrary(m_pszModule,TRUE);
		}

		if( m_module )
		{		
			pfn = ::GetProcAddress(m_module,pszFunctionName);
			if( pfn )
				ips = ipsAvailable;
			else
				ips = ipsMissing;
		}

		return pfn; 
	}


	/*****************************************************************************
	* NAME: 
	*  CLateLoadBase::~CLateLoadBase
	* 
	* DESCRIPTION: 
	*  Description goes here...
	* 
	*******************************************************************************/
public:
	~CLateLoadBase()
	{
		if( m_bManaged && m_module )
			::FreeLibrary(m_module);
		m_module  = NULL;
		m_bManaged = FALSE;
		if( m_pszModule )
			delete m_pszModule;
		m_pszModule = NULL;
	}

	/*****************************************************************************
	* NAME: 
	*  CLateLoadBase::dll_GetLoadError
	* 
	* DESCRIPTION: 
	*  If LoadLibrary failed, returns the error code.
	*  If there was no error, returns zero
	* 
	*******************************************************************************/
public:
	DWORD dll_GetLoadError()  
	{
		return m_dwLoadLibError;     
	}

	/*****************************************************************************
	* NAME: 
	*  CLateLoadBase::dll_IsLoaded
	* 
	* DESCRIPTION: 
	*  Is the DLL loaded?
	* 
	*******************************************************************************/
public:
	BOOL dll_IsLoaded()      
	{ 
		return NULL!=m_module;       
	}

	/*****************************************************************************
	* NAME: 
	*  CLateLoadBase::dll_GetModuleName
	* 
	* DESCRIPTION: 
	*  return the name of the module loaded
	* 
	*******************************************************************************/
public:
	LPCTSTR dll_GetModuleName() 
	{ 
		return (LPCTSTR)m_pszModule; 
	}
};


//--------------------------------------------------------------------------------------
//
// Start of macros
//
//--------------------------------------------------------------------------------------


//
// Start, Declares the name of the class
//
// ClassName   = the name of the class
// ModuleName  = the DLL name
// bLoadDllNow = if true, the DLL will be loaded in the constructor.  
//               if false, it will ONLY be loaded when any bound function is first used
// bManaged    = if true, FreeLibrary will be called in the destructor
//
#define LATELOAD_BEGIN_CLASS(ClassName,ModuleName,bLoadDllNow,bManaged) \
class ClassName : public CLateLoadBase \
{ \
public:\
	ClassName()\
	{\
		/*Automagicaly blank out all the function pointers and ImportedProcState member vars*/ \
		/*that will be declared in following LATELOAD_FUNC_* declarations, very handy. */ \
		ZeroMemory(static_cast<ClassName*>(this),sizeof(ClassName)); \
		m_bManaged = bManaged; \
		/*and load the DLL*/ \
		dll_LoadLibrary((LPCTSTR)#ModuleName,bLoadDllNow); \
	}


//
// Function Declaration, Zero Parameters, returns a value 
//
// ErrorResult, Default return value if the function could not be loaded & it is called anyways
// ReturnType,  type of value that the function returns
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
//
// A function prototype that looked like...
//   typedef BOOL (CALLBACK* SOMETHING)();
//  or
//   BOOL CALLBACK Something();
//
// Would be changed to...
//   LATELOAD_FUNC_0(0,BOOL,CALLBACK,Something)
//
// If "Something" could not be loaded, and it was called - it would return 0
//
#define LATELOAD_FUNC_0(ErrorResult,ReturnType,CallingConv,FuncName) \
protected: \
	typedef ReturnType(CallingConv * TYPE_##FuncName)(); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	ReturnType FuncName() \
	{\
		if( !Is_##FuncName() ) \
			return ErrorResult; \
		return m_pf##FuncName(); \
	}


//
// Function Declaration, One Parameter, returns a value 
//
// ErrorResult, Default return value if the function could not be loaded & it is called anyways
// ReturnType,  type of value that the function returns
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
// A function prototype that looked like...
//   typedef BOOL (CALLBACK* SOMETHING)(BOOL);
//  or
//   BOOL CALLBACK Something(BOOL bEnable);
//
// Would be changed to...
//   LATELOAD_FUNC_1(0,BOOL,CALLBACK,Something,BOOL)
//
// If "Something" could not be loaded, and it was called - it would return 0
//
#define LATELOAD_FUNC_1(ErrorResult,ReturnType,CallingConv,FuncName,ParamType1) \
protected: \
	typedef ReturnType(CallingConv * TYPE_##FuncName)(ParamType1); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	ReturnType FuncName(ParamType1 p1) \
	{ \
		if( !Is_##FuncName() ) \
			return (ReturnType) ErrorResult; \
		return (ReturnType) m_pf##FuncName(p1); \
	} \

//
// Function Declaration, Two Parameters, returns a value 
//
// ErrorResult, Default return value if the function could not be loaded & it is called anyways
// ReturnType,  type of value that the function returns
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
// A function prototype that looked like...
//   typedef BOOL (CALLBACK* SOMETHING)(BOOL,INT);
//  or
//   BOOL CALLBACK Something(BOOL bEnable, INT nNum);
//
// Would be changed to...
//   LATELOAD_FUNC_2(0,BOOL,CALLBACK,Something,BOOL,INT)
//
// If "Something" could not be loaded, and it was called - it would return 0
//
#define LATELOAD_FUNC_2(ErrorResult,ReturnType,CallingConv,FuncName,ParamType1,ParamType2) \
protected: \
	typedef ReturnType(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	ReturnType FuncName(ParamType1 p1,ParamType2 p2) \
	{\
		if( !Is_##FuncName() ) \
			return ErrorResult; \
		return m_pf##FuncName(p1,p2); \
	}


//
// Function Declaration, Three Parameters, returns a value 
//
// ErrorResult, Default return value if the function could not be loaded & it is called anyways
// ReturnType,  type of value that the function returns
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_3(ErrorResult,ReturnType,CallingConv,FuncName,ParamType1,ParamType2,ParamType3) \
protected: \
	typedef ReturnType(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	ReturnType FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3) \
	{\
		if( !Is_##FuncName() ) \
			return ErrorResult; \
		return m_pf##FuncName(p1,p2,p3); \
	}


//
// Function Declaration, Four Parameters, returns a value 
//
// ErrorResult, Default return value if the function could not be loaded & it is called anyways
// ReturnType,  type of value that the function returns
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_4(ErrorResult,ReturnType,CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4) \
protected: \
	typedef ReturnType(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	ReturnType FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4) \
	{\
		if( !Is_##FuncName() ) \
			return ErrorResult; \
		return m_pf##FuncName(p1,p2,p3,p4); \
	}


//
// Function Declaration, Five Parameters, returns a value 
//
// ErrorResult, Default return value if the function could not be loaded & it is called anyways
// ReturnType,  type of value that the function returns
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_5(ErrorResult,ReturnType,CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4,ParamType5) \
protected: \
	typedef ReturnType(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4,ParamType5); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	ReturnType FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4,ParamType5 p5) \
	{\
		if( !Is_##FuncName() ) \
			return ErrorResult; \
		return m_pf##FuncName(p1,p2,p3,p4,p5); \
	}


//
// Function Declaration, Six Parameters, returns a value 
//
// ErrorResult, Default return value if the function could not be loaded & it is called anyways
// ReturnType,  type of value that the function returns
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_6(ErrorResult,ReturnType,CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6) \
protected: \
	typedef ReturnType(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	ReturnType FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4,ParamType5 p5,ParamType6 p6) \
	{\
		if( !Is_##FuncName() ) \
			return ErrorResult; \
		return m_pf##FuncName(p1,p2,p3,p4,p5,p6); \
	}


//
// Function Declaration, Seven Parameters, returns a value 
//
// ErrorResult, Default return value if the function could not be loaded & it is called anyways
// ReturnType,  type of value that the function returns
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_7(ErrorResult,ReturnType,CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7) \
protected: \
	typedef ReturnType(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	ReturnType FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4,ParamType5 p5,ParamType6 p6,ParamType7 p7) \
	{\
		if( !Is_##FuncName() ) \
			return ErrorResult; \
		return m_pf##FuncName(p1,p2,p3,p4,p5,p6,p7); \
	}


//
// Function Declaration, Eight Parameters, returns a value 
//
// ErrorResult, Default return value if the function could not be loaded & it is called anyways
// ReturnType,  type of value that the function returns
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_8(ErrorResult,ReturnType,CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7,ParamType8) \
protected: \
	typedef ReturnType(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7,ParamType8); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	ReturnType FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4,ParamType5 p5,ParamType6 p6,ParamType7 p7,ParamType8 p8) \
	{\
		if( !Is_##FuncName() ) \
			return ErrorResult; \
		return m_pf##FuncName(p1,p2,p3,p4,p5,p6,p7,p8); \
	}


//
// Function Declaration, Nine Parameters, returns a value 
//
// ErrorResult, Default return value if the function could not be loaded & it is called anyways
// ReturnType,  type of value that the function returns
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_9(ErrorResult,ReturnType,CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7,ParamType8,ParamType9) \
protected: \
	typedef ReturnType(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7,ParamType8,ParamType9); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	ReturnType FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4,ParamType5 p5,ParamType6 p6,ParamType7 p7,ParamType8 p8,ParamType9 p9) \
	{\
		if( !Is_##FuncName() ) \
			return ErrorResult; \
		return m_pf##FuncName(p1,p2,p3,p4,p5,p6,p7,p8,p9); \
	}


//
// Function Declaration, Zero Parameters, returns nothing 
//
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
//
// A function prototype that looked like...
//   typedef VOID (CALLBACK* SOMETHING)();
//  or
//   VOID CALLBACK Something();
//
// Would be changed to...
//   LATELOAD_FUNC_0_VOID(CALLBACK,Something)
//
#define LATELOAD_FUNC_0_VOID(CallingConv,FuncName) \
protected: \
	typedef void(CallingConv * TYPE_##FuncName)(); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	void FuncName() \
	{\
		if( Is_##FuncName() ) \
			m_pf##FuncName(); \
	}


//
// Function Declaration, One Parameters, returns nothing 
//
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
// A function prototype that looked like...
//   typedef VOID (CALLBACK* SOMETHING)(BOOL);
//  or
//   VOID CALLBACK Something(BOOL bEnable);
//
// Would be changed to...
//   LATELOAD_FUNC_1_VOID(CALLBACK,Something,BOOL)
//
#define LATELOAD_FUNC_1_VOID(CallingConv,FuncName,ParamType1) \
protected: \
	typedef void(CallingConv * TYPE_##FuncName)(ParamType1); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	void FuncName(ParamType1 p1) \
	{\
		if( Is_##FuncName() ) \
			m_pf##FuncName(p1); \
	}


//
// Function Declaration, Two Parameters, returns nothing 
//
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
// A function prototype that looked like...
//   typedef VOID (CALLBACK* SOMETHING)(BOOL,INT);
//  or
//   VOID CALLBACK Something(BOOL bEnable, INT nNumber);
//
// Would be changed to...
//   LATELOAD_FUNC_2_VOID(CALLBACK,Something,BOOL,INT)
//
#define LATELOAD_FUNC_2_VOID(CallingConv,FuncName,ParamType1,ParamType2) \
protected: \
	typedef void(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	void FuncName(ParamType1 p1,ParamType2 p2) \
	{\
		if( Is_##FuncName() ) \
			m_pf##FuncName(p1,p2); \
	}


//
// Function Declaration, Three Parameters, returns nothing 
//
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_3_VOID(CallingConv,FuncName,ParamType1,ParamType2,ParamType3) \
protected: \
	typedef void(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	void FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3) \
	{\
		if( Is_##FuncName() ) \
			m_pf##FuncName(p1,p2,p3); \
	}


//
// Function Declaration, Four Parameters, returns nothing 
//
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_4_VOID(CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4) \
protected: \
	typedef void(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	void FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4) \
	{\
		if( Is_##FuncName() ) \
			m_pf##FuncName(p1,p2,p3,p4); \
	}


//
// Function Declaration, Five Parameters, returns nothing 
//
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_5_VOID(CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4,ParamType5) \
protected: \
	typedef void(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4,ParamType5); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	void FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4,ParamType5 p5) \
	{\
		if( Is_##FuncName() ) \
			m_pf##FuncName(p1,p2,p3,p4,p5); \
	}


//
// Function Declaration, Six Parameters, returns nothing 
//
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_6_VOID(CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6) \
protected: \
	typedef void(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	void FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4,ParamType5 p5,ParamType6 p6) \
	{\
		if( Is_##FuncName() ) \
			m_pf##FuncName(p1,p2,p3,p4,p5,p6); \
	}


//
// Function Declaration, Seven Parameters, returns nothing 
//
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_7_VOID(CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7) \
protected: \
	typedef void(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	void FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4,ParamType5 p5,ParamType6 p6,ParamType7 p7) \
	{\
		if( Is_##FuncName() ) \
			m_pf##FuncName(p1,p2,p3,p4,p5,p6,p7); \
	}


//
// Function Declaration, Eight Parameters, returns nothing 
//
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_8_VOID(CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7,ParamType8) \
protected: \
	typedef void(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7,ParamType8); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	void FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4,ParamType5 p5,ParamType6 p6,ParamType7 p7,ParamType8 p8) \
	{\
		if( Is_##FuncName() ) \
			m_pf##FuncName(p1,p2,p3,p4,p5,p6,p7,p8); \
	}


//
// Function Declaration, Nine Parameters, returns nothing 
//
// CallingConv, Calling convention of the function
// FuncName,    Name of the function
// ParamN       types of the function parameters
//
#define LATELOAD_FUNC_9_VOID(CallingConv,FuncName,ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7,ParamType8,ParamType9) \
protected: \
	typedef void(CallingConv * TYPE_##FuncName)(ParamType1,ParamType2,ParamType3,ParamType4,ParamType5,ParamType6,ParamType7,ParamType8,ParamType9); \
	TYPE_##FuncName m_pf##FuncName; \
	ImportedProcState m_ips##FuncName;\
public: \
	BOOL Is_##FuncName() \
	{ \
		if(ipsUnknown == m_ips##FuncName) \
			m_pf##FuncName = (TYPE_##FuncName)dll_GetProcAddress(#FuncName, m_ips##FuncName); \
		return(ipsAvailable == m_ips##FuncName); \
	} \
	void FuncName(ParamType1 p1,ParamType2 p2,ParamType3 p3,ParamType4 p4,ParamType5 p5,ParamType6 p6,ParamType7 p7,ParamType8 p8,ParamType9) \
	{\
		if( Is_##FuncName() ) \
			m_pf##FuncName(p1,p2,p3,p4,p5,p6,p7,p8,p9); \
	}



//
// End of the class
//
#define LATELOAD_END_CLASS()  };


#endif //__LATE_LOAD__H__
