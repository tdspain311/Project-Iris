#pragma once

#include "windows.h"

class FPSCalculator
{
private:
	LARGE_INTEGER frequency;
	LARGE_INTEGER previousTime;
	LARGE_INTEGER currentTime;
	int currentlyCalclatedFrameRate;
	int frameRate;
	bool isFrameRateReady;

public:
	FPSCalculator();
	bool IsFrameRateReady();
	int GetFrameRate();
	void Tick();
};
