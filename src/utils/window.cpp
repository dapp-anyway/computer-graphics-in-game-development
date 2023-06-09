#include "window.h"

#include <windowsx.h>

using namespace cg::utils;
using namespace std::string_literals;

HWND window::hwnd = nullptr;

int cg::utils::window::run(cg::renderer::renderer* renderer, HINSTANCE hinstance, int ncmdshow)
{
	// Initialize Win32 RAW INPUT because it is fast
	RAWINPUTDEVICE Rid[2];

	Rid[0].usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
	Rid[0].usUsage = 0x02; // HID_USAGE_GENERIC_MOUSE
	Rid[0].dwFlags = RIDEV_NOLEGACY; // adds mouse and also ignores legacy mouse messages
	Rid[0].hwndTarget = 0;

	Rid[1].usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
	Rid[1].usUsage = 0x06; // HID_USAGE_GENERIC_KEYBOARD
	Rid[1].dwFlags = RIDEV_NOLEGACY; // adds keyboard and also ignores legacy keyboard messages
	Rid[1].hwndTarget = 0;

	if (RegisterRawInputDevices(Rid, 2, sizeof Rid[0]) == FALSE)
	{
		return -1;
	}

	// Register the window class.
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

	// Create the window.
	HWND hWindow = CreateWindowEx(
		0, // Optional window styles.
		windowClassName, // Window class
		windowName, // Window text
		windowStyle, // Window style
		CW_USEDEFAULT, // Position x
		CW_USEDEFAULT, // Position y
		windowBox.right - windowBox.left, // Window width
		windowBox.bottom - windowBox.top, // Window height
		nullptr, // Parent window
		nullptr, // Menu
		hinstance, // Instance handle
		renderer // Additional application data
	);

	if (hWindow == nullptr)
	{
		THROW_ERROR("Failed to create a window")
	}

	ShowWindow(hWindow, SW_MAXIMIZE);
	hwnd = hWindow;

	// Initialize the sample. OnInit is defined in each child-implementation of DXSample.
	renderer->init();

	// Main sample loop.
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
	// Return this part of the WM_QUIT message to Windows.
	return static_cast<int>(msg.wParam);
}

LRESULT cg::utils::window::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	cg::renderer::renderer* renderer = reinterpret_cast<cg::renderer::renderer*>(
		GetWindowLongPtr(hwnd, GWLP_USERDATA));

	// Movement states are kept to make motion continuous
	static float move_x = 0.0f;
	static float move_y = 0.0f;
	static float move_z = 0.0f;

	switch (message)
	{
		case WM_CREATE:
		{
			// Save the Renderer* passed in to CreateWindow.
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lparam);
			SetWindowLongPtr(
				hwnd, GWLP_USERDATA,
				reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
			return 0;
		}

		case WM_PAINT:
		{
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

		case WM_INPUT:
		{
			unsigned raw_size = sizeof(RAWINPUT);
			static RAWINPUT raw[sizeof(RAWINPUT)];
			GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, raw, &raw_size, sizeof(RAWINPUTHEADER));

			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				LONG x = raw->data.mouse.lLastX;
				LONG y = raw->data.mouse.lLastY;

				renderer->move_yaw(0.1f * x);
				renderer->move_pitch(-0.1f * y); // Y-axis is inverted
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
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hwnd, message, wparam, lparam);
}
