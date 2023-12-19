// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "Win32DllCAPIs.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void dllcapi_init()
{

}

void dllcapi_release()
{

}

int dllcapi_add_four_numbers(int a1, int a2, int a3, int a4)
{
    return a1 + a2 + a3 + a4;
}