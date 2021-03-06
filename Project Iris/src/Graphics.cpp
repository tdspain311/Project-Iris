#include <iostream>

#include "pxcprojection.h"

#include "Graphics.h"
#include "Utilities.h"

static BOOL DEBUG = FALSE;


Graphics::~Graphics()
{
}

Graphics::Graphics(HWND window, PXCSession* session) : Renderer(window), m_session(session)
{
}

bool Graphics::ProjectVertex(const PXCPoint3DF32 &v, int &x, int &y, int radius)
{
	x = int (m_outputImageInfo.width * (0.5f + 0.001f * v.x));
	y = int (m_outputImageInfo.height * (0.5f - 0.001f * v.y));

	return ((radius <= x) && (x < m_outputImageInfo.width-radius) && (radius <= y) && (y < m_outputImageInfo.height-radius));
}

void Graphics::DrawGraphics(PXCFaceData* faceOutput)
{
	assert(faceOutput != NULL);
	if (!m_bitmap) return;

	const int numFaces = faceOutput->QueryNumberOfDetectedFaces();
	for (int i = 0; i < numFaces; ++i) 
	{
		PXCFaceData::Face* trackedFace = faceOutput->QueryFaceByIndex(i);		
		assert(trackedFace != NULL);

		if (trackedFace->QueryLandmarks() != NULL)
			DrawLandmark(trackedFace);
			DrawPose(trackedFace);
	}
}

void Graphics::DrawBitmap(PXCCapture::Sample* sample)
{
	PXCImage *imageDepth = sample->depth;
	assert(imageDepth);
	PXCImage::ImageInfo imageDepthInfo = imageDepth->QueryInfo();

	m_outputImageInfo.width = 1024;
	m_outputImageInfo.height = 1024;
	m_outputImageInfo.format = PXCImage::PIXEL_FORMAT_RGB32;
	m_outputImageInfo.reserved = 0;

	m_outputImage = m_session->CreateImage(&m_outputImageInfo);
	assert(m_outputImage);
	
	PXCImage::ImageData imageDepthData;
	if(imageDepth->AcquireAccess(PXCImage::ACCESS_READ, PXCImage::PIXEL_FORMAT_RGB32, &imageDepthData) >= PXC_STATUS_NO_ERROR)
	{
		memset(&m_outputImageData, 0, sizeof(m_outputImageData));
		pxcStatus status = m_outputImage->AcquireAccess(PXCImage::ACCESS_WRITE, PXCImage::PIXEL_FORMAT_RGB32, &m_outputImageData);
		if(status < PXC_STATUS_NO_ERROR) return;

		int stridePixels = m_outputImageData.pitches[0];
		pxcBYTE *pixels = reinterpret_cast<pxcBYTE*> (m_outputImageData.planes[0]);
		memset(pixels, 0, stridePixels * m_outputImageInfo.height);

		// get access to depth data
		PXCPoint3DF32* vertices = new PXCPoint3DF32[imageDepthInfo.width * imageDepthInfo.height];
		PXCProjection* projection(m_senseManager->QueryCaptureManager()->QueryDevice()->CreateProjection());
		if (!projection)
		{
			if (vertices) delete[] vertices;
			return;
		}

		projection->QueryVertices(imageDepth, vertices);
		projection->Release();
		int strideVertices = imageDepthInfo.width;

		// render vertices
		int numVertices = 0;
		for (int y = 0; y < imageDepthInfo.height; y++)
		{
			const PXCPoint3DF32 *verticesRow = vertices + y * strideVertices;
			for (int x = 0; x < imageDepthInfo.width; x++)
			{
				const PXCPoint3DF32 &v = verticesRow[x];
				if (v.z <= 0.0f)
				{
					continue;
				}

				int ix = 0, iy = 0;
				if(ProjectVertex(v, ix, iy))
				{
					pxcBYTE *ptr = m_outputImageData.planes[0];
					ptr += iy * m_outputImageData.pitches[0];
					ptr += ix * 4;
					ptr[0] = pxcBYTE(255.0f * 0.5f);
					ptr[1] = pxcBYTE(255.0f * 0.5f);
					ptr[2] = pxcBYTE(255.0f * 0.5f);
					ptr[3] = pxcBYTE(255.0f);
				}

				numVertices++;
			}
		}
		if (vertices) delete[] vertices;

		if (m_bitmap)
		{
			DeleteObject(m_bitmap);
			m_bitmap = 0;
		}

		HWND hwndPanel = GetDlgItem(m_window, IDC_PANEL);
		HDC dc = GetDC(hwndPanel);
		BITMAPINFO binfo;
		memset(&binfo, 0, sizeof(binfo));
		binfo.bmiHeader.biWidth = m_outputImageData.pitches[0] / 4;
		binfo.bmiHeader.biHeight = -(int)m_outputImageInfo.height;
		binfo.bmiHeader.biBitCount = 32;
		binfo.bmiHeader.biPlanes = 1;
		binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		binfo.bmiHeader.biCompression = BI_RGB;
		Sleep(1);
		m_bitmap = CreateDIBitmap(dc, &binfo.bmiHeader, CBM_INIT, m_outputImageData.planes[0], &binfo, DIB_RGB_COLORS);

		ReleaseDC(hwndPanel, dc);

		m_outputImage->ReleaseAccess(&m_outputImageData);
		imageDepth->ReleaseAccess(&imageDepthData);
		m_outputImage->Release();
	}
}

void Graphics::DrawLandmark(PXCFaceData::Face* trackedFace)
{
	const PXCFaceData::LandmarksData *landmarkData = trackedFace->QueryLandmarks();

	if (!landmarkData)
		return;

	HWND panelWindow = GetDlgItem(m_window, IDC_PANEL);
	HDC dc1 = GetDC(panelWindow);
	HDC dc2 = CreateCompatibleDC(dc1);

	if (!dc2)
	{
		ReleaseDC(panelWindow, dc1);
		return;
	}

	HFONT hFont = CreateFont(48, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"MONOSPACE");

	if (!hFont)
	{
		DeleteDC(dc2);
		ReleaseDC(panelWindow, dc1);
		return;
	}

	SetBkMode(dc2, TRANSPARENT);

	SelectObject(dc2, m_bitmap);
	SelectObject(dc2, hFont);

	BITMAP bitmap;
	GetObject(m_bitmap, sizeof(bitmap), &bitmap);

	pxcI32 numPoints = landmarkData->QueryNumPoints();

	if (numPoints != m_numLandmarks)
	{
		DeleteObject(hFont);
		DeleteDC(dc2);
		ReleaseDC(panelWindow, dc1);
		return;
	}

	PXCFaceData::LandmarkPoint* points = new PXCFaceData::LandmarkPoint[numPoints];

	//initialize Array
	for (int l = 0; l < numPoints; l++) 
	{
		points[l].world.x = 0.0;
		points[l].world.y = 0.0;
		points[l].world.z = 0.0;
	}
	
	landmarkData->QueryPoints(points);

	PXCFaceData::LandmarkPoint EyeLeft = points[landmarkData->QueryPointIndex(PXCFaceData::LandmarkType::LANDMARK_EYE_LEFT_CENTER)];
	PXCFaceData::LandmarkPoint EyeRight = points[landmarkData->QueryPointIndex(PXCFaceData::LandmarkType::LANDMARK_EYE_RIGHT_CENTER)];

	// Scale to canvas coordinate
	EyeLeft.world.x *= 1000.0f;
	EyeLeft.world.y *= 1000.0f;
	EyeLeft.world.z *= 1000.0f;

	EyeRight.world.x *= 1000.0f;
	EyeRight.world.y *= 1000.0f;
	EyeRight.world.z *= 1000.0f;
	
	int iLx = 0, iLy = 0, iRx = 0, iRy = 0;

	if (ProjectVertex(EyeLeft.world, iLx, iLy, 1) && ProjectVertex(EyeRight.world, iRx, iRy, 1))
	{
		if (EyeLeft.confidenceWorld > 0 && EyeRight.confidenceWorld > 0)
		{
			SetTextColor(dc2, RGB(255, 0, 255));
			TextOut(dc2, iLx, iLy, L"�", 1);
			TextOut(dc2, iRx, iRy, L"�", 1);
		}
	}
	
	if (points)
		delete[] points;

	DeleteObject(hFont);
	DeleteDC(dc2);
	ReleaseDC(panelWindow, dc1);
}

void Graphics::DrawPose(PXCFaceData::Face* trackedFace)
{
	HWND panelWindow = GetDlgItem(m_window, IDC_PANEL);
	HDC dc1 = GetDC(panelWindow);
	HDC dc2 = CreateCompatibleDC(dc1);

	if (!dc2) 
	{
		ReleaseDC(panelWindow, dc1);
		return;
	}

	HFONT hFont = CreateFont(28, 18, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, L"MONOSPACE");

	if (!hFont)
	{
		DeleteDC(dc2);
		ReleaseDC(panelWindow, dc1);
		return;
	}

	SetBkMode(dc2, TRANSPARENT);

	SelectObject(dc2, m_bitmap);
	SelectObject(dc2, hFont);

	BITMAP bitmap;
	GetObject(m_bitmap, sizeof(bitmap), &bitmap);

	const PXCFaceData::PoseData *poseData = trackedFace->QueryPose();

	if(poseData == NULL)
	{
		DeleteObject(hFont);
		DeleteDC(dc2);
		ReleaseDC(panelWindow, dc1);
		return;
	}
	PXCFaceData::HeadPosition outFaceCenterPoint;
	poseData->QueryHeadPosition(&outFaceCenterPoint);

	PXCFaceData::PoseQuaternion poseQuaternion;
	poseData->QueryPoseQuaternion(&poseQuaternion);

	if (DEBUG)
	{
		std::cout << "Quaternion X: " << poseQuaternion.x << " Y: " << poseQuaternion.y << " Z: " << poseQuaternion.z << " Scale: " << poseQuaternion.w << std::endl;
	}

	PXCPoint3DF32 pose;
	pose.x = poseQuaternion.x;
	pose.y = poseQuaternion.y;
	pose.z = poseQuaternion.z;

	int pose_x = 0, pose_y = 0;
	int headCenter_x = 0, headCenter_y = 0;	
	if(ProjectVertex(outFaceCenterPoint.headCenter, headCenter_x, headCenter_y, 1) && ProjectVertex(pose, pose_x, pose_y, 1))
	{
		
		if(poseData->QueryConfidence() > 0 && outFaceCenterPoint.confidence > 0)
		{
			SetTextColor(dc2, RGB(0, 0, 255));
			TextOut(dc2, headCenter_x, headCenter_y, L"�", 1);
		}
	
		const PXCFaceData::LandmarksData *landmarkData = trackedFace->QueryLandmarks();

		if (!landmarkData)
		{
			DeleteObject(hFont);
			DeleteDC(dc2);
			ReleaseDC(panelWindow, dc1);
			return;
		}

		PXCFaceData::LandmarkPoint* points = new PXCFaceData::LandmarkPoint[landmarkData->QueryNumPoints()];
		landmarkData->QueryPoints(points);

		PXCFaceData::LandmarkPoint nosePoint = points[landmarkData->QueryPointIndex(PXCFaceData::LandmarkType::LANDMARK_NOSE_TIP)];

		// Initialize nose point to 0		
		nosePoint.world.x = 0.0;
		nosePoint.world.y = 0.0;
		nosePoint.world.z = 0.0;
		
		// Scale in coords
		nosePoint.world.x *= 1000.0f;
		nosePoint.world.y *= 1000.0f;
		nosePoint.world.z *= 1000.0f;

		int noseTip_x = 0, noseTip_y = 0;

		if (ProjectVertex(nosePoint.world, noseTip_x, noseTip_y, 1))
		{
			PXCPointI32 direction;

			direction.x = noseTip_x - headCenter_x;
			direction.y = noseTip_y - headCenter_y;
			
			HPEN poseLine;
			
			if (poseData->QueryConfidence() > 0)
			{
				poseLine = CreatePen(PS_SOLID, 5, RGB(255 , 0, 0));
			}

			if (!poseLine)
			{
				DeleteObject(hFont);
				DeleteDC(dc2);
				ReleaseDC(panelWindow, dc1);
				return;
			}
			SelectObject(dc2, poseLine);

			int endPoseX = int(pose_x - (1.5 * (double)direction.x));
			int endPoseY = int(pose_y - (1.5 * (double)direction.y));

			MoveToEx(dc2, headCenter_x, headCenter_y, 0);
			LineTo(dc2, endPoseX, endPoseY);

			DeleteObject(poseLine);
		}

		if (points) delete[] points;
	}

	
	DeleteObject(hFont);
	DeleteDC(dc2);
	ReleaseDC(panelWindow, dc1);
}