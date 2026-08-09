#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace microtonos::sidebar::icons
{

/** The one colour every icon below is authored in.

    JUCE's SVG parser does not understand `currentColor`, so the icons carry an
    explicit colour and it is swapped at load time with Drawable::replaceColour.
    Every fill and every stroke uses this same value so a single replacement
    recolours the whole icon.
*/
inline const juce::Colour authoredColour { 0xffffffff };

//==============================================================================
// Icons are from docs/sidebar.md, on a 48x48 viewBox.

/** Presets: a bookmark. Chosen over file/save/folder icons — see the comment in
    docs/sidebar.md. */
inline constexpr const char* presets = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48">
  <path fill="none" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round"
        d="M14.7 4.5h-2.3c-2.2 0-4 1.8-4 4v31c0 2.2 1.8 4 4 4h2.3m19.6-39v11L30.8 12l-3.5 3.5v-11H14.7v39h20.9c2.2 0 4-1.8 4-4v-31c0-2.2-1.8-4-4-4z"/>
</svg>
)SVG";

/** Continuous controllers: knobs wired into a panel. */
inline constexpr const char* controllers = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48">
  <path fill="none" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round"
        d="M36.038 15.505a3.88 3.88 0 1 1-3.723-4.03m-14.84 6.764a3.88 3.88 0 1 1 .296-5.478m17.134 22.631a3.88 3.88 0 1 1 0-5.487m-20.021-1.136a3.88 3.88 0 1 1-3.88 3.88m3.88-.001l-4.35-3.33m26.931 3.501l-5.303-.17M20.283 15.531l-5.57-.22m21.328-3.441l-3.88 3.482"/>
  <rect width="37" height="37" x="5.5" y="5.5" fill="none" stroke="#ffffff"
        stroke-linecap="round" stroke-linejoin="round" rx="4" ry="4"/>
</svg>
)SVG";

/** Tuning: a tuning fork over concentric arcs. */
inline constexpr const char* tuning = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48">
  <path fill="none" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round"
        d="M10.27 22.313c.1-3.295 1.388-6.341 3.63-8.584c1.83-1.828 4.203-3.032 6.827-3.461m17.006 17.036c-.434 2.613-1.635 4.977-3.457 6.799c-2.236 2.236-5.272 3.524-8.556 3.63M5.504 21.446c.207-4.316 1.934-8.292 4.879-11.237c2.53-2.531 5.834-4.172 9.479-4.709m22.637 22.671c-.541 3.634-2.18 6.927-4.704 9.451h0c-2.937 2.937-6.9 4.663-11.203 4.878m-15.409-4.096a1.42 1.42 0 0 0 2.009.008l.007-.008l5.668-5.668l.1.1c2.15.19 5.07 2.139 9.628-2.17l12.85-12.85a1.42 1.42 0 0 0 .006-2.008l-.007-.007l-1.712-1.712a1.42 1.42 0 0 0-2.009-.007l-.007.007L23.76 28.043l-3.8-3.8L33.917 10.29a1.42 1.42 0 0 0 .007-2.008l-.007-.007l-1.712-1.712a1.42 1.42 0 0 0-2.009-.007l-.007.007l-12.85 12.847c-4.31 4.558-2.36 7.478-2.17 9.626l.1.101l-5.667 5.67a1.42 1.42 0 0 0-.007 2.008l.007.007z"/>
</svg>
)SVG";

/** Volume: a speaker with radiating arcs. */
inline constexpr const char* volume = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48">
  <circle cx="24" cy="24" r="21.5" fill="none" stroke="#ffffff"
          stroke-linecap="round" stroke-linejoin="round"/>
  <path fill="none" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round"
        d="M29.442 19.05c1.2.9 2.2 3.2 2 5.7c-.2 1.9-1.2 3.5-2 4.1m-18-9.1v8.7h5.8l7.5 6v-20.9l-7.5 6.2zm21.5-4.6c2.1 1.6 3.8 5.7 3.6 10.2c-.3 3.4-2 6.2-3.6 7.4"/>
</svg>
)SVG";

/** All sound off: an exclamation mark in a circle. */
inline constexpr const char* panic = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48">
  <circle cx="24" cy="34.748" r=".75" fill="#ffffff"/>
  <path fill="none" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round"
        d="M23.975 30.275V12.502"/>
  <circle cx="24" cy="24" r="21.5" fill="none" stroke="#ffffff"
          stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)SVG";

/** Recency: a clock. Used by the controllers page to order mappings by when
    they were added rather than by name. Its `currentColor` strokes are swapped
    for the authored colour like every other icon here. */
inline constexpr const char* clock = R"SVG(
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 48 48">
  <path fill="none" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round"
        d="M24 2.5A21.5 21.5 0 1 1 2.5 24A21.51 21.51 0 0 1 24 2.5"/>
  <circle cx="24" cy="24" r="2.5" fill="none" stroke="#ffffff"
          stroke-linecap="round" stroke-linejoin="round"/>
  <path fill="none" stroke="#ffffff" stroke-linecap="round" stroke-linejoin="round"
        d="M24 21.5V11.44m2.1 13.91l12.2 7.8"/>
</svg>
)SVG";

//==============================================================================
/** Parses one of the strings above and recolours it.

    Returns a Drawable rather than a Component: since JUCE 9 a Drawable is not a
    Component, so it cannot be added as a child. DrawableButton takes Drawables
    directly, which is what the rail uses; anywhere else it needs to be wrapped
    in a DrawableComponent.
*/
inline std::unique_ptr<juce::Drawable> load (const char* svg, juce::Colour colour)
{
    auto drawable = juce::Drawable::createFromSVGString (juce::String::fromUTF8 (svg));

    if (drawable != nullptr)
        drawable->replaceColour (authoredColour, colour);

    return drawable;
}

} // namespace microtonos::sidebar::icons
