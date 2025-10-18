#pragma once
#include <string_view>
#include <vector>
#include <utility>

class SimpleHeadersParser {
private:
    std::vector<std::pair<std::string_view, std::string_view>> headers_;

public:
    bool parse(std::string_view headers_section) {
        headers_.clear();
        
        size_t pos = 0;
        while (pos < headers_section.size()) {
            // Конец строки
            size_t line_end = headers_section.find("\r\n", pos);
            if (line_end == std::string_view::npos) {
                line_end = headers_section.size();
            }
            
            // Пустая строка - конец
            if (line_end == pos) break;
            
            std::string_view line = headers_section.substr(pos, line_end - pos);
            
            // Парсим заголовок
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string_view::npos) {
                std::string_view key = line.substr(0, colon_pos);
                std::string_view value = line.substr(colon_pos + 1);
                
                // Тримминг пробелов
                while (!key.empty() && (key.front() == ' ' || key.front() == '\t')) {
                    key.remove_prefix(1);
                }
                while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) {
                    key.remove_suffix(1);
                }
                
                while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
                    value.remove_prefix(1);
                }
                while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
                    value.remove_suffix(1);
                }
                
                if (!key.empty()) {
                    headers_.emplace_back(key, value);
                }
            }
            
            pos = line_end + 2;
        }
        
        return true;
    }
    
    const auto& headers() const { return headers_; }
};