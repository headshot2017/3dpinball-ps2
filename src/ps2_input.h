#pragma once

class ps2_input
{
public:
    static void Initialize();
    static void ScanPads();

    static bool Exit();
    static bool Pause();
    static bool NewGame();

    static bool LaunchBallDown();
    static bool LaunchBallUp();

    static bool MoveLeftPaddleDown();
    static bool MoveLeftPaddleUp();
    static bool MoveRightPaddleDown();
    static bool MoveRightPaddleUp();

    static bool NudgeLeftDown();
    static bool NudgeLeftUp();
    static bool NudgeRightDown();
    static bool NudgeRightUp();
    static bool NudgeUpDown();
    static bool NudgeUpUp();

    static bool SkipError();

private:
    static unsigned short ps2ButtonsDown;
    static unsigned short ps2ButtonsUp;
    static unsigned short ps2ButtonsHeld;
};