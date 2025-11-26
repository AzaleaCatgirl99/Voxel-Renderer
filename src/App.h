#pragma once

class Window;
class Renderer;

// App utility.
class App final {
public:
    static void Run();

    static constexpr void Close() noexcept {
        sRunning = false;
    }

    static constexpr Window* GetWindow() noexcept {
        return sWindow;
    }
private:
    static Window* sWindow;
    static bool sRunning;

    static void Init();

    static void MainLoop();

    static void Cleanup();
};
