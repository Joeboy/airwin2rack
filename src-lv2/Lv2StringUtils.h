#pragma once

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>

inline std::string trimWhitespace(const std::string& s)
{
    size_t start = 0;
    size_t end = s.size();
    while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

inline bool tryParseDouble(const std::string& s, double& out)
{
    const std::string t = trimWhitespace(s);
    if (t.empty()) return false;

    char* endPtr = nullptr;
    out = std::strtod(t.c_str(), &endPtr);
    if (!endPtr || endPtr == t.c_str()) return false;

    while (*endPtr != '\0')
    {
        if (!std::isspace(static_cast<unsigned char>(*endPtr))) return false;
        ++endPtr;
    }

    return std::isfinite(out);
}