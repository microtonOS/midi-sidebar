# =============================================================================
#  Checks
#  ---------------------------------------------------------------------------
#  One console app per header worth exercising, run by `ctest`. Consoles rather
#  than a test framework: the module's logic headers are deliberately free of
#  the GUI and of juce_audio_processors, so each one can be linked on its own
#  and asserted against without a plugin, a host or a window.
#
#  These live in the repository rather than in a scratch directory for a reason
#  worth recording: an earlier round of them was written in a session temp
#  folder, passed, and was gone the next day. A check that is not a build target
#  is not a check.
#
#      cmake --build build && ctest --test-dir build --output-on-failure
#
#  Reusable *fixtures* — the byte sequences, the synthetic gestures, the scale
#  files — belong in the skills instead, since they are facts about MIDI rather
#  than about this project. See .claude/skills/midi-1_0/scripts/ and
#  .claude/skills/midi-microtuning/scripts/.
# =============================================================================

set(SIDEBAR_CHECKS
    RegisterCheck        # MSB/LSB registers, through the router
    MidiLearnerCheck     # learning a control from a gesture
    DeviceControlCheck   # Master Volume system exclusive
    MtsSysexCheck        # the MIDI Tuning Standard messages
    PeriodCheck          # period inference
    ScalaCheck)          # .scl and .kbm

foreach(check ${SIDEBAR_CHECKS})
    juce_add_console_app(${check} PRODUCT_NAME ${check})

    target_sources(${check} PRIVATE ${check}.cpp)
    target_include_directories(${check} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

    target_compile_definitions(${check}
        PRIVATE
            JUCE_STANDALONE_APPLICATION=1
            JUCE_USE_CURL=0
            JUCE_WEB_BROWSER=0)

    target_link_libraries(${check}
        PRIVATE
            microtonos::midi_sidebar
            juce::juce_audio_basics
            # juce_gui_basics because the module is one library: its pages are
            # widgets even where the header under test is not. Nothing here
            # opens a window.
            juce::juce_gui_basics
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    # JUCE puts a console app's binary under <target>_artefacts/, so the path is
    # asked for rather than assumed.
    add_test(NAME ${check} COMMAND $<TARGET_FILE:${check}>)
endforeach()
