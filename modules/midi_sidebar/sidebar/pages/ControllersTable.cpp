#include "ControllersTable.h"

namespace microtonos::sidebar
{

using namespace controllers;

namespace
{
    /** Any non-zero value: the two sort buttons are the only radio group that
        shares this component as a parent. */
    constexpr int sortGroupId = 1;

    /** An unset controller number shows nothing at all — an empty cell says
        "no value" without having to be read. */
    juce::String ccText (const std::optional<int>& cc)
    {
        return cc.has_value() ? juce::String (*cc) : juce::String();
    }

    /** Trailing zeroes are noise on a limit: 10 rather than 10.00, but 12.5
        when that is what was typed. */
    juce::String numberText (double value)
    {
        const auto whole = juce::exactlyEqual (value, (double) (juce::int64) value);
        return juce::String (value, whole ? 0 : 2);
    }
}

//==============================================================================
/** A cell the end-user picks from: the parameter, the channel, the mode. */
struct ControllersTable::ChoiceCell final : public juce::Component
{
    explicit ChoiceCell (ControllersTable& t) : owner (t)
    {
        // onChoice fires only for a menu pick, so unlike a ComboBox there is
        // nothing to guard against while the cell is being pointed at a
        // different row.
        button.onChoice = [this] (int index) { owner.commitChoice (row, columnId, index); };

        // Listen to the button's own events too, or clicking it would open the
        // menu without ever selecting the row it belongs to.
        button.addMouseListener (this, true);
        addAndMakeVisible (button);
    }

    void mouseDown (const juce::MouseEvent& e) override { owner.selectRowFromCell (row, e.mods); }

    void update (int newRow, int newColumnId)
    {
        row = newRow;
        columnId = newColumnId;

        button.setName (owner.nameFor (columnId));
        button.setItems (owner.itemsFor (columnId), owner.separatorFor (columnId));
        button.setSelectedIndex (owner.choiceFor (row, columnId));
    }

    void resized() override { button.setBounds (getLocalBounds().reduced (metrics::tableCellInset)); }

    ControllersTable& owner;
    ChoiceButton button;
    int row = 0, columnId = 0;
};

//==============================================================================
/** A cell the end-user types a number into.

    A `Label` rather than a `TextEditor`: a table cell that only becomes an
    editor when you double-click it is the JUCE convention, it costs no extra
    widget, and it is what lets the value be *displayed* with its unit while
    being *edited* as a bare number.
*/
struct ControllersTable::NumberCell final : public juce::Component
{
    explicit NumberCell (ControllersTable& t) : owner (t)
    {
        label.setEditable (false, true, false);
        label.setJustificationType (juce::Justification::centredLeft);

        // Strip the unit as the editor opens, so the end-user types "10" and
        // not "10 %" — and put it back on commit.
        label.onEditorShow = [this]
        {
            if (auto* editor = label.getCurrentTextEditor())
            {
                editor->setInputRestrictions (0, "-0123456789.");
                editor->setText (owner.editableTextFor (row, columnId), false);
                editor->selectAll();
            }
        };

        label.onTextChange = [this]
        {
            if (! updating)
                owner.commitText (row, columnId, label.getText());
        };

        label.addMouseListener (this, true);
        addAndMakeVisible (label);
    }

    void mouseDown (const juce::MouseEvent& e) override { owner.selectRowFromCell (row, e.mods); }

    void update (int newRow, int newColumnId)
    {
        row = newRow;
        columnId = newColumnId;

        const juce::ScopedValueSetter<bool> guard (updating, true);

        label.setText (owner.textFor (row, columnId), juce::dontSendNotification);
        setEnabled (! (columnId == ControllersTable::lsb && owner.ignoresLsb (row)));
    }

    void resized() override { label.setBounds (getLocalBounds().reduced (metrics::tableCellInset)); }

    ControllersTable& owner;
    juce::Label label;
    int row = 0, columnId = 0;
    bool updating = false;
};

//==============================================================================
ControllersTable::ControllersTable()
{
    table.setHeaderHeight (metrics::tableHeaderHeight);
    table.setRowHeight (metrics::tableRowHeight);

    // visible only: no resizing, dragging, sorting or column menu. The columns
    // are the specification, not a preference.
    auto& header = table.getHeader();
    const auto fixed = juce::TableHeaderComponent::visible;

    // The two menu columns are measured, not chosen: as wide as their longest
    // entry — or their own title, whichever is worse — plus the standard
    // padding, and no wider. Every pixel saved here is one the scrolling area
    // gets, which at 248px is the difference between seeing three columns and
    // four.
    header.addColumn ("channel", channel, widthForContents ("channel", itemsFor (channel)), 0, -1, fixed);
    header.addColumn ("MSB",     msb,     metrics::tableCcWidth,    0, -1, fixed);
    header.addColumn ("LSB",     lsb,     metrics::tableCcWidth,    0, -1, fixed);
    header.addColumn ("mode",    mode,    widthForContents ("mode", itemsFor (mode)), 0, -1, fixed);
    header.addColumn ("min",     minimum, metrics::tableLimitWidth, 0, -1, fixed);
    header.addColumn ("max",     maximum, metrics::tableLimitWidth, 0, -1, fixed);

    // The sort toggle. Radio-grouped so exactly one is on, and connected so
    // the pair reads as one control rather than two switches.
    for (auto* b : { (juce::Button*) &recentButton, (juce::Button*) &alphabeticalButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (sortGroupId);
        addAndMakeVisible (*b);
    }

    // A little more of the button given over to the icon than DrawableButton's
    // default, which leaves a 12px clock in an 18px strip looking timid.
    recentButton.setEdgeIndent (metrics::tableCellInset);

    recentButton      .setConnectedEdges (juce::Button::ConnectedOnRight);
    alphabeticalButton.setConnectedEdges (juce::Button::ConnectedOnLeft);

    recentButton      .onClick = [this] { setOrder (Order::recent); };
    alphabeticalButton.onClick = [this] { setOrder (Order::alphabetical); };

    recentButton.setToggleState (true, juce::dontSendNotification);

    frozenColumn.setRowHeight (metrics::tableRowHeight);

    // The frozen column follows the table; it is never scrolled directly, so it
    // shows no scrollbars of its own and does not take mouse clicks. Its
    // ComboBoxes still do — children are unaffected — while a wheel over the
    // background falls through to this component, which forwards it to the
    // table. Without that, spinning the wheel over the names would slide them
    // out of step with the rows beside them.
    frozenColumn.getViewport()->setScrollBarsShown (false, false);
    frozenColumn.setInterceptsMouseClicks (false, true);

    table.getVerticalScrollBar().addListener (this);

    addAndMakeVisible (frozenColumn);
    addAndMakeVisible (table);
}

ControllersTable::~ControllersTable()
{
    table.getVerticalScrollBar().removeListener (this);
}

//==============================================================================
void ControllersTable::setParameters (juce::Array<controllers::Parameter> newParameters)
{
    parameters = std::move (newParameters);
    refreshRows();
}

void ControllersTable::setMappings (juce::Array<controllers::Mapping> newMappings)
{
    mappings = std::move (newMappings);
    refreshRows();
}

void ControllersTable::addMapping()
{
    mappings.add ({});
    refreshRows();

    // Selected wherever it landed, which under `recent` is the top row and
    // under `alphabetical` is wherever its parameter's name puts it.
    table.selectRow (displayOrder.indexOf (mappings.size() - 1));
    changed();
}

void ControllersTable::removeSelectedMapping()
{
    if (mappings.isEmpty())
        return;

    const auto selected = mappingIndexFor (table.getSelectedRow());
    mappings.remove (selected >= 0 ? selected : mappings.size() - 1);

    refreshRows();
    changed();
}

void ControllersTable::changed()
{
    if (onMappingsChanged != nullptr)
        onMappingsChanged();
}

void ControllersTable::setOrder (Order newOrder)
{
    if (order == newOrder)
        return;

    order = newOrder;
    refreshRows();
}

void ControllersTable::selectRowFromCell (int row, const juce::ModifierKeys& mods)
{
    table.selectRowsBasedOnModifierKeys (row, mods, false);
}

int ControllersTable::mappingIndexFor (int row) const
{
    return juce::isPositiveAndBelow (row, displayOrder.size()) ? displayOrder[row] : -1;
}

void ControllersTable::rebuildOrder()
{
    displayOrder.clearQuick();

    for (int i = 0; i < mappings.size(); ++i)
        displayOrder.add (i);

    if (order == Order::recent)
    {
        // Newest first. `mappings` is append-ordered, so this is simply the
        // reverse — no timestamps to keep.
        std::reverse (displayOrder.begin(), displayOrder.end());
        return;
    }

    // By parameter name, a at the top. Stable, so mappings sharing a parameter
    // stay in the order they were added rather than shuffling on every rebuild.
    const auto nameOf = [this] (int index)
    {
        const auto parameterIndex = mappings[index].parameterIndex;

        return juce::isPositiveAndBelow (parameterIndex, parameters.size())
                   ? parameters[parameterIndex].name
                   : juce::String();
    };

    std::stable_sort (displayOrder.begin(), displayOrder.end(),
                      [&nameOf] (int a, int b)
                      {
                          return nameOf (a).compareIgnoreCase (nameOf (b)) < 0;
                      });
}

void ControllersTable::refreshRows()
{
    rebuildOrder();

    table.updateContent();
    frozenColumn.updateContent();

    table.repaint();
    frozenColumn.repaint();
}

int ControllersTable::widthForContents (const juce::String& title, const juce::StringArray& items)
{
    // Each is measured in the font it will actually be drawn in: a cell's
    // button is short, so LookAndFeel scales its label down well below
    // bodyFontHeight, while the column title is drawn bold at half the header's
    // height. Measuring both at the same size would leave one of them wrong.
    const auto titleFont = SidebarLookAndFeel::font (metrics::tableHeaderHeight * 0.5f, true);
    const auto itemFont  = SidebarLookAndFeel::buttonFont (metrics::tableRowHeight
                                                               - metrics::tableCellInset * 2);

    auto widest = juce::GlyphArrangement::getStringWidthInt (titleFont, title);

    for (const auto& item : items)
        widest = juce::jmax (widest, juce::GlyphArrangement::getStringWidthInt (itemFont, item));

    return widest + metrics::tableTextPadding;
}

//==============================================================================
int ControllersTable::getNumRows()
{
    return mappings.size();
}

const controllers::Parameter* ControllersTable::parameterFor (int row) const
{
    const auto mapping = mappingIndexFor (row);

    if (mapping < 0)
        return nullptr;

    const auto index = mappings[mapping].parameterIndex;

    return juce::isPositiveAndBelow (index, parameters.size()) ? &parameters.getReference (index)
                                                               : nullptr;
}

bool ControllersTable::ignoresLsb (int row) const
{
    const auto mapping = mappingIndexFor (row);

    if (mapping < 0)
        return false;

    const auto m = mappings[mapping].mode;
    return m == Mode::toggle || m == Mode::increment;
}

//==============================================================================
juce::String ControllersTable::textFor (int row, int columnId) const
{
    const auto index = mappingIndexFor (row);

    if (index < 0)
        return {};

    const auto& mapping = mappings[index];

    // Limits carry the unit their *parameter* names, so a row retargeted at a
    // different parameter relabels itself with nothing to keep in step.
    const auto withUnit = [this, row] (double value)
    {
        auto* parameter = parameterFor (row);
        const auto unit = parameter != nullptr ? parameter->unit : juce::String();

        return unit.isEmpty() ? numberText (value) : numberText (value) + " " + unit;
    };

    switch (columnId)
    {
        case msb:     return ccText (mapping.msb);
        case lsb:     return ccText (mapping.lsb);
        case minimum: return withUnit (mapping.min);
        case maximum: return withUnit (mapping.max);
        default:      break;
    }

    return {};
}

juce::String ControllersTable::editableTextFor (int row, int columnId) const
{
    const auto index = mappingIndexFor (row);

    if (index < 0)
        return {};

    const auto& mapping = mappings[index];

    switch (columnId)
    {
        case msb:     return mapping.msb.has_value() ? juce::String (*mapping.msb) : juce::String();
        case lsb:     return mapping.lsb.has_value() ? juce::String (*mapping.lsb) : juce::String();
        case minimum: return numberText (mapping.min);
        case maximum: return numberText (mapping.max);
        default:      break;
    }

    return {};
}

void ControllersTable::commitText (int row, int columnId, const juce::String& text)
{
    const auto index = mappingIndexFor (row);

    if (index < 0)
        return;

    auto& mapping = mappings.getReference (index);
    const auto trimmed = text.trim();

    // An emptied controller number means "there is none", which is a legal
    // state; an emptied limit is not, so it keeps what it had.
    const auto asCc = [&trimmed]() -> std::optional<int>
    {
        if (trimmed.isEmpty())
            return std::nullopt;

        return juce::jlimit (0, 127, trimmed.getIntValue());
    };

    switch (columnId)
    {
        case msb:     mapping.msb = asCc(); break;
        case lsb:     mapping.lsb = asCc(); break;
        case minimum: if (trimmed.isNotEmpty()) mapping.min = trimmed.getDoubleValue(); break;
        case maximum: if (trimmed.isNotEmpty()) mapping.max = trimmed.getDoubleValue(); break;
        default:      return;
    }

    refreshRows();
    changed();
}

//==============================================================================
juce::String ControllersTable::nameFor (int columnId) const
{
    // Every cell in a column shares a name, so `--click chan` takes the first
    // row's — which is what you want when checking that a menu opens at all.
    return columnId == param   ? "param"
         : columnId == channel ? "chan"
         : columnId == mode    ? "mode"
                               : juce::String();
}

juce::StringArray ControllersTable::itemsFor (int columnId) const
{
    if (columnId == param)
    {
        juce::StringArray names;

        for (const auto& parameter : parameters)
            names.add (parameter.name);

        return names;
    }

    if (columnId == channel)
    {
        // Index 0 is omni, so the list's indices are already the channel values
        // `controllers::omniChannel`, 1, 2 … and nothing has to be converted.
        juce::StringArray channels { "omni" };

        for (int c = firstChannel; c <= lastChannel; ++c)
            channels.add (juce::String (c));

        return channels;
    }

    return columnId == mode ? modeNames : juce::StringArray();
}

int ControllersTable::separatorFor (int columnId) const
{
    // The rule the sketch draws in the mode menu: everything below it ignores
    // the LSB.
    return columnId == mode ? modesBeforeSeparator : -1;
}

int ControllersTable::choiceFor (int row, int columnId) const
{
    const auto index = mappingIndexFor (row);

    if (index < 0)
        return -1;

    const auto& mapping = mappings[index];

    switch (columnId)
    {
        case param:   return mapping.parameterIndex;
        case channel: return mapping.channel;
        case mode:    return static_cast<int> (mapping.mode);
        default:      break;
    }

    return -1;
}

void ControllersTable::commitChoice (int row, int columnId, int index)
{
    const auto mappingIndex = mappingIndexFor (row);

    if (index < 0 || mappingIndex < 0)
        return;

    auto& mapping = mappings.getReference (mappingIndex);

    switch (columnId)
    {
        case param:   mapping.parameterIndex = index; break;
        case channel: mapping.channel        = index; break;
        case mode:    mapping.mode           = static_cast<Mode> (index); break;
        default:      return;
    }

    // The parameter decides the unit and the mode decides whether the LSB is
    // live, so both redraw the row rather than just their own cell.
    refreshRows();
    changed();
}

//==============================================================================
void ControllersTable::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    paintRowBackground (g, row, width, height, selected);
}

void ControllersTable::paintRowBackground (juce::Graphics& g, int, int, int, bool selected)
{
    if (selected)
        g.fillAll (findColour (ChoiceStrip::selectedColourId).withAlpha (shades::selectedRow));
}

void ControllersTable::paintCell (juce::Graphics&, int, int, int, int, bool)
{
    // Every cell holds a widget, so there is nothing left to draw.
}

juce::Component* ControllersTable::refreshComponentForRow (int row, bool, juce::Component* existing)
{
    if (mappingIndexFor (row) < 0)
    {
        delete existing;
        return nullptr;
    }

    auto* cell = dynamic_cast<ChoiceCell*> (existing);

    if (cell == nullptr)
    {
        delete existing;
        cell = new ChoiceCell (*this);
    }

    cell->update (row, param);
    return cell;
}

juce::Component* ControllersTable::refreshComponentForCell (int row, int columnId, bool,
                                                           juce::Component* existing)
{
    if (mappingIndexFor (row) < 0)
    {
        delete existing;
        return nullptr;
    }

    if (columnId == channel || columnId == mode)
    {
        auto* cell = dynamic_cast<ChoiceCell*> (existing);

        if (cell == nullptr)
        {
            delete existing;
            cell = new ChoiceCell (*this);
        }

        cell->update (row, columnId);
        return cell;
    }

    auto* cell = dynamic_cast<NumberCell*> (existing);

    if (cell == nullptr)
    {
        delete existing;
        cell = new NumberCell (*this);
    }

    cell->update (row, columnId);
    return cell;
}

void ControllersTable::selectedRowsChanged (int lastRowSelected)
{
    // Mirrored, not scrolled to: the frozen column's position is the table's,
    // and moving it here would fight the scroll tie below.
    frozenColumn.selectRow (lastRowSelected, true, true);
}

//==============================================================================
void ControllersTable::scrollBarMoved (juce::ScrollBar*, double newRangeStart)
{
    // A Viewport's scrollbar is ranged over its content in *pixels*, not rows
    // (`Viewport::updateVisibleArea` sets the limits from the content height),
    // so this is already the offset to copy across. Both lists hold the same
    // number of rows at the same height, so the same offset means the same row.
    //
    // One direction only: the frozen column has no scrollbar and takes no
    // wheel, so nothing else can move it and there is no loop to guard against.
    frozenColumn.getViewport()->setViewPosition (0, juce::roundToInt (newRangeStart));
}

void ControllersTable::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    // Arrives from the frozen column, which does not intercept mouse events.
    // Forwarded to the table's *viewport* — `Component::mouseWheelMove` on the
    // table itself does nothing, since scrolling belongs to the viewport.
    if (auto* viewport = table.getViewport())
        viewport->useMouseWheelMoveIfNeeded (e.getEventRelativeTo (viewport), wheel);
}

//==============================================================================
void ControllersTable::lookAndFeelChanged()
{
    if (! getLookAndFeel().isColourSpecified (ReadOutField::backgroundColourId))
        return;

    const auto accent = findColour (ChoiceStrip::selectedColourId);
    const auto onText = findColour (ChoiceStrip::selectedTextColourId);

    // The chosen order takes the same accent as every other "this one is on" in
    // the sidebar. The clock is a Drawable, so its selected state is a second
    // copy of the icon in the contrasting colour rather than a text colour.
    alphabeticalButton.setColour (juce::TextButton::buttonOnColourId, accent);
    alphabeticalButton.setColour (juce::TextButton::textColourOnId,   onText);

    // TextButton's ids, not DrawableButton's: with ImageOnButtonBackground the
    // background is drawn by drawButtonBackground, which reads those — so the
    // clock and "abc" take their selected colour from the same place.
    recentButton.setColour (juce::TextButton::buttonOnColourId, accent);

    recentButton.setImages (icons::load (icons::clock, findColour (ReadOutField::textColourId)).get(),
                            nullptr, nullptr, nullptr,
                            icons::load (icons::clock, onText).get());

    // The tables sit on the panel like a read-out does — recessed, same
    // hairline — rather than introducing a third surface to the page.
    for (auto* list : { (juce::ListBox*) &frozenColumn, (juce::ListBox*) &table })
    {
        list->setColour (juce::ListBox::backgroundColourId, findColour (ReadOutField::backgroundColourId));
        list->setColour (juce::ListBox::outlineColourId,    findColour (ReadOutField::outlineColourId));
        list->setOutlineThickness (1);
    }
}

void ControllersTable::resized()
{
    auto bounds = getLocalBounds();

    // The frozen column starts below the table's header, so its first row lines
    // up with the table's first row rather than with the header. The strip that
    // leaves free above it is where the sort toggle goes: the parameter column
    // has no title of its own, so the space is there for the taking.
    auto frozen = bounds.removeFromLeft (metrics::tableFrozenColumnWidth);
    const auto sortStrip = frozen.removeFromTop (metrics::tableHeaderHeight);

    frozenColumn.setBounds (frozen);

    // Two equal flexible tracks rather than a width halved: the odd pixel is
    // then distributed by the layout instead of being abandoned at one edge,
    // which on a connected pair would show as a seam off centre.
    juce::Grid sort;

    sort.templateRows    = { juce::Grid::TrackInfo (juce::Grid::Fr (1)) };
    sort.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)),
                             juce::Grid::TrackInfo (juce::Grid::Fr (1)) };

    sort.items = { juce::GridItem (recentButton), juce::GridItem (alphabeticalButton) };
    sort.performLayout (sortStrip);

    bounds.removeFromLeft (metrics::pageColumnGap);
    table.setBounds (bounds);
}

} // namespace microtonos::sidebar
