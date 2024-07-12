#include "pch.h"
#include "loader.h"
#include "Sound.h"
#include "winmain.h"

#include <audsrv.h>

#include "adpenc/main.h"
#include "ps2gskit_graphics.h"

int Sound::num_channels;
bool Sound::enabled_flag = false;
int* Sound::TimeStamps = nullptr;

bool Sound::Init(int channels, bool enableFlag)
{
	if (audsrv_init())
		printf(" === Failed to init audsrv === \n");
	if (audsrv_adpcm_init())
		printf(" === Failed to init audsrv_adpcm === \n");

	SetChannels(channels);
	Enable(enableFlag);
	return true;
}

void Sound::Enable(bool enableFlag)
{
	enabled_flag = enableFlag;
}

void Sound::Activate()
{
	
}

void Sound::Deactivate()
{
	
}

void Sound::Close()
{
	audsrv_quit();

	delete[] TimeStamps;
	TimeStamps = nullptr;
}

void Sound::PlaySound(uint8_t* buf, int time, int size, int samplerate)
{
	if (!enabled_flag || !buf) return;
	int channel = audsrv_ch_play_adpcm(-1, (audsrv_adpcm_t*)buf);
	if (channel >= 0)
		TimeStamps[channel] = time;
}

uint8_t* Sound::LoadWaveFile(const std::string& lpName)
{
	audsrv_adpcm_t* sample = new audsrv_adpcm_t;
	memset(sample, 0, sizeof(audsrv_adpcm_t));

	std::string adpName = lpName.substr(0, lpName.size()-4) + ".ADP";

	FILE* f = fopen(adpName.c_str(), "rb");
	if (!f)
	{
		std::string buf = "Converting WAV sounds to ADPCM...\n" + lpName;
		ps2gskit_graphics::ShowSplash(buf);

		printf("Converting '%s' to ADPCM...\n", lpName.c_str());
		if (adpenc_main(lpName.c_str(), adpName.c_str(), 0) != 0)
		{
			printf("Failed to convert '%s' to ADPCM\n", lpName.c_str());
			delete sample;
			return 0;
		}
		f = fopen(adpName.c_str(), "rb");
	}

	fseek(f, 0, SEEK_END);
	uint32_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	uint8_t* buffer = new uint8_t[size];

	fread(buffer, 1, size, f);
	fclose(f);

	// flush cache, otherwise it will load garbage
	FlushCache(0);

	if (audsrv_load_adpcm(sample, buffer, size))
	{
		printf("Failed to init ADPCM WAV %s\n", lpName.c_str());
		delete[] buffer;
		delete sample;
		return 0;
	}

	printf("loaded wav %s\n", lpName.c_str());
	delete[] buffer;

	return (uint8_t*)sample;
}

void Sound::FreeSound(uint8_t* wave)
{
	if (wave)
	{
		audsrv_free_adpcm((audsrv_adpcm_t*)wave);
		delete wave;
	}
}

void Sound::SetChannels(int channels)
{
	if (channels <= 0)
		channels = 8;

	num_channels = channels;
	delete[] TimeStamps;
	TimeStamps = new int[num_channels]();
}
