#include "mandelbrot.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <memory>
#include <sstream>

#include "cyclic_gradient.h"
#include "open_sans_semibold.h"
#include "vector_util.h"

// For use by our constructor.
// Returns the appropriate initial world bounds.
// Should always contain the coordinates bounded in (-2.5, -1.5) to (0.5, 1.5).
std::pair<sf::Vector2<double>, sf::Vector2<double>>
MandelbrotViewer::getInitialWorldBoundsForWindowSize(unsigned int windowWidth,
                                                     unsigned int windowHeight) {
    assert(windowWidth > 0);
    assert(windowHeight > 0);
    const double aspectRatio = static_cast<double>(windowWidth) / windowHeight;
    const sf::Vector2<double> worldCenter{-1., 0.};
    sf::Vector2<double> initWorldSize = {3., 3.};
    if (windowWidth >= windowHeight) {
        // world height will be 3
        initWorldSize.x = 3 * aspectRatio;
    } else {
        // world width will be 3
        initWorldSize.y = 3 / aspectRatio;
    }
    return {worldCenter - initWorldSize / 2., worldCenter + initWorldSize / 2.};
}

MandelbrotViewer::MandelbrotViewer(unsigned int windowWidth, unsigned int windowHeight)
    : mWindow(sf::VideoMode(sf::Vector2u{windowWidth, windowHeight}), "Mandelbrot set viewer"),
      mWindowSize(windowWidth, windowHeight),
      mFont(EmbeddedFonts::OpenSans_SemiBold_ttf, EmbeddedFonts::OpenSans_SemiBold_ttf_len),
      mCursorWorldPosText(mFont),
      mCursorWorldPosTextShadow(mFont),
      mViewBuffer(sf::Vector2u{windowWidth, windowHeight}, sf::Color::Blue),
      mViewBufferGPU(mViewBuffer),
      mViewSprite(mViewBufferGPU) {
    assert(windowWidth > 0);
    assert(windowHeight > 0);
    std::tie(mMinPointWorld, mMaxPointWorld) =
        getInitialWorldBoundsForWindowSize(windowWidth, windowHeight);

    // disable key repeat
    mWindow.setKeyRepeatEnabled(false);

    // Initially no text to show
    mCursorWorldPosText.setString("");
    mCursorWorldPosTextShadow.setString("");
    // Character size of 14 pixels
    mCursorWorldPosText.setCharacterSize(14);
    mCursorWorldPosTextShadow.setCharacterSize(14);
    // Text is white, shadow is black.
    mCursorWorldPosText.setFillColor(sf::Color::White);
    mCursorWorldPosTextShadow.setFillColor(sf::Color(0, 0, 0, 130));
    // Set text position
    mCursorWorldPosText.setPosition({10, 10});
    mCursorWorldPosTextShadow.setPosition({10, 10});
    // Give text shadow extra thickness.
    mCursorWorldPosTextShadow.setOutlineColor(sf::Color(0, 0, 0, 130));
    mCursorWorldPosTextShadow.setOutlineThickness(3);
}

void MandelbrotViewer::run() {
    sf::Clock clock;
    sf::Time lastTime = sf::Time();
    int maxIters = MAX_ITERS_LOWER_BOUND;
    while (true) {
        sf::Time curTime = clock.getElapsedTime();
        // Handle inputs
        InputSummary inputSummary = readInputs();
        if (inputSummary.shouldClose) {
            mWindow.close();
            return;
        }
        // Determine whether any of the inputs cause the view to change.
        bool viewChanged = (inputSummary.zoomDistance != 0) ||
                           (inputSummary.panVector != sf::Vector2<double>{0., 0.}) ||
                           inputSummary.shouldResize;
        // Update
        updateViewState(inputSummary, curTime - lastTime);
        updateUIText(inputSummary.mousePosition);
        maxIters = (viewChanged ? MAX_ITERS_LOWER_BOUND
                                : std::min(maxIters * ITERS_MULTIPLIER, MAX_ITERS_UPPER_BOUND));
        // Render:
        drawIntoViewBuffer(maxIters);
        copyViewBufferToGPU();
        draw();
        // update timing stats
        lastTime = curTime;
    }
}

MandelbrotViewer::InputSummary MandelbrotViewer::readInputs() {
    InputSummary ret{};
    while (const std::optional<sf::Event> event = mWindow.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            ret.shouldClose = true;
            return ret;  // just stop early
        } else if (const auto* mouseScrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
            // If multiple of these, it adds up.
            ret.zoomDistance += mouseScrolled->delta;
        } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            ret.desiredWindowSize = resized->size;
            ret.shouldResize = true;
        }
    }
    // pan is a continuous action, so we'll just check current state for each key. Note, NOT
    // else if.
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::W)) {
        ret.panVector += {0., 1};
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::S)) {
        ret.panVector += {0., -1.};
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::A)) {
        ret.panVector += {-1., 0.};
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::D)) {
        ret.panVector += {1., 0.};
    }
    ret.mousePosition = sf::Mouse::getPosition(mWindow);
    return ret;
}

void MandelbrotViewer::updateViewState(const InputSummary& inputs, sf::Time deltaTime) {
    if (inputs.shouldResize) {
        handleWindowResize(inputs.desiredWindowSize);
    }
    if (inputs.zoomDistance != 0) {
        handleZoom(inputs.zoomDistance, inputs.mousePosition);
    }
    if (inputs.panVector != sf::Vector2<double>{0., 0.}) {
        handlePans(inputs.panVector, deltaTime);
    }
}

void MandelbrotViewer::handleZoom(double scrollDistance, sf::Vector2i mousePosition) {
    double worldViewFactor = std::pow(ZOOM_EXPONENT_BASE, scrollDistance);

    // The world point under the cursor now; it must still be under the cursor after the zoom.
    const sf::Vector2<double> cursorWorld = windowPosToWorld(
        sf::Vector2<double>(static_cast<double>(mousePosition.x),
                            static_cast<double>(mousePosition.y)));
    // Where the cursor sits in the window, as a fraction in [0, 1] (y measured from the top).
    const sf::Vector2<double> cursorFraction(
        static_cast<double>(mousePosition.x) / static_cast<double>(mWindowSize.x),
        static_cast<double>(mousePosition.y) / static_cast<double>(mWindowSize.y));

    const sf::Vector2<double> newWorldSize =
        (mMaxPointWorld - mMinPointWorld) * worldViewFactor;
    // Place the new rectangle so that cursorWorld stays at the same window fraction: the distance
    // from the left edge to the cursor is cursorFraction.x of the new width, and from the top edge
    // to the cursor is cursorFraction.y of the new height (world y grows upward, hence the flip).
    mMinPointWorld = sf::Vector2<double>(cursorWorld.x - cursorFraction.x * newWorldSize.x,
                                         cursorWorld.y - (1. - cursorFraction.y) * newWorldSize.y);
    mMaxPointWorld = mMinPointWorld + newWorldSize;
}

void MandelbrotViewer::handleWindowResize(sf::Vector2u newSize)  // newSize is in window coords.
{
    // IMPORTANT: after resizing window, need to fix view, or else original world
    // rectangle gets scaled to new window size instead. This line ensures that when the window
    // expands, "more of the world" is correspondingly viewable.
    mWindow.setView(sf::View(
        sf::FloatRect({0, 0}, {static_cast<float>(newSize.x), static_cast<float>(newSize.y)})));

    // Keep the world centre fixed and scale the world rectangle by the same factor the window was
    // scaled in each dimension. Since the world view already matches the window's aspect ratio,
    // scaling per axis keeps it matched, so the resize only crops or extends the view.
    const sf::Vector2<double> worldCenter = (mMinPointWorld + mMaxPointWorld) * 0.5;
    const sf::Vector2<double> scaleFactor(static_cast<double>(newSize.x) / mWindowSize.x,
                                          static_cast<double>(newSize.y) / mWindowSize.y);
    const sf::Vector2<double> newWorldSize = (mMaxPointWorld - mMinPointWorld) * scaleFactor;
    mMinPointWorld = worldCenter - newWorldSize * 0.5;
    mMaxPointWorld = worldCenter + newWorldSize * 0.5;

    // update CPU-side image buffer size to have enough memory for all the pixels:
    mViewBuffer.resize(newSize);
    // update GPU-side texture to match the new window size:
    (void)mViewBufferGPU.resize(newSize);
    // The sprite will have an incorrect view into the texture after resize, so we update:
    mViewSprite.setTextureRect(sf::IntRect({0, 0}, sf::Vector2i(newSize)));
    mWindowSize = newSize;  // update mWindowSize.
}

void MandelbrotViewer::handlePans(sf::Vector2<double> basePanVector, sf::Time deltaTime) {
    const sf::Vector2<double> worldViewSize = mMaxPointWorld - mMinPointWorld;
    const double panScale = (worldViewSize.x + worldViewSize.y) * 0.5;
    const auto panDelta =
        basePanVector * static_cast<double>(deltaTime.asMilliseconds()) * panScale * PAN_FACTOR;
    mMinPointWorld += panDelta;
    mMaxPointWorld += panDelta;
}

std::string MandelbrotViewer::formatCoords(const sf::Vector2<double>& pos) {
    std::stringstream ss;
    ss << std::setprecision(15) << "(" << pos.x << ", " << pos.y << ")";
    return ss.str();
}

// updateUIText updates mCursorWorldPosText and mCursorWorldPosTextShadow
// to show the cursor's current **world coordinates** position
void MandelbrotViewer::updateUIText(sf::Vector2i mouseWindowCoords) {
    auto coordsString = formatCoords(windowPosToWorld(sf::Vector2<double>(mouseWindowCoords)));
    mCursorWorldPosText.setString(coordsString);
    mCursorWorldPosTextShadow.setString(coordsString);
}

double MandelbrotViewer::mandelbrot(double cX, double cY, int maxIters) const {
    // Iterate z_{n} = z_{n-1}^2 + c starting from z_0 = 0. The point escapes as soon as its
    // modulus exceeds 2, and the escape time is the iteration on which that first happens.
    double zX = 0.;
    double zY = 0.;
    for (int n = 1; n <= maxIters; ++n) {
        // (zX + i zY)^2 + c = (zX^2 - zY^2 + cX) + i (2 zX zY + cY)
        const double zPrimeX = zX * zX - zY * zY + cX;
        const double zPrimeY = 2. * zX * zY + cY;
        zX = zPrimeX;
        zY = zPrimeY;
        // Compare the squared modulus against 4 to avoid an unnecessary std::sqrt.
        if (zX * zX + zY * zY > 4.) {
            return static_cast<double>(n);
        }
    }
    // Did not escape within the budget, so treat the point as (possibly) in the set.
    return std::numeric_limits<double>::infinity();
}

double MandelbrotViewer::mandelbrotSmooth(double cX, double cY, int maxIters) const {
    // Same iteration as mandelbrot(), but returning a fractional count n + 1 - log(log|z_n|)/log 2
    // so that neighbouring escape times blend smoothly instead of banding. The modulus at the
    // escape step must be safely above 1 for the double log to be well behaved, so use a radius of
    // 4 rather than 2 (divergence is unaffected, only slightly more iterations are simulated).
    constexpr double ESCAPE_RADIUS_SQUARED = 16.;  // radius 4, squared
    double zX = 0.;
    double zY = 0.;
    for (int n = 1; n <= maxIters; ++n) {
        const double zPrimeX = zX * zX - zY * zY + cX;
        const double zPrimeY = 2. * zX * zY + cY;
        zX = zPrimeX;
        zY = zPrimeY;
        const double modulusSquared = zX * zX + zY * zY;
        if (modulusSquared > ESCAPE_RADIUS_SQUARED) {
            const double modulus = std::sqrt(modulusSquared);
            return static_cast<double>(n) + 1. - std::log(std::log(modulus)) / LOG_2;
        }
    }
    return std::numeric_limits<double>::infinity();
}

// windowPosToWorld takes a point in window coordinates and converts it to world coordinates
sf::Vector2<double> MandelbrotViewer::windowPosToWorld(const sf::Vector2<double>& pWindow) {
    // How far across the window the point is, as a fraction in [0, 1]. Window y grows downward
    // from the top, so it is divided by the window height directly.
    const sf::Vector2<double> fraction(pWindow.x / static_cast<double>(mWindowSize.x),
                                       pWindow.y / static_cast<double>(mWindowSize.y));
    const sf::Vector2<double> worldSize = mMaxPointWorld - mMinPointWorld;
    // World y grows upward, so the top of the window (fraction.y == 0) is mMaxPointWorld.y and the
    // fraction is subtracted rather than added.
    return sf::Vector2<double>(mMinPointWorld.x + fraction.x * worldSize.x,
                               mMaxPointWorld.y - fraction.y * worldSize.y);
}

// drawIntoBuffer renders the current world view (bounded by mMinPointWorld and mMaxPointWorld)
// into mViewBuffer
void MandelbrotViewer::drawIntoViewBuffer(int maxIters) {
    for (unsigned int y = 0; y < mWindowSize.y; ++y) {
        for (unsigned int x = 0; x < mWindowSize.x; ++x) {
            // Sample at the pixel centre, i.e. half a pixel in from its top-left corner.
            const sf::Vector2<double> world =
                windowPosToWorld(sf::Vector2<double>(x + 0.5, y + 0.5));
            const double escapeTime = mandelbrotSmooth(world.x, world.y, maxIters);
            // Points that never escape are (possibly) in the set, and are drawn black. All others
            // are coloured by their escape time through the cyclic gradient.
            const sf::Color color = std::isfinite(escapeTime)
                                        ? CyclicGradient::DEFAULT_GRADIENT(escapeTime)
                                        : sf::Color::Black;
            mViewBuffer.setPixel(sf::Vector2u(x, y), color);
        }
    }
}

// copyViewBufferToGPU takes the drawn CPU-side buffer mViewBuffer and copies it to the
// GPU-side.
void MandelbrotViewer::copyViewBufferToGPU() {
    // Push the freshly drawn CPU-side image into the GPU-side texture.
    mViewBufferGPU.update(mViewBuffer);
}

// draw clears the window, draws the view, as well as the text with its shadow underneath
// it. Finally, the window is displayed.
void MandelbrotViewer::draw() {
    mWindow.clear();
    mWindow.draw(mViewSprite);
    mWindow.draw(mCursorWorldPosTextShadow);
    mWindow.draw(mCursorWorldPosText);
    mWindow.display();
}
