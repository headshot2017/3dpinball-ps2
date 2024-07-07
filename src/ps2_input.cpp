#include "ps2_input.h"
#include <libpad.h>

static char padBuf0[256] __attribute__((aligned(64)));

unsigned short ps2_input::ps2ButtonsDown = 0;
unsigned short ps2_input::ps2ButtonsUp = 0;
unsigned short ps2_input::ps2ButtonsHeld = 0;

static unsigned short nudgeKey, launchKey, leftKey, rightKey, upKey;

void ps2_input::Initialize()
{
	padInit(0);
	padPortOpen(0, 0, padBuf0);

	leftKey = PAD_LEFT | PAD_SQUARE | PAD_L1 | PAD_L2;
	rightKey = PAD_RIGHT | PAD_CIRCLE | PAD_R1 | PAD_R2;
	launchKey = PAD_DOWN | PAD_CROSS;
	upKey = PAD_UP;
	nudgeKey = PAD_TRIANGLE;
}

void ps2_input::ScanPads()
{
	static unsigned short lastBtns;

	struct padButtonStatus pad;
	int state = padGetState(0, 0);
	if (state != PAD_STATE_STABLE || padRead(0, 0, &pad) == 0) return;
	pad.btns ^= 0xffff; // flip bits

	ps2ButtonsDown = pad.btns &~ lastBtns;
	ps2ButtonsUp = lastBtns &~ pad.btns;
	ps2ButtonsHeld = pad.btns;

	lastBtns = pad.btns;
}

bool ps2_input::Exit()
{
	return (ps2ButtonsHeld & PAD_START) && (ps2ButtonsHeld & PAD_SELECT);
}

bool ps2_input::Pause()
{
	return ps2ButtonsDown & PAD_START;
}

bool ps2_input::NewGame()
{
	return ps2ButtonsDown & PAD_SELECT;
}

bool ps2_input::LaunchBallDown()
{
	return !(ps2ButtonsHeld & nudgeKey) && (ps2ButtonsDown & launchKey);
}

bool ps2_input::LaunchBallUp()
{
	return !(ps2ButtonsHeld & nudgeKey) && (ps2ButtonsUp & launchKey);
}

bool ps2_input::MoveLeftPaddleDown()
{
	return !(ps2ButtonsHeld & nudgeKey) && (ps2ButtonsDown & leftKey);
}

bool ps2_input::MoveLeftPaddleUp()
{
	return !(ps2ButtonsHeld & nudgeKey) && (ps2ButtonsUp & leftKey);
}

bool ps2_input::MoveRightPaddleDown()
{
	return !(ps2ButtonsHeld & nudgeKey) && (ps2ButtonsDown & rightKey);
}

bool ps2_input::MoveRightPaddleUp()
{
	return !(ps2ButtonsHeld & nudgeKey) && (ps2ButtonsUp & rightKey);
}

bool ps2_input::NudgeLeftDown()
{
	return (ps2ButtonsHeld & nudgeKey) && (ps2ButtonsDown & leftKey);
}

bool ps2_input::NudgeLeftUp()
{
	return (ps2ButtonsHeld & nudgeKey) && (ps2ButtonsUp & leftKey);
}

bool ps2_input::NudgeRightDown()
{
	return (ps2ButtonsHeld & nudgeKey) && (ps2ButtonsDown & rightKey);
}

bool ps2_input::NudgeRightUp()
{
	return (ps2ButtonsHeld & nudgeKey) && (ps2ButtonsUp & rightKey);
}

bool ps2_input::NudgeUpDown()
{
	return (ps2ButtonsHeld & nudgeKey) && (ps2ButtonsDown & upKey);
}

bool ps2_input::NudgeUpUp()
{
	return (ps2ButtonsHeld & nudgeKey) && (ps2ButtonsUp & upKey);
}

bool ps2_input::SkipError()
{
	return ps2ButtonsDown & PAD_CROSS;
}