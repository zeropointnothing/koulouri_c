add_library(koulouri_shared STATIC alsasilencer.cpp)

# not all systems use ALSA, we should account for this!
find_library(ALSA_LIB NAMES asound)

if(ALSA_LIB)
    message(STATUS "Found ALSA (should be silenced!): ${ALSA_LIB}")
    target_link_libraries(koulouri_shared PRIVATE ${ALSA_LIB})
    target_compile_definitions(koulouri_shared PRIVATE HAS_ALSA=1)
else()
    message(STATUS "System does not support ALSA code!")
endif()
# ...
target_include_directories(libkoulouri PUBLIC "${CMAKE_SOURCE_DIR}/koulouri_shared/..")
