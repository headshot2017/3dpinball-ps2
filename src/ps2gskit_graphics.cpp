#include "ps2gskit_graphics.h"
#include "pch.h"
#include "maths.h"
#include "render.h"

#include <cstdio>
#include <cstring>
#include <malloc.h>

#include <gsKit.h>
#include <dmaKit.h>

static GSGLOBAL *gsGlobal;
static GSTEXTURE screen;
static GSTEXTURE splash;

extern unsigned char splash_raw[];
extern unsigned int  size_splash_raw;

void ps2gskit_graphics::Initialize()
{
	// default initialization
	gsGlobal = gsKit_init_global();
	gsGlobal->PSM = GS_PSM_CT24;
	gsGlobal->PSMZ = GS_PSMZ_16S;

	// default dmaKit initialization
	dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
	dmaKit_chan_init(DMA_CHANNEL_GIF);

	gsKit_init_screen(gsGlobal);

	gsKit_mode_switch(gsGlobal, GS_ONESHOT);

	gsKit_TexManager_init(gsGlobal);
}

void ps2gskit_graphics::SetupEnv()
{
	screen.Width = render::vscreen->Width;
	screen.Height = render::vscreen->Height;
	screen.PSM = GS_PSM_CT32;
	screen.Filter = GS_FILTER_NEAREST;
	screen.Mem = (u32*)render::vscreen->BmpBufPtr1;
}

void ps2gskit_graphics::ShowSplash(const char* text)
{
	if (splash.Vram == 0)
	{
		splash.Width = 320;
		splash.Height = 222;
		splash.PSM = GS_PSM_CT24;
		splash.Filter = GS_FILTER_NEAREST;
		splash.Mem = (u32*)malloc(gsKit_texture_size_ee(splash.Width, splash.Height, splash.PSM));

		memcpy(splash.Mem, splash_raw, gsKit_texture_size_ee(splash.Width, splash.Height, splash.PSM));
	}

	int startX = (gsGlobal->Width/2 - splash.Width/2);
	int startY = (gsGlobal->Height/2 - splash.Height/2);

	gsKit_TexManager_invalidate(gsGlobal, &splash);
	gsKit_TexManager_bind(gsGlobal, &splash);

	gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0, 0));
	gsKit_prim_sprite_texture(gsGlobal, &splash,
							startX,  // X1
							startY,  // Y2
							0.0f,  // U1
							0.0f,  // V1
							startX + splash.Width, // X2
							startY + splash.Height, // Y2
							splash.Width, // U2
							splash.Height, // V2
							0,
							GS_SETREG_RGBAQ(128,128,128, 0, 0));

	SwapBuffers();
}

void ps2gskit_graphics::SwapBuffers()
{
	gsKit_queue_exec(gsGlobal);
	gsKit_sync_flip(gsGlobal);
	gsKit_TexManager_nextFrame(gsGlobal);
}

void ps2gskit_graphics::Update()
{
	render::get_dirty_regions().clear();

	gsKit_TexManager_invalidate(gsGlobal, &screen);
	gsKit_TexManager_bind(gsGlobal, &screen);

	// center
	int startX = (gsGlobal->Width/2 - render::vscreen->Width/2);
	int startY = (gsGlobal->Height/2 - render::vscreen->Height/2);
	float divider = 1.65f;

	gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0, 0)); // what this actually does is draw sprites of the specified color...
	gsKit_prim_sprite_texture(gsGlobal, &screen,
							startX + render::get_offset_x(),  // X1
							startY + render::get_offset_y(),  // Y2
							0.0f,  // U1
							0.0f,  // V1
							startX + render::get_offset_x() + screen.Width/divider, // X2
							startY + render::get_offset_y() + screen.Height, // Y2
							screen.Width/divider, // U2
							screen.Height, // V2
							0,
							GS_SETREG_RGBAQ(128,128,128, 0, 0));
	gsKit_prim_sprite_texture(gsGlobal, &screen,
							startX + screen.Width/divider,  // X1
							startY,  // Y2
							screen.Width/divider,  // U1
							0.0f,  // V1
							startX + screen.Width, // X2
							startY + screen.Height, // Y2
							screen.Width, // U2
							screen.Height, // V2
							0,
							GS_SETREG_RGBAQ(128,128,128, 0, 0));
}
