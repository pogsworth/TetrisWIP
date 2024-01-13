#include <stdio.h>
#include "Arena.h"
#include "Font.h"

Font Font::LoadFont(const char* filename, Arena& arena)
{
	struct FontHeader
	{
		u32 CharacterMin;
		u32 CharacterCount;
		u32 CharacterWidth;
		u32 CharacterHeight;
		u32 FixedWidth;
		const char BitmapFilename[32] = {};
	} FontFileHeader;

	Font LoadedFont = {};
	FILE* fp;
	if (!fopen_s(&fp, filename, "rb"))
	{
		fread(&FontFileHeader, sizeof(FontFileHeader), 1, fp);
		LoadedFont.CharacterMin = FontFileHeader.CharacterMin;
		LoadedFont.CharacterCount = FontFileHeader.CharacterCount;
		LoadedFont.CharacterWidth = FontFileHeader.CharacterWidth;
		LoadedFont.CharacterHeight = FontFileHeader.CharacterHeight;
		if (!FontFileHeader.FixedWidth)
		{
			LoadedFont.CharacterRects = (Bitmap::Rect*)arena.Allocate(FontFileHeader.CharacterCount * sizeof(Bitmap::Rect));
			fread(LoadedFont.CharacterRects, sizeof(Bitmap::Rect), FontFileHeader.CharacterCount, fp);
		}
		fclose(fp);
	}
	if (FontFileHeader.BitmapFilename[0] != 0)
	{
		LoadedFont.FontBitmap = Bitmap::LoadBitmap(FontFileHeader.BitmapFilename, arena);
	}
	return LoadedFont;
}
