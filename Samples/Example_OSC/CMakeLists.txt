cmake_minimum_required(VERSION 3.19)
project(Example_OSC)

# OSC Music Visualization Example

set(INSTALLED_ENGINE OFF)

if (${INSTALLED_ENGINE})
    find_package(WickedEngine REQUIRED)
endif()

set(SOURCE_FILES
    main.cpp
)

set(LIB_DXCOMPILER "libdxcompiler.so")

add_executable(Example_OSC ${SOURCE_FILES})

target_link_libraries(Example_OSC PUBLIC
    $<$<NOT:$<BOOL:${INSTALLED_ENGINE}>>:WickedEngine>
    $<$<BOOL:${INSTALLED_ENGINE}>:WickedEngine::WickedEngine>
)

if(WICKED_USE_IPO)
set_target_properties(Example_OSC PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION ON
    INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
)
endif()

if (${INSTALLED_ENGINE})
    get_property(LIBDXCOMPILER_PATH
        TARGET WickedEngine::dxcompiler
        PROPERTY IMPORTED_LOCATION)
    get_filename_component(WICKED_LIBFOLDER ${LIBDXCOMPILER_PATH} DIRECTORY)
else()
    set(LIBDXCOMPILER_PATH "${WICKED_ROOT_DIR}/WickedEngine/${LIB_DXCOMPILER}")
endif()
message("libdxcompiler found at ${LIBDXCOMPILER_PATH}")

add_custom_command(
    TARGET Example_OSC POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBDXCOMPILER_PATH} ${CMAKE_CURRENT_BINARY_DIR}
)

# Create symlink to Content directory
add_custom_command(
    TARGET Example_OSC POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E ${COPY_OR_SYMLINK_DIR_CMD} ${WICKED_ROOT_DIR}/Content ${CMAKE_CURRENT_BINARY_DIR}/Content
    COMMENT "Copying/symlinking Content directory..."
)
