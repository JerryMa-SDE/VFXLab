cmake_minimum_required(VERSION 3.10)
project(VFXLab)

option(RADCPP_PATH "Path to radcpp repo." "radcpp/")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(VFXLAB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(VFXLAB_SOURCE_FILES
    Source/main.cpp
    Source/VFXLab.h
    Source/VFXLab.cpp
    Source/Logging.h
    Source/Logging.cpp
    Source/Effect.h
    Source/Effect.cpp
    Source/EffectFactory.h
    Source/EffectFactory.cpp
    Source/JsonDeserialize.h
    Source/JsonDeserialize.cpp
    Source/Shadertoy/Shadertoy.h
    Source/Shadertoy/Shadertoy.cpp
    Source/Shadertoy/ShadertoyLoad.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VFXLAB_SOURCE_FILES})

if (EXISTS ${RADCPP_PATH})
    add_subdirectory(${RADCPP_PATH} radcpp/)
endif()

add_executable(VFXLab ${VFXLAB_SOURCE_FILES})

set_target_properties(VFXLab PROPERTIES
    VS_DPI_AWARE "PerMonitor"
)

target_include_directories(VFXLab
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/
    PRIVATE ${RADCPP_PATH}
    PRIVATE ${RADCPP_PATH}/imported/Compressonator/include/
)

target_link_libraries(VFXLab
    PRIVATE rad
    PRIVATE stb
    PRIVATE DirectMedia
    PRIVATE VulkanEngine
)

find_package(SDL2 CONFIG REQUIRED)
target_link_libraries(VFXLab
    PUBLIC
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

add_custom_command(TARGET VFXLab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copy resources ..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${RADCPP_PATH}/libs/VulkanEngine/rad/VulkanEngine/Shaders/
        ${CMAKE_CURRENT_BINARY_DIR}/Shaders/
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Shaders/
        ${CMAKE_CURRENT_BINARY_DIR}/Shaders/
)

if(WIN32)

target_link_libraries(VFXLab
    PRIVATE ${RADCPP_PATH}/imported/Compressonator/lib/VS2019/x64/CMP_Core_MT_DLL.lib
    PRIVATE ${RADCPP_PATH}/imported/Compressonator/lib/VS2019/x64/CMP_Framework_MT_DLL.lib
    PRIVATE ${RADCPP_PATH}/imported/Compressonator/lib/VS2019/x64/Compressonator_MT_DLL.lib
)

add_custom_command(TARGET VFXLab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Copy dlls ..."
    COMMAND ${CMAKE_COMMAND} -E copy
        ${RADCPP_PATH}/imported/spvgen/build/Release/spvgen.dll
        ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${RADCPP_PATH}/imported/Compressonator/lib/VS2019/x64/CMP_Core_MT_DLL.dll
        ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${RADCPP_PATH}/imported/Compressonator/lib/VS2019/x64/CMP_Framework_MT_DLL.dll
        ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
        ${RADCPP_PATH}/imported/Compressonator/lib/VS2019/x64/Compressonator_MT_DLL.dll
        ${CMAKE_CURRENT_BINARY_DIR}
)

endif()
