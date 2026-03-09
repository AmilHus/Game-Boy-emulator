file(GLOB sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/lib/*.c")
file(GLOB headers CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.h")

add_library(emu STATIC ${sources} ${headers})

target_include_directories(emu PUBLIC ${PROJECT_SOURCE_DIR}/include )

if(APPLE)
    target_include_directories(emu PUBLIC
        /opt/homebrew/opt/sdl2/include
        /opt/homebrew/opt/sdl2_ttf/include
    )
    target_link_libraries(emu PUBLIC
        /opt/homebrew/opt/sdl2/lib/libSDL2.dylib
        /opt/homebrew/opt/sdl2_ttf/lib/libSDL2_ttf.dylib
    )
endif()

