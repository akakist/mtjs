#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include <string.h>
#include "base62.h"

std::string base62::encode(const std::string &input) 
{
    return encode((const uint8_t*)input.data(), input.size());
}
std::string base62::encode(const std::vector<uint8_t> &input) 
{
    return encode(input.data(), input.size());
}
std::string base62::encode(const uint8_t* data, size_t length) 
{
 static const char hex_chars[] = "0123456789abcdef";
        std::string result;
        result.reserve(length * 2);
        
        for (size_t i = 0; i < length; i++) {
            result.push_back(hex_chars[data[i] >> 4]);
            result.push_back(hex_chars[data[i] & 0x0F]);
        }
        return result;

#ifdef KALL
    static const char alphabet[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    // Преобразуем бинарный буфер в большое число (bigint)
    std::string num;
    num.resize(length);
    memcpy(num.data(), data, length);   
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
    for (size_t i = 0; i < length && data[i] == 0; i++) {
        result.push_back(alphabet[0]);
    }

    // Переворачиваем строку (так как остатки шли в обратном порядке)
    std::reverse(result.begin(), result.end());
    return result.empty() ? "0" : result;
#endif
}
std::string base62::decode(const std::string& data) 
{
    return decode((const uint8_t*)data.data(), data.size());
}
std::string base62::decode(const std::vector<uint8_t>& data) 
{
    return decode((const uint8_t*)data.data(), data.size());
}

std::string base62::decode(const uint8_t* data, size_t length) 
{

          if (length % 2 != 0) {
            throw std::invalid_argument("Hex string length must be even");
        }
        
        std::string result;
        result.reserve(length / 2);
        
        for (size_t i = 0; i < length; i += 2) {
            uint8_t high = hex_char_to_value(data[i]);
            uint8_t low = hex_char_to_value(data[i + 1]);
            result.push_back((high << 4) | low);
        }
        return result;
#ifdef KALL        
    // printf("data %s\n",std::string((const char*)data,length).c_str());
    static const char alphabet[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static int lookup[256]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
        30,31,32,33,34,35,-1,-1,-1,-1,-1,-1,
        36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
        ,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
        ,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    // Таблица для быстрого поиска значения символа
    // int lookup[256];
    // for (int i = 0; i < 256; i++) lookup[i] = -1;
    // for (int i = 0; i < 62; i++) lookup[(unsigned char)alphabet[i]] = i;

    // Представляем строку как большое число в базе 62
    std::string num; // big-endian

    for (size_t i = 0; i < length; i++) {
        char c = data[i];   
        int val = lookup[(unsigned char)c];
        if (val == -1) throw std::invalid_argument("Invalid Base62 character "+std::string(1,c));

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
#endif
}


#ifdef ___1

#include <iostream>

int main() {

    auto enc=base62::encode((const uint8_t*)"Sergey Belyalov", 15);
    std::cout<<"Base62: "<<enc<<std::endl;
    auto dec=base62::decode((const uint8_t*)enc.data(), enc.size());
    std::cout<<"Decoded: "<<std::string((char*)dec.data(),dec.size())<<std::endl;

}
#endif