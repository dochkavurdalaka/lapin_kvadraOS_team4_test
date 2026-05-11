cmake_minimum_required(VERSION 3.15)
project(lapin_kvadraOS_team4_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем папку include для всех таргетов
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Определяем список программ
set(PROGRAMS scaner scaner_client scaner_server)

# Функция для настройки санитайзеров
function(setup_sanitizers target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Asan>:-g>
            $<$<CONFIG:Asan>:-fsanitize=address>
            $<$<CONFIG:Asan>:-fsanitize=undefined>
            $<$<CONFIG:Asan>:-fno-omit-frame-pointer>
            $<$<CONFIG:Asan>:-O2>
        )
        
        target_link_options(${target} PRIVATE
            $<$<CONFIG:Asan>:-fsanitize=address>
            $<$<CONFIG:Asan>:-fsanitize=undefined>
        )
    endif()
endfunction()

# Создаем каждую программу
foreach(program IN LISTS PROGRAMS)
    # Определяем путь к исходному файлу
    set(SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/${program}.cpp")
    
    # Проверяем наличие файла
    if(NOT EXISTS ${SOURCE_FILE})
        message(WARNING "Source file for ${program} not found: ${SOURCE_FILE}")
        continue()  # Пропускаем эту программу
    endif()
    
    add_executable(${program} ${SOURCE_FILE})
    
    # Общие флаги компиляции
    target_compile_options(${program} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
    
    # Флаги для Debug конфигурации
    target_compile_options(${program} PRIVATE
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-DDEBUG>
    )
    
    # Флаги для Release конфигурации
    target_compile_options(${program} PRIVATE
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Release>:-march=native>
    )
    
    # Настройка санитайзеров (вызываем функцию)
    setup_sanitizers(${program})
    
    # Организация выходных файлов
    set_target_properties(${program} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    )
    
    message(STATUS "Added target: ${program}")
endforeach()