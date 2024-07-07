#include "ps2_graphics.h"
#include "pch.h"
#include "maths.h"
#include "render.h"

#include <cstdio>
#include <cstring>
#include <malloc.h>

static framebuffer_t fb_colors;
static zbuffer_t     fb_depth;
static int display_mode;

void ps2_graphics::Initialize()
{
	dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);

	display_mode = graph_get_region();
	int w = 640, h = (display_mode == GRAPH_MODE_PAL) ? 512 : 448;

	graph_shutdown();
	graph_vram_clear();

	fb_colors.width   = w;
	fb_colors.height  = h;
	fb_colors.mask    = 0;
	fb_colors.psm     = GS_PSM_32;
	fb_colors.address = graph_vram_allocate(fb_colors.width, fb_colors.height, fb_colors.psm, GRAPH_ALIGN_PAGE);

	fb_depth.enable      = 1;
	fb_depth.method      = ZTEST_METHOD_ALLPASS;
	fb_depth.mask        = 0;
	fb_depth.zsm         = GS_ZBUF_24;
	fb_depth.address     = graph_vram_allocate(fb_colors.width, fb_colors.height, fb_depth.zsm, GRAPH_ALIGN_PAGE);


	int interlaced = display_mode == GRAPH_MODE_NTSC || display_mode == GRAPH_MODE_PAL || display_mode == GRAPH_MODE_HDTV_1080I;
	int mode       = interlaced ? GRAPH_MODE_INTERLACED : GRAPH_MODE_NONINTERLACED;
	int display    = interlaced ? GRAPH_MODE_FIELD      : GRAPH_MODE_FRAME;

	graph_set_mode(mode, display_mode, display, GRAPH_ENABLE);
	graph_set_screen(0, 0, fb_colors.width, fb_colors.height);

	graph_set_bgcolor(0, 0, 0);
	graph_set_framebuffer_filtered(fb_colors.address, fb_colors.width, fb_colors.psm, 0, 0);
	graph_enable_output();


	packet_t* packet = packet_init(100, PACKET_NORMAL);
	qword_t* q = packet->data;

	q = draw_setup_environment(q, 0, &fb_colors, &fb_depth);
	q = draw_clear(q, 0, 0, 0,
					fb_colors.width, fb_colors.height, 0, 0, 0);
	q = draw_finish(q);

	dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();
	packet_free(packet);
}

void ps2_graphics::SwapBuffers()
{
	graph_wait_vsync();
}

void ps2_graphics::Update()
{
	render::get_dirty_regions().clear();

	FlushCache(0);
	
	packet_t* packet = packet_init(200, PACKET_NORMAL);
	qword_t* q = packet->data;

	q = draw_texture_transfer(q, render::vscreen->BmpBufPtr1, render::vscreen->Width, render::vscreen->Height, GS_PSM_32, 
								 fb_colors.address, fb_colors.width);
	q = draw_texture_flush(q);

	dma_channel_send_chain(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
	dma_wait_fast();
	packet_free(packet);
}
