#include "RendererManager.h"

extern HANDLE g_hMutex;

RendererManager::RendererManager(Graphics* renderer3D) :
	m_renderer3D(renderer3D), m_currentRenderer(NULL)
{
	m_rendererSignal = CreateEvent(NULL, FALSE, FALSE, NULL);
}

RendererManager::~RendererManager(void)
{
	CloseHandle(m_rendererSignal);

	if(m_renderer3D != NULL)
		delete m_renderer3D;
}

void RendererManager::SetRendererType(Renderer::RendererType type)
{
	DWORD dwWaitResult;
	dwWaitResult = WaitForSingleObject(g_hMutex,	INFINITE);
	if(dwWaitResult == WAIT_OBJECT_0)
	{

		m_currentRenderer = m_renderer3D;

		if(!ReleaseMutex(g_hMutex))
		{
			throw std::exception("Failed to release mutex");
			return;
		}
	}
}

void RendererManager::Render()
{
	WaitForSingleObject(m_rendererSignal, INFINITE);

	m_currentRenderer->Render();

	m_callback();
}

void RendererManager::SetSenseManager(PXCSenseManager* senseManager)
{
	m_renderer3D->SetSenseManager(senseManager);
}

void RendererManager::SetNumberOfLandmarks(int numLandmarks)
{
	m_renderer3D->SetNumberOfLandmarks(numLandmarks);
}

void RendererManager::SetCallback(OnFinishedRenderingCallback callback)
{
	m_callback = callback;
}

void RendererManager::DrawBitmap(PXCCapture::Sample* sample)
{
	m_currentRenderer->DrawBitmap(sample);
}

void RendererManager::SetOutput(PXCFaceData* output)
{
	m_renderer3D->SetOutput(output);
}

void RendererManager::SignalRenderer()
{
	SetEvent(m_rendererSignal);
}

void RendererManager::SignalProcessor()
{
	SetEvent(GetRenderingFinishedSignal());
}

HANDLE& RendererManager::GetRenderingFinishedSignal()
{
	static HANDLE renderingFinishedSignal = CreateEvent(NULL, FALSE, TRUE, NULL);
	return renderingFinishedSignal;
}