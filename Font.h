#pragma once 

#include "Bitmap.h"

template<typename T>
T max(T a, T b)
{
	return a >= b ? a : b;
}

struct Font {

	u32 CharacterMin;
	u32 CharacterCount;
	u32 CharacterWidth;
	u32 CharacterHeight;
	Bitmap::Rect* CharacterRects;
	Bitmap FontBitmap;

	Bitmap::Rect GetTextSize(const char* Text)
	{
		// FixedWidth Font
		if (!CharacterRects)
		{
			// TODO: don't count characters that aren't mapped
			s32 len = 0;
			const char* p = Text;
			while (*p++)
				len++;
			Bitmap::Rect r = { 0, 0, len * (s32)CharacterWidth, (s32)CharacterHeight };
			return r;
		}
		else
		{
			Bitmap::Rect r = { 0,0 };
			for (const char* p = Text; *p; p++)
			{
				u8 c = *p - CharacterMin;
				if (c > 0 && c < CharacterCount)
				{
					r.w += CharacterRects[c].w;
					r.h = max(r.h, CharacterRects[c].h);
				}
			}
			return r;
		}
	}

	// TODO: Add support for BGRA fonts and anti-aliased alpha fonts
	void DrawText(s32 x, s32 y, const char* Text, u32 color, Bitmap destination)
	{
		// FixedWidth Font - assume characters are packed Bitmap.Width / Font.CharacterWidth per row
		if (!CharacterRects)
		{
			u32 CharactersPerRow = FontBitmap.Width / CharacterWidth;
			s32 cursor = x;
			for (const char* p = Text; *p; p++, cursor += CharacterWidth)
			{
				char c = *p - CharacterMin;
				s32 sx = (c % CharactersPerRow) * CharacterWidth;
				s32 sy = (c / CharactersPerRow) * CharacterHeight;

				destination.PasteBitmapSelection(cursor, y, FontBitmap, { sx, sy, (s32)CharacterWidth, (s32)CharacterHeight }, color);
			}
		}
		else
		{
			s32 cursor = x;
			for (const char* p = Text; *p; p++)
			{
				u8 c = *p - CharacterMin;
				if (c > 0 && c < CharacterCount)
				{
					destination.PasteBitmapSelection(cursor, y, FontBitmap, CharacterRects[c], color);
				}
			}
		}
	}

	static Font LoadFont(const char* filename, Arena& arena);
};
