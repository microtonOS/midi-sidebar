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
#include "sidebar/PopupHost.h"
#include "sidebar/LevelMeter.h"
#include "sidebar/VolumeStrip.h"
#include "sidebar/widgets/ChoiceButton.h"
#include "sidebar/widgets/ChoiceStrip.h"
#include "sidebar/widgets/NumberStepper.h"
#include "sidebar/widgets/ReadOutField.h"
#include "sidebar/pages/PageGrid.h"
#include "sidebar/pages/TuningState.h"
#include "sidebar/pages/ControllersState.h"
#include "sidebar/pages/ControllersTable.h"
#include "sidebar/pages/ControllersPage.h"
#include "sidebar/pages/PresetsState.h"
#include "sidebar/pages/PresetsPage.h"
#include "sidebar/pages/ChannelSelector.h"
#include "sidebar/pages/TuningPage.h"
#include "sidebar/SidebarPanel.h"
#include "sidebar/Sidebar.h"
#include "sidebar/ParameterMenu.h"
