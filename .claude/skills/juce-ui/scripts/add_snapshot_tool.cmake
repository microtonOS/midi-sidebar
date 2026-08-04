# =============================================================================
#  add_snapshot_tool.cmake — wire SnapshotTool.cpp into a JUCE project.
#  ---------------------------------------------------------------------------
#  Include this file from your CMakeLists.txt and call the function once:
#
#      include(.claude/skills/juce-ui/scripts/add_snapshot_tool.cmake)
#
#      juce_gui_add_snapshot_tool(
#          TARGET           MyPlugin_snapshot
#          PLUGIN_TARGET    MyPlugin              # the juce_add_plugin target
#          PROCESSOR_CLASS  MyPluginAudioProcessor
#          PROCESSOR_HEADER "PluginProcessor.h"
#          INCLUDE_DIRS     "${CMAKE_CURRENT_SOURCE_DIR}/plugin")
#
#  Then:  cmake --build build --target MyPlugin_snapshot
#
#  The tool is a developer aid. Guard the call with something like
#  `if (PROJECT_IS_TOP_LEVEL)` if you do not want it built by downstream
#  consumers of your library.
# =============================================================================

function(juce_gui_add_snapshot_tool)
    set(options "")
    set(oneValueArgs TARGET PLUGIN_TARGET PROCESSOR_CLASS PROCESSOR_HEADER)
    set(multiValueArgs INCLUDE_DIRS EXTRA_LIBS)
    cmake_parse_arguments(SNAP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(required TARGET PLUGIN_TARGET PROCESSOR_CLASS PROCESSOR_HEADER)
        if(NOT SNAP_${required})
            message(FATAL_ERROR "juce_gui_add_snapshot_tool: ${required} is required")
        endif()
    endforeach()

    juce_add_console_app(${SNAP_TARGET}
        PRODUCT_NAME "${SNAP_TARGET}")

    target_sources(${SNAP_TARGET}
        PRIVATE "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/SnapshotTool.cpp")

    target_compile_definitions(${SNAP_TARGET}
        PRIVATE
            JUCE_SNAPSHOT_PROCESSOR_CLASS=${SNAP_PROCESSOR_CLASS}
            JUCE_SNAPSHOT_PROCESSOR_HEADER="${SNAP_PROCESSOR_HEADER}"
            # Required for MessageManager::runDispatchLoopUntil, which the tool
            # uses to let timers and parameter attachments settle before it
            # paints. Without it the snapshot can disagree with the running
            # plugin. This is a dev-only target, so permitting modal loops here
            # has no effect on the shipped plugin.
            JUCE_MODAL_LOOPS_PERMITTED=1
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)

    if(SNAP_INCLUDE_DIRS)
        target_include_directories(${SNAP_TARGET} PRIVATE ${SNAP_INCLUDE_DIRS})
    endif()

    # Linking the plugin's shared-code target gives us the processor and editor
    # without recompiling them. If this fails to link because of plug-in
    # wrapper symbols, see the "If linking fails" section in README.md.
    target_link_libraries(${SNAP_TARGET}
        PRIVATE
            ${SNAP_PLUGIN_TARGET}
            juce::juce_audio_processors
            juce::juce_gui_extra
            ${SNAP_EXTRA_LIBS}
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
endfunction()
