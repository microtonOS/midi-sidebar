juce_add_plugin(SidebarDemo
    PRODUCT_NAME                "Sidebar Demo"
    COMPANY_NAME                "microtonOS"
    PLUGIN_MANUFACTURER_CODE    Mtos
    PLUGIN_CODE                 Sbdm
    FORMATS                     Standalone
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE

    # Without these the standalone dies on launch with a TCC privacy violation
    # when macOS enumerates Bluetooth MIDI devices. See the juce-macos skill.
    BLUETOOTH_PERMISSION_ENABLED TRUE
    BLUETOOTH_PERMISSION_TEXT    "Sidebar Demo uses Bluetooth to receive MIDI from wireless controllers.")

target_sources(SidebarDemo
    PRIVATE
        DemoProcessor.cpp
        DemoEditor.cpp)

target_compile_definitions(SidebarDemo
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SidebarDemo
    PRIVATE
        microtonos::midi_sidebar
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# =============================================================================
#  Snapshot tool — renders the editor to a PNG with no window, so the GUI can
#  be looked at from a terminal or a sandbox. See
#  .claude/skills/juce-ui/scripts/README.md
# =============================================================================

include("${CMAKE_SOURCE_DIR}/.claude/skills/juce-ui/scripts/add_snapshot_tool.cmake")

juce_gui_add_snapshot_tool(
    TARGET           SidebarDemo_snapshot
    PLUGIN_TARGET    SidebarDemo
    PROCESSOR_CLASS  microtonos::sidebar::demo::DemoProcessor
    PROCESSOR_HEADER "DemoProcessor.h"
    INCLUDE_DIRS     "${CMAKE_CURRENT_SOURCE_DIR}")
