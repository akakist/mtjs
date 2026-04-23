#include <iostream>
#include <string>
#include <map>
#include <cctype>
#include <stdint.h>

// Таблица соответствий: Unicode codepoint → ASCII uppercase
std::map<uint32_t, char> mapping_upper = {
    // Кириллица uppercase
    {0x0410,'A'}, {0x0412,'B'}, {0x0421,'C'}, {0x0420,'P'}, {0x041E,'O'},
    {0x0415,'E'}, {0x0425,'X'}, {0x041C,'M'}, {0x0422,'T'}, {0x041A,'K'},
    {0x041D,'H'}, {0x0423,'Y'}, {0x0417,'3'},

    // Кириллица lowercase → ASCII uppercase
    {0x0430,'A'}, {0x0441,'C'}, {0x0435,'E'}, {0x043E,'O'}, {0x0440,'P'},
    {0x0445,'X'}, {0x043C,'M'}, {0x0442,'T'}, {0x043A,'K'}, {0x043D,'H'},
    {0x0443,'Y'}, {0x0437,'3'}
};

// UTF-8 декодер: возвращает Unicode codepoint и сдвигает индекс
uint32_t decode_utf8(const std::string& s, size_t& i) {
    unsigned char c = s[i];
    if (c < 0x80) { // ASCII
        return s[i++];
    } else if ((c >> 5) == 0x6) { // 2 байта
        uint32_t cp = ((s[i] & 0x1F) << 6) | (s[i+1] & 0x3F);
        i += 2;
        return cp;
    } else if ((c >> 4) == 0xE) { // 3 байта
        uint32_t cp = ((s[i] & 0x0F) << 12) |
                      ((s[i+1] & 0x3F) << 6) |
                      (s[i+2] & 0x3F);
        i += 3;
        return cp;
    } else if ((c >> 3) == 0x1E) { // 4 байта
        uint32_t cp = ((s[i] & 0x07) << 18) |
                      ((s[i+1] & 0x3F) << 12) |
                      ((s[i+2] & 0x3F) << 6) |
                      (s[i+3] & 0x3F);
        i += 4;
        return cp;
    }
    return s[i++]; // fallback
}

std::string normalizeNick(const std::string& input)
{
    std::string result;
    for (size_t i = 0; i < input.size();) {
        uint32_t cp = decode_utf8(input, i);
        if (cp < 128) {
            // ASCII → uppercase
            result.push_back(std::toupper((char)cp));
        } else {
            auto it = mapping_upper.find(cp);
            if (it != mapping_upper.end()) {
                result.push_back(it->second);
            }
            // если символ не в таблице — можно игнорировать или оставить
        }
    }
    return result;
}
#ifdef ___1
int main() {
    std::string nick1 = u8"Привет";
    std::string nick2 = u8"privet";
    std::string nick3 = u8"пРиВеТ";
    std::string nick4 = u8"р1vеt"; // русские р и е

    std::cout << normalizeNick(nick1) << std::endl; // PRIVET
    std::cout << normalizeNick(nick2) << std::endl; // PRIVET
    std::cout << normalizeNick(nick3) << std::endl; // PRIVET
    std::cout << normalizeNick(nick4) << std::endl; // P1VET
}
#endif