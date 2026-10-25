# Build the optional pinned-retail observer separately from the Linux runtime.
find_program(TH06_MINGW_CXX i686-w64-mingw32-g++)
if(NOT TH06_MINGW_CXX)
  message(FATAL_ERROR "TH06_WINE_REPLAY requires i686-w64-mingw32-g++")
endif()
add_executable(th06_retail_abi tools/wine-replay/generate_abi.cpp)
target_link_libraries(th06_retail_abi PRIVATE th06_native)
# The pinned legacy classes intentionally use the compiler's i386 offsetof
# extension; their original size/alignment assertions remain active.
target_compile_options(th06_retail_abi PRIVATE -Wno-invalid-offsetof)
set(_wine_src ${CMAKE_CURRENT_SOURCE_DIR}/tools/wine-replay)
set(_wine_out ${CMAKE_CURRENT_BINARY_DIR}/wine-replay)
add_custom_command(OUTPUT ${_wine_out}/retail_abi.hpp
  COMMAND ${CMAKE_COMMAND} -E make_directory ${_wine_out}
  COMMAND th06_retail_abi ${_wine_out}/retail_abi.hpp
  DEPENDS th06_retail_abi VERBATIM)
add_custom_command(OUTPUT ${_wine_out}/launch.exe
  COMMAND ${TH06_MINGW_CXX} -std=c++17 -O2 -static ${_wine_src}/launch.cpp
    -ladvapi32 -o ${_wine_out}/launch.new.exe
  COMMAND ${CMAKE_COMMAND} -E rename ${_wine_out}/launch.new.exe ${_wine_out}/launch.exe
  DEPENDS tools/wine-replay/launch.cpp tools/wine-replay/identity.hpp
    ${_wine_out}/retail_abi.hpp VERBATIM)
add_custom_command(OUTPUT ${_wine_out}/observer.dll
  COMMAND ${TH06_MINGW_CXX} -std=c++17 -O2 -static -shared -I${_wine_out}
    ${_wine_src}/observer.cpp -ladvapi32 -Wl,--kill-at -o ${_wine_out}/observer.new.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${_wine_out}/observer.new.dll ${_wine_out}/observer.dll
  DEPENDS tools/wine-replay/observer.cpp tools/wine-replay/identity.hpp
    ${_wine_out}/retail_abi.hpp VERBATIM)
add_custom_target(th06_wine_replay ALL DEPENDS ${_wine_out}/launch.exe ${_wine_out}/observer.dll)
