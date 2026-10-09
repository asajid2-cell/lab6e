include_guard(GLOBAL)

set(SFML_ROOT "${CMAKE_CURRENT_LIST_DIR}/../extern/sfml-3.1.0-linux-x86_64-ubuntu20.04")

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
    message(FATAL_ERROR "Bundled SFML is meant for the lab machines and it requires Linux x86-64 (Ubuntu 20.04); detected ${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}. Use CMake 3.28 that provides FetchContent on other platforms.")
endif()

if(NOT EXISTS "${SFML_ROOT}/include/SFML/Config.hpp")
    message(FATAL_ERROR "Bundled SFML headers not found: ${SFML_ROOT}/include/SFML/Config.hpp. Copy the complete bundle, keeping cmake/ and extern/ as siblings.")
endif()

foreach(_sfml_component System Window Graphics Audio Network)
    string(TOLOWER "${_sfml_component}" _sfml_library)
    set(_sfml_path "${SFML_ROOT}/lib/libsfml-${_sfml_library}.so.3.1")
    if(NOT EXISTS "${_sfml_path}")
        message(FATAL_ERROR "Bundled SFML library not found: ${_sfml_path}. Copy the complete SFML bundle.")
    endif()
    add_library(SFML::${_sfml_component} SHARED IMPORTED GLOBAL)
    set_target_properties(SFML::${_sfml_component} PROPERTIES
        IMPORTED_LOCATION "${_sfml_path}"
        INTERFACE_INCLUDE_DIRECTORIES "${SFML_ROOT}/include")
endforeach()

target_link_libraries(SFML::Window INTERFACE SFML::System)
target_link_libraries(SFML::Graphics INTERFACE SFML::Window)
target_link_libraries(SFML::Audio INTERFACE SFML::System)
target_link_libraries(SFML::Network INTERFACE SFML::System)
