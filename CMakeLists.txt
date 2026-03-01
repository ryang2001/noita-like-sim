cmake_minimum_required(VERSION 3.15)
project(NoitaLike_Demo VERSION 2.0.0 LANGUAGES C CXX)

# C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
if(MSVC)
    add_compile_options(/W3 /WX- /MP /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    # Release模式下启用优化，Debug模式不使用/O2以避免与/RTC1冲突
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    add_compile_options(-Wall -Wextra -O3)
endif()

# 查找依赖
find_package(Threads REQUIRED)

# SDL2配置
if(WIN32)
    set(SDL2_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2-2.30.1/include)
    set(SDL2_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2-2.30.1/lib/x64/SDL2.lib)
    set(SDL2MAIN_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/external/SDL2-2.30.1/lib/x64/SDL2main.lib)
    
    if(EXISTS ${SDL2_INCLUDE_DIR}/SDL.h)
        include_directories(${SDL2_INCLUDE_DIR})
        set(SDL2_LIBRARIES ${SDL2_LIBRARY} ${SDL2MAIN_LIBRARY})
        message(STATUS "Found SDL2: ${SDL2_LIBRARY}")
    else()
        message(FATAL_ERROR "SDL2 not found at ${SDL2_INCLUDE_DIR}")
    endif()
else()
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    set(SDL2_LIBRARIES ${SDL2_LIBRARIES})
endif()

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 源文件
set(CORE_SOURCES
    src/core/Chunk.cpp
    src/core/World.cpp
)

set(MATERIAL_SOURCES
    src/materials/MaterialRegistry.cpp
)

set(THREADING_SOURCES
    src/threading/WorkStealingPool.cpp
)

set(ALL_SOURCES
    ${CORE_SOURCES}
    ${MATERIAL_SOURCES}
    ${THREADING_SOURCES}
)

# Demo可执行文件
add_executable(demo
    src/demo_simple.cpp
    ${ALL_SOURCES}
)

# 链接库
set(LINK_LIBS Threads::Threads ${SDL2_LIBRARIES})

if(WIN32)
    list(APPEND LINK_LIBS opengl32 gdi32 winmm)
endif()

target_link_libraries(demo PRIVATE ${LINK_LIBS})

# Windows特定定义
if(WIN32)
    target_compile_definitions(demo PRIVATE SDL_MAIN_HANDLED)
endif()

# 配置文件复制 - 复制到所有可能的输出目录
foreach(CONFIG_TYPE Debug Release RelWithDebInfo MinSizeRel)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/materials/basic_materials.json
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_TYPE}/config/materials)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/reactions/basic_reactions.json
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_TYPE}/config/reactions)
endforeach()

# 同时复制到根构建目录（用于单配置生成器）
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/materials/basic_materials.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config/materials)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/reactions/basic_reactions.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/config/reactions)

# 安装
install(TARGETS demo RUNTIME DESTINATION bin)
install(DIRECTORY config/ DESTINATION bin/config)

# 打印信息
message(STATUS "")
message(STATUS "=== Noita-like Demo ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "SDL2: ${SDL2_LIBRARIES}")
message(STATUS "======================")
message(STATUS "")
