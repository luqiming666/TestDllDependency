#pragma once

#include "ADefs.h"

class DLL_A_API MyClassA
{
public:
	MyClassA();
	~MyClassA();

	int Add(int num1, int num2);

private:
	// Test: 只允许 DLLB 来加载我！
	inline bool CheckParent() {
		HMODULE hModule = GetModuleHandle("DLLB");
		if (hModule != NULL) {
			return true;
		}
		return false;
	}
};

