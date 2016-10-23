#include "FPSCalculator.h"

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

