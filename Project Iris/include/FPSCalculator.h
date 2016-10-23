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
	FPSCalculator() : frameRate(0), currentlyCalclatedFrameRate(0), currentTime(), isFrameRateReady(false)
	{
		QueryPerformanceCounter(&previousTime);
		QueryPerformanceFrequency(&frequency);
	}
	bool IsFrameRateReady();
	int GetFrameRate();
	void Tick();
};
