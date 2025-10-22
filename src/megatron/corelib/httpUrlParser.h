#pragma once
#include <string_view>
#include <vector>
#include <utility>

#include <string_view>
#include <map>
#include <utility>

class URIParser {
public:
    std::string_view full_uri_;
    std::string_view path_;
    std::string_view query_;
    std::string_view fragment_;
    
    std::map<std::string_view, std::string_view> params_;

public:
    URIParser() = default;
    
    explicit URIParser(std::string_view uri) {
        parse(uri);
    }
    
    bool parse(std::string_view uri) {
        full_uri_ = uri;
        params_.clear();
        
        if (uri.empty()) {
            path_ = "/";
            return true;
        }
        
        // URI может начинаться с пути или быть полным URL
        // Но в HTTP заголовке обычно только путь + query + fragment
        
        std::string_view remaining = uri;
        
        // Проверяем, не начинается ли со схемы (на всякий случай)
        if (uri.find("://") != std::string_view::npos) {
            // Это полный URL - пропускаем схему и хост
            size_t scheme_end = uri.find("://");
            remaining = uri.substr(scheme_end + 3);
            
            // Пропускаем хост (до первого / или до конца)
            size_t host_end = remaining.find('/');
            if (host_end != std::string_view::npos) {
                remaining = remaining.substr(host_end);
            } else {
                // Нет пути - только хост
                path_ = "/";
                return true;
            }
        }
        
        // Теперь remaining содержит только путь, query и fragment
        parse_path_components(remaining);
        return true;
    }
    
private:
    void parse_path_components(std::string_view path) {
        // Ищем query
        size_t query_start = path.find('?');
        if (query_start != std::string_view::npos) {
            query_ = path.substr(query_start + 1);
            path_ = path.substr(0, query_start);
            
            // Ищем fragment в query
            size_t fragment_start = query_.find('#');
            if (fragment_start != std::string_view::npos) {
                fragment_ = query_.substr(fragment_start + 1);
                query_ = query_.substr(0, fragment_start);
            }
            
            // Парсим параметры
            parse_query_params();
        } else {
            // Ищем fragment в path
            size_t fragment_start = path.find('#');
            if (fragment_start != std::string_view::npos) {
                fragment_ = path.substr(fragment_start + 1);
                path_ = path.substr(0, fragment_start);
            } else {
                path_ = path;
            }
        }
        
        // Если путь пустой - устанавливаем корневой
        if (path_.empty()) {
            path_ = "/";
        }
    }
    
    void parse_query_params() {
        std::string_view query = query_;
        size_t start = 0;
        
        while (start < query.size()) {
            size_t end = query.find('&', start);
            if (end == std::string_view::npos) {
                end = query.size();
            }
            
            std::string_view pair = query.substr(start, end - start);
            if (!pair.empty()) {
                size_t eq_pos = pair.find('=');
                if (eq_pos != std::string_view::npos) {
                    params_[pair.substr(0, eq_pos)]=pair.substr(eq_pos + 1);
                } else {
                    // Параметр без значения
                    params_[pair]=std::string_view();
                }
            }
            
            start = end + 1;
        }
    }

public:
    // Геттеры
    std::string_view full() const { return full_uri_; }
    std::string_view path() const { return path_; }
    std::string_view query() const { return query_; }
    std::string_view fragment() const { return fragment_; }
    
    // Работа с параметрами
    const auto& params() const { return params_; }
    size_t param_count() const { return params_.size(); }
    
    std::string_view get_param(std::string_view key, std::string_view default_val = "") const {
        for (const auto& [k, v] : params_) {
            if (k == key) {
                return v;
            }
        }
        return default_val;
    }
    
    bool has_param(std::string_view key) const {
        for (const auto& [k, v] : params_) {
            if (k == key) {
                return true;
            }
        }
        return false;
    }
    
    void debug_print() const {
        printf("URI: %.*s\n", (int)full_uri_.size(), full_uri_.data());
        printf("Path: %.*s\n", (int)path_.size(), path_.data());
        printf("Query: %.*s\n", (int)query_.size(), query_.data());
        printf("Fragment: %.*s\n", (int)fragment_.size(), fragment_.data());
        printf("Params: %zu\n", params_.size());
        
        for (const auto& [key, value] : params_) {
            printf("  %.*s = %.*s\n", 
                   (int)key.size(), key.data(),
                   (int)value.size(), value.data());
        }
    }
};