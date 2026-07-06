#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <regex>

namespace fs = std::filesystem;

struct SymbolInfo {
    std::string name;
    uint32_t line_number;
    std::string file_path;
    uint32_t hash; // Добавляем хеш
};

class HeaderParser {
private:
    std::map<std::string, SymbolInfo>  symbols;
    std::regex pattern;

public:
    HeaderParser() : pattern(R"(@g_\w+)") {}

    // Constexpr хеш-функция (FNV-1a)
    constexpr uint32_t compute_hash(const char* str) {
        uint32_t hash = 2166136261u;
        while (*str) {
            hash ^= static_cast<uint32_t>(*str++);
            hash *= 16777619u;
        }
        return hash;
    }

    // Рекурсивный обход дерева директорий
    void traverseHeaders(const fs::path& directory) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file() && isHeaderFile(entry.path())) {
                    parseHeaderFile(entry.path());
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }
    }

    // Парсинг одного header файла
    void parseHeaderFile(const fs::path& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Cannot open file: " << file_path << std::endl;
            return;
        }

        std::string line;
        uint32_t line_num = 0;

        while (std::getline(file, line)) {
            line_num++;
            processLine(line, line_num, file_path);
        }
    }

    // Обработка строки и поиск символов @g_
    void processLine(const std::string& line, uint32_t line_num, const fs::path& file_path) {
        std::sregex_iterator it(line.begin(), line.end(), pattern);
        std::sregex_iterator end;

        while (it != end) {
            SymbolInfo symbol;
            symbol.name = it->str();
            symbol.line_number = line_num;
            symbol.file_path = file_path.string();
            symbol.hash = compute_hash(symbol.name.c_str()); // Вычисляем хеш
            symbols.insert({    symbol.name,symbol});
            ++it;
        }
    }

    // Генерация switch-case кода с хешами
    void generateSwitchCode() const {
        std::cout << "// Auto-generated switch case with hashes\n";
        std::cout << "const char* get_symbol_name(uint32_t hash) {\n";
        std::cout << "    switch (hash) {\n";

        for (const auto& symbol : symbols) {
            std::cout << "        case ghash(\"" << symbol.second. name << "\") : return \"" << symbol.second.name.substr(3) << "\";\n";
        }

        std::cout << "        default: return \"unknown\";\n";
        std::cout << "    }\n";
        std::cout << "}\n\n";
    }

    // Генерация reverse lookup функции
    void generateReverseLookup() const {
        std::cout << "// Reverse lookup: string to hash\n";
        std::cout << "uint32_t get_symbol_hash(const char* name) {\n";
        
        for (const auto& symbol : symbols) {
            std::cout << "    if (strcmp(name, \"" << symbol.second.name << "\") == 0) return " 
                      << symbol.second.hash << ";\n";
        }
        
        std::cout << "    return 0;\n";
        std::cout << "}\n\n";
    }

    // Генерация constexpr версии
    void generateConstexprSwitch() const {
        std::cout << "// Constexpr version for compile-time use\n";
        std::cout << "consteval const char* get_symbol_name_ce(uint32_t hash) {\n";
        
        for (const auto& symbol : symbols) {
            std::cout << "    if (hash == ghash(\"" << symbol.second.hash << "\")  return \"" << symbol.second.name << "\";\n";
        }
        
        std::cout << "    return \"unknown\";\n";
        std::cout << "}\n\n";
    }

    void printStats() const {
        std::cout << "Found " << symbols.size() << " symbols starting with @g_\n";
        for (const auto& symbol : symbols) {
            std::cout << "  " << symbol.second.name << " -> 0x" << std::hex << symbol.second.hash 
                      << std::dec << " at " << symbol.second.file_path 
                      << ":" << symbol.second.line_number << std::endl;
        }
    }

private:
    bool isHeaderFile(const fs::path& path) const {
        std::string ext = path.extension().string();
        return ext == ".h" || ext == ".hpp" || ext == ".hxx" || ext == ".hh";
    }
};

// Пример использования
int main() {
    std::cout << "/*\n";
    HeaderParser parser;
    
    // Обход текущей директории и всех поддиректорий
    parser.traverseHeaders(".");
    
    // Вывод статистики
    parser.printStats();
    // Генерация кода
    std::cout << "*/\n";
    std::cout << "\n/// === GENERATED CODE ===\n";
    std::cout << "#include <stdint.h>\n";

    parser.generateSwitchCode();
//    parser.generateReverseLookup();
//    parser.generateConstexprSwitch();
    
    return 0;
}