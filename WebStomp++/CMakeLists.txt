add_compile_options(-Wall -Wextra -pedantic -Werror)

# 头文件查找目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# 库文件查找目录
link_directories(${PROJECT_SOURCE_DIR}/lib)

SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output)

set(SRC_LIST
        ./WebStompClient.cpp
        ./WebStompType.cpp
         )

add_library(webstomp++ SHARED ${SRC_LIST})