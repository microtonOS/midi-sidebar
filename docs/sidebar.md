# Sidebar

The sidebar exists in both a collapsed and an expanded state.[^hypothesis]
The developer can pick a speed for the animation between collapsed and expanded states.
The developer can choose whether the panel should be on the left or the right.
In its collapsed state it looks like in Figure 1.

<div style="display: grid; grid-template-columns: repeat(1, 1fr); gap: 15px; width: 50%; max-width: 600px;">
    <!-- I went with a bookmarks icon for the presets, I also considered file, save, and folder icons -->
    <div>
        <button style="width:3.5em; height:3.5em">
            <svg xmlns="http://www.w3.org/2000/svg" width="2em" height="2em" viewBox="0 0 48 48"><path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M14.7 4.5h-2.3c-2.2 0-4 1.8-4 4v31c0 2.2 1.8 4 4 4h2.3m19.6-39v11L30.8 12l-3.5 3.5v-11H14.7v39h20.9c2.2 0 4-1.8 4-4v-31c0-2.2-1.8-4-4-4z"/></svg>
        </button>
    </div>
    <!-- Always a bit uncertain whetehr to have this only as a rightlick menu or as both sideclick and sidebar --->
    <div>
        <button style="width:3.5em; height:3.5em">
            <svg xmlns="http://www.w3.org/2000/svg" width="2em" height="2em" viewBox="0 0 48 48"><path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M36.038 15.505a3.88 3.88 0 1 1-3.723-4.03m-14.84 6.764a3.88 3.88 0 1 1 .296-5.478m17.134 22.631a3.88 3.88 0 1 1 0-5.487m-20.021-1.136a3.88 3.88 0 1 1-3.88 3.88m3.88-.001l-4.35-3.33m26.931 3.501l-5.303-.17M20.283 15.531l-5.57-.22m21.328-3.441l-3.88 3.482"/><rect width="37" height="37" x="5.5" y="5.5" fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" rx="4" ry="4"/></svg>
        </button>
    </div>
    <div>
        <button style="width:3.5em; height:3.5em">
            <svg xmlns="http://www.w3.org/2000/svg" width="2em" height="2em" viewBox="0 0 48 48"><path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M10.27 22.313c.1-3.295 1.388-6.341 3.63-8.584c1.83-1.828 4.203-3.032 6.827-3.461m17.006 17.036c-.434 2.613-1.635 4.977-3.457 6.799c-2.236 2.236-5.272 3.524-8.556 3.63M5.504 21.446c.207-4.316 1.934-8.292 4.879-11.237c2.53-2.531 5.834-4.172 9.479-4.709m22.637 22.671c-.541 3.634-2.18 6.927-4.704 9.451h0c-2.937 2.937-6.9 4.663-11.203 4.878m-15.409-4.096a1.42 1.42 0 0 0 2.009.008l.007-.008l5.668-5.668l.1.1c2.15.19 5.07 2.139 9.628-2.17l12.85-12.85a1.42 1.42 0 0 0 .006-2.008l-.007-.007l-1.712-1.712a1.42 1.42 0 0 0-2.009-.007l-.007.007L23.76 28.043l-3.8-3.8L33.917 10.29a1.42 1.42 0 0 0 .007-2.008l-.007-.007l-1.712-1.712a1.42 1.42 0 0 0-2.009-.007l-.007.007l-12.85 12.847c-4.31 4.558-2.36 7.478-2.17 9.626l.1.101l-5.667 5.67a1.42 1.42 0 0 0-.007 2.008l.007.007z" /></svg>
        </button>
    </div>
    <!-- A future version may also have may also have a button for tempo-related settings.
    <div>
        <button style="width:3.5em; height:3.5em">
            <svg xmlns="http://www.w3.org/2000/svg" width="2em" height="2em" viewBox="0 0 48 48"><path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M40.838 17.352a3.03 3.03 0 1 1-4.893-3.574a3.03 3.03 0 0 1 4.893 3.574m-17.116 18.3L36.605 18.01m3.574-4.893l1.699-2.326M21.27 23.503h4.848m-3.636-2.827h2.424m-3.636 8.483h4.848m-3.636-2.828h2.424m-3.636-8.483h4.848m-3.636-2.828h2.424m8.584 2.367l-1.918-5.397c-1.985-4.567-3.69-7.49-7.826-7.49s-6.687 2.854-7.93 7.49L6.123 39.258V43.5h35.146v-4.242L35.81 23.911m-2.818 3.853l2.821 7.858H11.576l8.483-23.632h7.272l3.327 9.27"/></svg>
        </button>
    </div>
    -->
    <div>
        <!-- Difficult to think of an icon for the abstract concept of channel management. What I have considered beyound "CH" is a filter icon as you are filtering different channels as well as an fm radio tuner or remote control for dialling in channels. Make the linewidths of the letters consistent with the other (Arcticon) icons. Arctiton even have letter icons from which you could build a CH icon. -->
        <button style="width:3.5em; height:3.5em">CH</button>
    </div>
    <div>
        <button style="width:3.5em; height:3.5em">
            <svg xmlns="http://www.w3.org/2000/svg" width="2em" height="2em" viewBox="0 0 48 48"><circle cx="24" cy="24" r="21.5" fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/><path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M29.442 19.05c1.2.9 2.2 3.2 2 5.7c-.2 1.9-1.2 3.5-2 4.1m-18-9.1v8.7h5.8l7.5 6v-20.9l-7.5 6.2zm21.5-4.6c2.1 1.6 3.8 5.7 3.6 10.2c-.3 3.4-2 6.2-3.6 7.4"/></svg>
        </button>
    </div>
    <div>
        <button style="width:3.5em; height:3.5em">
            <svg xmlns="http://www.w3.org/2000/svg" width="2em" height="2em" viewBox="0 0 48 48"><circle cx="24" cy="34.748" r=".75" fill="currentColor"/><path fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round" d="M23.975 30.275V12.502"/><circle cx="24" cy="24" r="21.5" fill="none" stroke="currentColor" stroke-linecap="round" stroke-linejoin="round"/></svg>
        </button>
    </div>
</div>

**Figure 1.**

From top to bottom.

1. The top button is the [presets menu](presets.md).
2. The second buttons is the [controllers](controllers.md) menu.
3. The third button is the [tuning menu](tuning.md).
4. The fourth button is the [channel management](channels.md) menu. 

Either or none of these can be active. The menu is collapsed if and only if none.
You can toggle between them if expanded.

5. The fifth button is the volume button. When pressed a volume slider appears. In addition there is a meter for the volume. The slider and the meter are parallel and use a common scale.[^surge] CC7 is hardcoded for volume. See [Volume and metering](#volume-and-metering).
6. All sound off (and implicitly all notes off) button. CC120 is hardcoded to this control.


## Placement and expansion

The rail rides the *inner* edge of the panel, so it always sits between the
panel and the host's content: with the sidebar on the left, an open page reads
panel, then rail, then content. This is the hypothes.is arrangement[^hypothesis]
rather than the browser-sidebar one, where the rail stays pinned to the window
edge. The cost, accepted deliberately, is that all five buttons move by the
panel's width whenever a page opens or closes.

The panel **lies over** the host's UI rather than pushing it aside. Only the
rail's width is permanently reserved; what the panel covers while open is hidden
on purpose. This is what lets the sidebar be dropped into an existing plugin
without that plugin's layout having to respond to it — its bounds never change.


## Heights

There is one arrangement at every height: the three page buttons at the top, the
volume control and the all-sound-off button anchored to the bottom, and one
flexible gap between them that absorbs whatever is left over. Growing the window
only grows that gap.

Only the volume control changes with height:

| window height | volume control |
|---|---|
| below the breakpoint | an icon button; pressing it opens the fader and meter in a pop-up |
| at or above it | the fader and meter inline in the rail |

Both the minimum height and the breakpoint are **derived from the content**, not
chosen: the minimum is the height at which the five controls and their gaps
exactly fit, and the breakpoint is the same sum with the fader-and-meter strip
in place of the volume button. Changing a button size or a gap moves both.


## Volume and metering

The fader and the meter are drawn to the same rectangle, so they can be read
against each other. That only means something if they share a scale, so:

- **Both are in decibels**, from 0 dB at the top down to a floor of −60 dB,
  below which the fader is off and the meter is empty. Not a linear amplitude
  or a percentage: MIDI defines CC7 as a dB curve — the General MIDI
  specifications give `dB = 40 × log₁₀(cc/127)`, so CC7 64 is about −12 dB
  rather than half volume — and a meter is inherently logarithmic. A linear
  scale would crush everything below about −20 dB into the bottom pixel.
- **The meter is post-fader.** It shows what actually leaves the plugin, so
  pulling the fader down takes the meter with it. The fader then reads as a
  ceiling with the signal beneath it, and the space between them is headroom.
  (Note that Surge XT's meter, cited above, is *not* coupled to a fader this
  way — the shared scale is this project's own idea, not something borrowed.)
- **The meter is stereo**, two bars side by side, left then right. A mono source
  feeds both.
- The fader's value appears in a bubble while it is being dragged rather than in
  a permanent read-out, to one decimal place, as `-6.0 dB` or `-∞ dB`.

The two are inset from the edges of their column so that **the distance from the
column's left edge to the fader's track equals the distance from its right edge
to the outer meter bar**. That is measured from the drawn marks, not from the
widgets' bounding boxes — the fader's box is wider than its track, because the
thumb overhangs it, so laying the boxes out symmetrically leaves the visible
marks lopsided.




[^hypothesis]: The design is inspired by the [hypothes.is](https://web.hypothes.is/) browser addon.
[^surge]: This is similar to Surge XT but here it is vertical instead of horizontal.