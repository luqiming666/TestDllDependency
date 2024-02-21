#include "pch.h"
#include "framework.h"
#include "DLLBAPIs.h"
#include <cmath>

int dllbapi_add(int a, int b)
{
	return a + b;
}

int MySpace::dllbapi_minus(int a, int b)
{
	return a - b;
}

int MySpace::dllbapi_power(int a, int b)
{
	return pow(a, b);
}