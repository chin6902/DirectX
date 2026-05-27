/*============================================================================
Contents   :  [main.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/05/27
-----------------------------------------------------------------------------
Windows program
============================================================================*/
#include <Windows.h>

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	MessageBox(nullptr, "メッセージ", "キャプション", MB_YESNOCANCEL | MB_DEFBUTTON1);
	MessageBox(nullptr, "メッセージ", "キャプション", MB_YESNOCANCEL | MB_DEFBUTTON2);
	MessageBox(nullptr, "メッセージ", "キャプション", MB_YESNOCANCEL | MB_DEFBUTTON3);
	MessageBox(nullptr, "メッセージ", "キャプション", MB_YESNOCANCEL | MB_DEFBUTTON4);

	return 0;
}
