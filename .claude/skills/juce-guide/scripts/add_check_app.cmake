# =============================================================================
#  juce_add_check_app — build a throwaway console app against a JUCE module
#  ---------------------------------------------------------------------------
#  A JUCE module's logic headers can usually be exercised without a plugin, a
#  host or a window: they are headers, and a console app can link them. What
#  stops people doing it is that setting up the CMake takes twenty minutes and
#  the result is thrown away, so the check gets skipped instead.
#
#  This is that twenty minutes, once.
#
#      include(/path/to/add_check_app.cmake)
#
#      juce_add_check_app(
#          TARGET   MyCheck
#          SOURCES  MyCheck.cpp
#          MODULES  mycompany::my_module
#          LINK     juce::juce_audio_basics)
#
#  The binary lands at ${CMAKE_BINARY_DIR}/MyCheck_artefacts/MyCheck — JUCE's
#  own layout, which is why the path is asked for with $<TARGET_FILE:...>
#  rather than assumed.
#
#  Registers the target with CTest unless NO_TEST is given, so `ctest` runs it.
#  A check should return the number of failures from main(), which is what CTest
#  reads as a non-zero exit.
#
#  See also scripts/check.sh, which does the same thing for a module sitting
#  outside any project — no CMakeLists of your own required.
# =============================================================================

function(juce_add_check_app)
    cmake_parse_arguments(CHECK "NO_TEST" "TARGET" "SOURCES;MODULES;LINK;DEFINITIONS;INCLUDES" ${ARGN})

    if(NOT CHECK_TARGET)
        message(FATAL_ERROR "juce_add_check_app: TARGET is required")
    endif()

    if(NOT CHECK_SOURCES)
        message(FATAL_ERROR "juce_add_check_app: SOURCES is required")
    endif()

    juce_add_console_app(${CHECK_TARGET} PRODUCT_NAME ${CHECK_TARGET})

    target_sources(${CHECK_TARGET} PRIVATE ${CHECK_SOURCES})

    if(CHECK_INCLUDES)
        target_include_directories(${CHECK_TARGET} PRIVATE ${CHECK_INCLUDES})
    endif()

    # JUCE_STANDALONE_APPLICATION is what makes a console app link without the
    # plugin client wrappers. The other two keep a check from dragging in curl
    # and a web browser it will never use.
    target_compile_definitions(${CHECK_TARGET}
        PRIVATE
            JUCE_STANDALONE_APPLICATION=1
            JUCE_USE_CURL=0
            JUCE_WEB_BROWSER=0
            ${CHECK_DEFINITIONS})

    target_link_libraries(${CHECK_TARGET}
        PRIVATE
            ${CHECK_MODULES}
            ${CHECK_LINK}
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    if(NOT CHECK_NO_TEST)
        add_test(NAME ${CHECK_TARGET} COMMAND $<TARGET_FILE:${CHECK_TARGET}>)
    endif()
endfunction()
