// client.cpp
#include <iostream>
#include "httplib.h"

int main() {
    // Создаём клиента для localhost на порту 1234
    httplib::Client cli("localhost", 1234);

    // 1. Простой GET-запрос на "/"
    std::cout << "=== Запрос на /media_files ===" << std::endl;
    auto res = cli.Get("/media_files");
    if (res && res->status == 200) {
        std::cout << res->body << std::endl;
    } else {
        std::cout << "Ошибка!" << std::endl;
    }

    return 0;
}