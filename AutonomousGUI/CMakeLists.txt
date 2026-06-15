cmake_minimum_required(VERSION 3.16)
project(AutonomousGUI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. BẮT BUỘC: Bật tính năng tự động xử lý Q_OBJECT, UI và Resource của Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 2. TÌM THƯ VIỆN QT6 (Chỉ giữ lại các module cốt lõi cho giao diện)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort)

# 3. KHAI BÁO CÁC FILE NGUỒN CỦA DỰ ÁN
add_executable(AutonomousGUI
    main.cpp
    MainWindow.cpp
    MainWindow.h
    resources.qrc
)

# 4. LIÊN KẾT CÁC FILE THƯ VIỆN QT6
target_link_libraries(AutonomousGUI PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
)