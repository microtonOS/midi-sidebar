cmake_minimum_required(VERSION 3.22)

project(JUCESidebar VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
#  JUCE
#  ---------------------------------------------------------------------------
#  Sidebar is a JUCE module, so a project consuming it will normally already
#  have brought JUCE in. In that case we must NOT add a second copy: two JUCE
#  trees in one build produce duplicate targets and duplicate symbols. The guard
#  below means the parent's JUCE always wins, and we only fetch our own when
#  this project is being built on its own.
#
#  JUCE_SIDEBAR_JUCE_DIR points at a JUCE checkout. It defaults to ~/JUCE, which
#  is convenient but not pinned — when this project gets CI, replace the default
#  with a submodule or FetchContent and nothing else here has to change.
# =============================================================================

if(NOT TARGET juce::juce_gui_basics)
    set(JUCE_SIDEBAR_JUCE_DIR "$ENV{HOME}/JUCE"
        CACHE PATH "Path to a JUCE checkout, used only for standalone builds of this project")

    if(NOT EXISTS "${JUCE_SIDEBAR_JUCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "No JUCE at '${JUCE_SIDEBAR_JUCE_DIR}'.\n"
            "Point JUCE_SIDEBAR_JUCE_DIR at a JUCE checkout, e.g.\n"
            "  cmake -B build -DJUCE_SIDEBAR_JUCE_DIR=/path/to/JUCE")
    endif()

    add_subdirectory("${JUCE_SIDEBAR_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
endif()

# =============================================================================
#  The module itself
# =============================================================================

juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/modules/midi_sidebar"
    ALIAS_NAMESPACE microtonos)

# =============================================================================
#  Development-only targets
#  ---------------------------------------------------------------------------
#  The demo plugin exists so there is an editor to load in a host and to point
#  the snapshot tool at; a module on its own cannot be rendered. It is also the
#  worked example for anyone consuming the module. Skipped when we are a
#  subdirectory of someone else's project.
# =============================================================================

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(demo)
endif()
