# Минимальный образ для сборки C++20 проекта
FROM ubuntu:24.04

# Только самое необходимое для сборки
RUN apt-get update && apt-get install -y \
    g++-13 \
    cmake \
    make \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Настройка компилятора по умолчанию
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100

WORKDIR /workspace

CMD ["/bin/bash"]