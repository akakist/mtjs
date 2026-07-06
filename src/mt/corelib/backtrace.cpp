#include <iostream>
#include <execinfo.h>  // для backtrace
#include <unistd.h>    // для getpid
#include <cxxabi.h>    // для demangling имён

void print_stacktrace() {
    const int max_stack_size = 100;
    void* stack_addrs[max_stack_size];

    // Получаем адреса функций в стеке
    int stack_depth = backtrace(stack_addrs, max_stack_size);

    // Преобразуем адреса в символы (имена функций)
    char** stack_strings = backtrace_symbols(stack_addrs, stack_depth);

    if (stack_strings == nullptr) {
        std::cerr << "Ошибка при получении backtrace_symbols" << std::endl;
        return;
    }

    // Выводим стек вызовов
    for (int i = 0; i < stack_depth; ++i) {
        // Деманглинг C++ имён (если нужно)
        char* mangled_name = nullptr;
        char* offset_begin = nullptr;
        char* offset_end = nullptr;

        // Парсим строку символа (формат зависит от компилятора)
        for (char* p = stack_strings[i]; *p; ++p) {
            if (*p == '(') {
                mangled_name = p + 1;
            } else if (*p == '+') {
                offset_begin = p;
            } else if (*p == ')') {
                offset_end = p;
                break;
            }
        }

        if (mangled_name && offset_begin && offset_end && mangled_name < offset_begin) {
            *mangled_name-- = '\0';
            *offset_begin++ = '\0';
            *offset_end++ = '\0';

            int demangle_status = 0;
            char* demangled_name = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &demangle_status);

            if (demangle_status == 0) {
                std::cout << "#" << i << " " << stack_strings[i] << " : " << demangled_name << "+" << offset_begin << std::endl;
            } else {
                std::cout << "#" << i << " " << stack_strings[i] << " : " << mangled_name << "+" << offset_begin << std::endl;
            }

            free(demangled_name);
        } else {
            std::cout << "#" << i << " " << stack_strings[i] << std::endl;
        }
    }

    free(stack_strings);
}

