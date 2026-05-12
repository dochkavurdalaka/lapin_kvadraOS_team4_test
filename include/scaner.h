#pragma once

#include <iostream>
#include <tuple>
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <optional>
#include <pwd.h>
#include <unistd.h>
#include <cstring>


namespace fs = std::filesystem;

struct Filter {
    std::unordered_set<std::string> audio_extensions = {".mp3", ".wav", ".flac", ".ogg",  ".aac",
                                                        ".m4a", ".wma", ".opus", ".aiff", ".alac",
                                                        ".ape", ".dts", ".ac3",  ".amr",  ".ra"};

    std::unordered_set<std::string> video_extensions = {
        ".mp4", ".mkv", ".avi", ".mov", ".wmv",  ".flv", ".webm", ".m4v", ".mpg", ".mpeg",
        ".3gp", ".3g2", ".ogv", ".mts", ".m2ts", ".vob", ".rmvb", ".f4v", ".divx"};

    std::unordered_set<std::string> images_extensions = {
        ".png", ".jpeg", ".jpg", ".gif", ".bmp", ".tiff", ".tif", ".webp", ".heic", ".heif",
        ".svg", ".ico",  ".raw", ".cr2", ".nef", ".arw",  ".dng", ".psd",  ".ai",   ".eps"};

    std::optional<std::string> Check(const std::string& extension) const {
        if (audio_extensions.contains(extension)) {
            return "audio";
        } else if (video_extensions.contains(extension)) {
            return "video";
        } else if (images_extensions.contains(extension)) {
            return "images";
        }
        return std::nullopt;
    }
};

// Шаблонный хелпер, который содержит общую логику итерации
template <typename DirectoryIterator>
void ProcessDirectory(const fs::path& path,
                      std::unordered_map<std::string, std::vector<std::string>>* result,
                      const Filter& filter, bool full_path) {
    std::error_code ec;
    auto options = fs::directory_options::skip_permission_denied;

    for (auto it = DirectoryIterator(path, options, ec); it != DirectoryIterator();
         it.increment(ec)) {

        const auto& entry = *it;
        // Если при создании итератора произошла ошибка, выходим.
        if (ec) {
            // Ошибки доступа уже обработаны опцией skip_permission_denied,
            // здесь могут быть другие, более серьезные ошибки.
            std::cerr << "Ошибка при доступе к директории " << path << ": " << ec.message()
                      << std::endl;
            return;
        }

        // Проверяем, является ли объект обычным файлом
        if (!fs::is_regular_file(entry.path(), ec) or ec) {
            continue;  // Пропускаем директории, ссылки и файлы с ошибками доступа
        }

        std::string ext = entry.path().extension().string();

        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (auto check_result = filter.Check(ext)) {
            std::string entry_path_str =
                full_path ? entry.path().string() : entry.path().lexically_relative(path).string();

            (*result)[*check_result].push_back(std::move(entry_path_str));
        }
    }

    if (ec) {
        std::cerr << "Произошла ошибка во время итерации по " << path << ": " << ec.message()
                  << std::endl;
    }
}

std::unordered_map<std::string, std::vector<std::string>> ListFiles(const fs::path& path,
                                                                    const Filter& filter,
                                                                    bool recursive = true,
                                                                    bool full_path = false) {
    std::unordered_map<std::string, std::vector<std::string>> result;
    if (!fs::exists(path)) {
        // Эта проверка все еще полезна для более ясного сообщения об ошибке
        std::cout << "Путь не существует: " << path << std::endl;
        return result;
    }

    if (recursive) {
        ProcessDirectory<fs::recursive_directory_iterator>(path, &result, filter, full_path);
    } else {
        ProcessDirectory<fs::directory_iterator>(path, &result, filter, full_path);
    }

    return result;
}

std::string GetHomeDirectory() {
    // Пытаемся получить из переменной окружения
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home);
    }

    // Если не получилось, используем getpwuid
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir) {
        return std::string(pw->pw_dir);
    }

    // Если ничего не сработало
    return "";
}

struct AppOptions {
    bool recursive = true;
    bool full_path = false;
    int interval_seconds = 5;
    int exit_code = 0; // 0 - ok, 1 - error, -1 - help printed
};

AppOptions GetArguments(int argc, char* argv[]) {
    // Значения по умолчанию
    bool recursive = true;   // ПО УМОЛЧАНИЮ: рекурсивный обход ВКЛЮЧЕН
    bool full_path = false;  // ПО УМОЛЧАНИЮ: полные пути ВЫКЛЮЧЕНЫ

    int interval_seconds = 5;

    // Парсим аргументы командной строки
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-t" or arg == "--time") {
            if (i + 1 < argc) {
                try {
                    interval_seconds = std::stoi(argv[++i]);

                    // Единственное разумное ограничение - положительное число
                    if (interval_seconds <= 0) {
                        std::cerr << "Ошибка: интервал должен быть положительным числом"
                                  << std::endl;
                        return {false, false, -1, 1};
                    }

                } catch (const std::exception& e) {
                    std::cerr << "Ошибка: неверный формат интервала: " << argv[i] << std::endl;
                    return {false, false, -1, 1};
                }
            } else {
                std::cerr << "Ошибка: после -t нужно указать интервал в секундах" << std::endl;
                return {false, false, 5, 1};
            }
        } else if (arg == "-r" or arg == "--recursive") {
            recursive = true;
        } else if (arg == "-R" or arg == "--no-recursive") {
            recursive = false;
        } else if (arg == "-f" or arg == "--full-path") {
            full_path = true;
        } else if (arg == "-F" or arg == "--relative-path") {
            full_path = false;
        } else if (arg == "-h" or arg == "--help") {
            std::cout
                << "Использование програмы: " << " [опции] [путь]\n\n"
                << "Опции:\n"
                << "  -r, --recursive         Рекурсивный обход поддиректорий (по умолчанию)\n"
                << "  -R, --no-recursive      Явно отключить рекурсивный обход (только домашний "
                   "каталог \n"
                << "  -f, --full-path         Показывать полные пути (по умолчанию)\n"
                << "  -F, --relative-path     Показывать относительные пути\n"
                << "  -t, --time              Временной интервал между запусками\n"
                << "  -h, --help              Показать эту справку\n";
            return {false, false, -1, -1};
        } else {
            std::cerr << "Неизвестный флаг: " << arg << std::endl;
            std::cerr << "Используйте -h для справки" << std::endl;
            return {false, false, -1, 1};
        }
    }

    return {recursive, full_path, interval_seconds, 0};
}