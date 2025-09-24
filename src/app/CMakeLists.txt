# 실행 파일 생성
add_executable(QtKohzuController)

# 애플리케이션 소스 파일들을 자동으로 찾습니다.
file(GLOB_RECURSE APP_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/**/*.cpp"
)
# Qt Designer UI 파일도 자동으로 찾습니다.
file(GLOB APP_UIS
    "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow/*.ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/axiscontrollwidget/*.ui"
)

target_sources(QtKohzuController PRIVATE ${APP_SRCS} ${APP_UIS})

# 헤더 파일 경로 지정
target_include_directories(QtKohzuController PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mainwindow
    ${CMAKE_CURRENT_SOURCE_DIR}/axiscontrollwidget
)

# 라이브러리 연결
target_link_libraries(QtKohzuController
    PRIVATE
        # 1. 우리가 만든 제어 라이브러리를 연결합니다.
        # qt-kohzu-manager 라이브러리가 kohzu-controller를 포함하고 있으므로,
        # 여기에 kohzu-controller를 따로 명시할 필요 없음
        qt-kohzu-manager

        # 2. Qt 라이브러리를 연결합니다.
        Qt6::Widgets
)
