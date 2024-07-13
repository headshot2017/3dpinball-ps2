#include "pch.h"
#include "ogg.h"

#include <audsrv.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#define BUF_SIZE 2048

// simple OGG player for audsrv

static bool playing;
static int mutexID;
static int threadID;
static ee_sema_t mutex;
static ee_thread_t oggThread;

static char* pcm = 0;
static OggVorbis_File vf;


int ogg::oggThreadFunc(void* arg)
{
	int current_section = 0;

	while (playing)
	{
		SignalSema(mutexID);

		int decoded = ov_read(&vf, pcm, BUF_SIZE, 0, 2, 1, &current_section);
		if (decoded == 0)
		{
			// loop
			ov_raw_seek(&vf, 0);
			decoded = ov_read(&vf, pcm, BUF_SIZE, 0, 2, 1, &current_section);
		}

		audsrv_wait_audio(decoded);
		audsrv_play_audio(pcm, decoded);
	}

	audsrv_stop_audio();

	return 0;
}


void ogg::Init()
{
	if (pcm) return;

	int main_id = GetThreadId();
	ChangeThreadPriority(main_id, 80);

	pcm = (char*)malloc(BUF_SIZE);

	mutex.init_count = 1;
	mutex.max_count = 1;
	mutex.option = 0;
	mutexID = CreateSema(&mutex);
	printf("ogg Init\n");
}

void ogg::Play(u8* buf, u32 bufSize)
{
	if (playing) return;

	printf("ogg Play\n");
	FILE* f = fmemopen(buf, bufSize, "rb");
	if (ov_open(f, &vf, NULL, 0) < 0)
	{
		printf("Not an OGG file\n");
		fclose(f);
		return;
	}

	playing = true;

	oggThread = {0};
	oggThread.func        = (void*)oggThreadFunc;
	oggThread.stack       = malloc(512*1024);
	oggThread.stack_size  = 512*1024;
	oggThread.gp_reg      = &_gp;
	oggThread.initial_priority = 18;
	threadID = CreateThread(&oggThread);
	StartThread(threadID, NULL);

	printf("ogg Started (Thread %d)\n", threadID);
}

void ogg::Stop()
{
	if (!playing) return;

	WaitSema(mutexID);
	playing = false;

	// join thread
	ee_thread_status_t info;
	do
		ReferThreadStatus(threadID, &info);
	while (info.status != THS_DORMANT);

	TerminateThread(threadID);
	free(oggThread.stack);

	printf("ogg Stopped\n");
}
