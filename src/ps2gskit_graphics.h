#pragma once

#include <string>

class ps2gskit_graphics
{
public:
	static void Initialize();
	static void SetupEnv();

	static void ShowSplash(std::string text);

	static void SwapBuffers();
	static void Update();
};