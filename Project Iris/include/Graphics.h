#pragma once

#include <memory>
#include "Renderer.h"
#include "service/pxcsessionservice.h"

class Graphics : public Renderer
{
public:
	Graphics(HWND window, PXCSession* session);
	virtual ~Graphics();

	void DrawBitmap(PXCCapture::Sample* sample);

private:
	void DrawGraphics(PXCFaceData* faceOutput);
	void DrawLandmark(PXCFaceData::Face* trackedFace);
	bool ProjectVertex(const PXCPoint3DF32 &v, int &x, int &y, int radius = 0);
	void DrawPose(PXCFaceData::Face* trackedFace);

	PXCSession* m_session;
	PXCImage::ImageInfo m_outputImageInfo;
	PXCImage* m_outputImage;
	PXCImage::ImageData m_outputImageData;
};