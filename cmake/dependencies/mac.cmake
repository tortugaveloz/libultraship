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
# Use pre-built OpenAL Soft from submodule for consistent API (AL/al.h instead of OpenAL/al.h)
# Note: OpenAL Soft should be built separately to avoid header conflicts with libultraship
set(OPENAL_SOFT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/openal-soft")
set(OPENAL_SOFT_BUILD_DIR "${OPENAL_SOFT_DIR}/build")
if(EXISTS "${OPENAL_SOFT_DIR}/include/AL/al.h")
    set(OPENAL_INCLUDE_DIR "${OPENAL_SOFT_DIR}/include")
    # Check for pre-built library
    if(EXISTS "${OPENAL_SOFT_BUILD_DIR}/libopenal.a")
        set(OPENAL_LIBRARY "${OPENAL_SOFT_BUILD_DIR}/libopenal.a")
        message(STATUS "Using pre-built OpenAL Soft from submodule: ${OPENAL_LIBRARY}")
    elseif(EXISTS "${OPENAL_SOFT_BUILD_DIR}/libopenal.dylib")
        set(OPENAL_LIBRARY "${OPENAL_SOFT_BUILD_DIR}/libopenal.dylib")
        message(STATUS "Using pre-built OpenAL Soft from submodule: ${OPENAL_LIBRARY}")
    else()
        message(WARNING "OpenAL Soft submodule found but not built. Please run:")
        message(WARNING "  cd ${OPENAL_SOFT_DIR} && mkdir -p build && cd build && cmake -DLIBTYPE=STATIC .. && make")
        # Fall back to system OpenAL
        find_package(OpenAL REQUIRED)
    endif()
else()
    message(WARNING "OpenAL Soft submodule not found. Using system OpenAL.")
    # Fall back to system OpenAL (uses OpenAL/al.h on macOS)
    find_package(OpenAL REQUIRED)
endif()
