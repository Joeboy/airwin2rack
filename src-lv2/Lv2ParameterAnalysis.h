#pragma once

#include "AirwinRegistry.h"
#include "Lv2StringUtils.h"

#include <cmath>
#include <string>
#include <utility>
#include <vector>

struct Lv2NumericDisplayDomain {
    float defaultValue = 0.0f;
    float minimum = 0.0f;
    float maximum = 1.0f;
    bool hasExternalDomain = false;
    bool isIntegerEnumeration = false;
    std::vector<std::pair<std::string, int>> integerScalePoints;

    bool needsTextToValueMapping() const
    {
        return hasExternalDomain;
    }
};

inline Lv2NumericDisplayDomain analyzeNumericDisplayDomain(
    AirwinConsolidatedBase* effect,
    int paramIndex,
    int sampleCount = 512)
{
    Lv2NumericDisplayDomain result;
    result.defaultValue = effect->getParameter(paramIndex);

    if (!effect->canConvertParameterTextToValue(paramIndex))
        return result;

    const float original = result.defaultValue;
    std::vector<std::pair<std::string, double>> transitions;
    transitions.reserve(sampleCount + 1);

    bool monotonic = true;
    double minValue = 0.0;
    double maxValue = 0.0;
    double prevValue = 0.0;
    std::string prevText;
    bool firstTransition = true;

    for (int i = 0; i <= sampleCount; ++i)
    {
        const float norm = static_cast<float>(i) / static_cast<float>(sampleCount);
        effect->setParameter(paramIndex, norm);

        char displayBuf[kVstMaxParamStrLen] = {0};
        effect->getParameterDisplay(paramIndex, displayBuf);
        std::string displayText = trimWhitespace(displayBuf);

        if (!firstTransition && displayText == prevText)
            continue;

        double parsed = 0.0;
        if (!tryParseDouble(displayText, parsed))
        {
            effect->setParameter(paramIndex, original);
            return result;
        }

        transitions.emplace_back(displayText, parsed);
        if (firstTransition)
        {
            minValue = maxValue = parsed;
        }
        else
        {
            if (parsed < prevValue - 1e-6) monotonic = false;
            if (parsed < minValue) minValue = parsed;
            if (parsed > maxValue) maxValue = parsed;
        }

        prevValue = parsed;
        prevText = displayText;
        firstTransition = false;
    }

    effect->setParameter(paramIndex, original);
    if (transitions.empty())
        return result;

    const bool differsFromNormalized =
        (std::fabs(minValue - 0.0) > 1e-3) || (std::fabs(maxValue - 1.0) > 1e-3);
    if (!differsFromNormalized)
        return result;

    result.hasExternalDomain = true;
    result.minimum = static_cast<float>(minValue);
    result.maximum = static_cast<float>(maxValue);

    char defDisplayBuf[kVstMaxParamStrLen] = {0};
    effect->setParameter(paramIndex, original);
    effect->getParameterDisplay(paramIndex, defDisplayBuf);
    double defDisplay = original;
    if (tryParseDouble(defDisplayBuf, defDisplay))
        result.defaultValue = static_cast<float>(defDisplay);

    bool allIntegers = true;
    for (const auto& transition : transitions)
    {
        if (std::fabs(transition.second - std::round(transition.second)) > 1e-6)
        {
            allIntegers = false;
            break;
        }
    }

    if (monotonic && allIntegers && transitions.size() >= 2 && transitions.size() <= 128)
    {
        result.isIntegerEnumeration = true;
        result.integerScalePoints.reserve(transitions.size());
        for (const auto& transition : transitions)
        {
            result.integerScalePoints.emplace_back(
                transition.first, static_cast<int>(std::lround(transition.second)));
        }
        result.minimum = static_cast<float>(result.integerScalePoints.front().second);
        result.maximum = static_cast<float>(result.integerScalePoints.back().second);
        result.defaultValue = static_cast<float>(std::lround(result.defaultValue));
    }

    effect->setParameter(paramIndex, original);
    return result;
}