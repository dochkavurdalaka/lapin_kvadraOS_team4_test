#pragma once

#include <atomic>
#include <iostream>
#include <format>
#include <csignal>
#include <system_error>
#include <sys/timerfd.h>
#include <unistd.h>
#include <poll.h>

std::atomic<bool> running{true};

void SignalHandler(int) {
    running.store(false, std::memory_order_release);
}

template <class Functor>
int TimerStart(Functor& functor, int delay = 5) {
    // Установка обработчиков сигналов для graceful shutdown через sigaction
    struct sigaction sa{};
    // Очищаем маску сигналов
    sigemptyset(&sa.sa_mask);
    // Назначаем функцию-обработчик
    sa.sa_handler = SignalHandler;
    // Устанавливаем флаги:
    // SA_RESTART - перезапускать системные вызовы после сигнала
    // Это важно для poll, read и других вызовов
    sa.sa_flags = SA_RESTART;
    // Регистрируем обработчик для SIGINT (Ctrl+C)
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        std::cerr << "Ошибка регистрации обработчика SIGINT: " << strerror(errno) << std::endl;
        return 1;
    }
    // Регистрируем обработчик для SIGTERM (завершение процесса)
    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        std::cerr << "Ошибка регистрации обработчика SIGTERM: " << strerror(errno) << std::endl;
        return 1;
    }

    // Регистрируем обработчик для SIGINT (Ctrl+Z)
    if (sigaction(SIGTSTP, &sa, nullptr) == -1) {
        std::cerr << "Ошибка регистрации обработчика SIGTSTP: " << strerror(errno) << std::endl;
        return 1;
    }

    // Создаем таймер
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1) {
        std::cerr << "Ошибка создания таймера: " << strerror(errno) << std::endl;
        return 1;
    }

    // Настройка интервала (5 секунд)
    struct itimerspec interval;
    interval.it_value.tv_sec = 2;  // Первый запуск через 2 секунды
    interval.it_value.tv_nsec = 0;
    interval.it_interval.tv_sec = delay;  // Затем каждые delay секунд
    interval.it_interval.tv_nsec = 0;

    if (timerfd_settime(timer_fd, 0, &interval, nullptr) == -1) {
        std::cerr << "Ошибка настройки таймера: " << strerror(errno) << std::endl;
        close(timer_fd);
        return 1;
    }

    std::cout << std::format("Таймер запущен. Интервал: {} сек", delay) << std::endl;
    std::cout << "Нажмите Ctrl+C для остановки" << std::endl;

    // Используем poll для ожидания (можно заменить на epoll для многих таймеров)
    struct pollfd fds;
    fds.fd = timer_fd;
    fds.events = POLLIN;

    uint64_t expirations;

    while (running.load(std::memory_order_acquire)) {
        int ret = poll(&fds, 1, 1000);  // Таймаут 1 секунда для проверки running

        if (ret == -1) {
            if (errno == EINTR)
                continue;  // Прервано сигналом
            std::cerr << "Ошибка poll: " << strerror(errno) << std::endl;
            break;
        }

        if (ret > 0 and (fds.revents & POLLIN)) {
            // Читаем количество срабатываний таймера
            ssize_t s = read(timer_fd, &expirations, sizeof(expirations));
            if (s != sizeof(expirations)) {
                std::cerr << "Ошибка чтения timerfd" << std::endl;
                break;
            }

            // Выполняем процедуру нужное количество раз
            for (uint64_t i = 0; i < expirations; ++i) {
                if (!running)
                    break;
                functor();
            }
        }
    }

    close(timer_fd);
    return 0;
}
