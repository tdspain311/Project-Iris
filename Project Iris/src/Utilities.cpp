#include "Utilities.h"
#include <WindowsX.h>
#include <commctrl.h>
#include "resource.h"

bool Utilities::GetPlaybackState(HWND DialogWindow)
{
	return (GetMenuState(GetMenu(DialogWindow), ID_MODE_PLAYBACK, MF_BYCOMMAND) & MF_CHECKED) != 0;
}

void Utilities::SetStatus(HWND dialogWindow, pxcCHAR *line, StatusWindowPart part)
{
	HWND hwndStatus = GetDlgItem(dialogWindow, IDC_STATUS);
	SendMessage(hwndStatus, SB_SETTEXT, (WPARAM)(INT) part, (LPARAM) (LPSTR) line);
	UpdateWindow(dialogWindow);
}
