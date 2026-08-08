/*
  ==============================================================================

  BEGIN_JUCE_MODULE_DECLARATION

    ID:                 midi_sidebar
    vendor:             microtonOS
    version:            0.1.0
    name:               Sidebar
    description:        A sidebar for managing microtunings, program changes and continuous controllers.
    website:            https://github.com/microtonOS/midi-sidebar
    license:            TBD
    minimumCppStandard: 17

    dependencies:       juce_gui_basics

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "sidebar/SidebarLookAndFeel.h"
#include "sidebar/SidebarIcons.h"
#include "sidebar/LevelMeter.h"
#include "sidebar/VolumeStrip.h"
#include "sidebar/SidebarPanel.h"
#include "sidebar/Sidebar.h"
