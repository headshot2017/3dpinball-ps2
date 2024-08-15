#pragma once

class ogg
{
public:
	static bool Init();
	static void Play(u8* buf, u32 bufSize);
	static void Stop();

private:
	static int oggThreadFunc(void* arg);
};
