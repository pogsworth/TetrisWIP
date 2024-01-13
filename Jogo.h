#pragma once
#include "int_types.h"

namespace Jogo
{

	class JogoApp
	{
		static const char* JogoName;
	public:
		int Width = 1024;
		int Height = 1024;

		JogoApp();

		virtual const char* GetName() const { return JogoName; }

		virtual void Init() {}
		virtual bool Tick(float dt) { return false; }
		virtual void Draw() {}
		virtual void Resize(int width, int height) { Width = width, Height = height; }

		//input
		virtual void KeyDown(u32 key) {}
		virtual void KeyUp(u32 key) {}
		virtual void MouseDown(s32 x, s32 y, u32 buttons) {}
		virtual void MouseUp(s32 x, s32 y, u32 buttons) {}
		virtual void MouseMove(s32 x, s32 y, u32 buttons) {}

		const int KEY_LEFT;
		const int KEY_RIGHT;
		const int KEY_UP;
		const int KEY_DOWN;
		const int KEY_ESC;
		const int KEY_ENTER;
		const int BUTTON_LEFT;
		const int BUTTON_RIGHT;
		const int BUTTON_MIDDLE;
	};

	// event loop
	void Run(JogoApp& App, int TargetFPS);

	// memory
	void* Allocate(size_t Size);
	void Free(void* Memory);

	// graphics
	void Show(u32* Buffer, int Width, int Height);
	void DrawString(int x, int y, const char* string);

	// input
	bool IsKeyPressed(int key);
	void GetMousePos(int& x, int& y);

	// debug
	void DebugOut(char* message);

	struct Timer
	{
		static u64 Frequency;
		u64 Last = 0;

		Timer();
		u64 Start();
		double GetSecondsSinceLast();
	};

	struct Random
	{
		u32 State;

		u32 GetNext();
	};
}