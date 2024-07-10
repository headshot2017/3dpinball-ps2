#pragma once

class ps2gskit_graphics
{
public:
	static void Initialize();
	static void SetupEnv();

	static void ShowSplash(const char* text);

	static void SwapBuffers();
	static void Update();
};