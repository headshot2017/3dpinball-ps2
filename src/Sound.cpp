#include "pch.h"
#include "loader.h"
#include "Sound.h"
#include "winmain.h"

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#include <SDL2/SDL.h>

int Sound::num_channels;
bool Sound::enabled_flag = false;
int* Sound::TimeStamps = nullptr;

static SDL_AudioDeviceID dev;

bool Sound::Init(int channels, bool enableFlag)
{
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		printf("SDL_INIT_AUDIO failed: %s\n", SDL_GetError());
		return false;
	}
	printf("SDL_INIT_AUDIO success\n");

	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = 44100;
	want.format = AUDIO_S16;
	want.channels = 1;
	want.samples = 512;
	want.callback = nullptr;
	dev = SDL_OpenAudioDevice(nullptr, 0, nullptr, nullptr, 0);
	if (dev == 0)
	{
		printf("SDL_OpenAudio error: %s\n", SDL_GetError());
		return false;
	}
	printf("SDL_OpenAudio success\n");
	SDL_PauseAudioDevice(dev, 0);

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
	SDL_PauseAudioDevice(dev, 0);
}

void Sound::Deactivate()
{
	SDL_PauseAudioDevice(dev, 1);
}

void Sound::Close()
{
	SDL_CloseAudioDevice(dev);
	SDL_Quit();

	delete[] TimeStamps;
	TimeStamps = nullptr;
}

void Sound::PlaySound(uint8_t* buf, int time, int size, int samplerate)
{
	if (!enabled_flag) return;
	SDL_QueueAudio(dev, buf, size);
	//TimeStamps[channel] = time;
}

uint8_t* Sound::LoadWaveFile(const std::string& lpName)
{
	drwav wavfp;

	if (!drwav_init_file(&wavfp, lpName.c_str(), NULL))
		return 0;

	s16* pSampleData = new s16[(u32)wavfp.totalPCMFrameCount * wavfp.channels];
	if (pSampleData == 0)
	{
		drwav_uninit(&wavfp);
		return 0;
	}

	u32 totalRead = drwav_read_pcm_frames(&wavfp, wavfp.totalPCMFrameCount, pSampleData);
	if (!totalRead)
	{
		drwav_uninit(&wavfp);
		delete[] pSampleData;
		return 0;
	}

	if (wavfp.bitsPerSample == 8) // 8 bit
	{
		s16* _8bitdata = new s16[(u32)wavfp.totalPCMFrameCount * wavfp.channels];
		drwav_u8_to_s16((drwav_int16*)_8bitdata, (drwav_uint8*)pSampleData, wavfp.totalPCMFrameCount);
		delete[] pSampleData;
		pSampleData = _8bitdata;
	}

	drwav_uninit(&wavfp);
	return (uint8_t*)pSampleData;

}

void Sound::FreeSound(uint8_t* wave)
{
	s16* buf = (s16*)wave;
	if (buf)
		delete[] buf;
}

void Sound::SetChannels(int channels)
{
	if (channels <= 0)
		channels = 8;

	num_channels = channels;
	delete[] TimeStamps;
	TimeStamps = new int[num_channels]();
}
