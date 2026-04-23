#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include "base62.h"

std::string base62::encode(const std::string& data) {
    static const char alphabet[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    // Преобразуем бинарный буфер в большое число (bigint)
    std::string num = data; // копия для деления
    std::string result;

    // Пока число не пустое
    while (!num.empty()) {
        // Делим big-endian число на 62
        uint32_t remainder = 0;
        std::string newNum;
        newNum.reserve(num.size());

        for (size_t i = 0; i < num.size(); i++) {
            uint32_t cur = (remainder << 8) + num[i];
            uint8_t digit = cur / 62;
            remainder = cur % 62;
            if (!newNum.empty() || digit != 0) {
                newNum.push_back(digit);
            }
        }

        result.push_back(alphabet[remainder]);
        num.swap(newNum);
    }

    // Обработка ведущих нулей
    for (size_t i = 0; i < data.size() && data[i] == 0; i++) {
        result.push_back(alphabet[0]);
    }

    // Переворачиваем строку (так как остатки шли в обратном порядке)
    std::reverse(result.begin(), result.end());
    return result.empty() ? "0" : result;
}

std::string base62::decode(const std::string& str) {
    static const char alphabet[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    // Таблица для быстрого поиска значения символа
    int lookup[256];
    for (int i = 0; i < 256; i++) lookup[i] = -1;
    for (int i = 0; i < 62; i++) lookup[(unsigned char)alphabet[i]] = i;

    // Представляем строку как большое число в базе 62
    std::string num; // big-endian

    for (char c : str) {
        int val = lookup[(unsigned char)c];
        if (val == -1) throw std::invalid_argument("Invalid Base62 character");

        // Умножаем текущее число на 62 и добавляем val
        int carry = val;
        for (int i = (int)num.size() - 1; i >= 0; i--) {
            int cur = num[i] * 62 + carry;
            num[i] = cur & 0xFF;
            carry = cur >> 8;
        }
        if (carry > 0) {
            num.insert(num.begin(), carry & 0xFF);
            if (carry >> 8) num.insert(num.begin(), (carry >> 8) & 0xFF);
        }
    }

    // Убираем ведущие нули
    while (!num.empty() && num[0] == 0) {
        num.erase(num.begin());
    }

    return num;
}


#ifdef ___1

#include <iostream>

int main() {

    auto enc=base62::encode("Sergey Belyalov");
    std::cout<<"Base62: "<<enc<<std::endl;
    auto dec=base62::decode(enc);
    std::cout<<"Decoded: "<<std::string((char*)dec.data(),dec.size())<<std::endl;

}
#endif