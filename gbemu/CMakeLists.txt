set(MAIN_SOURCES
  main.c
)

file(GLOB HEADERS "${PROJECT_SOURCE_DIR}/include/*.h")

add_executable(gbemu ${HEADERS} ${MAIN_SOURCES})
target_link_libraries(gbemu emu)
target_include_directories(gbemu PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Копирование шрифта для всех платформ
add_custom_command(
  TARGET gbemu POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${PROJECT_SOURCE_DIR}/NotoSansMono-Medium.ttf
  $<TARGET_FILE_DIR:gbemu>
  VERBATIM)

if(WIN32)
  set(ARCH_DIR x64)
  if(${CMAKE_SIZEOF_VOID_P} MATCHES 4)
    set(ARCH_DIR x86)
  endif()

  # Копирование DLL для Windows
  foreach(DLL SDL2 SDL2_ttf zlib1 libfreetype-6)
    add_custom_command(
      TARGET gbemu POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      ${PROJECT_SOURCE_DIR}/../windows_deps/${DLL}/lib/${ARCH_DIR}/${DLL}.dll
      $<TARGET_FILE_DIR:gbemu>
      VERBATIM)
  endforeach()
elseif(APPLE)
  # Копирование dylib для macOS
  foreach(DYLIB SDL2 SDL2_ttf)
    add_custom_command(
      TARGET gbemu POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      /opt/homebrew/opt/${DYLIB}/lib/lib${DYLIB}.dylib
      $<TARGET_FILE_DIR:gbemu>
      VERBATIM)
  endforeach()
endif()

install(TARGETS gbemu
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
