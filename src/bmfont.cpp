#include "pch.h"
#include "bmfont.h"

#include <cstdio>
#include <cstring>
#include <malloc.h>
#include <unordered_map>

#include <gsKit.h>
#include <dmaKit.h>

// https://github.com/vladimirgamalyan/fontbm

struct BMFont
{
	BMFont()
	{
		info.fontName = 0;
	}
	~BMFont()
	{
		if (info.fontName)
		{
			printf("delete font name\n");
			delete[] info.fontName;
		}
		for (auto it = pages.begin(); it != pages.end(); it++)
			delete[] it->second;
	}

#pragma pack(push, 1)
	struct InfoBlock
	{
		int16_t fontSize;
		int8_t smooth:1;
		int8_t unicode:1;
		int8_t italic:1;
		int8_t bold:1;
		int8_t reserved:4;
		uint8_t charSet;
		uint16_t stretchH;
		int8_t aa;
		uint8_t paddingUp;
		uint8_t paddingRight;
		uint8_t paddingDown;
		uint8_t paddingLeft;
		uint8_t spacingHoriz;
		uint8_t spacingVert;
		uint8_t outline;
		char* fontName;
	} info;
	static_assert(sizeof(InfoBlock) == 18, "InfoBlock size is not 18");

	struct CommonBlock
	{
		uint16_t lineHeight;
		uint16_t base;
		uint16_t scaleW;
		uint16_t scaleH;
		uint16_t pages;
		uint8_t reserved:7;
		uint8_t packed:1;
		uint8_t alphaChnl;
		uint8_t redChnl;
		uint8_t greenChnl;
		uint8_t blueChnl;
	} common;
	static_assert(sizeof(CommonBlock) == 15, "CommonBlock size is not 15");

	struct CharBlock
	{
		uint32_t id;
		uint16_t x;
		uint16_t y;
		uint16_t width;
		uint16_t height;
		int16_t xoffset;
		int16_t yoffset;
		int16_t xadvance;
		uint8_t page;
		uint8_t channel;
	};
	static_assert(sizeof(CharBlock) == 20, "CharBlock size is not 20");

	struct KerningPairsBlock
	{
		uint32_t first;
		uint32_t second;
		int16_t amount;
	};
	static_assert(sizeof(KerningPairsBlock) == 10, "KerningPairsBlock size is not 10");
#pragma pack(pop)

	std::unordered_map<int, char*> pages;
	std::unordered_map<int, CharBlock> chars;
	std::vector<KerningPairsBlock> kernings;
};


BMFont* bmfont::Init(u8* buf, u32 bufSize)
{
	printf("Loading BMFont... (size %d)\n", bufSize);

	if (!buf || !bufSize)
	{
		printf("BMFont: Buffer is invalid\n");
		return 0;
	}

	FILE* f = fmemopen(buf, bufSize, "rb");

	char header[3]; uint8_t version;
	fread(header, 3, 1, f);
	fread(&version, 1, 1, f);
	if (header[0] != 'B' || header[1] != 'M' || header[2] != 'F')
	{
		printf("BMFont: Buffer is not binary BMFont file\n");
		fclose(f);
		return 0;
	}

	BMFont* font = new BMFont;
	uint8_t currBlock; int blockSize;

	// Block 1: Info
	fread(&currBlock, 1, 1, f);
	fread(&blockSize, 4, 1, f);

	if (currBlock != 1)
	{
		printf("BMFont: Failed loading font (currBlock: expected 1, got %d)\n", currBlock);
		fclose(f);
		delete font;
		return 0;
	}
	if (blockSize <= 0)
	{
		printf("BMFont: Failed loading font on block %d (block size is %d)\n", currBlock, blockSize);
		fclose(f);
		delete font;
		return 0;
	}

	fread(&font->info, sizeof(BMFont::InfoBlock) - sizeof(char*), 1, f);

	int fontNameSize = blockSize - (sizeof(BMFont::InfoBlock) - sizeof(char*));
	font->info.fontName = new char[fontNameSize];
	fread(font->info.fontName, fontNameSize, 1, f);
	printf("Font name is '%s'\n", font->info.fontName);

	// Block 2: Common
	fread(&currBlock, 1, 1, f);
	fread(&blockSize, 4, 1, f);

	if (currBlock != 2)
	{
		printf("BMFont: Failed loading font (currBlock: expected 2, got %d) (%d)\n", currBlock, blockSize);
		fclose(f);
		delete font;
		return 0;
	}
	if (blockSize <= 0)
	{
		printf("BMFont: Failed loading font on block %d (block size is %d)\n", currBlock, blockSize);
		fclose(f);
		delete font;
		return 0;
	}

	fread(&font->common, sizeof(BMFont::CommonBlock), 1, f);

	// Block 3: Pages
	fread(&currBlock, 1, 1, f);
	fread(&blockSize, 4, 1, f);

	if (currBlock != 3)
	{
		printf("BMFont: Failed loading font (currBlock: expected 2, got %d) (%d)\n", currBlock, blockSize);
		fclose(f);
		delete font;
		return 0;
	}
	if (blockSize <= 0)
	{
		printf("BMFont: Failed loading font on block %d (block size is %d)\n", currBlock, blockSize);
		fclose(f);
		delete font;
		return 0;
	}

	int i = 0;
	while (blockSize > 0)
	{
		char currChar;

		u32 startPos = ftell(f);
		do {
			fread(&currChar, 1, 1, f);
		} while (currChar != 0);
		u32 endPos = ftell(f); // to include null terminator
		u32 pageSize = endPos - startPos;

		char* pageName = new char[pageSize];
		fseek(f, startPos, SEEK_SET);
		fread(pageName, pageSize, 1, f);
		
		font->pages[i++] = pageName;
		blockSize -= pageSize;
	}

	// Block 4: Chars
	fread(&currBlock, 1, 1, f);
	fread(&blockSize, 4, 1, f);

	if (currBlock != 4)
	{
		printf("BMFont: Failed loading font (currBlock: expected 4, got %d)\n", currBlock);
		fclose(f);
		delete font;
		return 0;
	}
	if (blockSize <= 0)
	{
		printf("BMFont: Failed loading font on block %d (block size is %d)\n", currBlock, blockSize);
		fclose(f);
		delete font;
		return 0;
	}

	int charCount = blockSize / sizeof(BMFont::CharBlock);
	for (int i=0; i<charCount; i++)
	{
		BMFont::CharBlock block;
		fread(&block, sizeof(BMFont::CharBlock), 1, f);
		font->chars[block.id] = block;
	}

	// Block 5 (optional): Kernings
	if (fread(&currBlock, 1, 1, f) < 1)
	{
		// No kernings. We're done
		printf("BMFont loaded successfully (no kernings)\n");
		fclose(f);
		return font;
	}
	fread(&blockSize, 4, 1, f);

	if (currBlock != 5)
	{
		printf("BMFont: Failed loading font (currBlock: expected 5, got %d)\n", currBlock);
		fclose(f);
		delete font;
		return 0;
	}
	if (blockSize <= 0)
	{
		printf("BMFont: Failed loading font on block %d (block size is %d)\n", currBlock, blockSize);
		fclose(f);
		delete font;
		return 0;
	}

	int kernCount = blockSize / sizeof(BMFont::KerningPairsBlock);
	for (int i=0; i<kernCount; i++)
	{
		BMFont::KerningPairsBlock block;
		fread(&block, sizeof(BMFont::KerningPairsBlock), 1, f);
		font->kernings.push_back(block);
	}

	printf("BMFont loaded successfully\n");
	fclose(f);
	return font;
}

void bmfont::Free(BMFont* font)
{
	if (!font) return;
	delete font;
}

void bmfont::Render(void* gsGlobal, void* gsTex, BMFont* font, std::string text, float x, float y, float scale, bool center)
{
	if (!font) return;

	gsKit_TexManager_bind((GSGLOBAL*)gsGlobal, (GSTEXTURE*)gsTex);

	float offsetX = (center) ? GetCenter(font, text, scale) : 0;
	x -= offsetX;

	float startX = x;
	for (u32 i=0; i<text.size(); i++)
	{
		char c = text.at(i);
		if (c == '\n')
		{
			x = startX;
			y += font->common.lineHeight*scale;
			continue;
		}
		if (!font->chars.count(c))
		{
			x += 32*scale;
			continue;
		}

		gsKit_prim_sprite_texture((GSGLOBAL*)gsGlobal, (GSTEXTURE*)gsTex,
								x + font->chars[c].xoffset*scale,  // X1
								y + font->chars[c].yoffset*scale,  // Y1
								font->chars[c].x,  // U1
								font->chars[c].y,  // V1
								x + (font->chars[c].xoffset + font->chars[c].width)*scale, // X2
								y + (font->chars[c].yoffset + font->chars[c].height)*scale, // Y2
								font->chars[c].x + font->chars[c].width, // U2
								font->chars[c].y + font->chars[c].height, // V2
								1,
								GS_SETREG_RGBAQ(128,128,128, 0, 0));

		x += font->chars[c].xadvance*scale;
	}
}

float bmfont::GetCenter(BMFont* font, std::string text, float scale)
{
	float maxWidth = 0, x=0;

	for (u32 i=0; i<text.size(); i++)
	{
		char c = text.at(i);
		if (c == '\n')
		{
			x = 0;
			continue;
		}
		if (!font->chars.count(c))
		{
			x += 32*scale;
			continue;
		}

		x += font->chars[c].xadvance*scale;
		if (x > maxWidth) maxWidth = x;
	}

	return maxWidth/2;
}