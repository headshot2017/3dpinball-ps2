#include "pch.h"
#include "winmain.h"

#include <malloc.h>

#include "control.h"
#include "midi.h"
#include "options.h"
#include "pb.h"
#include "pinball.h"
#include "render.h"
#include "Sound.h"
#include "ps2gskit_graphics.h"
#include "ps2_input.h"

#include <sifrpc.h>
#include <iopheap.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sbv_patches.h>

int winmain::bQuit = 0;
int winmain::activated;
int winmain::DispFrameRate = 0;
int winmain::DispGRhistory = 0;
int winmain::single_step = 0;
int winmain::last_mouse_x;
int winmain::last_mouse_y;
int winmain::mouse_down;
int winmain::no_time_loss;

std::string winmain::DatFileName;
bool winmain::ShowSpriteViewer = false;
bool winmain::LaunchBallEnabled = true;
bool winmain::HighScoresEnabled = true;
bool winmain::DemoActive = false;
char *winmain::BasePath;
std::string winmain::FpsDetails;
double winmain::UpdateToFrameRatio;
winmain::DurationMs winmain::TargetFrameTime;
optionsStruct &winmain::Options = options::Options;


static SifRpcClientData_t client __attribute__((aligned(64)));
#define	MASS_USB_ID	0x500C0F1
#define STRINGIFY(a) #a
#define SifLoadBuffer(name) \
	extern unsigned char name[]; \
	extern unsigned int  size_ ## name; \
	ret = SifExecModuleBuffer(name, size_ ## name, 0, NULL, NULL); \
    if (ret < 0) PrintFatalError("SifExecModuleBuffer " STRINGIFY(name) " failed: %d", &ret);


int winmain::WinMain(LPCSTR lpCmdLine)
{
	std::set_new_handler(memalloc_failure);

	ps2gskit_graphics::Initialize();
	ps2gskit_graphics::ShowSplash("");

	// Reset the IOP
	SifInitRpc(0);
	while (!SifIopReset("", 0)) { }
	while (!SifIopSync())       { }

	SifInitRpc(0);
	SifLoadFileInit();
	SifInitIopHeap();
	sbv_patch_enable_lmb(); // Allows loading IRX modules from a buffer in EE RAM

	int ret;

	// Load modules

	// serial I/O module (needed for memory card & input pad modules)
	ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
	if (ret < 0) PrintFatalError("SifLoadModule SIO2MAN failed: %d\n", &ret);

	// memory card module
	ret = SifLoadModule("rom0:MCMAN",   0, NULL);
	if (ret < 0) PrintFatalError("SifLoadModule MCMAN failed: %d\n", &ret);

	// memory card server module
	ret = SifLoadModule("rom0:MCSERV",  0, NULL);
	if (ret < 0) PrintFatalError("SifLoadModule MCSERV failed: %d\n", &ret);

	// Input pad module
	ret = SifLoadModule("rom0:PADMAN",  0, NULL);
	if (ret < 0) PrintFatalError("SifLoadModule PADMAN failed: %d\n", &ret);

	SifLoadBuffer(usbd_irx);
	SifLoadBuffer(bdm_irx);
	SifLoadBuffer(bdmfs_fatfs_irx);
	SifLoadBuffer(usbmass_bd_irx);
	SifLoadBuffer(freesd_irx);
	SifLoadBuffer(audsrv_irx);

	ps2gskit_graphics::ShowSplash("Loading USB driver");

	// Init USB
	int retryCount = 0x1000;
	while(retryCount--) {
			ret = SifBindRpc( &client, MASS_USB_ID, 0);
			if ( ret  < 0)  {
			   retryCount = 0;
			   break;
			}
			if (client.server != 0) break;

			// short delay 
			ret = 0x10000;
			while(ret--) asm("nop\nnop\nnop\nnop");
	}
	if (!retryCount) PrintFatalError("Failed to init USB RPC: %d\n", &ret);


	// Initialize input
	ps2_input::Initialize();

	// Set the base path for PINBALL.DAT

	BasePath = (char *)"./3DPINBALL/";

	pinball::quickFlag = 0; // strstr(lpCmdLine, "-quick") != nullptr;
	DatFileName = options::get_string("Pinball Data", pinball::get_rc_string(168, 0));

	// Check for full tilt .dat file and switch to it automatically

	auto pinballDat = fopen(pinball::make_path_name(DatFileName).c_str(), "rb");
	bool hasPinball = !!(pinballDat);
	if (pinballDat) fclose(pinballDat);

	auto cadetFilePath = pinball::make_path_name("CADET.DAT");
	auto cadetDat = fopen(cadetFilePath.c_str(), "r");
	bool hasCadet = !!(cadetDat);
	if (cadetDat) fclose(cadetDat);

	if (!hasPinball && hasCadet)
	{
		DatFileName = "CADET.DAT";
		pb::FullTiltMode = true;
	}
	else if (hasPinball && hasCadet)
	{
		// pick a game

		while (true)
		{
			ps2_input::ScanPads();

			if (ps2_input::Button1())
				break;
			if (ps2_input::Button2())
			{
				DatFileName = "CADET.DAT";
				pb::FullTiltMode = true;
				break;
			}

			ps2gskit_graphics::ShowSplash("Press CROSS to play 3D Pinball Space Cadet\nPress CIRCLE to play Full Tilt! Pinball");
		}

		ps2_input::Clear();
	}

	std::string gameName = (pb::FullTiltMode) ? "Full Tilt! Pinball" : "3D Pinball";
	ps2gskit_graphics::ShowSplash("Loading " + gameName);

	// PB init from message handler

	{
		options::init();
		if (!Sound::Init(16, Options.Sounds))
			Options.Sounds = false;

		if (!pinball::quickFlag && !midi::music_init())
			Options.Music = false;

		if (pb::init())
		{
			PrintFatalError("Could not load game data:\n%s is missing.\n", pinball::make_path_name(DatFileName).c_str());
		}
	}

	if (!ps2gskit_graphics::SetupEnv())
		PrintFatalError("Could not run SetupEnv. Not enough memory(?)\n");

	// Initialize game

	pb::reset_table();
	pb::firsttime_setup();
	pb::replay_level(0);

	// Begin main loop

	bQuit = false;

	while (!bQuit)
	{
		// Input

		ps2_input::ScanPads();

		if (ps2_input::Exit())
			break;

		if (ps2_input::Pause())
			pause();

		if (ps2_input::NewGame())
			new_game();

		pb::keydown();
		pb::keyup();

		if (!single_step)
		{
			// Update game when not paused

			pb::frame(1000.0f / ps2gskit_graphics::GetFrameRate());
		}

		// Copy game screen buffer to texture
		ps2gskit_graphics::Update();

		ps2gskit_graphics::SwapBuffers();
	}

	printf("Uninitializing...\n");

	end_pause();

	options::uninit();
	midi::music_shutdown();
	pb::uninit();
	Sound::Close();

	return 0;
}

void winmain::memalloc_failure()
{
	midi::music_stop();
	Sound::Close();
	char *caption = pinball::get_rc_string(170, 0);
	char *text = pinball::get_rc_string(179, 0);

	PrintFatalError("%s %s\n", caption, text);
}

void winmain::end_pause()
{
	if (single_step)
	{
		pb::pause_continue();
		no_time_loss = 1;
	}
}

void winmain::new_game()
{
	end_pause();
	pb::replay_level(0);
}

void winmain::pause()
{
	pb::pause_continue();
	no_time_loss = 1;
}

void winmain::UpdateFrameRate()
{
	// UPS >= FPS
	auto fps = Options.FramesPerSecond, ups = Options.UpdatesPerSecond;
	UpdateToFrameRatio = static_cast<double>(ups) / fps;
	TargetFrameTime = DurationMs(1000.0 / ups);
}

void winmain::PrintFatalError(const char *message, ...)
{
	char buf[256] = {0};
	va_list args;
	va_start(args, message);
	vsprintf(buf, message, args);
	va_end(args);

	strcat(buf, "\nPress X to exit.\n");

	while (true)
	{
		ps2_input::ScanPads();

		if (ps2_input::Button1())
			break;

		ps2gskit_graphics::ShowSplash(buf);
	}

	exit(EXIT_FAILURE);
}