#include "scaner.h"

#include "linux_timer.h"
#include "json.hpp"

using json = nlohmann::json;

void MainTask(const std::string& home_dir, const Filter& filter, bool recursive, bool full_path) {
    std::cout << "Сканирование начато: " << home_dir << std::endl;
    // std::cout << "----------------------------------------" << std::endl;
    std::unordered_map<std::string, std::vector<std::string>> result =
        ListFiles(home_dir, filter, recursive, full_path);
    std::cout << "Сканирование завершено\n";
    if (not result.contains("video")) {
        result["video"] = {};
    }
    if (not result.contains("audio")) {
        result["audio"] = {};
    }
    if (not result.contains("images")) {
        result["images"] = {};
    }
    json res = result;

    std::string media_file_path = home_dir + "/.media_files";
    std::ofstream media_file(media_file_path);

    if (!media_file.is_open()) {
        std::cout << "Ошибка: не удалось открыть файл " << media_file_path << std::endl;
        return;
    }
    media_file << res.dump();
    media_file.close();
    std::cout << "JSON успешно записан в " << media_file_path << std::endl;
}

int main(int argc, char* argv[]) {
    auto [recursive, full_path, time_interval, code] = GetArguments(argc, argv);
    if (code == -1) {
        return 0;
    } else if (code == 1) {
        return 1;
    }

    // Получаем домашний каталог
    std::string home_dir = GetHomeDirectory();
    if (home_dir.empty()) {
        std::cerr << "Не удалось получить домашний каталог" << std::endl;
        return 1;
    }

    Filter filter;

    auto func = [&]() { MainTask(home_dir, filter, recursive, full_path); };

    code = TimerStart(func, time_interval);

    if (code == 0) {
        std::cout << "Приложение остановлено" << std::endl;
    }
    return code;
}