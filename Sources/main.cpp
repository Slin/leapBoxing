#include "stdafx.h"
#include "LBApplication.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	return LB::Application::GetInstance().Run(hInstance, nCmdShow);
}
