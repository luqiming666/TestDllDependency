// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "Win32DllCAPIs.h"

HMODULE gDllModule = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        gDllModule = hModule;
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void dllcapi_init()
{
    // 获取主进程的完整路径
    TCHAR szPath[MAX_PATH] = { 0 };
    ::GetModuleFileName(NULL, szPath, MAX_PATH);

    // 获取本DLL模块的完整路径 - 方法1
    TCHAR szPath2[MAX_PATH] = { 0 };
    HMODULE hModuleSelf = GetModuleHandle("Win32DllC");
    ::GetModuleFileName(hModuleSelf, szPath2, MAX_PATH);

    // 获取本DLL模块的完整路径 - 方法2
    TCHAR szPath3[MAX_PATH] = { 0 };
    ::GetModuleFileName(gDllModule, szPath3, MAX_PATH);
}

void dllcapi_release()
{

}

int dllcapi_add_four_numbers(int a1, int a2, int a3, int a4)
{
    return a1 + a2 + a3 + a4;
}