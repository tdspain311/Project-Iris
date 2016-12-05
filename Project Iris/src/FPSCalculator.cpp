#include "FPSCalculator.h"

FPSCalculator::FPSCalculator() : frameRate(0), currentlyCalclatedFrameRate(0), currentTime(), isFrameRateReady(false)
{
	QueryPerformanceCounter(&previousTime);
	QueryPerformanceFrequency(&frequency);
}

bool FPSCalculator::IsFrameRateReady()
{
	return isFrameRateReady;
}

int FPSCalculator::GetFrameRate()
{
	isFrameRateReady = false;
	return frameRate;
}

void FPSCalculator::Tick()
{
	QueryPerformanceCounter(&currentTime);
	++currentlyCalclatedFrameRate;
	if (currentTime.QuadPart - previousTime.QuadPart > frequency.QuadPart)
	{
		isFrameRateReady = true;
		previousTime = currentTime;
		frameRate = currentlyCalclatedFrameRate;
		currentlyCalclatedFrameRate = 0;
	}
}

