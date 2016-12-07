#include <Windows.h>
#include <WindowsX.h>
#include <commctrl.h>
#include <io.h>
#include <map>
#include "math.h"
#include "resource.h"
#include "pxcfacemodule.h"
#include "pxcfacedata.h"
#include "pxcvideomodule.h"
#include "pxcfaceconfiguration.h"
#include "pxcmetadata.h"
#include "service/pxcsessionservice.h"

#include "FPSCalculator.h"
#include "RendererManager.h"
#include "Graphics.h"
#include "Utilities.h"
#include "Processor.h"
#include "Strsafe.h"
#include <string.h>
#include <iostream>

WCHAR m_Username[40];
pxcCHAR m_CalibFilename[1024] = { 0 };
pxcCHAR m_rssdkFilename[1024] = { 0 };
PXCSession* session = NULL;
RendererManager* renderer = NULL;
Processor* processor = NULL;

HANDLE g_hMutex = NULL;
HWND g_hWnd = NULL;
HWND g_hWndEyeBack = NULL;
HWND g_hWndEyePoint = NULL;
HWND g_hWndCalibPoint = NULL;
HINSTANCE g_hInstance = NULL;

volatile bool isRunning = false;
volatile bool isStopped = false;
volatile bool isPaused = false;
volatile bool isActiveApp = true;
volatile bool isLoadCalibFile = false;

static int controls[] = { ID_START, ID_STOP, ID_LOAD_CALIB, ID_NEW_CALIB };
static RECT layout[3 + sizeof(controls) / sizeof(controls[0])];

volatile int eye_point_x = 2000;
volatile int eye_point_y = 2000;
extern volatile float eye_horizontal_angle = 0;
extern volatile float eye_vertical_angle = 0;
int gaze_point_x = 0;
int gaze_point_y = 0;

const int max_path_length = 1024;

int cursor_pos_x = 0;
int cursor_pos_y = 0;
CalibMode modeWork = mode_calib;

extern short calibBuffersize;
extern unsigned char* calibBuffer;
extern int calib_status;
extern int dominant_eye;

typedef DWORD (WINAPI *make_layered)(HWND, DWORD, BYTE, DWORD);
static make_layered set_layered_window = NULL;
static BOOL dll_initialized = FALSE;

static BOOL DEBUG = TRUE;
static int MAX_ANGLE = 30;

bool make_transparent(HWND hWnd_) {

	if (!dll_initialized) {

		HMODULE hLib = LoadLibraryA ("user32");
		set_layered_window = (make_layered) GetProcAddress(hLib, "SetLayeredWindowAttributes");
		dll_initialized = TRUE;

	}

	if (set_layered_window == NULL) return FALSE;
	SetLastError(0);

	SetWindowLong(hWnd_, GWL_EXSTYLE , GetWindowLong(hWnd_, GWL_EXSTYLE) | WS_EX_LAYERED);
	if (GetLastError()) return FALSE;

	return set_layered_window(hWnd_, TRANSPARENT, 0, LWA_COLORKEY) != NULL;
}

LRESULT CALLBACK WndProc(HWND hWnd_, UINT message_, WPARAM wParam_, LPARAM lParam_) {

	switch (message_) {

	case WM_KEYDOWN:

		switch (wParam_) {

			case VK_ESCAPE:
				PostMessage(g_hWnd, WM_COMMAND, ID_STOP, 0);
				CloseWindow(hWnd_);
				break;
		}

	case WM_PAINT:
		if (hWnd_ == g_hWndEyePoint) {

			HDC dc = GetDC(g_hWndEyePoint);
			HBRUSH hbrush = CreateSolidBrush(RGB(0, 255, 0));
			HPEN hPen = CreatePen(PS_SOLID, 3, RGB(0, 255, 0));

			SelectObject(dc, hPen);
			SelectObject(dc, hbrush);
			Ellipse(dc, 5, 5, 25, 25);

			DeleteObject(hPen);
			DeleteObject(hbrush);
			ReleaseDC(g_hWndEyePoint, dc);
		}
		break;
	}

	// default message handling
	if (message_) return DefWindowProc(hWnd_, message_, wParam_, lParam_);
	else return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance_, DWORD color_, WCHAR* name_) {

	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance_;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)CreateSolidBrush(color_);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= name_;
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);

}

bool InitSimpleWindow(HWND* hwnd_, int size_, DWORD color_, WCHAR* name_) {

	if (hwnd_ == NULL) return false;

	// create transparent eye point window
	if (*hwnd_ == NULL) {

		MyRegisterClass(g_hInstance, color_, name_);

		*hwnd_ = CreateWindow(name_, name_, WS_POPUP,
			-500, -500, size_, size_, g_hWndEyeBack, NULL, g_hInstance, NULL);

		if (!hwnd_) return false;

	}

	ShowWindow(*hwnd_, SW_SHOW);
	UpdateWindow(*hwnd_);

	return true;

}

bool InitTransWindow(HWND* hwnd_, int size_, DWORD color_, WCHAR* name_) {

	if (hwnd_ == NULL) return false;

	// create transparent eye point window
	if (*hwnd_ == NULL) {

		MyRegisterClass(g_hInstance, TRANSPARENT, name_);
	
		*hwnd_ = CreateWindow(name_, name_, WS_POPUP,
			-500, -500, size_, size_, g_hWndEyeBack, NULL, g_hInstance, NULL);

		if (!hwnd_) return false;

	}

	make_transparent(*hwnd_);
	ShowWindow(*hwnd_, SW_SHOW);
	UpdateWindow(*hwnd_);

	return true;

}

bool InitBackWindow(HWND* hwnd_, DWORD color_, WCHAR* name_) {

	if (hwnd_ == NULL) return false;

	// create transparent eye point window
	if (*hwnd_ == NULL) {

		MyRegisterClass(g_hInstance, color_, name_);

		RECT rc;
		const HWND hDesktop = GetDesktopWindow();
		GetWindowRect(hDesktop, &rc);

		*hwnd_ = CreateWindow(name_, name_, WS_POPUP,
			0, 0, rc.right, rc.bottom, g_hWnd, NULL, g_hInstance, NULL);

		if (!hwnd_) return false;

	}

	// hide back window at first enable when API loaded
	ShowWindow(*hwnd_, SW_HIDE);
	UpdateWindow(*hwnd_);

	return true;

}

void CloseTransWindow(HWND* hwnd_) {

	// close EyePoint window
	if (*hwnd_) {

		DestroyWindow(*hwnd_);
		*hwnd_ = NULL;

	}

}

void CloseCalibWindows() {

	// close calibration windows
	CloseTransWindow(&g_hWndEyeBack);
	CloseTransWindow(&g_hWndCalibPoint);

}

void EnableBackWindow() {

	// show message box as latest point
	MessageBoxA(g_hWnd, "Calibration is required, please keep your head fixed and look at the red square that appears on the screen",
		"Calibration Required", MB_OK | MB_ICONINFORMATION);

	// enable back window
	ShowWindow(g_hWndEyeBack, SW_SHOW);
	UpdateWindow(g_hWndEyeBack);

}

void InitCalibWindows(CalibMode mode_) {

	// close the old windows
	CloseCalibWindows();

	// init calibration windows
	modeWork = mode_; 
	
	switch (mode_) {

	case mode_calib:
		InitBackWindow(&g_hWndEyeBack, RGB(240, 240, 240), L"Background");
		InitSimpleWindow(&g_hWndCalibPoint, 35, RGB(255, 0, 0), L"GPI Calibrate");  // 35-50
		break;

	case mode_live:
		InitTransWindow(&g_hWndEyePoint, 30, RGB(0, 255, 255), L"GPI Live");
		break;

	case mode_playback:
		InitTransWindow(&g_hWndEyePoint, 30, RGB(255, 255, 0), L"GPI Clip");
		break;
	}

	// focus on main window
	SetFocus(g_hWnd);

}

void UpdateTracking() {

	// set position of gaze point
	if (g_hWndEyePoint || g_hWndCalibPoint) {

		if (modeWork == mode_calib) {
			RECT rc;
			GetWindowRect(g_hWndCalibPoint, &rc);

			int width = rc.right - rc.left;
			int height = rc.bottom - rc.top;
			SetWindowPos(g_hWndCalibPoint, NULL, eye_point_x - width / 2, eye_point_y - height / 2, width, height, NULL);

		}
		else
		{
			// Gaze position
			RECT rc;
			RECT wrc;
			HWND hDesktop = GetDesktopWindow();
			GetWindowRect(hDesktop, &wrc);
			GetWindowRect(g_hWndEyePoint, &rc);

			int width = rc.right - rc.left;
			int height = rc.bottom - rc.top;

			HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
				
			if ((eye_horizontal_angle >= -MAX_ANGLE && eye_horizontal_angle <= MAX_ANGLE) &&
				(eye_vertical_angle >= -MAX_ANGLE && eye_vertical_angle <= MAX_ANGLE))
			{
				if ((eye_point_x <= wrc.right) && (eye_point_x >= wrc.left) &&
					(eye_point_y <= wrc.bottom) && (eye_point_y >= wrc.top))
				{
					// Trivial Accept
					SetConsoleTextAttribute(hConsole, 15);
					std::cout << "Gaze Position (x, y): " << eye_point_x << ", " << eye_point_y << " || Angle (horizontal, vertical): " << eye_horizontal_angle << ", " << eye_vertical_angle << std::endl;
				}
				else
				{
					// Modify to Accept
					SetConsoleTextAttribute(hConsole, 14);
					std::cout << "Gaze Position (x, y): " << eye_point_x << ", " << eye_point_y << " || Angle (horizontal, vertical): " << eye_horizontal_angle << ", " << eye_vertical_angle << std::endl;
					
					if (eye_point_x > wrc.right) // 1920
						eye_point_x = wrc.right - width;
					if (eye_point_x < wrc.left) // 0
						eye_point_x = wrc.left + width;
					if (eye_point_y > wrc.bottom) // 1080
						eye_point_y = wrc.bottom - height;
					if (eye_point_y < wrc.top) // 0
						eye_point_y = wrc.top + height;
				}
				gaze_point_x = eye_point_x;
				gaze_point_y = eye_point_y;
			}
			else
			{
				// Trivial Reject
				SetConsoleTextAttribute(hConsole, 4);
				std::cout << "Gaze Position (x, y): " << eye_point_x << ", " << eye_point_y << " || Angle (horizontal, vertical): " << eye_horizontal_angle << ", " << eye_vertical_angle << std::endl;
			}
			if (GetMenuState(GetSubMenu(GetMenu(g_hWnd), 1), ID_ALWAYSON_MOUSE, MF_BYCOMMAND) & MF_CHECKED)
				SetCursorPos(gaze_point_x, gaze_point_y);
			else if (GetMenuState(GetSubMenu(GetMenu(g_hWnd), 1), ID_ALWAYSON_GPI, MF_BYCOMMAND) & MF_CHECKED) 
				SetWindowPos(g_hWndEyePoint, NULL, gaze_point_x, gaze_point_y, width, height, NULL);
		}
	}
}

void GetPlaybackFile(void) {

	OPENFILENAME filename;
	memset(&filename, 0, sizeof(filename));

	filename.lStructSize = sizeof(filename);
	filename.lpstrFilter = L"RSSDK clip (*.rssdk)\0*.rssdk\0Old format clip (*.pcsdk)\0*.pcsdk\0All Files (*.*)\0*.*\0\0";
	filename.lpstrFile = m_rssdkFilename; 
	m_rssdkFilename[0] = 0;
	filename.nMaxFile = sizeof(m_rssdkFilename) / sizeof(pxcCHAR);
	filename.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	if (!GetOpenFileName(&filename)) m_rssdkFilename[0] = 0;

}

BOOL CALLBACK LoadCalibProc(HWND hwndDlg_, UINT message_, WPARAM wParam_, LPARAM lParam_) {

    switch (message_) { 

		case WM_INITDIALOG:

			{
				
				char temp[max_path_length];
				GetTempPathA(max_path_length, temp);
				strcat_s(temp, max_path_length, "*.bin");

				struct _finddata_t dirFile;
				intptr_t hFile;

				if (( hFile = _findfirst( temp, &dirFile )) != -1 ) {

					do {

						SendMessageA(GetDlgItem(hwndDlg_, IDC_LIST1), LB_ADDSTRING, 0, (LPARAM)dirFile.name);

					} while ( _findnext( hFile, &dirFile ) == 0 );

					_findclose( hFile );

				}

			}

			SendMessageA(GetDlgItem(hwndDlg_, IDC_LIST1), LB_SETCURSEL, 0, (LPARAM)0);
			break;

        case WM_COMMAND: 

            switch (LOWORD(wParam_)) { 

            case IDOK:			
				{
					WCHAR name[max_path_length];
					WCHAR temp[max_path_length];
					int index = SendMessage(GetDlgItem(hwndDlg_, IDC_LIST1), LB_GETCURSEL, (WPARAM)0 , (LPARAM)0);
					SendMessage(GetDlgItem(hwndDlg_, IDC_LIST1), LB_GETTEXT, (WPARAM)index , (LPARAM)name);
					GetTempPath(max_path_length, temp);
					
					StringCbPrintf(m_CalibFilename, sizeof(m_CalibFilename), L"%s%s", temp, name);
					EndDialog(hwndDlg_, true);
					return TRUE; 
				}

			case IDCANCEL:
			case IDCLOSE:
				m_CalibFilename[0] = 0;
                EndDialog(hwndDlg_, false); 
                return TRUE; 

        } 

		break;

    } 

    return FALSE; 

} 

bool GetCalibFile(void) {

	// use a special dialog
	return (DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_LOAD_CALIB), g_hWnd, (DLGPROC)LoadCalibProc)) != 0;
}

bool GetSaveCalibFile(void) {

	OPENFILENAME filename;
	memset(&filename, 0, sizeof(filename));

	filename.lStructSize = sizeof(filename);
	filename.lpstrFilter = L"Calibration File (*.bin)\0*.bin\0All Files (*.*)\0*.*\0\0";
	filename.lpstrFile = m_CalibFilename; 
	m_CalibFilename[0] = 0;
	filename.nMaxFile = sizeof(m_CalibFilename) / sizeof(pxcCHAR);
	filename.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	if (GetSaveFileName(&filename)) return true;

	m_CalibFilename[0] = 0;
	return false;

}

void GetRecordFile(void) {

	OPENFILENAME filename;
	memset(&filename, 0, sizeof(filename));

	filename.lStructSize = sizeof(filename);
	filename.lpstrFilter = L"RSSDK clip (*.rssdk)\0*.rssdk\0All Files (*.*)\0*.*\0\0";
	filename.lpstrFile = m_rssdkFilename; 
	m_rssdkFilename[0] = 0;
	filename.nMaxFile = sizeof(m_rssdkFilename) / sizeof(pxcCHAR);
	filename.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

	if (GetSaveFileName(&filename)) {

		if (filename.nFilterIndex==1 && filename.nFileExtension==0) {

			int len = std::char_traits<wchar_t>::length(m_rssdkFilename);
			
			if (len>1 && len<sizeof(m_rssdkFilename)/sizeof(pxcCHAR)-7) {

				wcscpy_s(&m_rssdkFilename[len], rsize_t(7), L".rssdk\0");

			}

		}

	} else m_rssdkFilename[0] = 0;

}

void SaveLayout(HWND dialogWindow_) {

	GetClientRect(dialogWindow_, &layout[0]);
	ClientToScreen(dialogWindow_, (LPPOINT)&layout[0].left);
	ClientToScreen(dialogWindow_, (LPPOINT)&layout[0].right);
	GetWindowRect(GetDlgItem(dialogWindow_, IDC_PANEL), &layout[1]);
	GetWindowRect(GetDlgItem(dialogWindow_, IDC_STATUS), &layout[2]);

	for (int i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i)
		GetWindowRect(GetDlgItem(dialogWindow_, controls[i]), &layout[3 + i]);

}

void RedoLayout(HWND dialogWindow_) {

	RECT rectangle;
	GetClientRect(dialogWindow_, &rectangle);

	/* Status */

	SetWindowPos(GetDlgItem(dialogWindow_, IDC_STATUS), dialogWindow_, 
		0,
		rectangle.bottom - (layout[2].bottom - layout[2].top),
		rectangle.right - rectangle.left,
		(layout[2].bottom - layout[2].top),
		SWP_NOZORDER);

	/* Panel */

	SetWindowPos(
		GetDlgItem(dialogWindow_,IDC_PANEL), dialogWindow_,
		(layout[1].left - layout[0].left),
		(layout[1].top - layout[0].top),
		rectangle.right - (layout[1].left-layout[0].left) - (layout[0].right - layout[1].right),
		rectangle.bottom - (layout[1].top - layout[0].top) - (layout[0].bottom - layout[1].bottom),
		SWP_NOZORDER);

	/* Buttons & CheckBoxes */
	
	for (int i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i) {

		SetWindowPos(
			GetDlgItem(dialogWindow_,controls[i]), dialogWindow_,
			rectangle.right - (layout[0].right - layout[3 + i].left),
			(layout[3 + i].top - layout[0].top),
			(layout[3 + i].right - layout[3 + i].left),
			(layout[3 + i].bottom - layout[3 + i].top),
			SWP_NOZORDER);
	
	}

}

static DWORD WINAPI RenderingThread(LPVOID arg_) {

	while (true) {

		// update tracking
		UpdateTracking();

		// render
		renderer->Render();

	}

}

static DWORD WINAPI ProcessingThread(LPVOID arg_) {

	HWND window = (HWND)arg_;
	processor->Process(window);

	isRunning = false;
	return 0;

}

BOOL CALLBACK ResultsProc(HWND hwndDlg_, UINT message_, WPARAM wParam_, LPARAM lParam_) {

    switch (message_) { 

	   case WM_KEYDOWN: 
		   // exit calibration

		switch (wParam_) {

		case VK_ESCAPE:

			if (calib_status == 3) { 
				// delete calibration

				delete [] calibBuffer;
				calibBuffer = NULL;
				calibBuffersize = 0;
				isLoadCalibFile = false;
				EndDialog(hwndDlg_, wParam_);
				PostMessage(g_hWnd, WM_COMMAND, ID_STOP, 0);

			} else { 
				// continue working

				if (!GetDlgItemText(hwndDlg_, IDC_EDIT1, m_Username, 40))	wsprintf(m_Username, L"default_user");
				EndDialog(hwndDlg_, wParam_); 

			}

			break;

		}

		case WM_INITDIALOG:
			
			SetDlgItemText(hwndDlg_, IDC_EDIT1, m_Username);
			wsprintf(m_Username, L"default_user");
			
			if (!calibBuffer) {

				Edit_Enable(GetDlgItem(hwndDlg_, IDC_EDIT1), false);
				Button_Enable(GetDlgItem(hwndDlg_, ID_DETAILS) , false);
				Button_Enable(GetDlgItem(hwndDlg_, ID_OK) , false);

			} else {

				Edit_Enable(GetDlgItem(hwndDlg_, IDD_CALIB_DIALOG), true);
				Button_Enable(GetDlgItem(hwndDlg_, ID_DETAILS) , true);
				Button_Enable(GetDlgItem(hwndDlg_, ID_OK) , true);

			}

			// show calibration results
			switch (calib_status) {

			case 0:
				SetDlgItemText(hwndDlg_, IDC_RESULTS, L"Calibration Status: SUCCESS");
				break;

			case 1:
				SetDlgItemText(hwndDlg_, IDC_RESULTS, L"Calibration Status: FAIR");
				break;

			case 2:
				SetDlgItemText(hwndDlg_, IDC_RESULTS, L"Calibration Status: POOR");
				break;

			case 3:
				SetDlgItemText(hwndDlg_, IDC_RESULTS, L"Calibration Status: FAILED");
				break;

			default:
				SetDlgItemText(hwndDlg_, IDC_RESULTS, L"Calibration Status: UNKNOWN");
				break;

			}

			// show dominant eye
			
			switch (dominant_eye) {

			case 0:
				SetDlgItemText(hwndDlg_, IDC_RESULTS_EYE, L"The current dominant eye for this calibration is: Right");
				break;

			case 1:
				SetDlgItemText(hwndDlg_, IDC_RESULTS_EYE, L"The current dominant eye for this calibration is: Left");
				break;

			case 2:
				SetDlgItemText(hwndDlg_, IDC_RESULTS_EYE, L"The current dominant eye for this calibration is: Both");
				break;

			default:
				SetDlgItemText(hwndDlg_, IDC_RESULTS_EYE, L"The current dominant eye for this calibration is: Unknown");
				break;

			}
			
			break;

        case WM_COMMAND: 

            switch (LOWORD(wParam_)) { 

            case ID_OK: 

				if (calib_status == 3) { 
					// delete calibration

					delete [] calibBuffer;
					calibBuffer = NULL;
					calibBuffersize = 0;
					isLoadCalibFile = false;
					EndDialog(hwndDlg_, wParam_);
					PostMessage(g_hWnd, WM_COMMAND, ID_STOP, 0);

				} else { 
					// continue working

					if (!GetDlgItemText(hwndDlg_, IDC_EDIT1, m_Username, 40))	wsprintf(m_Username, L"default_user");
					EndDialog(hwndDlg_, wParam_); 

				}

				return TRUE; 

			case ID_REPEAT: 
				// force repeated calibration

				delete [] calibBuffer;
				calibBuffer = NULL;
				calibBuffersize = 0;
				isLoadCalibFile = false;
                EndDialog(hwndDlg_, 0);
                return TRUE; 

            case ID_DETAILS: 
				//TODO: Implement this
				break;

			case IDOK:
			case IDCANCEL:
			case IDCLOSE:

				if (calib_status == 3) { 
					// delete calibration

					delete [] calibBuffer;
					calibBuffer = NULL;
					calibBuffersize = 0;
					isLoadCalibFile = false;
					EndDialog(hwndDlg_, wParam_);
					PostMessage(g_hWnd, WM_COMMAND, ID_STOP, 0);

				}

                EndDialog(hwndDlg_, wParam_); 
                return TRUE; 

        } 

		break;

    } 

    return FALSE; 

} 

INT_PTR CALLBACK MessageLoopThread(HWND dialogWindow_, UINT message_, WPARAM wParam_, LPARAM lParam_) { 

	HMENU menu1 = GetMenu(dialogWindow_);

	switch (message_) {

		case WM_INITDIALOG:
	
			SaveLayout(dialogWindow_);
			// TODO: Autoload "default_user" calib config

			return TRUE; 

			break;

		case WM_COMMAND:

			switch (LOWORD(wParam_)) {

				case IDCANCEL:

					isStopped = true;

					if (isRunning) {

						PostMessage(dialogWindow_, WM_COMMAND, IDCANCEL, 0);

					} else {

						DestroyWindow(dialogWindow_); 
						PostQuitMessage(0);

					}

					return TRUE;

				case ID_CALIB_LOADED: 
					// calibration was loaded
					InitCalibWindows(mode_live);
					break;

				case ID_CALIB_DONE: 
					// calibration was completed
			
					isPaused = true;

					CloseCalibWindows();

					if (calib_status == LOAD_CALIBRATION_ERROR) {

						MessageBoxA(g_hWnd, "Load calibration failed, data invalid", "Calibration Load status ", MB_OK | MB_ICONINFORMATION);
						InitCalibWindows(mode_calib);
						isPaused = false;
						break;

					}
	
					if (calib_status == 3) {

						MessageBoxA(g_hWnd, "Please repeat while being in front of the camera and observing the points.",
							"Calibration Required", MB_OK | MB_ICONINFORMATION);

						calibBuffer = NULL;
						calibBuffersize = 0;
						isLoadCalibFile = false;
						PostMessage(g_hWnd, WM_COMMAND, ID_STOP, 0);

					} else {

						if (DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CALIB_DIALOG), g_hWnd, (DLGPROC)ResultsProc)) {

							if (calibBuffer) {

								// save this data
								WCHAR temp[max_path_length];
								WCHAR buff[max_path_length];

								GetTempPath(max_path_length, temp);
								wsprintf(buff, L"%s%s.bin", temp, m_Username);

								FILE* my_file;
								errno_t err;
							
								err = _wfopen_s(&my_file, buff, L"wb");

								if (my_file) {

									fwrite(calibBuffer, calibBuffersize, sizeof(pxcBYTE), my_file);
									fclose(my_file);

								}

							}
				
							InitCalibWindows(mode_live);

						} else { 
							// repeat calibration
			
							// stop calibration thread
							isPaused = false;
							isStopped = true;
							Sleep(1000);

							// start new thread
							isStopped = false;
							isRunning = true;

							InitCalibWindows(mode_calib);

							if (processor) delete processor;
							processor = new Processor(dialogWindow_);
							CreateThread(0, 0, ProcessingThread, dialogWindow_, 0, 0);

							return TRUE;

						}

					}
			
					isPaused = false;
					break;

				case ID_LOAD_CALIB: 
					// load calibration file or force a new one
					isLoadCalibFile = GetCalibFile();
					Button_Enable(GetDlgItem(dialogWindow_, ID_START), m_CalibFilename[0] != 0);
					break;

				case ID_NEW_CALIB: 
					// start new calibration
					delete [] calibBuffer;
					calibBuffer = NULL;
					calibBuffersize = 0;
					isLoadCalibFile = false;

				case ID_START:

					Button_Enable(GetDlgItem(dialogWindow_, ID_LOAD_CALIB), false);
					Button_Enable(GetDlgItem(dialogWindow_, ID_NEW_CALIB), false);
					Button_Enable(GetDlgItem(dialogWindow_, ID_START), false);
					Button_Enable(GetDlgItem(dialogWindow_, ID_STOP), true);

					// Greys menu bar
					for (int i = 0;i < GetMenuItemCount(menu1); ++i)
						EnableMenuItem(menu1, i, MF_BYPOSITION | MF_GRAYED);
			
					DrawMenuBar(dialogWindow_);

					isStopped = false;
					isRunning = true;

					// init the calibration windows
					if (isLoadCalibFile == false && calibBuffersize == 0) {

						InitCalibWindows(mode_calib);

					} else if (GetMenuState(GetMenu(dialogWindow_), ID_MODE_PLAYBACK, MF_BYCOMMAND) & MF_CHECKED) {

						InitCalibWindows(mode_playback);

					} else {

						InitCalibWindows(mode_live);

					}

					if (processor) delete processor;

					processor = new Processor(dialogWindow_);
					CreateThread(0, 0, ProcessingThread, dialogWindow_, 0, 0);
			
					return TRUE;

				case ID_STOP:

					isStopped = true;
					CloseCalibWindows();

					if (isRunning) {

						PostMessage(dialogWindow_, WM_COMMAND, ID_STOP, 0);
						CloseCalibWindows();

					} else {
						// Enables menu bar
						for (int i = 0; i < GetMenuItemCount(menu1); ++i)
							EnableMenuItem(menu1, i, MF_BYPOSITION | MF_ENABLED);
				
						DrawMenuBar(dialogWindow_);
				
						Button_Enable(GetDlgItem(dialogWindow_, ID_START), true);
						Button_Enable(GetDlgItem(dialogWindow_, ID_STOP), false);
						Button_Enable(GetDlgItem(dialogWindow_, ID_LOAD_CALIB), true);
						Button_Enable(GetDlgItem(dialogWindow_, ID_NEW_CALIB), true);
					}

					return TRUE;

				case ID_MODE_LIVE:

					CheckMenuItem(menu1, ID_MODE_LIVE, MF_CHECKED);
					CheckMenuItem(menu1, ID_MODE_PLAYBACK, MF_UNCHECKED);
					
					return TRUE;

				case ID_MODE_PLAYBACK:

					CheckMenuItem(menu1, ID_MODE_LIVE, MF_UNCHECKED);
					CheckMenuItem(menu1, ID_MODE_PLAYBACK, MF_CHECKED);
					
					GetPlaybackFile();
					
					return TRUE;

				case ID_ALWAYSON_GPI:
					
					CheckMenuItem(GetSubMenu(menu1, 1), ID_ALWAYSON_GPI, MF_CHECKED);
					CheckMenuItem(GetSubMenu(menu1, 1), ID_ALWAYSON_MOUSE, MF_UNCHECKED); 
					
					break;
				
				case ID_ALWAYSON_MOUSE:
					
					CheckMenuItem(GetSubMenu(menu1, 1), ID_ALWAYSON_MOUSE, MF_CHECKED);
					CheckMenuItem(GetSubMenu(menu1, 1), ID_ALWAYSON_GPI, MF_UNCHECKED);
					
					break;
				
				case ID_HOTKEY_ASSIGNED:
					
					CheckMenuItem(GetSubMenu(menu1, 1), ID_ALWAYSON_MOUSE, MF_UNCHECKED);
					CheckMenuItem(GetSubMenu(menu1, 1), ID_ALWAYSON_GPI, MF_UNCHECKED);
					CheckMenuItem(GetSubMenu(menu1, 1), ID_HOTKEY_ASSIGNED, MF_CHECKED);
					
					break;
				} 
			break;

		case WM_SIZE:
			RedoLayout(dialogWindow_);
			return TRUE;

		case WM_ACTIVATEAPP:
			isActiveApp = wParam_ != 0;
			break;
	} 

	return FALSE; 
} 

int APIENTRY wWinMain(HINSTANCE hInstance_, HINSTANCE, LPTSTR, int) {
	
	if (DEBUG)
	{
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}
	
	// Initialize
	BOOL InitCommonControlsEx(
		_In_ const LPINITCOMMONCONTROLSEX lpInitCtrls
	);

	g_hInstance = hInstance_;

	// Session for device
	session = PXCSession::CreateInstance();

    HWND dialogWindow = CreateDialogW(hInstance_, MAKEINTRESOURCE(IDD_MAINFRAME), 0, MessageLoopThread);

	// set to global window var
	g_hWnd = dialogWindow;

	HWND statusWindow = CreateStatusWindow(WS_CHILD | WS_VISIBLE, L"", dialogWindow, IDC_STATUS);	

	int statusWindowParts[] = {230, -1};
	SendMessage(statusWindow, SB_SETPARTS, sizeof(statusWindowParts)/sizeof(int), (LPARAM) statusWindowParts);
	SendMessage(statusWindow, SB_SETTEXT, (WPARAM)(INT) 0, (LPARAM) (LPSTR) TEXT("Ready"));
	UpdateWindow(dialogWindow);
	
	Graphics* renderer3D = new Graphics(dialogWindow, session);
	if(renderer3D == NULL)
	{
		MessageBoxW(0, L"Failed to create 3D renderer", L"Face Viewer", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}
	renderer = new RendererManager(renderer3D);
	if(renderer == NULL)
	{
		MessageBoxW(0, L"Failed to create renderer manager", L"Face Viewer", MB_ICONEXCLAMATION | MB_OK);
		delete renderer3D;
		return 1;
	}

	// Create global mutex for shared threads
	g_hMutex = CreateMutex(NULL, FALSE, NULL);
	if (g_hMutex == NULL) 
	{
		MessageBoxW(0, L"Failed to create mutex", L"Face Viewer", MB_ICONEXCLAMATION | MB_OK);
		delete renderer;
		return 1;
	}

	renderer->SetRendererType(Graphics::R3D);

	// Create Rendering thread
	CreateThread(NULL, NULL, RenderingThread, NULL, NULL, NULL);

    MSG msg;

	while (GetMessage(&msg, NULL, 0, 0)) {

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		// let other threads breath
		Sleep(0);

    }

	CloseHandle(renderer->GetRenderingFinishedSignal());

	if (calibBuffer)
		delete [] calibBuffer;

	if (processor)
		delete processor;

	if (renderer)
		delete renderer;

	session->Release();
    return (int)msg.wParam;

}