# Changelog

This file lists the changes of boxe on top of the Godot version it is based on.
For the changes of Godot itself, see the
[Godot 4.7.2 changelog](https://github.com/godotengine/godot/blob/4.7.2-stable/CHANGELOG.md).

## 1.0 - 2026-10-24

Based on Godot 4.7.2.

### Added

- Style transitions: `StyleBox` has `transition_duration`, `transition_type` and `transition_ease`, and controls blend
  their style boxes, font colors and icon colors between states, like `LinkButton` and the close button of embedded windows.
- `StyleBoxLayered` with fill, outline and shadow layers. Checks, switches, slider grabbers and `Tree` checks can be
  drawn with style boxes.
- Theme sounds: a sound data type in `Theme` and `Node.sound_bus`. Buttons, sliders, spin boxes, line and text edits,
  item lists, trees, tabs, menus, menu bars, foldable and split containers, virtual joysticks, popups and dialogs play
  them, one sound for each user action.
- `Control.interactable`, raycast padding, composite properties and composite text effects.
- BBCode in `Label`, buttons, tabs, item lists, trees, menus, tooltips, window titles and dialogs, and the `[spacer]` tag.
- Localization: `text_arguments` with plural forms, localized textures, audio streams and fonts, a Localization dock to
  edit CSV and PO translations, translation keys picked in the inspector, and the selected locale kept between runs.
- Mobile: `MarginContainer.safe_area`, `DisplayServer.safe_area_changed`, `orientation_changed` on 180° rotations on
  Android, and the `application/config/version_code` and `display/window/android/refresh_rate` project settings.
- Embedded windows appear with an animation and popups have a theme shadow. `popup_separation` for buttons that open
  popups, `color_corner_radius` for `ColorPickerButton` and `editor_panel` for the cell editor of `Tree`.
- The default preview of the Theme editor shows every themed control.

### Fixed

- The modulation of a `CanvasGroup` and its parents is applied once.
- `Tree` column titles no longer grow taller after their text changes.
- The Swappy frame pacing modes on Android were inverted.
- Embedded windows no longer show a dark line along their edges at fractional scales.
- The `EditorFileSystem` singleton is cleared when it's freed.
