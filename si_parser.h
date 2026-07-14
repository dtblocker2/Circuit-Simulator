#ifndef SI_PARSER_H
#define SI_PARSER_H

#include <string>
#include <cctype>
#include <algorithm>
#include <stdexcept>

inline double parseSI(const std::string& input) {
    if (input.empty()) return 0.0;
    std::string str = input;
    double multiplier = 1.0;

    if (str.length() >= 3 && (str.substr(str.length()-3) == "Meg" || str.substr(str.length()-3) == "meg")) {
        multiplier = 1e6; str.erase(str.length()-3);
    } else if (!isdigit(str.back()) && str.back() != '.') {
        char suffix = str.back();
        str.pop_back();
        switch(suffix) {
            case 'f': case 'F': multiplier = 1e-15; break;
            case 'p': case 'P': multiplier = 1e-12; break;
            case 'n': case 'N': multiplier = 1e-9; break;
            case 'u': case 'U': multiplier = 1e-6; break;
            case 'm':           multiplier = 1e-3; break;
            case 'k': case 'K': multiplier = 1e3; break;
            case 'M':           multiplier = 1e6; break;
            case 'G': case 'g': multiplier = 1e9; break;
            default: break;
        }
    }
    return std::stod(str) * multiplier;
}

#endif // SI_PARSER_H