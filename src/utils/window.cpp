#include "window.h"

#include <windowsx.h>

using namespace cg::utils;
using namespace std::string_literals;

HWND window::hwnd = nullptr;

int cg::utils::window::run(cg::renderer::renderer* renderer, HINSTANCE hinstance, int ncmdshow)
{
	RAWINPUTDEVICE Rid[2];

	Rid[0].usUsagePage = 0x01;
	Rid[0].usUsage = 0x02;
	Rid[0].dwFlags = RIDEV_NOLEGACY;
	Rid[0].hwndTarget = 0;

	Rid[1].usUsagePage = 0x01;
	Rid[1].usUsage = 0x06;
	Rid[1].dwFlags = RIDEV_NOLEGACY;
	Rid[1].hwndTarget = 0;

	if (RegisterRawInputDevices(Rid, 2, sizeof Rid[0]) == FALSE)
	{
		return -1;
	}

	LPCWSTR windowClassName = L"DirectX Sample Window Class";
	LPCWSTR windowName = L"DirectX Sample Window";

	constexpr DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

	WNDCLASS windowClass;
	windowClass.lpfnWndProc = window_proc;
	windowClass.hInstance = hinstance;
	windowClass.lpszClassName = windowClassName;
	windowClass.style = 0;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(GRAY_BRUSH));
	windowClass.lpszMenuName = nullptr;

	if (RegisterClass(&windowClass) == 0)
	{
		THROW_ERROR("Failed to register a window class")
	}

	RECT windowBox;
	windowBox.left = 0;
	windowBox.top = 0;
	windowBox.right = static_cast<LONG>(renderer->get_width());
	windowBox.bottom = static_cast<LONG>(renderer->get_height());

	if (!AdjustWindowRect(&windowBox, windowStyle, false))
	{
		THROW_ERROR("Failed to adjust window rectangle")
	}

	HWND hWindow = CreateWindowEx(
			0,                               
			windowClassName,
			windowName,
			windowStyle,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowBox.right - windowBox.left,
			windowBox.bottom - windowBox.top,
			nullptr,
			nullptr,
			hinstance,
			renderer
	);

	if (hWindow == nullptr)
	{
		THROW_ERROR("Failed to create a window")
	}

	ShowWindow(hWindow, SW_MAXIMIZE);
	hwnd = hWindow;

	renderer->init();

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	renderer->destroy();
	return static_cast<int>(msg.wParam);
}

LRESULT cg::utils::window::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	cg::renderer::renderer* renderer = reinterpret_cast<cg::renderer::renderer*>(
			GetWindowLongPtr(hwnd, GWLP_USERDATA));

	static float move_x = 0.0f;
	static float move_y = 0.0f;
	static float move_z = 0.0f;

	switch (message)
	{
		case WM_CREATE: {
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lparam);
			SetWindowLongPtr(
					hwnd, GWLP_USERDATA,
					reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
			return 0;
		}

		case WM_PAINT: {
			if (renderer)
			{
				renderer->move_forward(0.01f * move_z);
				renderer->move_right(0.01f * move_x);
				renderer->move_up(0.01f * move_y);
				renderer->update();
				renderer->render();
			}
			return 0;
		}

		case WM_INPUT: {
			unsigned raw_size = sizeof(RAWINPUT);
			static RAWINPUT raw[sizeof(RAWINPUT)];
			GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, raw, &raw_size, sizeof(RAWINPUTHEADER));

			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				LONG x = raw->data.mouse.lLastX;
				LONG y = raw->data.mouse.lLastY;

				renderer->move_yaw(0.1f * x);
				renderer->move_pitch(-0.1f * y);
			}
			else if (raw->header.dwType == RIM_TYPEKEYBOARD)
			{
				if (raw->data.keyboard.Flags == RI_KEY_MAKE)
				{
					switch (raw->data.keyboard.VKey)
					{
						case 'W':
							move_z = 1.0f;
							break;
						case 'S':
							move_z = -1.0f;
							break;
						case 'D':
							move_x = 1.0f;
							break;
						case 'A':
							move_x = -1.0f;
							break;
						case 'E':
							move_y = 1.0f;
							break;
						case 'Q':
							move_y = -1.0f;
							break;
						case VK_ESCAPE:
							PostQuitMessage(0);
							break;
						default:
							break;
					}
				}
				else if (raw->data.keyboard.Flags == RI_KEY_BREAK)
				{
					switch (raw->data.keyboard.VKey)
					{
						case 'W':
							move_z = 0.0f;
							break;
						case 'S':
							move_z = 0.0f;
							break;
						case 'D':
							move_x = 0.0f;
							break;
						case 'A':
							move_x = 0.0f;
							break;
						case 'E':
							move_y = 0.0f;
							break;
						case 'Q':
							move_y = 0.0f;
							break;
					}
				}
			}
			return 0;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
	}
	return DefWindowProc(hwnd, message, wparam, lparam);
}