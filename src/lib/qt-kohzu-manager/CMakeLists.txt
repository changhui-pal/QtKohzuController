# QtKohzuManager 라이브러리 생성
add_library(qt-kohzu-manager STATIC)

file(GLOB_RECURSE MANAGER_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

target_sources(qt-kohzu-manager PRIVATE ${MANAGER_SRCS})

# 공개 헤더 파일 경로 지정
target_include_directories(qt-kohzu-manager
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

# 이 라이브러리가 필요로 하는 다른 라이브러리들을 연결
target_link_libraries(qt-kohzu-manager
    PUBLIC
        Qt6::Core
        kohzu-controller
)
