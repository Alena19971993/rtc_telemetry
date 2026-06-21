1. Версия языка и зависимости.
    C++17
    CMake 3.16+

    [libdatachannel](https://github.com/paullouisageneau/libdatachannel) Предварительно собранный .so-файл
        в depend/libdatachannel/ собран с GCC 11 (gcc-ar-11, gcc-ranlib-11)
    [nlohmann/json](https://github.com/nlohmann/json) Только заголовочный файл, включен в состав libdatachannel

2. Сборка и запуск проекта

    cmake -B build
    cmake --build build

# Terminal 1
    ./build/receiver

# Terminal 2
    ./build/sender

3. Способ signaling
    обмен SDP через файл

4. Ограничения текущего решения.
    1) работает только на одной машине, нет сетевого signaling сервера; 
    2) нет поддержки нескольких соединений;
    3) нет переподключения; 
    4) Нет таймаута в stop() (не стала разъединять насильно)
