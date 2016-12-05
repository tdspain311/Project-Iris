#include <map>

#include "pxccapture.h"

#include "Renderer.h"
#include "Utilities.h"

Renderer::~Renderer()
{
	if (m_landmarkPoints != NULL)
		delete[] m_landmarkPoints;
}

Renderer::Renderer(HWND window) : m_window(window)
{
	m_landmarkPoints = NULL;
	m_senseManager = NULL;
}

void Renderer::SetOutput(PXCFaceData* output)
{
	m_currentFrameOutput = output;
}

void Renderer::SetSenseManager(PXCSenseManager* senseManager)
{
	m_senseManager = senseManager;
}

PXCSenseManager* Renderer::GetSenseManager()
{
	return m_senseManager;
}

void Renderer::Render()
{
	DrawFrameRate();
	DrawGraphics(m_currentFrameOutput);
	RefreshUserInterface();
}

void Renderer::DrawFrameRate()
{
	m_frameRateCalcuator.Tick();
	if (m_frameRateCalcuator.IsFrameRateReady())
	{
		int fps = m_frameRateCalcuator.GetFrameRate();

		pxcCHAR line[1024];
		swprintf_s<1024>(line, L"Rate (%d fps)", fps);
		Utilities::SetStatus(m_window, line, statusPart);
	}
}

void Renderer::RefreshUserInterface()
{
	if (!m_bitmap) return;

	HWND panel = GetDlgItem(m_window, IDC_PANEL);
	RECT rc;
	GetClientRect(panel, &rc);

	HDC dc = GetDC(panel);
	if(!dc)
	{
		return;
	}

	HBITMAP bitmap = CreateCompatibleBitmap(dc, rc.right, rc.bottom);

	if(!bitmap)
	{

		ReleaseDC(panel, dc);
		return;
	}
	HDC dc2 = CreateCompatibleDC(dc);
	if (!dc2)
	{
		DeleteObject(bitmap);
		ReleaseDC(m_window,dc);
		return;
	}
	SelectObject(dc2, bitmap);
	SetStretchBltMode(dc2, COLORONCOLOR);

	/* Draw the main window */
	HDC dc3 = CreateCompatibleDC(dc);

	if (!dc3)
	{
		DeleteDC(dc2);
		DeleteObject(bitmap);
		ReleaseDC(m_window,dc);
		return;
	}

	SelectObject(dc3, m_bitmap);
	BITMAP bm;
	GetObject(m_bitmap, sizeof(BITMAP), &bm);

	bool scale = true;

	if (scale)
	{
		RECT rc1 = GetResizeRect(rc, bm);
		StretchBlt(dc2, rc1.left, rc1.top, rc1.right, rc1.bottom, dc3, 0, 0,bm.bmWidth, bm.bmHeight, SRCCOPY);	
	} 
	else
	{
		BitBlt(dc2, 0, 0, rc.right, rc.bottom, dc3, 0, 0, SRCCOPY);
	}

	DeleteDC(dc3);
	DeleteDC(dc2);
	ReleaseDC(m_window,dc);

	HBITMAP bitmap2 = (HBITMAP)SendMessage(panel, STM_GETIMAGE, 0, 0);
	if (bitmap2) DeleteObject(bitmap2);
	SendMessage(panel, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);
	InvalidateRect(panel, 0, TRUE);
	 DeleteObject(bitmap);
}

RECT Renderer::GetResizeRect(RECT rectangle, BITMAP bitmap)
{
	RECT resizedRectangle;
	float sx = (float)rectangle.right / (float)bitmap.bmWidth;
	float sy = (float)rectangle.bottom / (float)bitmap.bmHeight;
	float sxy = sx < sy ? sx : sy;
	resizedRectangle.right = (int)(bitmap.bmWidth * sxy);
	resizedRectangle.left = (rectangle.right - resizedRectangle.right) / 2 + rectangle.left;
	resizedRectangle.bottom = (int)(bitmap.bmHeight * sxy);
	resizedRectangle.top = (rectangle.bottom - resizedRectangle.bottom) / 2 + rectangle.top;
	return resizedRectangle;
}

void Renderer::SetNumberOfLandmarks(int numLandmarks)
{
	m_numLandmarks = numLandmarks;
	m_landmarkPoints = new PXCFaceData::LandmarkPoint[numLandmarks];
}