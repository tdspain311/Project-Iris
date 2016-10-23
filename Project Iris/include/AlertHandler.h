#pragma once

#include "pxcfaceconfiguration.h"
#include <windows.h>

class AlertHandler : public PXCFaceConfiguration::AlertHandler
{
public:
	AlertHandler(HWND hwndDlg) : dialogWindow(hwndDlg)
	{

	}
	virtual void PXCAPI OnFiredAlert(const PXCFaceData::AlertData *alertData);

private:
	HWND dialogWindow;
};