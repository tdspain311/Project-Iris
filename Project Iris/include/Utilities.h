#pragma once

#include <windows.h>
#include <map>
#include "pxcdefs.h"
#include "pxcfaceconfiguration.h"
#include "pxccapture.h"

enum StatusWindowPart { statusPart, alertPart };
extern std::map<int, PXCFaceConfiguration::TrackingModeType> s_profilesMap;
extern std::map<int, PXCCapture::DeviceInfo> g_deviceInfoMap;

class Utilities
{
public:
	static void SetStatus(HWND dialogWindow, pxcCHAR *line, StatusWindowPart part);
	static bool GetPlaybackState(HWND DialogWindow);
	static const int TextHeight = 16;
};

// Eye Calibration functions

enum CalibMode {

	mode_calib,
	mode_playback,
	mode_live

};

extern volatile int eye_point_x;
extern volatile int eye_point_y;
extern volatile bool isPaused;

extern volatile float eye_horizontal_angle;
extern volatile float eye_vertical_angle;

void InitCalibWindows(CalibMode mode);
void CloseCalibWindows();
void UpdateTracking();
void EnableBackWindow();
