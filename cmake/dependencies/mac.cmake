include(FetchContent)

#=================== spdlog ===================
# macports has issues with this because of fmt
# brew doesn't support building multiarch
find_package(spdlog QUIET)
if (NOT ${spdlog_FOUND})
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(spdlog)
endif()

#=================== Metal-cpp ===================
FetchContent_Declare(
    metalcpp
    GIT_REPOSITORY https://github.com/briaguya-ai/single-header-metal-cpp.git
    GIT_TAG macOS13_iOS16
)
FetchContent_MakeAvailable(metalcpp)
list(APPEND ADDITIONAL_LIB_INCLUDES ${metalcpp_SOURCE_DIR})

#=================== ImGui ===================
target_sources(ImGui
    PRIVATE
    ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
)

target_include_directories(ImGui PRIVATE ${metalcpp_SOURCE_DIR})
target_compile_definitions(ImGui PUBLIC IMGUI_IMPL_METAL_CPP)

find_package(SDL2 REQUIRED)
target_link_libraries(ImGui PUBLIC SDL2::SDL2)

find_package(GLEW REQUIRED)
target_link_libraries(ImGui PUBLIC ${OPENGL_opengl_LIBRARY} GLEW::GLEW)
set_target_properties(ImGui PROPERTIES
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
)

#=================== OpenAL Soft ===================
# Use OpenAL Soft from parent directory for consistent API (AL/al.h instead of OpenAL/al.h)
set(OPENAL_SOFT_DIR "${CMAKE_SOURCE_DIR}/../openal-soft")
if(EXISTS "${OPENAL_SOFT_DIR}")
    set(OPENAL_INCLUDE_DIR "${OPENAL_SOFT_DIR}/include")
    # Check if openal-soft has been built
    if(EXISTS "${OPENAL_SOFT_DIR}/build/libopenal.dylib")
        set(OPENAL_LIBRARY "${OPENAL_SOFT_DIR}/build/libopenal.dylib")
    elseif(EXISTS "${OPENAL_SOFT_DIR}/build/libopenal.a")
        set(OPENAL_LIBRARY "${OPENAL_SOFT_DIR}/build/libopenal.a")
    else()
        message(WARNING "OpenAL Soft found but not built. Please build openal-soft first.")
        # Fall back to system OpenAL
        find_package(OpenAL REQUIRED)
    endif()
else()
    # Fall back to system OpenAL (uses OpenAL/al.h on macOS)
    find_package(OpenAL REQUIRED)
endif()
