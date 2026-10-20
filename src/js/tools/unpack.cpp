#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void mkdirs(const std::string& path) {
    std::string cur;
    for (size_t i = 0; i < path.size()-1; ++i) {
        cur += path[i];
        if (path[i] == '/' || i + 1 == path.size()) {
            if (cur == "/" || cur.empty()) continue;
            std::string dir = cur;
            if (dir.back() == '/') dir.pop_back();
            if (dir.empty()) continue;
            ::mkdir(dir.c_str(), 0755);   // EEXIST — ок
        }
    }
}

static bool starts_with(const std::string& s, const char* p) {
    size_t n = strlen(p);
    return s.size() >= n && memcmp(s.data(), p, n) == 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <archive.txt>\n";
        return 1;
    }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in) { std::cerr << "cannot open: " << argv[1] << "\n"; return 1; }

    std::string line;
    std::string current_path;
    std::string body;
    int files = 0, deleted = 0;

    auto flush = [&]() {
        if (current_path.empty()) return;
        // DELETED marker: тело — одна строка "=== DELETED ===\n"
        // либо без \n в конце
        std::string t = body;
        if (!t.empty() && t.back() == '\n') t.pop_back();
        if (t == "=== DELETED ===") {
            if (::unlink(current_path.c_str()) == 0)
                std::cerr << "deleted: " << current_path << "\n";
            else
                std::cerr << "delete failed (may not exist): " << current_path << "\n";
            deleted++;
        } else {
            mkdirs(current_path);
            std::ofstream out(current_path, std::ios::binary);
            if (!out) {
                std::cerr << "cannot write: " << current_path << "\n";
            } else {
                out.write(body.data(), body.size());
                std::cerr << "wrote: " << current_path << " (" << body.size() << " bytes)\n";
                files++;
            }
        }
        current_path.clear();
        body.clear();
    };

    const std::string prefix = "=== FILE: ";
    const std::string suffix = " ===";

    while (std::getline(in, line)) {
        // getline strips \n; strip \r
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (starts_with(line, "=== FILE: ")) {
            // flush previous
            flush();
            // header: "=== FILE: path ==="
            if (line.size() < prefix.size() + suffix.size()) {
                std::cerr << "bad header: " << line << "\n";
                continue;
            }
            // must end with " ==="
            if (line.compare(line.size() - suffix.size(), suffix.size(), suffix) != 0) {
                std::cerr << "bad header (no suffix): " << line << "\n";
                continue;
            }
            current_path = line.substr(prefix.size(),
                                       line.size() - prefix.size() - suffix.size());
            // trim
            while (!current_path.empty() && (current_path.back() == ' ' || current_path.back() == '\t'))
                current_path.pop_back();
        } else {
            // part of body: restore the trailing \n that getline stripped,
            // но только если это не последняя строка файла (узнаем на flush)
            body += line;
            body += '\n';
        }
    }
    flush();

    std::cerr << "done: " << files << " written, " << deleted << " deleted\n";
    return 0;
}
