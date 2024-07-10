#include "ps2gskit_graphics.h"
#include "pch.h"
#include "maths.h"
#include "render.h"

#include <cstdio>
#include <cstring>
#include <malloc.h>

#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>

GSGLOBAL *gsGlobal;
GSTEXTURE screen;

void ps2gskit_graphics::Initialize()
{
	gsGlobal = gsKit_init_global();
	gsGlobal->PSM = GS_PSM_32;
	gsGlobal->PSMZ = GS_PSMZ_16S;

	dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
	dmaKit_chan_init(DMA_CHANNEL_GIF);

	gsKit_init_screen(gsGlobal);

	gsKit_mode_switch(gsGlobal, GS_PERSISTENT);

	u64 black = GS_SETREG_RGBAQ(0, 0, 0, 0, 0);
	gsKit_clear(gsGlobal, black);
}

void ps2gskit_graphics::SetupEnv()
{
	screen.Width = render::vscreen->Width;
	screen.Height = render::vscreen->Height;
	screen.PSM = GS_PSM_32;
	screen.Filter = GS_FILTER_NEAREST;

	int Size = gsKit_texture_size_ee(screen.Width, screen.Height, screen.PSM);
	screen.Mem = (u32*)memalign(128, Size);

	memcpy(screen.Mem, render::vscreen->BmpBufPtr1, Size);

	screen.Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(screen.Width, screen.Height, screen.PSM), GSKIT_ALLOC_USERBUFFER);
	gsKit_texture_upload(gsGlobal, &screen);

	int startX = (640/2 - render::vscreen->Width/2);
	int startY = (448/2 - render::vscreen->Height/2);

	gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0, 0, 0, 0, 0));
	gsKit_prim_sprite_texture(gsGlobal, &screen,
							startX,  // X1
							startY,  // Y2
							0.0f,  // U1
							0.0f,  // V1
							startX + screen.Width, // X2
							startY + screen.Height, // Y2
							screen.Width, // U2
							screen.Height, // V2
							0,
							GS_SETREG_RGBAQ(128,128,128, 0, 0));
}

void ps2gskit_graphics::SwapBuffers()
{
	gsKit_sync_flip(gsGlobal);
	gsKit_queue_exec(gsGlobal);
}

void ps2gskit_graphics::Update()
{
	render::get_dirty_regions().clear();

	int Size = gsKit_texture_size_ee(screen.Width, screen.Height, screen.PSM);
	memcpy(screen.Mem, render::vscreen->BmpBufPtr1, Size);
	gsKit_texture_upload(gsGlobal, &screen);
}
