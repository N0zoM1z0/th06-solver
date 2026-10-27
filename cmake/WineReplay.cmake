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
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
    ${_wine_src}/observer.cpp -ladvapi32 -Wl,--kill-at -o ${_wine_out}/observer.new.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${_wine_out}/observer.new.dll ${_wine_out}/observer.dll
  DEPENDS tools/wine-replay/observer.cpp tools/wine-replay/identity.hpp
    tools/wine-replay/render_timing.hpp tools/wine-replay/mapped_journal.hpp include/th06/render_timing.hpp include/th06/retail_clock.hpp
    tools/wine-replay/quad_batch.hpp include/th06/retail_quads.hpp
    ${_wine_out}/retail_abi.hpp VERBATIM)
add_custom_command(OUTPUT ${_wine_out}/d3d8_timing_abi.o
  COMMAND ${TH06_MINGW_CXX} -std=c++17 -c
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/wine-replay/d3d8_timing_abi.cpp
    -o ${_wine_out}/d3d8_timing_abi.o
  DEPENDS tools/wine-replay/d3d8_timing_abi.cpp VERBATIM)
add_custom_target(th06_wine_replay ALL DEPENDS ${_wine_out}/launch.exe ${_wine_out}/observer.dll ${_wine_out}/d3d8_timing_abi.o)
add_custom_command(OUTPUT ${_wine_out}/shadow.dll
  COMMAND ${TH06_MINGW_CXX} -std=c++17 -O2 -static -shared -I${_wine_out}
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/wine-shadow/observer.cpp
    -ladvapi32 -Wl,--kill-at -o ${_wine_out}/shadow.new.dll
  COMMAND ${CMAKE_COMMAND} -E rename ${_wine_out}/shadow.new.dll ${_wine_out}/shadow.dll
  DEPENDS tools/wine-shadow/observer.cpp tools/wine-replay/identity.hpp
    tools/wine-shadow/limiter_scope.hpp include/th06/retail_clock.hpp
    include/th06/shadow_bridge.hpp ${_wine_out}/retail_abi.hpp VERBATIM)
add_custom_target(th06_wine_shadow ALL DEPENDS ${_wine_out}/shadow.dll)
