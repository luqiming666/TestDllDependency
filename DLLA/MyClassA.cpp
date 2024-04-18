#include "pch.h"
#include "MyClassA.h"
#include <tchar.h>

MyClassA::MyClassA()
{

}

MyClassA::~MyClassA()
{

}

int MyClassA::Add(int num1, int num2)
{
	if (!CheckParent())
	{
		::MessageBox(NULL, _T("非法使用！"), _T("QM"), MB_OK | MB_ICONERROR);
		return -1;
	}

	return num1 + num2;
}