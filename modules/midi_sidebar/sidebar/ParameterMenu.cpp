#include "ParameterMenu.h"

#include <cmath>

namespace microtonos::sidebar
{

using namespace controllers;
using parameterMenu::Action;
using parameterMenu::Item;

namespace
{
    /** The parameter's description, wrapped, as a menu item.

        A `PopupMenu` sizes itself to its widest item, so a sentence in an
        ordinary item would make a menu as wide as the sentence. A custom
        component is the only way to say "this one is 240px and as tall as it
        needs to be", which is what `getIdealSize` is for.

        Its font and colour are handed in rather than looked up here, because
        `getIdealSize` is asked for a size while the component's place in the
        hierarchy is still being decided — and a colour resolved then would be
        the default LookAndFeel's rather than the menu's.
    */
    class InfoItem final : public juce::PopupMenu::CustomComponent
    {
    public:
        InfoItem (juce::String textToShow, juce::Font fontToUse, juce::Colour colourToUse)
            : juce::PopupMenu::CustomComponent (false),   // not clickable: it is here to be read
              text (std::move (textToShow)),
              font (std::move (fontToUse)),
              colour (colourToUse)
        {
        }

        void getIdealSize (int& idealWidth, int& idealHeight) override
        {
            idealWidth  = metrics::menuInfoWidth;
            idealHeight = (int) std::ceil (layoutFor ((float) (idealWidth - padding * 2)).getHeight())
                            + padding * 2;
        }

        void paint (juce::Graphics& g) override
        {
            layoutFor ((float) (getWidth() - padding * 2))
                .draw (g, getLocalBounds().reduced (padding).toFloat());
        }

    private:
        /** `TextLayout` rather than `drawFittedText`: the height has to be
            measured before it can be drawn, and only a layout answers both
            questions from the same wrapping. `drawFittedText` would shrink the
            text to fit a height we are in the middle of computing. */
        juce::TextLayout layoutFor (float width) const
        {
            juce::AttributedString attributed;
            attributed.append (text, font, colour);

            juce::TextLayout layout;
            layout.createLayout (attributed, width);

            return layout;
        }

        static constexpr int padding = metrics::readOutPadding;

        juce::String text;
        juce::Font font;
        juce::Colour colour;
    };
}

//==============================================================================
juce::Array<Item> parameterMenu::itemsFor (const Parameter& parameter,
                                           const juce::Array<Mapping>& mappings,
                                           int parameterIndex)
{
    auto assigned = false;

    for (const auto& mapping : mappings)
    {
        if (mapping.parameterIndex == parameterIndex)
        {
            assigned = true;
            break;
        }
    }

    juce::Array<Item> items;

    // Upper case because the sketch draws it that way and because a section
    // header is what it is: the name of the thing the menu is about, not a
    // choice. The developer supplies the name in their own casing, so unlike
    // the pages' titles — which are simply typed in capitals — this one has to
    // be converted.
    items.add ({ parameter.name.toUpperCase(), Action::none, false, true, false });

    // Disabled when the developer gave no description, rather than hidden: the
    // menu keeps its shape, and an item that is there but empty says "nobody
    // wrote one" where a missing item says nothing at all.
    items.add ({ "info", Action::info, parameter.info.isNotEmpty(), false, false });

    // The rule in the sketch goes here: above it is what the parameter is,
    // below it is what is assigned to it.
    items.add ({ assignmentSummary (mappings, parameterIndex), Action::none, false, false, true });

    items.add ({ "view in sidebar", Action::viewInSidebar, assigned, false, false });

    // Always available. Learning is how a parameter that has nothing gets
    // something, so this is the one item that must work when nothing is
    // assigned.
    items.add ({ "MIDI learn", Action::midiLearn, true, false, false });

    items.add ({ "unlearn", Action::unlearn, assigned, false, false });

    return items;
}

//==============================================================================
struct ParameterMenu::Attachment final : private juce::MouseListener
{
    Attachment (ParameterMenu& menuToShow, juce::Component& widgetToWatch,
                std::function<int()> index)
        : owner (menuToShow), widget (&widgetToWatch), parameterIndex (std::move (index))
    {
        // Nested children too: a Slider's value box is a child of it, and a
        // right-click that landed there would otherwise do nothing, which reads
        // as the menu being unreliable rather than as a miss.
        widgetToWatch.addMouseListener (this, true);
    }

    ~Attachment() override
    {
        if (widget != nullptr)
            widget->removeMouseListener (this);
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        if (event.mods.isPopupMenu() && widget != nullptr && parameterIndex != nullptr)
            owner.showFor (parameterIndex(), *widget);
    }

    ParameterMenu& owner;
    juce::Component::SafePointer<juce::Component> widget;
    const std::function<int()> parameterIndex;
};

//==============================================================================
ParameterMenu::ParameterMenu (Sidebar& sidebarToDriveAndRead)
    : sidebar (sidebarToDriveAndRead)
{
}

ParameterMenu::~ParameterMenu() = default;

void ParameterMenu::attachTo (juce::Component& widget, int parameterIndex)
{
    attachTo (widget, [parameterIndex] { return parameterIndex; });
}

void ParameterMenu::attachTo (juce::Component& widget, std::function<int()> parameterIndex)
{
    attachments.add (new Attachment (*this, widget, std::move (parameterIndex)));
}

//==============================================================================
void ParameterMenu::showFor (int parameterIndex, juce::Component& over)
{
    auto& page = sidebar.getControllersPage();
    const auto& parameters = page.getParameters();

    if (! juce::isPositiveAndBelow (parameterIndex, parameters.size()))
        return;

    const auto& parameter = parameters.getReference (parameterIndex);
    const auto items = parameterMenu::itemsFor (parameter, page.getMappings(), parameterIndex);

    auto& lookAndFeel = over.getLookAndFeel();

    juce::PopupMenu menu;
    menu.setLookAndFeel (&lookAndFeel);

    for (int i = 0; i < items.size(); ++i)
    {
        const auto& item = items[i];

        if (item.separatorBefore)
            menu.addSeparator();

        if (item.isHeader)
        {
            menu.addSectionHeader (item.text);
            continue;
        }

        if (item.action == Action::info)
        {
            juce::PopupMenu description;

            // Id 0 and not triggered automatically: the text is to be read, and
            // an item that closes the menu when you happen to click the words
            // you are reading is a small trap.
            description.addCustomItem (0, std::make_unique<InfoItem> (
                                              parameter.info,
                                              SidebarLookAndFeel::font (metrics::bodyFontHeight),
                                              lookAndFeel.findColour (juce::PopupMenu::textColourId)));

            menu.addSubMenu (item.text, description, item.enabled);
            continue;
        }

        // Ids are one-based positions in `items`, so the callback below needs no
        // second table mapping ids back to actions.
        menu.addItem (i + 1, item.text, item.enabled, false);
    }

    if (onExtendMenu != nullptr)
        onExtendMenu (menu, parameterIndex);

    juce::WeakReference<ParameterMenu> safe (this);

    menu.showMenuAsync (juce::PopupMenu::Options{}.withTargetComponent (&over),
                        [safe, items, parameterIndex] (int result)
                        {
                            if (safe == nullptr || ! juce::isPositiveAndBelow (result - 1, items.size()))
                                return;

                            safe->perform (items[result - 1].action, parameterIndex);
                        });
}

void ParameterMenu::perform (Action action, int parameterIndex)
{
    switch (action)
    {
        case Action::viewInSidebar:
        {
            sidebar.setActivePage (Sidebar::Page::controllers);

            // Asynchronously, because opening a page changes the sidebar's
            // preferred width and the owner is free to animate that: the table
            // has no useful bounds at this instant, and scrolling a row into
            // view before it has any is a no-op. The selection itself would
            // survive either way; it is the scroll that needs a laid-out table.
            juce::Component::SafePointer<ControllersPage> page (&sidebar.getControllersPage());

            juce::MessageManager::callAsync ([page, parameterIndex]
                                             {
                                                 if (page != nullptr)
                                                     page->showMappingsFor (parameterIndex);
                                             });
            break;
        }

        case Action::unlearn:
            sidebar.getControllersPage().removeMappingsFor (parameterIndex);
            break;

        case Action::midiLearn:
            if (onMidiLearnRequested != nullptr)
                onMidiLearnRequested (parameterIndex);

            break;

        case Action::info:
        case Action::none:
            break;
    }
}

} // namespace microtonos::sidebar
