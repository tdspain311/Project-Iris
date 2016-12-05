#pragma once

#include "Renderer.h"
#include "Graphics.h"

class RendererManager
{
public:
	RendererManager(Graphics* renderer3D);
	~RendererManager();

	void SetRendererType(Renderer::RendererType type);
	void Render();
	void SetSenseManager(PXCSenseManager* senseManager);
	void SetNumberOfLandmarks(int numLandmarks);
	void SetCallback(OnFinishedRenderingCallback callback);
	void DrawBitmap(PXCCapture::Sample* sample);
	void SetOutput(PXCFaceData* output);
	void SignalRenderer();

	static HANDLE& GetRenderingFinishedSignal();
	static void SignalProcessor();

private:
	Graphics* m_renderer3D;
	Renderer* m_currentRenderer;

	HANDLE m_rendererSignal;
	OnFinishedRenderingCallback m_callback;
};

