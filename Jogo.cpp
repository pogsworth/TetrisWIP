#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <timeapi.h>
#include "Jogo.h"

namespace Jogo
{
	const char* JogoApp::JogoName = "Jogo Default";

	HDC hdc;
	HWND hwnd;
	bool Pause = false;

	JogoApp::JogoApp() :
		KEY_LEFT(VK_LEFT),
		KEY_RIGHT(VK_RIGHT),
		KEY_UP(VK_UP),
		KEY_DOWN(VK_DOWN),
		KEY_ESC(VK_ESCAPE),
		KEY_ENTER(VK_RETURN),
		BUTTON_LEFT(VK_LBUTTON),
		BUTTON_RIGHT(VK_RBUTTON),
		BUTTON_MIDDLE(VK_MBUTTON)
	{}

	LRESULT CALLBACK JogoWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		JogoApp* app = (JogoApp*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		switch (uMsg)
		{
		case WM_KEYDOWN:
			if (app)
				app->KeyDown((u32)wParam);
			break;

		case WM_KEYUP:
			if (app)
				app->KeyUp((u32)wParam);
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
			SetCapture(hwnd);
			if (app)
				app->MouseDown((s16)LOWORD(lParam), (s16)HIWORD(lParam), (u32)wParam);
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
			ReleaseCapture();
			if (app)
				app->MouseUp((s16)LOWORD(lParam), (s16)HIWORD(lParam), (u32)wParam);
			break;

		case WM_MOUSEMOVE:
			if (app)
				app->MouseMove((s16)LOWORD(lParam), (s16)HIWORD(lParam), (u32)wParam);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_SIZE:
			{
				int width = LOWORD(lParam);
				int height = HIWORD(lParam);
				if (app)
					app->Resize(width, height);
				InvalidateRect(hwnd, NULL, FALSE);
			}
			break;

		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);
				if (app)
					app->Draw();
				EndPaint(hwnd, &ps);
			}
			return 0;

		case WM_ACTIVATE:
			if (wParam == WA_INACTIVE)
				Pause = true;
			else
				Pause = false;
			return 0;
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	template<class T>
	T Clamp(T input, T min, T max)
	{
		return input < min
			? min
			: input > max
			? max
			: input;
	}

	Timer::Timer()
	{
		if (!Frequency)
		{
			QueryPerformanceFrequency((LARGE_INTEGER*)&Frequency);
		}
		QueryPerformanceCounter((LARGE_INTEGER*)&Last);
	}

	u64 Timer::Start() 
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&Last);
		return Last;
	}

	double Timer::GetSecondsSinceLast()
	{
		u64 Now;
		QueryPerformanceCounter((LARGE_INTEGER*)&Now);
		return ((double)Now - Last) / Frequency;
		Last = Now;
	}

	u64 Timer::Frequency = 0;

	u32 Random::GetNext()
	{
		State ^= State << 13;
		State ^= State >> 17;
		State ^= State << 5;

		return State;
	}

	void Run(JogoApp& App, int TargetFPS)
	{
		// register window class
		WNDCLASS wc = {};
		wc.lpfnWndProc = JogoWndProc;
		HINSTANCE hInstance = GetModuleHandle(nullptr);
		wc.hInstance = hInstance;
		wc.lpszClassName = App.GetName();

		if (RegisterClass(&wc))
		{
			RECT appRect = { 0, 0, App.Width, App.Height };
			AdjustWindowRect(&appRect, WS_OVERLAPPEDWINDOW, 0);
			u32 WindowWidth = appRect.right - appRect.left;
			u32 WindowHeight = appRect.bottom - appRect.top;

			// create window
			if (hwnd = CreateWindowEx(0, App.GetName(), App.GetName(),
				WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, WindowWidth, WindowHeight,
				nullptr, nullptr, hInstance, nullptr))
			{
				SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)&App);
				hdc = GetDC(hwnd);
				ShowWindow(hwnd, SW_SHOWNORMAL);
				UpdateWindow(hwnd);

				// default 60Hz
				float TargetFrameTimeMS = 16.6666f;
				TargetFPS = Clamp(TargetFPS, 10, 1000);
				TargetFrameTimeMS = 1000.0f / TargetFPS;

				UINT DesiredSchedulerMS = 1;
				bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

				bool Done = false;
				Timer timer;
				timer.Start();
				while (!Done)
				{
					double Seconds = timer.GetSecondsSinceLast();
					float MSeconds = (float)(Seconds * 1000.0);
					int MSecondsLeft = (int)(TargetFrameTimeMS - MSeconds);

					if (MSecondsLeft > 0)
					{
						Sleep(MSecondsLeft);
					}
					float DT = (float)timer.GetSecondsSinceLast();

					if (!Pause)
					{
						timer.Start();

						DT = Clamp(DT, 0.001f, 0.1f);
						Done = App.Tick(DT);
						App.Draw();
					}
					// message pump
					MSG msg = {};
					while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);

						if (msg.message == WM_QUIT)
						{
							Done = true;
						}
					}
				}
			}
		}
	}

	void* Allocate(size_t Size)
	{
		return VirtualAlloc(nullptr, Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	}

	void Free(void* Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}

	void Show(u32* Buffer, int Width, int Height)
	{
		BITMAPINFO Info = {};
		Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
		Info.bmiHeader.biWidth = Width;
		Info.bmiHeader.biHeight = -Height;
		Info.bmiHeader.biPlanes = 1;
		Info.bmiHeader.biBitCount = 32;
		Info.bmiHeader.biCompression = BI_RGB;


		StretchDIBits(hdc,
			0, 0, Width, Height,
			0, 0, Width, Height,
			Buffer,
			&Info,
			DIB_RGB_COLORS, SRCCOPY);
	}

	void DrawString(int x, int y, const char* string)
	{
		RECT r = { x,y,0,0 };
		DrawText(hdc, string, -1, &r, DT_NOCLIP);
	}

	bool IsKeyPressed(int key)
	{
		return (GetAsyncKeyState(key) & 0x8000) != 0;
	}

	void GetMousePos(int& x, int& y)
	{
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(hwnd, &point);
		x = point.x;
		y = point.y;
	}

	void DebugOut(char* message)
	{
		OutputDebugString(message);
	}
}
