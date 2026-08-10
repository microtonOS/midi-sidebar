#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>
#include "../SidebarLookAndFeel.h"

namespace microtonos::sidebar
{

//==============================================================================
/** A whole number with a pair of inc/dec buttons, which may also be unset.

    Used for the program and bank on the tuning and presets pages: numbers you
    step through rather than type your way around, but which can also be simply
    absent — a tuning may state no bank at all.

    **Zero is the unset value**, shown as an empty box. A sentinel rather than a
    parallel `bool`, because a `Slider` always has a value: giving "nothing" a
    place inside the range is what lets one control cover both states without
    the two ever disagreeing. It costs nothing, since the numbers this is used
    for are 1-based anyway.

    The text box stays editable: any number in range is a legal destination, so
    typing 37 should not mean thirty-six clicks. That is the opposite of the
    tuning page's period chooser, where only the offered values are valid and
    the box is deliberately read-only.
*/
class NumberStepper final : public juce::Slider
{
public:
    /** @param name     the component name, for the tree and the snapshot tool
        @param highest  the largest value; 1 to this, plus zero for unset */
    NumberStepper (const juce::String& name, int highest)
        : juce::Slider (name)
    {
        setSliderStyle (juce::Slider::IncDecButtons);
        setTextBoxStyle (juce::Slider::TextBoxLeft, false,
                         metrics::incDecTextBoxWidth, metrics::pageRowHeight);
        setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Vertical);

        setRange (unsetValue, highest, 1.0);

        textFromValueFunction = [] (double value)
        {
            return value <= unsetValue ? juce::String() : juce::String (juce::roundToInt (value));
        };

        // An emptied box means "unset" rather than "unchanged", which is a
        // state the plugin can be in and so has to be reachable.
        valueFromTextFunction = [] (const juce::String& text)
        {
            return text.trim().isEmpty() ? (double) unsetValue : (double) text.getIntValue();
        };

        onValueChange = [this]
        {
            if (onNumberChosen != nullptr)
                onNumberChosen (getNumber());
        };
    }

    /** Nothing when the box is empty. */
    std::optional<int> getNumber() const
    {
        const auto value = juce::roundToInt (getValue());
        return value <= unsetValue ? std::optional<int>() : value;
    }

    /** Silent: this is the owner saying what the number is, and reporting it
        back would be an echo. */
    void setNumber (std::optional<int> number)
    {
        setValue (number.value_or (unsetValue), juce::dontSendNotification);
        updateText();
    }

    /** Called only when the end-user steps or types. */
    std::function<void (std::optional<int>)> onNumberChosen;

private:
    static constexpr int unsetValue = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NumberStepper)
};

} // namespace microtonos::sidebar
