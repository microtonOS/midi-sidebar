# Popups

This is a deliberately restricted set of things that popup.
Prefer these, and if something doesn't fit, please ask.

## Windows

A windows is available [here](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/DialogsDemo.h).
And [another one](https://github.com/juce-framework/JUCE/blob/master/examples/GUI/WindowsDemo.h).
Window frames can both be JUCE styled and use the styling of the native OS.

| name | description |
|------|-------------|
| `DocumentWindow` | A resizable window with a title bar and maximise, minimise and close buttons.
| `DialogWindow` | A `DocumentWindow` simply showing a message that can be clicked or ESC-pressed away. |
| `AlertWindow` | Somewhere in between the two above. The window can contain various widgets, but the user has to respond. |
| `FileChooser` | A file browser. For saving or loading files or directories. Can enable image preview. |

## Other

Other popups include a popup menu (suitable for combo boxes or right-clicking) and speech bubble suitable for displaying further information about a widget such as a value with associated unit.

### `PopupMenu`

Creates and displays a popup-menu.

To show a popup-menu, you create one of these, add some items to it, then call its show() method, which returns the id of the item the user selects.

E.g.

```c++
void MyWidget::mouseDown (const MouseEvent& e)
{
    PopupMenu m;
    m.addItem (1, "item 1");
    m.addItem (2, "item 2");
 
    m.showMenuAsync (PopupMenu::Options(),
                     [] (int result)
                     {
                         if (result == 0)
                         {
                             // user dismissed the menu without picking anything
                         }
                         else if (result == 1)
                         {
                             // user picked item 1
                         }
                         else if (result == 2)
                         {
                             // user picked item 2
                         }
                     });
}
```
Submenus are easy too:

```c++
void MyWidget::mouseDown (const MouseEvent& e)
{
    PopupMenu subMenu;
    subMenu.addItem (1, "item 1");
    subMenu.addItem (2, "item 2");
 
    PopupMenu mainMenu;
    mainMenu.addItem (3, "item 3");
    mainMenu.addSubMenu ("other choices", subMenu);
 
    m.showMenuAsync (...);
}
```

Layouts:
- Simple dropdown menu.
- Dropdown menu with several columns.
- Nested dropdown menu.
- Dropdown menu with custom items rather than plain text.


### `CallOutBox`

A small panel with an arrow pointing at whatever opened it. Good for a control
that has no room where it lives — a fader in a narrow rail, say.

```cpp
auto content = std::make_unique<MyContent>();
content->setSize (w, h);                       // must be sized before launching

juce::CallOutBox::launchAsynchronously (
    std::move (content),
    parent->getLocalArea (&button, button.getLocalBounds()),
    parent);
```

**Always pass a parent component.** With `nullptr` the box goes on the desktop
as its own window, and four things follow, none of them obvious:

- It has no parent component, so `getLookAndFeel()` walks up, finds nothing and
  returns the **default** LookAndFeel. Every custom `ColourId` fails to resolve
  and every drawing override is bypassed, so the same controls come out looking
  like another application's.
- It is outside the editor's component tree, so `createComponentSnapshot` cannot
  see it and a headless screenshot shows nothing.
- It has its own z-order, so anything parented to the editor — a slider's value
  bubble, for instance — ends up stranded *behind* it.
- `areaToPointTo` is then a screen coordinate. With a parent it is
  parent-relative, so use `getLocalArea` rather than `getScreenBounds()`.

The content must be sized before it can be attached, which means it is laid out
while still parentless — so anything it caches from the LookAndFeel is computed
from the wrong one. See the caching trap in [look and feel](look-and-feel.md#colours).

`LookAndFeel_V4` paints a call-out with `widgetBackground` at **0.8 alpha** over
a drop shadow, rimmed with a 2px `outline` stroke. If the box holds controls
lifted out of an opaque panel, that translucency and pale rim will not match
where they came from; override `drawCallOutBoxBackground` to fill opaquely in
the panel's own colour.

A call-out dismisses itself 200ms after opening if its process is not in the
foreground — which matters when rendering headlessly, since the screenshot has
to be taken inside that window.

### Styling a pop-up's contents

Two failures account for nearly every pop-up that comes out looking wrong, and
neither is visible in the pop-up's own code.

**1. Parent to the LookAndFeel, not to the top.** `getTopLevelComponent()` is
the obvious choice and it is wrong: in a standalone or a host, the top level is
a window that knows nothing about your LookAndFeel, so the pop-up resolves none
of your `ColourId`s. Climb to the largest ancestor that still *shares* your
LookAndFeel instead, and stop there:

```cpp
juce::Component* findPopupHost (juce::Component& c)
{
    auto& ours = c.getLookAndFeel();
    juce::Component* host = &c;

    for (auto* p = c.getParentComponent(); p != nullptr; p = p->getParentComponent())
    {
        if (&p->getLookAndFeel() != &ours)   // the boundary of what renders like us
            break;

        host = p;
    }

    return host;
}
```

Write it once and call it; the rule is too easy to forget at the call site. A
headless render does catch this, but only because it wraps the editor in a
host-like window — see [scripts](../scripts/README.md#the-host-window). With the
editor as the top-level component the two answers agree and the broken version
looks perfect, so do not reach for `--no-host-window` while working on a pop-up.

**2. Content built parentless never gets `lookAndFeelChanged`.** A `CallOutBox`
requires its content to be sized before it can be attached, so the content is
constructed and laid out with no parent, and *being attached sends
`parentHierarchyChanged`, not `lookAndFeelChanged`*. Anything a widget does in
`lookAndFeelChanged` — applying colours, re-reading a font, rebuilding a cached
image — therefore never happens inside a pop-up, while the identical widget on
the main surface looks right. Hook both:

```cpp
void parentHierarchyChanged() override { lookAndFeelChanged(); }
```

The two fire in different situations and a pop-up's contents need whichever
comes: attaching to an already-styled parent sends the first, restyling in place
sends the second.

### `BubbleMessageComponent`
A speech-bubble component that displays a short message.

This can be used to show a message with the tail of the speech bubble pointing to a particular component or location on the screen.

`BubbleComponent` has ColourIds for its `background` and its `outline` but
**none for its text**, so anything drawing text into a bubble has to borrow a
colour from elsewhere. `Slider`'s value bubble borrows `TooltipWindow::`
`textColourId`, which collides with the bubble's own background in
`LookAndFeel_V4`'s Light scheme and leaves the read-out invisible. See
[widgets](widgets.md#sliders) for the mechanism and the fix.

See also
BubbleComponent