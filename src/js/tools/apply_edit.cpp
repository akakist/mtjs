// apply_edit.cpp — вставка блоков по маркеру (без автоудаления).
// Сборка:  g++ -O2 -std=c++17 apply_edit.cpp -o apply_edit
// Запуск:  ./apply_edit <edits.txt> [--check]
//
// Формат edits.txt:
//   FILE: <путь к файлу, который правим>
//   INSERT_BEFORE_FUNC
//   FUNC: <уникальная подстрока-маркер (имя функции)>
//   ---
//   <строки, которые надо вставить (как есть)>
//   ---
//   END
//   ... (можно несколько INSERT_BEFORE_FUNC подряд для этого FILE)
//   FILE: <следующий файл>      (если нужно несколько файлов)
//   ...
//
// --check : dry-run — показать, что и куда вставится, файлы не менять.
// Старую функцию удаляешь руками (утилита ничего не удаляет).

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Edit {
    std::string marker;
    std::vector<std::string> block;
};

struct FileEdit {
    std::string path;
    std::vector<Edit> edits;
};

static std::vector<std::string> read_lines(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open: " + path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

static void write_lines(const std::string& path, const std::vector<std::string>& lines) {
    // backup .bak
    {
        std::ifstream src(path, std::ios::binary);
        std::ofstream dst(path + ".bak", std::ios::binary);
        dst << src.rdbuf();
    }
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot write: " + path);
    for (auto& l : lines) out << l << '\n';
}

static std::vector<FileEdit> parse_edits(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open edits: " + path);
    std::vector<FileEdit> files;
    FileEdit cur_file;
    Edit cur_edit;
    bool in_block = false;
    bool have_file = false;
    bool have_edit = false;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!in_block && (line.empty() || line[0] == '#')) continue;

        if (line.rfind("FILE:", 0) == 0) {
            if (have_file && !cur_file.edits.empty()) files.push_back(std::move(cur_file));
            cur_file = FileEdit{};
            cur_file.path = line.substr(5);
            size_t b = cur_file.path.find_first_not_of(" \t");
            size_t e = cur_file.path.find_last_not_of(" \t");
            if (b == std::string::npos) cur_file.path.clear();
            else cur_file.path = cur_file.path.substr(b, e - b + 1);
            have_file = true;
            have_edit = false;
            in_block = false;
        } else if (line == "INSERT_BEFORE_FUNC") {
            if (!have_file) throw std::runtime_error("INSERT_BEFORE_FUNC without FILE:");
            cur_edit = Edit{};
            have_edit = true;
            in_block = false;
        } else if (line.rfind("FUNC:", 0) == 0 && have_edit) {
            cur_edit.marker = line.substr(5);
            size_t b = cur_edit.marker.find_first_not_of(" \t");
            size_t e = cur_edit.marker.find_last_not_of(" \t");
            if (b == std::string::npos) cur_edit.marker.clear();
            else cur_edit.marker = cur_edit.marker.substr(b, e - b + 1);
        } else if (line == "---") {
            in_block = !in_block;
        } else if (line == "END" && have_edit) {
            cur_file.edits.push_back(std::move(cur_edit));
            cur_edit = Edit{};
            have_edit = false;
            in_block = false;
        } else if (have_edit && in_block) {
            cur_edit.block.push_back(line);
        }
    }
    if (have_edit) throw std::runtime_error("unterminated INSERT_BEFORE_FUNC (missing END)");
    if (have_file && !cur_file.edits.empty()) files.push_back(std::move(cur_file));
    return files;
}

static size_t find_unique(const std::vector<std::string>& lines,
                          const std::string& marker,
                          const std::string& path) {
    std::vector<size_t> idx;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find(marker) != std::string::npos) idx.push_back(i);
    }
    if (idx.empty()) {
        throw std::runtime_error("marker not found in " + path + ": " + marker);
    }
    if (idx.size() > 1) {
        std::ostringstream os;
        os << "marker not unique (" << idx.size() << ") in " << path
           << ": " << marker << " -> lines ";
        for (size_t k = 0; k < idx.size(); ++k) {
            if (k) os << ",";
            os << (idx[k] + 1);
        }
        throw std::runtime_error(os.str());
    }
    return idx[0];
}

static void apply_file(const FileEdit& fe, bool check) {
    auto lines = read_lines(fe.path);

    // сначала находим все позиции по исходному файлу, потом вставляем с конца
    std::vector<std::pair<size_t, const Edit*>> plan;
    for (auto& e : fe.edits) {
        if (e.marker.empty()) throw std::runtime_error("FUNC: marker missing in " + fe.path);
        size_t i = find_unique(lines, e.marker, fe.path);
        plan.emplace_back(i, &e);
    }
    std::sort(plan.begin(), plan.end(),
              [](auto& a, auto& b) { return a.first > b.first; });

    for (auto& [i, e] : plan) {
        if (check) {
            std::cout << "[check] " << fe.path << " INSERT_BEFORE line " << (i + 1)
                      << ": " << lines[i] << "\n";
            std::cout << "[check] block (" << e->block.size() << " lines):\n";
            for (auto& l : e->block) std::cout << "    " << l << "\n";
        }
        lines.insert(lines.begin() + i, e->block.begin(), e->block.end());
    }

    if (check) {
        std::cout << "[check] " << fe.edits.size() << " edit(s) would be applied to "
                  << fe.path << "\n";
        return;
    }
    write_lines(fe.path, lines);
    std::cout << "applied " << fe.edits.size() << " edit(s) to " << fe.path
              << " (backup: " << fe.path << ".bak)\n";
}

int main(int argc, char** argv) {
    try {
        bool check = false;
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--check") == 0) check = true;
            else args.emplace_back(argv[i]);
        }
        if (args.size() != 1) {
            std::cerr << "usage: " << argv[0] << " <edits.txt> [--check]\n";
            return 1;
        }

        auto files = parse_edits(args[0]);
        if (files.empty()) {
            std::cerr << "no edits parsed from " << args[0] << "\n";
            return 1;
        }
        for (auto& fe : files) apply_file(fe, check);
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
}