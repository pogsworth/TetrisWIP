#include <stdio.h>
#include "Arena.h"
#include "Bitmap.h"

Bitmap Bitmap::LoadBitmap(const char* filename, Arena& arena)
{
#pragma pack(push,2)
	struct BitmapHeader
	{
		u16 BMPSignature;	// usually 'BM'
		u32 FileSize;
		u16 Reserved;
		u16 Reserved2;
		u32 ImageOffset;

		// BitmapInfoHeader
		u32 StructureSize;
		s32 Width;
		s32 Height;
		u16 Planes;
		u16 BitCount;
		u32 Compression;
		u32 ImageSize;
		u32 XPixelsPerMeter;
		u32 YPixelsPerMeter;
		u32 ColorsUsed;
		u32 ColorImportant;
	} Header = {};
#pragma pack(pop)

	FILE* fp = nullptr;
	if (!fopen_s(&fp, filename, "rb"))
	{
		fread(&Header, sizeof(BitmapHeader), 1, fp);
		if (Header.ImageOffset != sizeof(BitmapHeader))
		{
			fseek(fp, Header.ImageOffset, SEEK_SET);
		}

		if (Header.BitCount == 8 || Header.BitCount == 24 || Header.BitCount == 32)
		{
			u32 FlipImage = Header.Height > 0;
			u32 ImageHeight = Header.Height < 0 ? -Header.Height : Header.Height;

			if (Header.Width <= 65536 && ImageHeight <= 65536)
			{
				u32 FilePixelSize = Header.BitCount / 8;
				// convert 24-bit bitmaps to 32-bit
				u32 ImagePixelSize = FilePixelSize;
				if (ImagePixelSize == 3)
					ImagePixelSize = 4;

				Bitmap Image = { (u32)Header.Width, ImageHeight, ImagePixelSize };
				Image.Pixels = arena.Allocate(Header.Width * ImageHeight * ImagePixelSize);
				if (Image.Pixels != nullptr)
				{
					u8* RowPtr = Image.PixelA;
					s32 ImageStride = Image.Width * ImagePixelSize;
					if (FlipImage)
					{
						RowPtr = Image.PixelA + (ImageHeight - 1) * Header.Width * ImagePixelSize;
						ImageStride = -ImageStride;
					}
					u32 LeftOver = Image.Width * FilePixelSize & 0x3;
					LeftOver = LeftOver ? 4 - LeftOver : 0;
					for (u32 i = 0; i < ImageHeight; i++)
					{
						fread(RowPtr, Image.Width * FilePixelSize, 1, fp);
						if (LeftOver)
						{
							fseek(fp, LeftOver, SEEK_CUR);
						}

						// convert 24-bit bitmaps to 32-bit
						if (FilePixelSize == 3)
						{
							u8* EndOfFileRow = RowPtr + Image.Width * FilePixelSize - 1;
							u8* EndOfImageRow = RowPtr + Image.Width * ImagePixelSize - 1;
							for (u32 j = 0; j < Image.Width; j++)
							{
								*EndOfImageRow-- = 0xff;
								*EndOfImageRow-- = *EndOfFileRow--;
								*EndOfImageRow-- = *EndOfFileRow--;
								*EndOfImageRow-- = *EndOfFileRow--;
							}
						}
						RowPtr += ImageStride;
					}

					fclose(fp);
					return Image;
				}
			}
		}
		fclose(fp);
	}

	Bitmap EmptyBitmap = {};
	return EmptyBitmap;
}
