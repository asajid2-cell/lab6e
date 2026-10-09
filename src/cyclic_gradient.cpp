#include "cyclic_gradient.h"

#include <algorithm>  // std::sort, std::upper_bound
#include <cmath>      // std::fmod

CyclicGradient::CyclicGradient(double length,
                               const std::vector<std::pair<double, sf::Color>>& curve)
    : mLength{length}, mKeyframes{curve} {
    for (auto& [time, _] : mKeyframes) {
        time = std::fmod(time, mLength);
    }
    std::sort(mKeyframes.begin(), mKeyframes.end(), cmp);
}

sf::Color CyclicGradient::operator()(double val) const {
    val = std::fmod(val, mLength);
    if (val < 0) {
        val += mLength;
    }
    // binary search for first keyframe after val.
    auto afterIter =
        std::upper_bound(mKeyframes.begin(), mKeyframes.end(), std::pair{val, sf::Color{}}, cmp);
    if (afterIter == mKeyframes.end()) {
        // if we are at the last keyframe time or later, cycle to the first element
        afterIter = mKeyframes.begin();
    }
    const int endIdx = &(*afterIter) - &mKeyframes[0];
    const int startIdx = (endIdx == 0) ? (mKeyframes.size() - 1) : (endIdx - 1);
    // linear interpolation
    double length = mKeyframes[endIdx].first - mKeyframes[startIdx].first;
    double coveredLength = val - mKeyframes[startIdx].first;
    if (length < 0) {
        length = mKeyframes[endIdx].first + (mLength - mKeyframes[startIdx].first);
        if (val <= mKeyframes[endIdx].first) {
            // time is in [0, endIdx]
            coveredLength = val + (mLength - mKeyframes[startIdx].first);
        } else {
            // time is in [startIdx, mLength)
            coveredLength = val - mKeyframes[startIdx].first;
        }
    }
    assert(length > 0);
    assert(coveredLength >= 0);
    const double t = coveredLength / length;
    return lerp(mKeyframes[startIdx].second, mKeyframes[endIdx].second, t);
}

const CyclicGradient CyclicGradient::DEFAULT_GRADIENT =
    CyclicGradient(40., {{6.1, sf::Color(5, 7, 140)},       // 0 7 113
                         {22.3, sf::Color(237, 255, 255)},  //
                         {31.2, sf::Color(255, 170, 0)},    //
                         {39.8, sf::Color(0, 2, 0)}});
