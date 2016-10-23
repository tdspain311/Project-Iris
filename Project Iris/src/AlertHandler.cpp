#include "AlertHandler.h"
#include "Utilities.h"

AlertHandler::AlertHandler(HWND hwndDlg) : dialogWindow(hwndDlg)
{

}

void PXCAPI AlertHandler::OnFiredAlert(const PXCFaceData::AlertData *alertData)
{
	switch(alertData->label)
	{
	case PXCFaceData::AlertData::ALERT_NEW_FACE_DETECTED:
		Utilities::SetStatus(dialogWindow, L"ALERT_NEW_FACE_DETECTED", alertPart);
		break;
	case PXCFaceData::AlertData::ALERT_FACE_OUT_OF_FOV:
		Utilities::SetStatus(dialogWindow, L"ALERT_FACE_OUT_OF_FOV", alertPart);
		break;
	case PXCFaceData::AlertData::ALERT_FACE_BACK_TO_FOV:
		Utilities::SetStatus(dialogWindow, L"ALERT_FACE_BACK_TO_FOV", alertPart);
		break;
	case PXCFaceData::AlertData::ALERT_FACE_OCCLUDED:
		Utilities::SetStatus(dialogWindow, L"ALERT_FACE_OCCLUDED", alertPart);
		break;
	case PXCFaceData::AlertData::ALERT_FACE_NO_LONGER_OCCLUDED:
		Utilities::SetStatus(dialogWindow, L"ALERT_FACE_NO_LONGER_OCCLUDED", alertPart);
		break;
	case PXCFaceData::AlertData::ALERT_FACE_LOST:
		Utilities::SetStatus(dialogWindow, L"ALERT_FACE_LOST", alertPart);
		break;
	default:
		Utilities::SetStatus(dialogWindow, L"UNKNOWN_ALERT", alertPart);
		break;
	}
}
