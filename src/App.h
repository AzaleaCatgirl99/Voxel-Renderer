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

    static constexpr Renderer* GetRenderer() noexcept {
        return sRenderer;
    }
private:
    static Window* sWindow;
    static Renderer* sRenderer;
    static bool sRunning;

    static void Init();

    static void MainLoop();

    static void Cleanup();
};
