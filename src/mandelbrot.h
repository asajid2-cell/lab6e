#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <string>
#include <utility>

#include <SFML/Graphics.hpp>

// (way more precision than a double can actually store...)
constexpr double LOG_2 = 0.6931471805599453094172321214581765680755001;

class MandelbrotViewer {
    constexpr static double PAN_FACTOR = 0.0002;
    constexpr static int MAX_ITERS_LOWER_BOUND =
        50;  // if our view changed last frame, use this as the maxIters to calls to mandelbrot()
    constexpr static int MAX_ITERS_UPPER_BOUND = 3200;  // cap maxIters at this (inclusive).
    constexpr static int ITERS_MULTIPLIER =
        2;  // multiply last frame's maxIters by this, if our view did NOT change last frame.
    constexpr static double ZOOM_EXPONENT_BASE = 1.01;

public:
    MandelbrotViewer(unsigned int windowWidth, unsigned int windowHeight);

    void run();

private:
    // For use by our constructor.
    // Returns the appropriate initial world bounds.
    // Should always contain the coordinates bounded in (-2.5, -1.5) to (0.5, 1.5).
    static std::pair<sf::Vector2<double>, sf::Vector2<double>> getInitialWorldBoundsForWindowSize(
        unsigned int windowWidth, unsigned int windowHeight);

    struct InputSummary {
        bool shouldClose = false;  // did the user close the window?
        bool shouldResize = false;
        sf::Vector2u desiredWindowSize = {
            0, 0};                // if not zero, should resize to this new window size.
        double zoomDistance = 0;  // total amount of "distance" zoomed (e.g. on the scrollwheel)
        sf::Vector2<double> panVector = {0., 0.};  // vector inside square between (-1, -1) to (1,
                                                   // 1) indicating desired base translation vector.
        sf::Vector2i mousePosition;
    };

    // Inputs
    InputSummary readInputs();
    // Updating view state based on input summaries:
    void updateViewState(const InputSummary& inputs, sf::Time deltaTime);
    void handleZoom(double scrollDistance, sf::Vector2i mousePosition);
    void handleWindowResize(sf::Vector2u newSize);
    void handlePans(sf::Vector2<double> basePanVector, sf::Time deltaTime);
    std::string formatCoords(const sf::Vector2<double>& pos);
    // updateUIText updates mCursorWorldPosText and mCursorWorldPosTextShadow
    // to show the cursor's current **world coordinates** position
    void updateUIText(sf::Vector2i mouseWindowCoords);

    // Rendering code:
    double mandelbrot(double cX, double cY, int maxIters) const;
    double mandelbrotSmooth(double cX, double cY, int maxIters) const;
    // windowPosToWorld takes a point in window coordinates and converts it to a world
    // coordinate.
    sf::Vector2<double> windowPosToWorld(const sf::Vector2<double>& pWindow);
    // drawIntoBuffer renders the current world view (bounded by mMinPointWorld and mMaxPointWorld)
    // into mViewBuffer
    void drawIntoViewBuffer(int maxIters);
    // copyViewBufferToGPU takes the drawn CPU-side buffer mViewBuffer and copies it to the
    // GPU-side.
    void copyViewBufferToGPU();
    // draw clears the window, draws the view, as well as the text with its shadow underneath
    // it. Finally, the window is displayed.
    void draw();

    // MEMBER FIELDS

    sf::RenderWindow mWindow;
    sf::Vector2u mWindowSize;  // window size on last frame.
    sf::Font mFont;
    sf::Text mCursorWorldPosText;  // text showing our current cursor position **in world
                                   // coordinates**
    sf::Text mCursorWorldPosTextShadow;

    sf::Image mViewBuffer;  // the image we want to display on this frame, stored in CPU-side RAM
    sf::Texture mViewBufferGPU;  // mViewBuffer, but in GPU VRAM.
    sf::Sprite mViewSprite;      // the sprite that will actually get rendered.
    sf::Vector2<double>
        mMinPointWorld;  // the minimum (x, y) world-space coords we can see (bottom-left).
    sf::Vector2<double>
        mMaxPointWorld;  // the maximum (x, y) world-space coords we can see (top-right).
};

#endif
