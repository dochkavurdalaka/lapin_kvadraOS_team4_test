#include "scaner.h"
#include "json.hpp"
#include "httplib.h"
#include "linux_timer.h"

using json = nlohmann::json;


std::string MainTask(const std::string& home_dir, const Filter& filter, bool recursive,
                     bool full_path) {
    std::cout << "Сканирование начато:" << home_dir << std::endl;
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

    return res.dump(4);
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

    httplib::Server svr;
    std::string scan_result;
    std::mutex scan_mutex;
    // Обработчик GET-запроса на путь "/media_files"
    svr.Get("/media_files",
            [&scan_mutex, &scan_result](const httplib::Request&, httplib::Response& res) {
                std::unique_lock lock(scan_mutex);
                auto result = scan_result;
                lock.unlock();
                res.set_content(result, "application/json");
            });

    // Запускаем сервер в отдельном потоке
    std::jthread server_thread([&svr]() { svr.listen("localhost", 1234); });

    auto func = [&]() {
        auto result = MainTask(home_dir, filter, recursive, full_path);
        std::lock_guard guard{scan_mutex};
        scan_result = std::move(result);
    };
    code = TimerStart(func, time_interval);

    svr.stop();

    if (code == 0) {
        std::cout << "Приложение остановлено" << std::endl;
    }
    return code;
}
