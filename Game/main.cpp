/*============================================================================
Contents   :  [main.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/05/27
-----------------------------------------------------------------------------
Windows program
============================================================================*/
#include <Windows.h>

#include "debug_ostream.h"

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	int result = MessageBox(
		nullptr,
		"Delete?",
		"Confirmation",
		MB_YESNOCANCEL | MB_ICONEXCLAMATION
);

	switch (result)
	{
	case IDYES:
		hal::dout << "You chose Yes." << std::endl;
		break;
	case IDNO:
		hal::dout << "You chose No." << std::endl;
		break;
	case IDCANCEL:
		hal::dout << "You chose Cancel." << std::endl;
		break;
	default:
		break;
	}

	return 0;
}
