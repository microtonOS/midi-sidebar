#include "ControllersTable.h"

namespace microtonos::sidebar
{

using namespace controllers;

namespace
{
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

        // Only the parameter column is marked. The menu asks per item so the
        // glyph beside a name while choosing is the one that stays after.
        if (columnId == ControllersTable::param)
        {
            button.iconForItem = [this] (int i) { return owner.markerForParameter (i); };
            button.setIcon (owner.markerFor (row));
        }

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

        // The LSB cell is disabled where the mode ignores it — the two
        // threshold modes read a single byte against 64, so a low byte would be
        // a number with no effect. Disabled rather than hidden: it is still
        // part of the mapping and comes back when the mode changes.
        setEnabled (! (columnId == ControllersTable::lsb && owner.ignoresLsb (row)));

        // Cheap enough to ask on every refresh, and asking here rather than in
        // paint keeps the answer stable while the cell is on screen.
        invalid = owner.isCellInvalid (row, columnId);
        repaint();
    }

    /** Behind the label, not over it: the number has to stay readable, and a
        wash is what says "this cell" where a border would compete with the
        table's own dividers. See docs/controllers.md. */
    void paint (juce::Graphics& g) override
    {
        if (! invalid)
            return;

        g.setColour (findColour (pageColours::invalidColourId)
                         .withMultipliedAlpha (shades::invalidFill));
        g.fillRect (getLocalBounds().reduced (metrics::tableCellInset));
    }

    void resized() override { label.setBounds (getLocalBounds().reduced (metrics::tableCellInset)); }

    ControllersTable& owner;
    juce::Label label;
    int row = 0, columnId = 0;
    bool updating = false;
    bool invalid = false;
};

//==============================================================================
ControllersTable::ControllersTable()
{
    setName ("Controllers table");   // see the note in Sidebar's constructor

    // Our own header, for the three-state cycle: JUCE's stops at ascending and
    // descending, and never lets go of the column. See SortingHeader.
    auto ownedHeader = std::make_unique<SortingHeader>();
    tableHeader = ownedHeader.get();
    table.setHeader (std::move (ownedHeader));

    table.setHeaderHeight (metrics::tableHeaderHeight);
    table.setRowHeight (metrics::tableRowHeight);

    // visible only: no resizing, dragging or column menu. The columns are the
    // specification, not a preference.
    auto& header = table.getHeader();
    const auto fixed = juce::TableHeaderComponent::visible;

    // Sortable where an order means something. The channel, the two controller
    // numbers and the mode all have one: the numbers count, and the other two
    // follow their own menus — which is what makes the sorted table read in the
    // same order as the menu you picked from.
    //
    // `min` and `max` are deliberately not sortable. Their unit belongs to the
    // parameter, so one row's 100 is a percentage and the next row's is a
    // frequency; sorting them would be ordering by a number that means a
    // different thing in every row.
    const auto sortable = fixed | juce::TableHeaderComponent::sortable;

    // `mode` is measured, not chosen: as wide as its longest entry — or its own
    // title, whichever is worse — plus the standard padding, and no wider. Every
    // pixel saved here is one the scrolling area gets, which at the panel's
    // minimum width is the difference between seeing three columns and four.
    //
    // The three numbers share `tableCcWidth`: two digits and a sort arrow is the
    // same shape whether the number is a channel or a controller. `ch` rather
    // than `channel` for the same reason — the column is as wide as its values,
    // and the values are 1 to 16.
    header.addColumn ("ch",      channel, metrics::tableCcWidth,    0, -1, sortable);
    header.addColumn ("MSB",     msb,     metrics::tableCcWidth,    0, -1, sortable);
    header.addColumn ("LSB",     lsb,     metrics::tableCcWidth,    0, -1, sortable);
    header.addColumn ("mode",    mode,    widthForContents ("mode", itemsFor (mode), true), 0, -1, sortable);
    header.addColumn ("min",     minimum, metrics::tableLimitWidth, 0, -1, fixed);
    header.addColumn ("max",     maximum, metrics::tableLimitWidth, 0, -1, fixed);

    tableHeader->onColumnClicked = [this] (int columnId) { cycleSort (columnId); };

    // The frozen column gets a header of its own rather than a pair of buttons
    // dressed as one. It is then drawn by `drawTableHeaderColumn` like the
    // header beside it — same background, same divider, same sort arrow — and
    // nothing here paints a header cell by hand. `ListBox` insets its list by
    // whatever height this component has, so the two sets of rows line up.
    auto frozen = std::make_unique<SortingHeader>();
    frozenHeader = frozen.get();

    // Only the height is the header's own to choose: `ListBox` positions it and
    // gives it the list's width less the outline on each side. The column's
    // width is set in `resized`, since the frozen column is a third of the table
    // and the table's width is not known until then.
    frozen->setSize (0, metrics::tableHeaderHeight);
    frozen->addColumn ("param", param, metrics::pageRowHeight, 0, -1, sortable);
    frozen->onColumnClicked = [this] (int columnId) { cycleSort (columnId); };

    frozenColumn.setHeaderComponent (std::move (frozen));

    // One parameter can be mapped several times, and "view in sidebar" points
    // at all of them at once — so the selection has to be able to hold more
    // than one row. A plain click still selects exactly one; only the modifier
    // keys add, which is what `selectRowsBasedOnModifierKeys` already does.
    table       .setMultipleSelectionEnabled (true);
    frozenColumn.setMultipleSelectionEnabled (true);

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

    // The owner replacing the whole list is not an edit, so there is nothing to
    // undo back to — and undoing *into* a list the owner has since replaced
    // would put back rows it never asked for.
    undoStack.clearQuick();
    redoStack.clearQuick();

    refreshRows();

    if (onHistoryChanged != nullptr)
        onHistoryChanged();
}

void ControllersTable::pushUndo()
{
    undoStack.add (mappings);

    // Oldest first out. A cap rather than no cap because a long session of
    // small edits would otherwise keep every one of them for ever.
    while (undoStack.size() > metrics::undoDepth)
        undoStack.remove (0);

    // A fresh edit is a new branch: whatever was undone is no longer reachable.
    redoStack.clearQuick();
}

void ControllersTable::undo()
{
    if (undoStack.isEmpty())
        return;

    redoStack.add (mappings);
    mappings = undoStack.getLast();
    undoStack.removeLast();

    refreshRows();
    changed();
}

void ControllersTable::redo()
{
    if (redoStack.isEmpty())
        return;

    undoStack.add (mappings);
    mappings = redoStack.getLast();
    redoStack.removeLast();

    refreshRows();
    changed();
}

void ControllersTable::addMapping (controllers::Source source)
{
    Mapping mapping;
    mapping.source = source;

    addMapping (mapping);
}

void ControllersTable::addMapping (controllers::Mapping mapping)
{
    pushUndo();

    // Its limits come from whatever it now points at, so a learned row is
    // usable the moment it appears rather than driving the parameter over its
    // own range. `rangeFor` needs the row to exist, so this is the one place
    // that has to consult the parameter directly.
    if (juce::isPositiveAndBelow (mapping.parameterIndex, parameters.size()))
    {
        const auto& range = parameters[mapping.parameterIndex].range;

        mapping.min = range.lowest;
        mapping.max = range.highest;
    }

    mappings.add (mapping);
    refreshRows();

    // Selected wherever it landed, which under `recent` is the top row and
    // under `alphabetical` is wherever its parameter's name puts it.
    table.selectRow (displayOrder.indexOf (mappings.size() - 1));
    changed();
}

void ControllersTable::selectMappingsFor (int parameterIndex)
{
    // Display rows, not indices into `mappings` — everything the table shows
    // goes through `mappingIndexFor`, and this is what the end-user is being
    // pointed at.
    juce::SparseSet<int> rows;

    for (int row = 0; row < displayOrder.size(); ++row)
    {
        const auto index = mappingIndexFor (row);

        if (index >= 0 && mappings[index].parameterIndex == parameterIndex)
            rows.addRange ({ row, row + 1 });
    }

    table.setSelectedRows (rows, juce::dontSendNotification);

    if (! rows.isEmpty())
        table.scrollToEnsureRowIsOnscreen (rows[0]);

    // setSelectedRows was silent, so the frozen column has not been told; it
    // follows the table's selection through `selectedRowsChanged` otherwise.
    frozenColumn.setSelectedRows (rows, juce::dontSendNotification);
    frozenColumn.scrollToEnsureRowIsOnscreen (rows.isEmpty() ? 0 : rows[0]);
}

void ControllersTable::removeMappingsFor (int parameterIndex)
{
    // Every assignment, not the most recent one. `unlearn` is the undo of
    // `MIDI learn`, and a parameter that has been learned three times is a
    // parameter somebody wants to stop responding — clearing one of the three
    // leaves it still responding and looks like the command failed.
    juce::Array<int> doomed;

    for (int i = 0; i < mappings.size(); ++i)
        if (mappings[i].parameterIndex == parameterIndex)
            doomed.add (i);

    if (doomed.isEmpty())
        return;

    pushUndo();

    // Backwards, so each removal leaves the lower indices where they were.
    for (int i = doomed.size(); --i >= 0;)
        mappings.remove (doomed[i]);

    refreshRows();
    changed();
}

void ControllersTable::removeSelectedMapping()
{
    if (mappings.isEmpty())
        return;

    // **Every** selected row, not the first. The table is multi-select — MIDI
    // learn leaves several rows selected on purpose — so removing one and
    // leaving the rest looks like the button half worked.
    juce::Array<int> doomed;

    const auto selected = table.getSelectedRows();

    for (int i = 0; i < selected.size(); ++i)
        if (const auto index = mappingIndexFor (selected[i]); index >= 0)
            doomed.addIfNotAlreadyThere (index);

    // Nothing selected still removes the last row, so the button is never dead
    // while there is something to remove.
    if (doomed.isEmpty())
        doomed.add (mappings.size() - 1);

    doomed.sort();

    if (! juce::isPositiveAndBelow (doomed.getLast(), mappings.size()))
        return;

    pushUndo();

    // Backwards, so each removal leaves the lower indices where they were.
    for (int i = doomed.size(); --i >= 0;)
        mappings.remove (doomed[i]);

    refreshRows();
    changed();
}

const juce::Drawable* ControllersTable::markerForParameter (int parameterIndex) const
{
    if (! juce::isPositiveAndBelow (parameterIndex, parameters.size()))
        return nullptr;

    switch (parameters[parameterIndex].scope)
    {
        case Scope::perNote: return perNoteMarker.get();
        case Scope::global:  return globalMarker.get();
        case Scope::split:   break;
    }

    return nullptr;
}

const juce::Drawable* ControllersTable::markerFor (int row) const
{
    const auto* parameter = parameterFor (row);

    if (parameter == nullptr)
        return nullptr;

    switch (parameter->scope)
    {
        case Scope::perNote: return perNoteMarker.get();
        case Scope::global:  return globalMarker.get();
        case Scope::split:   break;
    }

    // The usual case is unmarked. A glyph on every row would be a column of
    // furniture saying "normal", and the two that matter would stop standing
    // out — which is the whole job of a marker.
    return nullptr;
}

controllers::Range ControllersTable::rangeFor (int row) const
{
    if (const auto* parameter = parameterFor (row))
        return parameter->range;

    return {};
}

void ControllersTable::changed()
{
    if (onMappingsChanged != nullptr)
        onMappingsChanged();

    if (onHistoryChanged != nullptr)
        onHistoryChanged();
}

bool ControllersTable::isSortable (int columnId) noexcept
{
    return columnId == param || columnId == channel || columnId == msb
        || columnId == lsb   || columnId == mode;
}

void ControllersTable::cycleSort (int columnId)
{
    if (! isSortable (columnId))
        return;

    // Ascending, then descending, then out — and out means the order they were
    // added in. A column that could only be sorted one way or the other would
    // leave no way back to that.
    if (sortColumn != columnId)
    {
        sortColumn   = columnId;
        sortForwards = true;
    }
    else if (sortForwards)
    {
        sortForwards = false;
    }
    else
    {
        sortColumn   = 0;
        sortForwards = true;
    }

    // Both headers are told, so the arrow appears on the one column doing the
    // sorting and on neither of the others. `setSortColumnId` notifies the
    // model rather than calling back through `columnClicked`, and this class
    // does not implement `sortOrderChanged`, so there is no loop to guard.
    frozenHeader->setSortColumnId (sortColumn, sortForwards);
    tableHeader ->setSortColumnId (sortColumn, sortForwards);

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

    applyOrdering();
}

void ControllersTable::applyOrdering()
{
    const auto nameOf = [this] (int index)
    {
        const auto parameterIndex = mappings[index].parameterIndex;

        return juce::isPositiveAndBelow (parameterIndex, parameters.size())
                   ? parameters[parameterIndex].name
                   : juce::String();
    };

    if (sortColumn == 0)
    {
        // No column sorting: newest first. `mappings` is append-ordered, so
        // this is simply the reverse — no timestamps to keep, and the row just
        // added is the one at the top, which is where you are looking.
        std::reverse (displayOrder.begin(), displayOrder.end());
        return;
    }

    // An absent controller number sorts after every present one rather than
    // before, since 127 is the largest a real one can be. "Not set" is not a
    // small number, and putting the blanks on top would bury the rows that
    // actually do something.
    const auto ccKey = [] (const std::optional<int>& cc) { return cc.value_or (metrics::highestCc + 1); };

    const auto ascending = [this, &nameOf, &ccKey] (int a, int b)
    {
        switch (sortColumn)
        {
            // The channel's own value *is* its menu position — see
            // `channelForIndex` — so counting sorts omni on, omni off, 1 … 16.
            case channel: return mappings[a].channel < mappings[b].channel;
            case msb:     return ccKey (mappings[a].msb) < ccKey (mappings[b].msb);
            case lsb:     return ccKey (mappings[a].lsb) < ccKey (mappings[b].lsb);

            // Likewise the mode: the enum's order is the menu's order, so this
            // groups the rows that behave alike and keeps the two that ignore
            // the LSB together at one end.
            case mode:    return (int) mappings[a].mode < (int) mappings[b].mode;
            case param:   return nameOf (a).compareIgnoreCase (nameOf (b)) < 0;
            default:      break;
        }

        return false;
    };

    // Stable both ways round, so rows the column cannot tell apart stay in the
    // order they were added instead of shuffling each time it is re-sorted.
    std::stable_sort (displayOrder.begin(), displayOrder.end(),
                      [this, &ascending] (int a, int b)
                      {
                          return sortForwards ? ascending (a, b) : ascending (b, a);
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

int ControllersTable::widthForContents (const juce::String& title, const juce::StringArray& items,
                                        bool sortable)
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

    // A sortable column has to hold its arrow as well, or the title elides to
    // make room for it the moment the column is sorted — "channel" became
    // "chan…" on the first click. The arrow is half the header's height; see
    // `drawTableHeaderColumn`.
    return widest + metrics::tableTextPadding
         + (sortable ? metrics::tableHeaderHeight / 2 : 0);
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

controllers::Source ControllersTable::sourceFor (int row) const
{
    const auto index = mappingIndexFor (row);
    return index >= 0 ? mappings[index].source : Source::control;
}

bool ControllersTable::ignoresLsb (int row) const
{
    const auto mapping = mappingIndexFor (row);

    if (mapping < 0)
        return false;

    const auto m = mappings[mapping].mode;
    return m == Mode::toggle || m == Mode::increment;
}

bool ControllersTable::isCellInvalid (int row, int columnId) const
{
    if (columnId != msb && columnId != lsb)
        return false;

    const auto index = mappingIndexFor (row);

    if (index < 0)
        return false;

    const auto& mapping = mappings[index];

    // A touch row has no controller numbers at all, so neither cell can be
    // wrong; `paintCell` writes the source's name across the pair instead.
    if (mapping.source != Source::control)
        return false;

    const auto number = columnId == msb ? mapping.msb : mapping.lsb;

    // Empty is not invalid. An LSB is optional, and an MSB that has not been
    // filled in yet is incomplete rather than wrong — `add` leaves one exactly
    // so.
    if (! number.has_value())
        return false;

    if (isCcUnavailable (*number))
        return true;

    // "If a CC is already used as an MSB, it cannot also be used as an LSB. If
    // so, both the MSB and the LSB are marked in red and both rows are ignored"
    // — docs/controllers.md. Any number may be a low byte here, which is the
    // freedom this table adds over the specification, but a number cannot be
    // *both* halves at once: the register would then reset itself on every
    // message it received.
    //
    // Symmetric, so the other row is marked too. Marking only one would leave
    // its partner looking fine while being ignored, which is the worse half of
    // the pair to hide.
    //
    // **Two rows sharing an MSB is still fine.** The same controller aimed at
    // two parameters is a thing people want, and `MidiRouter::process`
    // deliberately does not stop at the first match.
    const auto clashesWith = [this, number] (int otherColumn)
    {
        for (const auto& other : mappings)
        {
            if (other.source != Source::control)
                continue;

            const auto& theirs = otherColumn == msb ? other.msb : other.lsb;

            if (theirs.has_value() && *theirs == *number)
                return true;
        }

        return false;
    };

    return clashesWith (columnId == lsb ? msb : lsb);
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
        case channel: return juce::String (mapping.channel);
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
        case channel: return juce::String (mapping.channel);
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

    pushUndo();

    auto& mapping = mappings.getReference (index);
    const auto trimmed = text.trim();

    // An emptied controller number means "there is none", which is a legal
    // state; an emptied limit is not, so it keeps what it had.
    const auto asCc = [&trimmed]() -> std::optional<int>
    {
        if (trimmed.isEmpty())
            return std::nullopt;

        return juce::jlimit (0, metrics::highestCc, trimmed.getIntValue());
    };

    switch (columnId)
    {
        // A channel is never absent — every mapping listens somewhere — so an
        // emptied cell keeps what it had rather than becoming "no channel".
        case channel: if (trimmed.isNotEmpty())
                          mapping.channel = juce::jlimit (firstChannel, lastChannel,
                                                          trimmed.getIntValue());
                      break;

        case msb:     mapping.msb = asCc(); break;
        case lsb:     mapping.lsb = asCc(); break;
        // Clamped and snapped to what the target can take. A limit outside the
        // parameter's own range would ask a controller to drive it somewhere it
        // cannot go, and a fractional value typed into an integer parameter — a
        // bank number, say — would never be reachable. Both are corrected here
        // and written back, so the cell shows what the plugin actually holds.
        case minimum: if (trimmed.isNotEmpty())
                          mapping.min = rangeFor (row).snap (trimmed.getDoubleValue());
                      break;

        case maximum: if (trimmed.isNotEmpty())
                          mapping.max = rangeFor (row).snap (trimmed.getDoubleValue());
                      break;
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
    return columnId == param ? "param"
         : columnId == mode  ? "mode"
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

    return columnId == mode ? modeNames : juce::StringArray();
}

int ControllersTable::separatorFor (int columnId) const
{
    // One rule, in the mode menu: everything below the line ignores the LSB.
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

    pushUndo();

    auto& mapping = mappings.getReference (mappingIndex);

    switch (columnId)
    {
        case param:
            mapping.parameterIndex = index;

            // A different parameter means a different range, so limits that were
            // legal a moment ago may not be. Re-clamped rather than left: a row
            // pointed at a new target should be usable immediately, not silently
            // driving it out of bounds.
            mapping.min = rangeFor (row).snap (mapping.min);
            mapping.max = rangeFor (row).snap (mapping.max);
            break;

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

void ControllersTable::paintCell (juce::Graphics& g, int row, int columnId, int, int height, bool)
{
    // Every cell holds a widget, except the two that a touch row replaces with
    // one word — those are left empty by `refreshComponentForCell` so that this
    // can draw into them.
    if (columnId != msb && columnId != lsb)
        return;

    const auto source = sourceFor (row);

    if (source == Source::control)
        return;

    // **One word across two columns**, which is what the sketch draws and what
    // a TableListBox has no way of expressing: it does not span columns, and a
    // cell's component is clipped to its own cell. So the same string is drawn
    // into both cells, shifted left by the MSB column's width in the second,
    // and each cell's own clip keeps its half. The halves meet exactly, because
    // the columns are adjacent and the shift is that column's width.
    auto& header = table.getHeader();

    const auto msbWidth = header.getColumnWidth (msb);
    const auto span     = msbWidth + header.getColumnWidth (lsb);
    const auto inset    = metrics::tableCellInset + metrics::labelTextInset;

    // Read-only, and drawn like the monitor's read-outs rather than like an
    // editable cell: there is nothing here to type into.
    g.setColour (findColour (ReadOutField::textColourId).withMultipliedAlpha (shades::readOnly));
    g.setFont (SidebarLookAndFeel::font (metrics::bodyFontHeight));
    g.drawText (sourceNames[static_cast<int> (source)],
                (columnId == msb ? 0 : -msbWidth) + inset, 0, span - inset * 2, height,
                juce::Justification::centredLeft, true);
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

    // A touch row has no controller numbers, so those two cells hold nothing —
    // `paintCell` draws the source's name across the pair instead.
    if ((columnId == msb || columnId == lsb) && sourceFor (row) != Source::control)
    {
        delete existing;
        return nullptr;
    }

    if (columnId == mode)
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

void ControllersTable::listBoxItemClicked (int row, const juce::MouseEvent& e)
{
    // A click in the frozen column, on the space beside a parameter button
    // rather than on it. Routed to the table so that *one* list decides what is
    // selected — clicking there used to move the frozen column's own selection
    // and leave the table's alone, which is how the two came to disagree.
    selectRowFromCell (row, e.mods);
}

void ControllersTable::selectedRowsChanged (int)
{
    // **The whole set, not the last row.** This used to mirror
    // `lastRowSelected` alone, so a multi-row selection — which is what
    // `selectMappingsFor` makes after MIDI learn — showed as one row in the
    // frozen column while the table held several. The two views then disagreed
    // about what was selected, and anything reading the selection got whichever
    // list it happened to ask.
    //
    // Not scrolled to: the frozen column's position is the table's, and moving
    // it here would fight the scroll tie.
    if (syncingSelection)
        return;

    const juce::ScopedValueSetter<bool> guard (syncingSelection, true);

    const auto rows = table.getSelectedRows();

    if (frozenColumn.getSelectedRows() != rows)
        frozenColumn.setSelectedRows (rows, juce::dontSendNotification);
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

    // The tables sit on the panel like a read-out does — recessed, same
    // hairline — rather than introducing a third surface to the page.
    for (auto* list : { (juce::ListBox*) &frozenColumn, (juce::ListBox*) &table })
    {
        list->setColour (juce::ListBox::backgroundColourId, findColour (ReadOutField::backgroundColourId));
        list->setColour (juce::ListBox::outlineColourId,    findColour (ReadOutField::outlineColourId));
        list->setOutlineThickness (metrics::tableOutline);
    }

    // Rebuilt rather than repainted: `replaceColour` bakes the colour into the
    // Drawable, so a theme change has to reparse. This is the caching trap the
    // juce-ui skill describes, and the reason these live on the table instead
    // of on each cell.
    const auto marker = findColour (ReadOutField::textColourId)
                            .withMultipliedAlpha (shades::readOnly);

    perNoteMarker = icons::load (icons::perNote, marker);
    globalMarker  = icons::load (icons::global,  marker);
}

void ControllersTable::paint (juce::Graphics& g)
{
    // **The rectangle the two lists sit on.** Both paint an opaque background of
    // their own — `ListBox::backgroundColourId`, set in `lookAndFeelChanged` —
    // so the only part of this that shows is the seam between them.
    //
    // Filled with the colour their *outlines* are drawn in, so the gap reads as
    // one thick divider inside a single table rather than as space between two
    // widgets: the frozen column's right hairline, this, and the table's left
    // hairline meet as one continuous line.
    //
    // A background rather than a measured strip because there is then no
    // coordinate to keep in step with `resized` — move the columns and the seam
    // follows by itself.
    g.fillAll (findColour (ReadOutField::outlineColourId));
}

void ControllersTable::resized()
{
    auto bounds = getLocalBounds();

    // The frozen column starts below the table's header, so its first row lines
    // up with the table's first row rather than with the header. The strip that
    // leaves free above it is where the sort toggle goes: the parameter column
    // has no title of its own, so the space is there for the taking.
    // The frozen column carries its own header now, and `ListBox` puts it at
    // the top and starts the list beneath it — so the two sets of rows line up
    // without this having to reserve a strip and lay anything out in it.
    // The parameter column takes the row's first third and the gap after it is
    // the grid's own gutter, so the seam between the two lists falls in the same
    // place as the gap between `delete` and `undo` below — at every width, since
    // both are computed from the same six columns. It is also why this is the
    // only column that grows when the panel is dragged wider: the scrolling
    // columns are sized by their contents and keep what they have.
    frozenColumn.setBounds (bounds.removeFromLeft (metrics::rowFirstThird (getWidth())));

    bounds.removeFromLeft (metrics::pageColumnGap);
    table.setBounds (bounds);

    // The header is inside the outline, so its one column is the list's width
    // less the outline on each side. Setting the full width instead would push
    // the column's right-hand divider under the outline, where it is clipped.
    if (frozenHeader != nullptr)
        frozenHeader->setColumnWidth (param, frozenColumn.getWidth() - metrics::tableOutline * 2);
}

} // namespace microtonos::sidebar
