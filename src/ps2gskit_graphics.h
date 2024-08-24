#pragma once

#include <string>

class ps2gskit_graphics
{
public:
	static void Initialize();
	static bool SetupEnv();

	static void ShowSplash(std::string text);
	static int GetFrameRate();

	static void SwapBuffers();
	static void Update();
};