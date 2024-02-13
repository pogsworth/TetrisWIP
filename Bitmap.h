#pragma once

#include <intrin.h>
#include <stdint.h>
#include "int_types.h"
#include "Arena.h"

struct Bitmap
{
	u32 Width;
	u32 Height;
	u32 PixelSize;		// in bytes
	union
	{
		void* Pixels;
		u8* PixelA;
		u32* PixelBGRA;
	};

	struct Rect
	{
		s32 x, y, w, h;
	};

	void Erase(u32 color)
	{
		FillRect({ 0,0,(s32)Width,(s32)Height }, color);
	}

	void FillRect(const Rect& r, u32 color)
	{
		Rect clip;

		if (!ClipRect(r,clip))
			return;

		if (PixelSize == 1)
		{
			u8* row = PixelA + (clip.y * Width + clip.x) * PixelSize;
			for (s32 i = 0; i < clip.h; i++)
			{
				__stosb(row, (unsigned char)color, (size_t)clip.w);
				row += Width;
			}
		}
		else if (PixelSize == 4)
		{
			u32* row = PixelBGRA + (clip.y * Width + clip.x);
			for (s32 i = 0; i < clip.h; i++)
			{
				__stosd((unsigned long*)row, (unsigned long)color, (size_t)clip.w);
				row += Width;
			}
		}
	}

	bool ClipRect(const Rect &r, Rect& Clipped)
	{
		Clipped = r;

		// test the all included case first
		if (r.x >= 0 && r.x + r.w <= (s32)Width && r.y >= 0 && r.y + r.h <= (s32)Height)
			return true;

		// test all excluded next
		if (r.x >= (signed)Width || r.y >= (signed)Height)
			return false;

		// now alter any coords that need to be fixed...
		if (r.x + r.w <= 0)
			return false;

		if (r.y + r.h <= 0)
			return false;

		if (r.x < 0)
		{
			Clipped.w += r.x;
			Clipped.x = 0;
		}

		if (r.y < 0)
		{
			Clipped.h += r.y;
			Clipped.y = 0;
		}

		if (Clipped.x + Clipped.w > (signed)Width)
		{
			Clipped.w = Width - Clipped.x;
		}

		if (Clipped.y + Clipped.h > (signed)Height)
		{
			Clipped.h = Height - Clipped.y;
		}

		return true;
	}

	void PasteBitmap(int x, int y, Bitmap source, u32 color)
	{
		PasteBitmapSelection(x, y, source, { 0, 0, (s32)source.Width, (s32)source.Height }, color);
	}

	void PasteBitmapSelection(int x, int y, Bitmap source, const Rect& srcRect, u32 color)
	{
		if (!source.Pixels)
			return;

		Rect SrcClip;
		if (!source.ClipRect(srcRect, SrcClip))
			return;

		Rect DstClip;
		if (!ClipRect({ x, y, SrcClip.w, SrcClip.h }, DstClip))
			return;

		SrcClip.x += DstClip.x - x;
		SrcClip.y += DstClip.y - y;
		u8* SrcRow = source.PixelA + SrcClip.y * source.Width * source.PixelSize + SrcClip.x * source.PixelSize;
		u8* DstRow = PixelA + DstClip.y * Width * PixelSize + DstClip.x * PixelSize;

		if (PixelSize == source.PixelSize)
		{
			for (s32 j = 0; j < DstClip.h; j++)
			{
				__movsb(DstRow, SrcRow, DstClip.w * PixelSize);
				SrcRow += source.Width * PixelSize;
				DstRow += Width * PixelSize;
			}
		}
		else if (PixelSize == 4)
		{
			for (s32 j = 0; j < DstClip.h; j++)
			{
				for (s32 i = 0; i < DstClip.w; i++)
				{
					if (*SrcRow)
					{
						*(u32*)DstRow = color;
					}
					SrcRow += source.PixelSize;
					DstRow += PixelSize;
				}
				SrcRow += (source.Width - DstClip.w) * source.PixelSize;
				DstRow += (Width - DstClip.w) * PixelSize;
			}
		}
	}

	void SetPixel(s32 x, s32 y, u32 color)
	{
		if (x < 0 || x >= (s32)Width || y < 0 || y >= (s32)Height)
			return;

		if (PixelSize == 1)
			*(PixelA + y * Width + x) = color;
		else
			*(PixelBGRA + y * Width + x) = color;
	}

	bool ClipLine(s32& x1, s32& y1, s32& x2, s32& y2, Rect clipRect)
	{
		s32 dx = x2 - x1;
		s32 dy = y2 - y1;
		s32 x;
		s32 y;
		s32 right = clipRect.x + clipRect.w;
		s32 bottom = clipRect.y + clipRect.h;

		// clip the line to the dimensions of the Bitmap
		u32 code1 = ((x1 < clipRect.x) << 3) | ((y1 < clipRect.y) << 2) | ((x1 >= clipRect.x + clipRect.w) << 1) | (y1 >= clipRect.y + clipRect.h);
		u32 code2 = ((x2 < clipRect.x) << 3) | ((y2 < clipRect.y) << 2) | ((x2 >= clipRect.x + clipRect.h) << 1) | (y2 >= clipRect.y + clipRect.h);
		
		// if both endpoints are outside of any of the same edge, then the whole line is out
		if (code1 & code2)
			return false;

		while (code1 | code2)
		{
			u32 outcode = code1 > code2 ? code1 : code2;

			if (outcode & 8)
			{
				y = y2 - dy * (x2 - clipRect.x) / dx;
				x = clipRect.x;
			}
			else if (outcode & 4)
			{
				x = x2 - dx * (y2 - clipRect.y) / dy;
				y = clipRect.y;
			}
			else if (outcode & 2)
			{
				y = y2 - dy * (x2 - right + 1) / dx;
				x = right - 1;
			}
			else if (outcode & 1)
			{
				x = x2 - dx * (y2 - bottom + 1) / dy;
				y = bottom - 1;
			}

			if (outcode == code1)
			{
				x1 = x;
				y1 = y;
				code1 = ((x1 < clipRect.x) << 3) | ((y1 < clipRect.y) << 2) | ((x1 >= clipRect.x + clipRect.w) << 1) | (y1 >= clipRect.y + clipRect.h);
			}
			else
			{
				x2 = x;
				y2 = y;
				code2 = ((x2 < clipRect.x) << 3) | ((y2 < clipRect.y) << 2) | ((x2 >= clipRect.x + clipRect.h) << 1) | (y2 >= clipRect.y + clipRect.h);
			}
			if (code1 & code2)
				return false;
		}
		return true;
	}

	template<typename T>
	T abs(T x)
	{
		return x >= 0 ? x : -x;
	}

	void DrawLine(s32 x1, s32 y1, s32 x2, s32 y2, u32 color)
	{
		if (ClipLine(x1, y1, x2, y2, { 0,0,(s32)Width,(s32)Height }))
		{
			s32 dx = abs(x2 - x1);
			s32 sx = x1 < x2 ? 1 : -1;
			s32 dy = -abs(y2 - y1);
			s32 sy = y1 < y2 ? 1 : -1;
			s32 error = dx + dy;
			s32 x = x1;
			s32 y = y1;

			while (true)
			{
				SetPixel(x, y, color);
				if (x == x2 && y == y2)
					break;

				s32 e2 = error + error;
				if (e2 >= dy)
				{ 
					if (x == x2)
						break;
					error += dy;
					x += sx;
				}
				if (e2 <= dx)
				{
					if (y == y2)
						break;
					error += dx;
					y += sy;
				}
			}
		}
	}

	void DrawRect(const Rect& box, u32 color)
	{
		s32 right = box.x + box.w - 1;
		s32 bottom = box.y + box.h - 1;

		DrawLine(box.x, box.y, right, box.y, color);
		DrawLine(right, box.y, right, bottom, color);
		DrawLine(right, bottom, box.x, bottom, color);
		DrawLine(box.x, bottom, box.x, box.y, color);
	}

	void DrawHLine(s32 y, s32 x1, s32 x2, u32 color)
	{
		if (ClipLine(x1, y, x2, y, { 0,0,(s32)Width,(s32)Height }))
		{
			for (s32 x = x1; x <= x2; x++)
			{
				SetPixel(x, y, color);
			}
		}
	}

	void DrawVLine(s32 x, s32 y1, s32 y2, u32 color)
	{
		if (ClipLine(x, y1, x, y2, { 0,0,(s32)Width,(s32)Height }))
		{
			for (s32 y = y1; y <= y2; y++)
			{
				SetPixel(x, y, color);
			}
		}
	}

	static Bitmap LoadBitmap(const char* filename, Arena& arena);
};
