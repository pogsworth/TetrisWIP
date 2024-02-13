#pragma once

#include "Jogo.h"
#include "Bitmap.h"
#include "Font.h"

namespace UI
{
	Bitmap Target;
	Font DefaultFont;
	const u32 FirstID = 0xf000;
	u32 NextID = FirstID;
	u32 HotID = 0;
	u32 ActiveID = 0;
	s32 CursorX;
	s32 CursorY;
	u32 ButtonColor = 0x808080;
	u32 HotColor = 0xa0a0a0;
	u32 HiLight = 0xc0c0c0;
	u32 LoLight = 0x404040;
	u32 TextColor = 0xf0f0f0;
	u32 CurrentColor;
	u32 HiColor;
	u32 LoColor;
	u32 Black = 0;
	u32 White = 0xffffff;

	void Init(Bitmap& InTarget, Font InDefaultFont)
	{
		Target = InTarget;
		DefaultFont = InDefaultFont;
	}

	u32 GetID()
	{
		return NextID++;
	}

	bool Interact(u32 Id, const Bitmap::Rect& r, s32 mousex, s32 mousey)
	{
		bool clicked = false;

		if (ActiveID == Id)
		{
			HiColor = LoLight;
			LoColor = HiLight;
			if (!Jogo::IsKeyPressed(Jogo::JogoApp::BUTTON_LEFT))
			{
				if (HotID == Id)
				{
					clicked = true;
				}
				ActiveID = 0;
			}
		}
		else if (Id == HotID)
		{
			if (Jogo::IsKeyPressed(Jogo::JogoApp::BUTTON_LEFT))
			{
				ActiveID = Id;
			}
		}

		// SetHot
		if (mousex >= r.x && mousex < r.x + r.w && mousey >= r.y && mousey < r.y + r.h)
		{
			if (ActiveID == Id || (ActiveID == 0 && !Jogo::IsKeyPressed(Jogo::JogoApp::BUTTON_LEFT)))
			{
				HotID = Id;
				CurrentColor = HotColor;
			}
		}
		else if (HotID == Id)
		{
			HotID = 0;
		}

		return clicked;
	}

	void DrawButton(Bitmap::Rect& r, const char* Text)
	{
		Target.FillRect(r, CurrentColor);
		Target.DrawHLine(CursorY, r.x, r.x + r.w - 1, HiColor);
		Target.DrawHLine(CursorY + r.h - 1, r.x, r.x + r.w - 1, LoColor);
		Target.DrawVLine(CursorX, r.y, r.y + r.h - 1, HiColor);
		Target.DrawVLine(CursorX + r.w, r.y, r.y + r.h - 1, LoColor);
		DefaultFont.DrawText(CursorX + 4, CursorY + 4, Text, TextColor, UI::Target);
	}

	bool Button(const char* Text)
	{
		bool clicked = false;

		u32 ButtonID = GetID();
		// get an id for this button
		// compare to hot and active
		// calc return value (based on mouse in rect for this button)
		Bitmap::Rect TextSize = DefaultFont.GetTextSize(Text);
		TextSize.x = CursorX;
		TextSize.y = CursorY;
		TextSize.w += 8;
		TextSize.h += 8;
		CurrentColor = ButtonColor;
		HiColor = HiLight;
		LoColor = LoLight;

		int x, y;
		Jogo::GetMousePos(x, y);

		// TODO: call Layout here?  to give a chance for UI layout to arrange things...

		clicked = Interact(ButtonID, TextSize, x, y);

		DrawButton(TextSize, Text);

		// advance layout cursor
		// TODO: maybe this belongs in a UI::Layout function?
		CursorY += TextSize.h + 1;

		return clicked;
	}

	// need to pass in input state to BeginFrame
	void BeginFrame(Bitmap::Rect Frame)
	{
		NextID = FirstID;
		CursorX = Frame.x;
		CursorY = Frame.y;
	}
}

