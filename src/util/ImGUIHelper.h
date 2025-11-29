#pragma once

// Helper utility for making ImGUI a bit easier to work with.
class ImGUIHelper final {
public:
    static void Initialize();
    static void ProcessEvents();
    static void BeginDraw(); // Gets called at the beginning of the main loop.
    static void CmdDraw(); // Gets called when recording commands.
    static void Destroy();
};
