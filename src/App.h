#pragma once

class Renderer;

// App utility.
class App final {
public:
    static void Run();

    static constexpr void Close() noexcept {
        sRunning = false;
    }

    static constexpr const float DeltaTime() noexcept {
        return sDeltaTime;
    }
private:
    static bool sRunning;
    static float sDeltaTime;
    static float sLastFrame;

    static void Init();
    static void MainLoop();
    static void Cleanup();
};
