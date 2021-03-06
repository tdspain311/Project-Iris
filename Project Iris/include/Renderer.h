#pragma once

#include <windows.h>
#include <WindowsX.h>
#include <wchar.h>
#include <string>
#include <assert.h>
#include <map>
#include "resource.h"
#include "pxcfacedata.h"
#include "FPSCalculator.h"
#include "pxcsensemanager.h"

typedef void (*OnFinishedRenderingCallback)();

class PXCImage;

class Renderer
{
public:
	enum RendererType { R2D, R3D };
	Renderer(HWND window);
	virtual ~Renderer();

	void SetNumberOfLandmarks(int numLandmarks);
	void SetOutput(PXCFaceData* output);
	void SetSenseManager(PXCSenseManager* senseManager);
	PXCSenseManager* GetSenseManager();
	virtual void DrawBitmap(PXCCapture::Sample* sample) = 0;
	void Render();

protected:
	static const int LANDMARK_ALIGNMENT = -3;
	int m_numLandmarks;
	PXCFaceData* m_currentFrameOutput;
	PXCSenseManager* m_senseManager;
	FPSCalculator m_frameRateCalcuator;
	PXCFaceData::LandmarkPoint* m_landmarkPoints;

	HWND m_window;
	HBITMAP m_bitmap;

	virtual void DrawGraphics(PXCFaceData* faceOutput) = 0;
	virtual void DrawLandmark(PXCFaceData::Face* trackedFace) = 0;
	void DrawFrameRate();
	void RefreshUserInterface();
	RECT GetResizeRect(RECT rectangle, BITMAP bitmap);
};
