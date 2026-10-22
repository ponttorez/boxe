<p align="center">
  <b>Godot, polished for UI.</b><br>
  <a href="https://ponttorez.github.io/boxe/">Play demo</a> ·
  <a href="#features">Features</a> ·
  <a href="#building">Building</a> ·
  <a href="CHANGELOG.md">Changelog</a>
</p>

**boxe** is a fork of [Godot Engine](https://github.com/godotengine/godot) 4.7.2 that fixes the papercuts of building game and
app interfaces: controls that snap between states, small buttons that are hard to tap, texts that can't be formatted
or pluralized, silent menus and screens that gray out while they load.

The new features are opt-in: a project looks and behaves like in Godot until it uses a new property, resource or
setting, and boxe opens Godot 4.7 projects as they are. A few [fixes](#fixes) change behavior that was broken in Godot.

<p align="center">
  <img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/motion.gif" alt="A short video of the problems of stock Godot next to their fixes in boxe" width="840">
</p>

## Features

| | |
|---|---|
| [Style transitions](#style-transitions) | Style boxes and colors blend between the states of controls. |
| [Layered style boxes](#layered-style-boxes) | Shadows, depth, rims, glows and pixel art from stacked layers. |
| [Theme sounds](#theme-sounds) | Controls play hover, press, toggle and typing sounds from the theme. |
| [Interactable](#interactable) | Block input without changing the look of a control. |
| [Hit areas](#hit-areas) | Grow or shrink the area that takes mouse and touch input. |
| [Composites](#composites) | Fade groups of controls and text effects as one image. |
| [BBCode everywhere](#bbcode-everywhere) | Formatted text in labels, buttons, tabs, lists, menus, tooltips and dialogs. |
| [Localization](#localization) | A dock to edit CSV and PO translations, plurals in texts and localized resources. |
| [Mobile](#mobile) | Safe areas, 180° rotations, refresh rate and version code on Android. |
| [Fixes](#fixes) | Fixes that change the behavior of stock Godot. |

### Style transitions

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/transitions.gif" alt="Buttons without and with transitions, and buttons with different easing" width="840">

**What.** `StyleBox` has `transition_duration`, `transition_type` and `transition_ease`. When a control changes state,
for example when a button is hovered, its style box, font color and icon color blend into the new state over that time.

**Why.** In Godot, controls switch their style at once, so interfaces feel stiff unless every state change is
animated by hand with tweens, which the theme can't describe.

**How.** Set a transition duration on the style boxes of the theme. Transitions work for every control that draws
theme style boxes: buttons, line edits, tabs, lists, trees, menus, sliders. Leaving a state midway takes only the part
of the transition that was already played, so quick hovers never lag behind. `StyleBoxFlat` and `StyleBoxLayered` blend
their properties. Colors never overshoot, even with the Elastic and Back curves.

### Layered style boxes

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/layered.gif" alt="Raised, glass, neon, pixel, skewed and ring buttons" width="840">

**What.** `StyleBoxLayered` draws a list of `StyleBoxLayer`s: fills, outlines and soft shadows, each with its own color,
offset, margins and corner radius. Corners are smooth, or stepped like pixel art.

**Why.** `StyleBoxFlat` draws one fill, one border and one shadow. Raised buttons with a depth, inner rims, glows or
pixel art needed textures, which don't scale and can't animate.

**How.** Create a `StyleBoxLayered` in the inspector and add layers. Style boxes with the same layers blend layer by
layer during transitions, so a pressed button can push its face down into its depth.

### Theme sounds

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/sounds.png" alt="Controls playing theme sounds, and a panel using another bus" width="840">

**What.** Themes have a sound data type. Buttons, sliders, spin boxes, line and text edits, item lists, trees, tabs,
menu bars, foldable and split containers and virtual joysticks play sounds when the user hovers, presses, toggles,
selects, drags or types, and popups, menus and dialogs play sounds when they open and close. Changes made from code stay
silent.

**Why.** In Godot, every interface sound needs a script on every control, or a global script that guesses what the
user did.

**How.** Add sounds to the theme like colors and fonts, for example `hover` and `pressed` of `BaseButton`.
Controls can override them. Sounds play on the bus of `Node.sound_bus`, which children inherit, or of the
`gui/theme/sound_bus` project setting. `Control.play_theme_sound()` plays a theme sound from a script.

### Interactable

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/interactable.gif" alt="A disabled form next to a non-interactable one" width="840">

**What.** `Control.interactable = false` blocks input for a control and all its children, but keeps their look.

**Why.** Locking a screen while it loads meant disabling every control, which grays them out and makes the screen
flash. Presses also went through to the controls behind.

**How.** Set `interactable` on the parent of what to lock. Presses are consumed and play the `denied` theme sound,
hover and focus are not taken, and a press in progress ends instead of leaving the button pressed.

### Hit areas

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/hit_areas.gif" alt="Small close buttons with and without a larger hit area" width="840">

**What.** `raycast_padding_left`, `_top`, `_right` and `_bottom` move each edge of the area of a control that takes
mouse and touch input, without changing its size or drawing.

**Why.** Small icons are hard to hit with a finger. In Godot, a larger hit area needs a larger control and a layout
that works around it.

**How.** Negative values grow the area, positive values shrink it. The editor outlines the area of the selected control.

### Composites

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/composites.gif" alt="A group and a text with effects faded without and with composites" width="840">

**What.** `Control.composite_enabled` draws a control and its children as one image, like a `CanvasGroup`.
`LabelSettings.composite_effects` draws the outline and the shadows of a text as layers.

**Why.** When a group of overlapping controls fades, each control fades on its own and the overlaps show through.
Translucent outlines darken where glyphs overlap and show through the text.

**How.** Enable `composite_enabled` on the parent of the group. Its `material` applies to the whole image, with
`composite_margin` around it for outlines and shadows.

### BBCode everywhere

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/bbcode.gif" alt="Formatted labels, buttons, a typewriter and trimmed text" width="840">

**What.** `Label`, `Button`, `LinkButton`, `TabBar`, `TabContainer`, `ItemList`, `Tree`, `PopupMenu`, `MenuBar`, the
title of `FoldableContainer`, tooltips, window titles and dialog texts can be BBCode, with the tags of `RichTextLabel`
except tables and effects. The new `[spacer]` tag pushes text to the end of its line, or centers it.

**Why.** Formatting a single word meant replacing a label with a `RichTextLabel`, which sizes, wraps and trims
differently, and buttons, tabs and lists couldn't be formatted at all.

**How.** Enable `bbcode_enabled`. Labels keep autowrap, ellipsis, `visible_characters` and `LabelSettings`, links emit
`meta_clicked` and `[hint]` shows tooltips. Bold and italic fonts come from the theme, or are synthesized.

### Localization

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/localization.gif" alt="The demo switching between English and Russian" width="840">

**What.** The Localization dock edits the CSV and PO translations of the project as a table of keys and locales, with
plural forms, contexts and comments. Edits are saved to the files at once, keeping the rest of them as they were, and the
editor shows the new texts right away. `text_arguments` fills `{name}` placeholders and picks the plural forms of
`{count|apple|apples}` by the rules of each language. `LocalizedTexture2D`, `LocalizedAudioStream` and `LocalizedFont`
use the resource of the current locale.

**Why.** Translations in Godot are CSV or PO files edited outside the engine, and plural forms need `tr_n()` in code
for every text. Textures, sounds and fonts that change with the language needed scripts.

**How.** Open the Localization dock: it lists the translations of Project Settings > Localization > Translations, and
creates new CSV or PO files. Add the texts of the scenes and scripts listed in Template Generation, and pick keys from
the inspector of any text. Set `text_arguments` on labels and buttons. With `remember_selected_locale`, the chosen
language is restored at the next launch.

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/editor_localization_dock.png" alt="The Localization dock" width="840">

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/editor_key_picker.png" alt="Picking a key in the inspector" width="410">

### Mobile

<img src="https://raw.githubusercontent.com/ponttorez/boxe/pages/media/phone.png" alt="The demo on a phone screen" width="260" align="right">

**What.** `MarginContainer.safe_area` keeps content out of display cutouts and system bars, or extends a background
under them. `DisplayServer.safe_area_changed` tells when they move. On Android, `orientation_changed` fires on 180°
rotations too, `display/window/android/refresh_rate` asks for the highest refresh rate, and
`application/config/version_code` sets the version code of exports.

**Why.** Godot left safe areas to scripts, didn't notice when a phone was turned upside down, and many phones kept
Godot apps at 60 Hz.

**How.** Put the interface in a `MarginContainer` with `safe_area = Inside`, and set the project settings.

<br clear="right">

### Fixes

These change the behavior of stock Godot, where it was broken:

- The modulation of a `CanvasGroup` and its parents is applied once, so an alpha of 0.5 gives 0.5 instead of 0.25.
- `Tree` column titles no longer grow taller after their text changes.
- On Android, the Swappy frame pacing modes, which were inverted, work as documented.

## Try it

- **In the browser:** [ponttorez.github.io/boxe](https://ponttorez.github.io/boxe/) runs the demo project, built with this fork.
- **In the editor:** [build the editor](#building), [get the demo](#demo) and open it. Every feature has its own
  section, and the scenes are regular scenes to look at and change.

## Building

boxe builds like Godot, with [SCons](https://scons.org):

```bash
scons platform=linuxbsd target=editor
```

See the [official guide to building Godot](https://docs.godotengine.org/en/stable/engine_details/development/compiling/index.html)
for the dependencies and the commands of each platform. The web build of the demo uses a single-threaded template:

```bash
scons platform=web target=template_release threads=no
```

### Demo

The demo project is in the [`demo`](https://github.com/ponttorez/boxe/tree/demo) branch. It goes into a `demo` folder
of this repository, which Git ignores here, as a worktree of that branch:

```bash
git fetch origin demo
```

```bash
git worktree add demo demo
```

The demo, its web build and every GIF and screenshot of this page are made by one script, see
[`tools/media.py`](https://github.com/ponttorez/boxe/blob/demo/tools/media.py) in the demo:

```bash
python3 demo/tools/media.py all
```

## License

boxe is distributed under the [MIT license](LICENSE.txt), like Godot Engine. See [COPYRIGHT.txt](COPYRIGHT.txt) for
the licenses of third-party code. The demo lists the fonts and icons it uses, with their licenses, and the changelog on its About page.
