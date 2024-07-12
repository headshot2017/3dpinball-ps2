#pragma once

struct BMFont;

class bmfont
{
public:
	static BMFont* Init(u8* buf, u32 bufSize);
	static void Free(BMFont* font);
	static void Render(void* gsGlobal, void* gsTex, BMFont* font, std::string text, float x, float y, float scale=1.f, bool center=false);

private:
	static float GetCenter(BMFont* font, std::string text, float scale);
};