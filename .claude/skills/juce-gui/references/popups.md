# Popups

## Windows

A windows is available [here](../assets/DialogsDemo.h).
And [another one](../assets/WindowsDemo.h).
Window frames can both be JUCE styled and use the styling of the native OS.

| name | description |
|------|-------------|
| `DocumentWindow` | A resizable window with a title bar and maximise, minimise and close buttons.
| `DialogWindow` | A `DocumentWindow` simply showing a message that can be clicked or ESC-pressed away. |
| `AlertWindow` | Somewhere in between the two above. The window can contain various widgets, but the user has to respond. |
| `FileChooser` | A file browser. For saving or loading files or directories. Can enable image preview. |

## Other

Other popups include a popup menu (suitible for combo boxes or right-clicking) and speech bubble suitible for displaying further information about a widget such as a value with associated unit.

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


### `BubbleMessageComponent`
A speech-bubble component that displays a short message.

This can be used to show a message with the tail of the speech bubble pointing to a particular component or location on the screen.

See also
BubbleComponent