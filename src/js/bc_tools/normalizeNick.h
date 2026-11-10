#pragma once
#include <string>

// ASCII-only нормализация ника:
//   toupper + схлопывание визуальных двойников:
//   0→O, 1→I, L→I, 2→Z, 5→S, 8→B, 6→G, 9→G
//   @ и . — разрешены (email-like), НЕ схлопываются
//   не-ASCII / недопустимый символ → ""
std::string normalizeNick(const std::string& input);


inline std::string normalizeNick(const std::string& input)
{
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if (c < 128 && (std::isalnum((unsigned char)c) || c == '_' || c == '@' || c == '.')) {
            char u = (char)std::toupper((unsigned char)c);
            switch (u) {
                case '0': u = 'O'; break;
                case '1': u = 'I'; break;
                case 'L': u = 'I'; break;
                case '2': u = 'Z'; break;
                case '5': u = 'S'; break;
                case '8': u = 'B'; break;
                // case '6': u = 'G'; break;
                // case '9': u = 'G'; break;
                default: break;
            }
            out.push_back(u);
        } else {
            return "";
        }
    }
    return out;
}
