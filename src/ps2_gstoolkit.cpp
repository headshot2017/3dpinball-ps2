#include <cstdio>
#include <cstring>
#include <malloc.h>

#include <gsToolkit.h>

int gsKit_texture_finish(GSGLOBAL *gsGlobal, GSTEXTURE *Texture)
{
	if(!Texture->Delayed)
	{
		Texture->Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(Texture->Width, Texture->Height, Texture->PSM), GSKIT_ALLOC_USERBUFFER);
		if(Texture->Vram == GSKIT_ALLOC_ERROR)
		{
			printf("VRAM Allocation Failed. Will not upload texture.\n");
			return -1;
		}

		if(Texture->Clut != NULL)
		{
			if(Texture->PSM == GS_PSM_T4)
				Texture->VramClut = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(8, 2, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);
			else
				Texture->VramClut = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(16, 16, GS_PSM_CT32), GSKIT_ALLOC_USERBUFFER);

			if(Texture->VramClut == GSKIT_ALLOC_ERROR)
			{
				printf("VRAM CLUT Allocation Failed. Will not upload texture.\n");
				return -1;
			}
		}

		// Upload texture
		gsKit_texture_upload(gsGlobal, Texture);
		// Free texture
		free(Texture->Mem);
		Texture->Mem = NULL;
		// Free texture CLUT
		if(Texture->Clut != NULL)
		{
			free(Texture->Clut);
			Texture->Clut = NULL;
		}
	}
	else
	{
		gsKit_setup_tbw(Texture);
	}

	return 0;
}

int gsKit_texture_bmp_mem(GSGLOBAL *gsGlobal, GSTEXTURE *Texture, u8* mem, u32 memSize)
{
	GSBITMAP Bitmap;
	u32 x, y;
	int cy;
	u32 FTexSize;
	u8  *image;
	u8  *p;
	u32 TextureSize;
	FILE* File;

	File = fmemopen(mem, memSize, "rb");
	if (File == NULL)
	{
		printf("BMP: Failed to load bitmap\n");
		return -1;
	}
	if (fread(&Bitmap.FileHeader, 1, sizeof(Bitmap.FileHeader), File) != sizeof(Bitmap.FileHeader))
	{
		printf("BMP: Could not load bitmap\n");
		fclose(File);
		return -1;
	}

	if (fread(&Bitmap.InfoHeader, 1, sizeof(Bitmap.InfoHeader), File) != sizeof(Bitmap.InfoHeader))
	{
		printf("BMP: Could not load bitmap\n");
		fclose(File);
		return -1;
	}

	Texture->Width = Bitmap.InfoHeader.Width;
	Texture->Height = Bitmap.InfoHeader.Height;
	Texture->Filter = GS_FILTER_NEAREST;

	if(Bitmap.InfoHeader.BitCount == 4)
	{
		GSBMCLUT *clut;
		int i;

		Texture->PSM = GS_PSM_T4;
		Texture->Clut = (u32*)memalign(128, gsKit_texture_size_ee(8, 2, GS_PSM_CT32));
		Texture->ClutPSM = GS_PSM_CT32;

		memset(Texture->Clut, 0, gsKit_texture_size_ee(8, 2, GS_PSM_CT32));
		fseek(File, 54, SEEK_SET);
		if (fread(Texture->Clut, 1, Bitmap.InfoHeader.ColorUsed*sizeof(u32), File) != Bitmap.InfoHeader.ColorUsed*sizeof(u32))
		{
			if (Texture->Clut) {
				free(Texture->Clut);
				Texture->Clut = NULL;
			}
			printf("BMP: Could not load bitmap\n");
			fclose(File);
			return -1;
		}

		clut = (GSBMCLUT *)Texture->Clut;
		for (i = Bitmap.InfoHeader.ColorUsed; i < 16; i++)
		{
			memset(&clut[i], 0, sizeof(clut[i]));
		}

		for (i = 0; i < 16; i++)
		{
			u8 tmp = clut[i].Blue;
			clut[i].Blue = clut[i].Red;
			clut[i].Red = tmp;
			clut[i].Alpha = 0x80;
		}

	}
	else if(Bitmap.InfoHeader.BitCount == 8)
	{
		GSBMCLUT *clut;
		int i;

		Texture->PSM = GS_PSM_T8;
		Texture->Clut = (u32*)memalign(128, gsKit_texture_size_ee(16, 16, GS_PSM_CT32));
		Texture->ClutPSM = GS_PSM_CT32;

		memset(Texture->Clut, 0, gsKit_texture_size_ee(16, 16, GS_PSM_CT32));
		fseek(File, 54, SEEK_SET);
		if (fread(Texture->Clut, Bitmap.InfoHeader.ColorUsed*sizeof(u32), 1, File) <= 0)
		{
			if (Texture->Clut) {
				free(Texture->Clut);
				Texture->Clut = NULL;
			}
			printf("BMP: Could not load bitmap\n");
			fclose(File);
			return -1;
		}

		clut = (GSBMCLUT *)Texture->Clut;
		for (i = Bitmap.InfoHeader.ColorUsed; i < 256; i++)
		{
			memset(&clut[i], 0, sizeof(clut[i]));
		}

		for (i = 0; i < 256; i++)
		{
			u8 tmp = clut[i].Blue;
			clut[i].Blue = clut[i].Red;
			clut[i].Red = tmp;
			clut[i].Alpha = 0x80;
		}

		// rotate clut
		for (i = 0; i < 256; i++)
		{
			if ((i&0x18) == 8)
			{
				GSBMCLUT tmp = clut[i];
				clut[i] = clut[i+8];
				clut[i+8] = tmp;
			}
		}
	}
	else if(Bitmap.InfoHeader.BitCount == 16)
	{
		Texture->PSM = GS_PSM_CT16;
		Texture->VramClut = 0;
		Texture->Clut = NULL;
	}
	else if(Bitmap.InfoHeader.BitCount == 24)
	{
		Texture->PSM = GS_PSM_CT24;
		Texture->VramClut = 0;
		Texture->Clut = NULL;
	}

	fseek(File, 0, SEEK_END);
	FTexSize = ftell(File);
	FTexSize -= Bitmap.FileHeader.Offset;

	fseek(File, Bitmap.FileHeader.Offset, SEEK_SET);

	TextureSize = gsKit_texture_size_ee(Texture->Width, Texture->Height, Texture->PSM);

	Texture->Mem = (u32*)memalign(128,TextureSize);

	if(Bitmap.InfoHeader.BitCount == 24)
	{
		image = (u8*)memalign(128, FTexSize);
		if (image == NULL) {
			printf("BMP: Failed to allocate memory\n");
			if (Texture->Mem) {
				free(Texture->Mem);
				Texture->Mem = NULL;
			}
			if (Texture->Clut) {
				free(Texture->Clut);
				Texture->Clut = NULL;
			}
			fclose(File);
			return -1;
		}

		fread(image, FTexSize, 1, File);
		p = (u8*)((u32)Texture->Mem);
		for (y = Texture->Height - 1, cy = 0; y >= 0; y--, cy++) {
			for (x = 0; x < Texture->Width; x++) {
				p[(y * Texture->Width + x) * 3 + 2] = image[(cy * Texture->Width + x) * 3 + 0];
				p[(y * Texture->Width + x) * 3 + 1] = image[(cy * Texture->Width + x) * 3 + 1];
				p[(y * Texture->Width + x) * 3 + 0] = image[(cy * Texture->Width + x) * 3 + 2];
			}
		}
		free(image);
		image = NULL;
	}
	else if(Bitmap.InfoHeader.BitCount == 16)
	{
		image = (u8*)memalign(128, FTexSize);
		if (image == NULL) {
			printf("BMP: Failed to allocate memory\n");
			if (Texture->Mem) {
				free(Texture->Mem);
				Texture->Mem = NULL;
			}
			if (Texture->Clut) {
				free(Texture->Clut);
				Texture->Clut = NULL;
			}
			fclose(File);
			return -1;
		}

		fread(image, FTexSize, 1, File);

		p = (u8*)((u32)Texture->Mem);
		for (y = Texture->Height - 1, cy = 0; y >= 0; y--, cy++) {
			for (x = 0; x < Texture->Width; x++) {
				u16 value;
				value = *(u16*)&image[(cy * Texture->Width + x) * 2];
				value = (value & 0x8000) | value << 10 | (value & 0x3E0) | (value & 0x7C00) >> 10;	//ARGB -> ABGR

				*(u16*)&p[(y * Texture->Width + x) * 2] = value;
			}
		}
		free(image);
		image = NULL;
	}
	else if(Bitmap.InfoHeader.BitCount == 8 || Bitmap.InfoHeader.BitCount == 4)
	{
		char *tex = (char *)((u32)Texture->Mem);
		image = (u8*)memalign(128,FTexSize);
		if (image == NULL) {
			printf("BMP: Failed to allocate memory\n");
			if (Texture->Mem) {
				free(Texture->Mem);
				Texture->Mem = NULL;
			}
			if (Texture->Clut) {
				free(Texture->Clut);
				Texture->Clut = NULL;
			}
			fclose(File);
			return -1;
		}

		if (fread(image, FTexSize, 1, File) != 1)
		{
			if (Texture->Mem) {
				free(Texture->Mem);
				Texture->Mem = NULL;
			}
			if (Texture->Clut) {
				free(Texture->Clut);
				Texture->Clut = NULL;
			}
			printf("BMP: Read failed!, Size %d\n", FTexSize);
			free(image);
			image = NULL;
			fclose(File);
			return -1;
		}
		for (y = Texture->Height - 1; y >= 0; y--)
		{
			if(Bitmap.InfoHeader.BitCount == 8)
				memcpy(&tex[y * Texture->Width], &image[(Texture->Height - y - 1) * Texture->Width], Texture->Width);
			else
				memcpy(&tex[y * (Texture->Width / 2)], &image[(Texture->Height - y - 1) * (Texture->Width / 2)], Texture->Width / 2);
		}
		free(image);
		image = NULL;

		if(Bitmap.InfoHeader.BitCount == 4)
		{
			u32 byte;
			u8 *tmpdst = (u8 *)((u32)Texture->Mem);
			u8 *tmpsrc = (u8 *)tex;

			for(byte = 0; byte < FTexSize; byte++)
			{
				tmpdst[byte] = (tmpsrc[byte] << 4) | (tmpsrc[byte] >> 4);
			}
		}
	}
	else
	{
		printf("BMP: Unknown bit depth format %d\n", Bitmap.InfoHeader.BitCount);
	}

	fclose(File);

	return gsKit_texture_finish(gsGlobal, Texture);
}
