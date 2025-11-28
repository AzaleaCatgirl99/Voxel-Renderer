#pragma once

class Renderer;

// App utility.
class App final {
public:
    static void Run();

    static constexpr void Close() noexcept {
        sRunning = false;
    }
private:
    static bool sRunning;

    static void Init();

    static void MainLoop();

    static void Cleanup();
};
