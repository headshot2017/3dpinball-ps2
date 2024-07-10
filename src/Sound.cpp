#include "pch.h"
#include "loader.h"
#include "Sound.h"
#include "winmain.h"

#include <audsrv.h>
#include "adpenc/main.h"

int Sound::num_channels;
bool Sound::enabled_flag = false;
int* Sound::TimeStamps = nullptr;

bool Sound::Init(int channels, bool enableFlag)
{
	/*
	// change priority to make SDL audio thread run properly
	int main_id = GetThreadId();
	ChangeThreadPriority(main_id, 72);

	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		printf("SDL_INIT_AUDIO failed: %s\n", SDL_GetError());
		return false;
	}

	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 512) < 0)
    {
		printf("Mix_OpenAudio failed\n");
        return false;    
    }
	*/

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
	//Mix_CloseAudio();
	//SDL_Quit();
	audsrv_quit();

	delete[] TimeStamps;
	TimeStamps = nullptr;
}

void Sound::PlaySound(uint8_t* buf, int time, int size, int samplerate)
{
	if (!enabled_flag || !buf) return;
	//int channel = Mix_PlayChannel(-1, (Mix_Chunk*)buf, 0);
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

	/*Mix_Chunk* snd = Mix_LoadWAV(lpName.c_str());
	if (!snd)
		printf("Failed to load sound '%s'\n", lpName.c_str());
	return (uint8_t*)snd;
	*/
	return (uint8_t*)sample;
}

void Sound::FreeSound(uint8_t* wave)
{
	//if (wave)
		//Mix_FreeChunk((Mix_Chunk*)wave);
}

void Sound::SetChannels(int channels)
{
	if (channels <= 0)
		channels = 8;

	num_channels = channels;
	delete[] TimeStamps;
	TimeStamps = new int[num_channels]();
}
