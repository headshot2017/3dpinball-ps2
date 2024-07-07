#include "pch.h"
#include "loader.h"
#include "Sound.h"
#include "winmain.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

int Sound::num_channels;
bool Sound::enabled_flag = false;
int* Sound::TimeStamps = nullptr;

bool Sound::Init(int channels, bool enableFlag)
{
	// change priority to make SDL audio thread run properly
	int main_id = GetThreadId();
	ChangeThreadPriority(main_id, 72);

	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		printf("SDL_INIT_AUDIO failed: %s\n", SDL_GetError());
		return false;
	}

	if (Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 512) < 0)
    {
		printf("Mix_OpenAudio failed\n");
        return false;    
    }

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
	Mix_CloseAudio();
	SDL_Quit();

	delete[] TimeStamps;
	TimeStamps = nullptr;
}

void Sound::PlaySound(uint8_t* buf, int time, int size, int samplerate)
{
	if (!enabled_flag) return;
	int channel = Mix_PlayChannel(-1, (Mix_Chunk*)buf, 0);
	if (channel >= 0)
		TimeStamps[channel] = time;
}

uint8_t* Sound::LoadWaveFile(const std::string& lpName)
{
	return (uint8_t*)Mix_LoadWAV(lpName.c_str());
}

void Sound::FreeSound(uint8_t* wave)
{
	if (wave)
		Mix_FreeChunk((Mix_Chunk*)wave);
}

void Sound::SetChannels(int channels)
{
	if (channels <= 0)
		channels = 8;

	num_channels = channels;
	delete[] TimeStamps;
	TimeStamps = new int[num_channels]();
}
