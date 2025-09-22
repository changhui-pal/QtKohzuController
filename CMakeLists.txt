# CMake 버전과 프로젝트 이름 설정
cmake_minimum_required(VERSION 3.19)
project(QtKohzuController VERSION 1.0.0 LANGUAGES CXX)

# Vcpkg 툴체인 파일 설정 (기존과 동일)
set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# 모든 하위 프로젝트에서 사용할 공통 패키지를 여기서 찾습니다.
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
find_package(Boost REQUIRED COMPONENTS asio)
find_package(spdlog REQUIRED)

# Qt 빌드 도구 활성화 (전역 설정)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 제어 라이브러리 빌드를 위해 서브디렉토리 추가
add_subdirectory(src/lib/kohzu-controller)

# 2. Qt 전용 비즈니스 로직 라이브러리 빌드를 위해 서브디렉토리 추가
add_subdirectory(src/lib/qt-kohzu-manager)

# 3. GUI 애플리케이션 빌드를 위해 서브디렉토리 추가
add_subdirectory(src/app)


