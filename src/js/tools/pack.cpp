#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open: " + path);
    std::string s((std::istreambuf_iterator<char>(f)),
                   std::istreambuf_iterator<char>());
    return s;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <list.txt> <archive.txt>\n"
                  << "  list.txt — один путь на строку, # комментарий, пустые пропускаются\n";
        return 1;
    }
    const char* list_path = argv[1];
    const char* out_path  = argv[2];

    std::ifstream lf(list_path);
    if (!lf) { std::cerr << "cannot open list: " << list_path << "\n"; return 1; }

    std::ofstream out(out_path, std::ios::binary);
    if (!out) { std::cerr << "cannot open output: " << out_path << "\n"; return 1; }

    std::string line;
    int count = 0;
    while (std::getline(lf, line)) {
        // strip \r (windows)
        if (!line.empty() && line.back() == '\r') line.pop_back();
        // trim leading/trailing whitespace
        size_t b = line.find_first_not_of(" \t");
        if (b == std::string::npos) continue;
        size_t e = line.find_last_not_of(" \t");
        std::string path = line.substr(b, e - b + 1);
        if (path.empty() || path[0] == '#') continue;

        std::string content = read_file(path);
        out << "=== FILE: " << path << " ===\n";
        out.write(content.data(), content.size());
        if (content.empty() || content.back() != '\n')
            out << '\n';
        count++;
    }
    std::cerr << "packed " << count << " file(s) into " << out_path << "\n";
    return 0;
}
