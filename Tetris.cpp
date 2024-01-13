#include "Jogo.h"
#include "Bitmap.h"
#include "Font.h"
#include "Arena.h"

class TetrisGame : public Jogo::JogoApp
{
	static const char* Name;
	Bitmap BackBuffer;

	struct Tetromino
	{
		u16 patterns[4];	// 4 rotated bit patterns of a tetris shape
		u32 color;
	};
	// these numbers represent the tetromino shapes with bits in a 4x4 pattern
	// for instance: 0xf = 1111 in binary
	// so 0x0f00 is the pattern:		another one is 0x0232:
	//	0000								0000
	//	1111								0010
	//	0000								0011
	//	0000								0010
	Tetromino TetrisShapes[7] =						// I, J, L, O, S, T, Z
	{
		{ {	0x0f00, 0x2222, 0x00f0, 0x4444 }, 0x00ffff},	// I
		{ {	0x0470, 0x0322, 0x0071, 0x0226 }, 0x0000ff},	// J
		{ {	0x0170, 0x0223, 0x0074, 0x0622 }, 0xff8000},	// L
		{ {	0x0660, 0x0660, 0x0660, 0x0660 }, 0xffff00},	// O
		{ {	0x0360, 0x0231, 0x0036, 0x0462 }, 0x00ff00},	// S
		{ {	0x0270, 0x0232, 0x0072, 0x0262 }, 0xff00ff},	// T
		{ {	0x0630, 0x0132, 0x0063, 0x0264 }, 0xff0000},	// Z
	};

	struct coord { s32 x; s32 y; };
	// Rotation Kicks for J L S T Z pieces.  O doesn't rotate and I has its own rotations below
	// rotations are multiples of 90 degress 0, R=90, 2=180, L=270
	// 4 from rotations, 2 directions, 5 successive offsets to test for collision
	coord RotationKicks[4][2][5] = {
		{	{ {0,0}, {-1,0}, {-1, 1}, {0,-2}, {-1,-2} },	// 0->R
			{ {0,0}, { 1,0}, { 1, 1}, {0,-2}, { 1,-2} } },	// 0->L
		{	{ {0,0}, { 1,0}, { 1,-1}, {0, 2}, { 1, 2} },	// R->2
			{ {0,0}, { 1,0}, { 1,-1}, {0, 2}, { 1, 2} } },	// R->0
		{	{ {0,0}, { 1,0}, { 1, 1}, {0,-2}, { 1,-2} },	// 2->L
			{ {0,0}, {-1,0}, {-1, 1}, {0,-2}, {-1,-2} } },	// 2->R
		{	{ {0,0}, {-1,0}, {-1,-1}, {0, 2}, {-1, 2} },	// L->0
			{ {0,0}, {-1,0}, {-1,-1}, {0, 2}, {-1, 2} } },	// L->2
	};

	coord IRotationKicks[4][2][5] = {
		{	{ {0,0}, {-2,0}, { 1, 0}, {-2,-1}, { 1, 2} },	// 0->R
			{ {0,0}, {-1,0}, { 2, 0}, {-1, 2}, { 2,-1} } },	// 0->L
		{	{ {0,0}, {-1,0}, { 2, 0}, {-1, 2}, { 2,-1} },	// R->2
			{ {0,0}, { 2,0}, {-1, 0}, { 2, 1}, {-1,-2} } },	// R->0
		{	{ {0,0}, { 2,0}, {-1, 0}, { 2, 1}, {-1,-2} },	// 2->L
			{ {0,0}, { 1,0}, {-2, 0}, { 1,-2}, {-2, 1} } },	// 2->R
		{	{ {0,0}, { 1,0}, {-2, 0}, { 1,-2}, {-2, 1} },	// L->0
			{ {0,0}, {-2,0}, { 1, 0}, {-2,-1}, { 1, 2} } },	// L->2
	};

	// list of which piece comes next, this gets shuffled every 7 pieces
	u32 NextPieceList[7] = { 0,1,2,3,4,5,6 };

	int DesiredFrameRate = 60;
	float LastFrameDT = 0.0f;
	float LastFrameRate = (float)DesiredFrameRate;
	float AvgFrameRate = LastFrameRate;

	float Step = 0.0f;
	float StepTime = 0.5;

	int CurrentPieceIndex = 0;
	int CurrentPiece = 0;
	int StartingRow = 21;
	int CurrentRow = StartingRow;
	int StartingCol = 5;
	int CurrentCol = StartingCol;
	int CurrentRotation = 0;
	int BlockSize = 25;
	static const int PlayFieldWidth = 10;
	static const int PlayFieldHeight = 25;
	int PlayField[PlayFieldHeight][PlayFieldWidth] = {};
	int PlayFieldTop = (Height - PlayFieldHeight * BlockSize) / 2;
	int PlayFieldBottom = PlayFieldTop + PlayFieldHeight * BlockSize;
	int PlayFieldLeft = (Width - PlayFieldWidth * BlockSize) / 2;
	bool GameOver = true;
	u32 HeartBeatCount = 0;
	u32 Score = 0;

	s32 mouseX, mouseY;
	s32 deltaX = 0, deltaY = 0;
	bool dragging = false;

	static const size_t GameArenaSize = 128 * 1024 * 1024;
	Arena GameArena;
	Bitmap TestBitmap;
	Font DefaultFont;
	Jogo::Random RandomNumber;
	Jogo::Timer GameTimer;
	char LastChar[2] = {};
public:

	TetrisGame()
	{
		RandomNumber.State = (u32)GameTimer.Start();
		GameArena = Arena::CreateArena(GameArenaSize);
		BackBuffer.Width = Width;
		BackBuffer.Height = Height;
		BackBuffer.PixelSize = 4;
		BackBuffer.Pixels = Jogo::Allocate(Width * Height * 4);
		TestBitmap = Bitmap::LoadBitmap("tetrisback.bmp", GameArena);
		DefaultFont = Font::LoadFont("Font16.fnt", GameArena);
		ShuffleNextList();
		CurrentPiece = NextPieceList[CurrentPieceIndex]; 
	}

	~TetrisGame()
	{
		GameArena.ReleaseMemory();
	}

	const char* GetName() const override { return (char*)Name; }

	void Init() override
	{
	}

	void DrawBlock(s32 row, s32 col, u32 color)
	{
		s32 Size = BlockSize > 1 ? BlockSize - 1 : 1;
		BackBuffer.FillRect({ PlayFieldLeft + (s32)col * BlockSize, PlayFieldBottom - (s32)(row + 1) * BlockSize, Size, Size }, color);
	}

	bool TestCollision(int Row, int Col, int Rotation)
	{
		int bits = TetrisShapes[CurrentPiece].patterns[Rotation];
		int Left = Col - 2;
		int Top = Row + 2;
		int testBit = 0x8000;

		for (int y = 0; y < 4; y++)
		{
			for (int x = 0; x < 4; x++, testBit>>=1)
			{
				if (testBit & bits)
				{
					if (Left + x < 0)
					{
						return true;
					}
					if (Left + x >= PlayFieldWidth)
					{
						return true;
					}
					if (Top - y < 0)
					{
						return true;
					}
					if ((Top - y) >= 0 && PlayField[Top - y][Left+x])
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	void PlacePiece()
	{
		int bits = TetrisShapes[CurrentPiece].patterns[CurrentRotation];
		int Left = CurrentCol - 2;
		int Top = CurrentRow + 2;
		for (int y = 0; y < 4; y++)
		{
			for (int x = 0; x < 4; x++)
			{
				if ((0x8000 >> (y * 4 + x)) & bits)
				{
					PlayField[Top-y][Left+x] = TetrisShapes[CurrentPiece].color;
				}
			}
		}
	}

	void SubtractLine(u32 line)
	{
		for (u32 j = line; j < PlayFieldHeight-1; j++)
		{
			for (u32 i = 0; i < PlayFieldWidth; i++)
			{
				PlayField[j][i] = PlayField[j + 1][i];
			}
		}
	}

	bool IsLineFull(u32 line)
	{
		for (int i = 0; i < PlayFieldWidth; i++)
		{
			if (PlayField[line][i] == 0)
			{
				return false;
			}
		}
		return true;
	}

	u32 RemoveFullLines()
	{
		u32 LinesRemoved = 0;
		for (u32 j = 0; j < PlayFieldHeight - 1; j++)
		{
			while (IsLineFull(j))
			{
				SubtractLine(j);
				LinesRemoved++;
			}
		}
		return LinesRemoved;
	}

	// TODO: move to text processing helpers
	char* itoa(s32 number, char* string)
	{
		s32 n = abs(number);
		char* p = string;
		do
		{
			*p++ = n % 10 + '0';
			n /= 10;
		} while (n);
		if (number < 0)
			*p++ = '-';
		*p-- = 0;
		char* b = string;
		while (b < p)
		{
			char s = *b;
			*b++ = *p;
			*p = s;
		}
		return string;
	}

	void ShuffleNextList()
	{
		for (u32 i = 0; i < 6; i++)
		{
			u32 r = RandomNumber.GetNext() % (7 - i) + i;
			u32 t = NextPieceList[i];
			NextPieceList[i] = NextPieceList[r];
			NextPieceList[r] = t;
		}

		// TODO: make a few text formatting functions
		// and also move this to debugging
		char PieceList[32] = {};
		char* p = PieceList;
		for (u32 i = 0; i < 7; i++)
		{
			p = itoa(NextPieceList[i], p);
			while (*p)
				p++;
			*p++ = ',';
			*p++ = ' ';
		}
		*p++ = '\n';
		*p++ = 0;

		Jogo::DebugOut(PieceList);
	}

	void KeyDown(u32 key) override
	{
		LastChar[0] = key;
		if (!GameOver)
		{
			if (key == KEY_LEFT)
			{
				if (!TestCollision(CurrentRow, CurrentCol - 1, CurrentRotation))
					CurrentCol--;
			}

			if (key == KEY_RIGHT)
			{
				if (!TestCollision(CurrentRow, CurrentCol + 1, CurrentRotation))
					CurrentCol++;
			}

			if (key == KEY_DOWN)
			{
				while (!TestCollision(CurrentRow - 1, CurrentCol, CurrentRotation))
				{
					CurrentRow--;
				}
				Step += StepTime;
			}

			if (key == KEY_UP)
			{
				int NewRotation = CurrentRotation + 1;
				if (NewRotation > 3)
					NewRotation = 0;
				// loop over RotationKicks
				// don't do rotation kicks for o piece
				if (CurrentPiece != 3)
				{
					for (int i = 0; i < 5; i++)
					{
						coord kick = { 0,0 };
						if (CurrentPiece == 0)	// I piece
						{
							kick = IRotationKicks[NewRotation][0][i];
						}
						else
						{
							kick = RotationKicks[NewRotation][0][i];
						}
						if (!TestCollision(CurrentRow + kick.y, CurrentCol + kick.x, NewRotation))
						{
							CurrentRow += kick.y;
							CurrentCol += kick.x;
							CurrentRotation = NewRotation;
							break;
						}
					}
				}
			}
		}
		else
		{
			if (key == KEY_ENTER)
			{
				GameOver = false;
				for (int i = 0; i < PlayFieldWidth * PlayFieldHeight; i++)
				{
					PlayField[0][i] = 0;
				}
				ShuffleNextList();
				CurrentPieceIndex = 0;
				CurrentPiece = NextPieceList[CurrentPieceIndex];
				CurrentRow = StartingRow;
				CurrentCol = StartingCol;
				CurrentRotation = 0;
				Score = 0;
			}
		}
	}

	// TODO: eventually remove mouse processing - not needed
	void MouseDown(s32 x, s32 y, u32 buttons) override
	{
		mouseX = x;
		mouseY = y;
		dragging = true;
	}

	void MouseMove(s32 x, s32 y, u32 buttons) override
	{
		if (dragging)
		{
			deltaX = x - mouseX;
			deltaY = y - mouseY;
		}
	}

	void MouseUp(s32 x, s32 y, u32 buttons) override
	{
		if (dragging)
		{
			dragging = false;
		}
	}

	bool Tick(float DT /* do we need anything else passed in here?*/) override
	{
		if (Jogo::IsKeyPressed(KEY_ESC))
		{
			return true;
		}

		bool HeartBeat = false;
		Step += DT;
		if (Step >= StepTime)
		{
			HeartBeat = true;
			HeartBeatCount++;
			Step -= StepTime;
		}

		if (!GameOver)
		{
			if (HeartBeat)
			{
				if (TestCollision(CurrentRow-1, CurrentCol, CurrentRotation))
				{
					// place the pieces in the playfield and start a new piece descending
					PlacePiece();

					u32 NumLinesRemoved = RemoveFullLines();
					if (NumLinesRemoved)
					{
						Score += (2 * NumLinesRemoved - 1) * 100;
						if (NumLinesRemoved == 4)
							Score += 100;
					}

					CurrentRow = StartingRow;
					CurrentCol = StartingCol;
					CurrentRotation = 0;
					CurrentPieceIndex++;
					if (CurrentPieceIndex > 6)
					{
						CurrentPieceIndex = 0;
						ShuffleNextList();
					}
					CurrentPiece = NextPieceList[CurrentPieceIndex];
					if (TestCollision(CurrentRow, CurrentCol, CurrentRotation))
					{
						GameOver = true;
					}
				}
				else
				{
					CurrentRow--;
				}
			}
		}

		LastFrameDT = DT;
		LastFrameRate = 1.0f / DT;
		AvgFrameRate = (31.0f * AvgFrameRate + LastFrameRate) / 32.0f;
		return false;
	}

	void ClearBuffer(u32 color)
	{
		BackBuffer.Erase(color);
	}

	void DrawPlayField()
	{
		for (int y = 0; y < PlayFieldHeight; y++)
		{
			for (int x = 0; x < PlayFieldWidth; x++)
			{
				if (int color = PlayField[y][x])
				{
					DrawBlock(y, x, color);
				}
			}
		}
	}

	void DrawPiece()
	{
		int Left = CurrentCol - 2;
		int Top = CurrentRow + 2;
		for (int y = 0; y < 4; y++)
		{
			for (int x = 0; x < 4; x++)
			{
				int ShiftRight = y * 4 + x;
				int bit = 0x8000 >> ShiftRight;
				if (bit & TetrisShapes[CurrentPiece].patterns[CurrentRotation])
				{
					DrawBlock(Top - y, Left + x, TetrisShapes[CurrentPiece].color);
				}
			}
		}
	}

	void DrawPlayfieldBorder()
	{
		Bitmap::Rect Border = { PlayFieldLeft - 2, PlayFieldTop - 2, PlayFieldWidth * BlockSize + 4, PlayFieldHeight * BlockSize + 4 };
		BackBuffer.DrawRect(Border, 0x804040);
		Border = { PlayFieldLeft - 4, PlayFieldTop - 4, PlayFieldWidth * BlockSize + 8, PlayFieldHeight * BlockSize + 8 };
		BackBuffer.DrawRect(Border, 0x804040);
	}

	template<typename T>
	T abs(T x)
	{
		return x >= 0 ? x : -x;
	}

	void Draw() override
	{
		u32 black = 0x00000000;
		ClearBuffer(black);

		// TODO: move to testing function
		BackBuffer.PasteBitmap(deltaX*4, deltaY*4, TestBitmap, 0xff0000);
		const char* HelloWorld = "Hello World!";
		u32 cursor = 250;
//		DefaultFont.DrawText(deltaX, deltaY, HelloWorld, 0xff0000, BackBuffer);
		s32 x1 = mouseX;
		s32 y1 = mouseY;
		s32 x2 = mouseY + deltaX;
		s32 y2 = mouseY + deltaY;
		bool clipped = BackBuffer.ClipLine(x1, y1, x2, y2, { 0,0, (s32)BackBuffer.Width, (s32)BackBuffer.Height });
		const char* Clipped = "Clipped!";
		const char* NotClipped = "";
		const char* Message = !clipped ? Clipped : NotClipped;
		DefaultFont.DrawText(0, 0, Message, 0xffffff, BackBuffer);

		// TODO: move to debug section
		char X[16] = {};
		char Y[16] = {};
		itoa(mouseX + deltaX, X);
		itoa(mouseY + deltaY, Y);

		DefaultFont.DrawText(0, 16, X, 0xffffff, BackBuffer);
		DefaultFont.DrawText(0, 32, Y, 0xffffff, BackBuffer);
		DefaultFont.DrawText(0, 48, LastChar, 0xffffff, BackBuffer);

//		BackBuffer.DrawLine(mouseX, mouseY, mouseX + deltaX, mouseY + deltaY, 0x00ff00);
		DrawPlayField();
		DrawPlayfieldBorder();

		if (!GameOver)
		{
			DrawPiece();
		}

		// TODO: move to DrawScore function
		char ScoreString[32] = "Score: 000000";
		char* p = ScoreString;
		while (*p) p++;
		p--;
		u32 PrintScore = Score;
		while (PrintScore)
		{
			*p-- = PrintScore % 10 + '0';
			PrintScore /= 10;
		}
		Bitmap::Rect ScoreSize = DefaultFont.GetTextSize(ScoreString);
		DefaultFont.DrawText((Width - ScoreSize.w) / 2, 0, ScoreString, 0xffffff, BackBuffer);

		if (GameOver)
		{
			if (HeartBeatCount & 1)
			{

				char GameOver[32] = "Game Over";
				Bitmap::Rect GameOverSize = DefaultFont.GetTextSize(GameOver);
				DefaultFont.DrawText((Width - GameOverSize.w)/2, (Height - DefaultFont.CharacterHeight)/2, GameOver, 0xffffff, BackBuffer);
				char PressEnterToPlay[32] = "Press Enter to Play";
				Bitmap::Rect MessageSize = DefaultFont.GetTextSize(PressEnterToPlay);
				DefaultFont.DrawText((Width - MessageSize.w)/2, (Height + DefaultFont.CharacterHeight*3)/2, PressEnterToPlay, 0xffffff, BackBuffer);
			}
		}

		Jogo::Show(BackBuffer.PixelBGRA, Width, Height);
	}

	// TODO: handle resizing BackBuffer here
	void Resize(int width, int height) override {}
};
const char* TetrisGame::Name = "Tetris";

int main(int argc, char* argv[])
{
	TetrisGame tetris;
	Jogo::Run(tetris, 60);
	return 0;
}