# Views Enhanced Table

Views Enhanced Table provides an enhanced **Table** style plugin for Backdrop Views.

It keeps the core table output and options, but adds a more convenient configuration UI and several additional display controls.

## Features
In the stock Views Table style, the fields (columns) are always rendered in the order in which they are defined at `Fields|Rearrange`. This module aloows the user to reorder the rendering-only order, independent of the query order.

All of the stock Table styling options are available (some in a more intuitive UI), with the following additions:

- **Presentation-only column ordering** (left/right controls).
- **Per-column width** (CSS width value; validated).
- Optional borders:
  - **Horizontal row borders**
  - **Vertical column borders**
  - **Outer table frame**
    - Thickness and color controls.

### Organization
- Style options are grouped into fieldsets:
  - **Columns**, **Styling**, **Mechanics**, **Admin**, plus **Help**.

## Requirements

- Backdrop CMS 1.x
- Views (core module)

## Installation

1. Place the module directory in `modules/enhancedtable` (or `modules/custom/enhancedtable`).
2. Enable **Views Enhanced Table** at `admin/modules`.

## Usage

1. Edit a View.
2. In the display’s **Format** section, choose:
   - **Show:** Table
   - **Style plugin:** **Enhanced table**
3. Click **Settings** next to the style plugin.
4. Use the **Columns** fieldset to reorder, merge, set separators, alignment, widths, and click-sorting.
5. Use **Styling** for borders/striping and row-class options.

## Configuration storage

Views Enhanced Table does not create its own module-level configuration object.

All settings are stored **per display** inside the View’s configuration (the display’s `style_plugin` and `style_options`).

## Disabling / uninstalling

If a View display is configured with the **Enhanced table** style plugin and the module is disabled, Views cannot load the missing plugin and that display may render no output.

Before disabling/uninstalling, switch affected displays back to the core **Table** style plugin.

(Planned improvement: an admin utility to audit/convert displays that use EnhancedTable.)

## Notes

- Column widths and border thickness are validated to conservative CSS length patterns (e.g. `120px`, `10em`, `25%`).
- Separators accept HTML. This option is intended for trusted administrators.

## Issues

- Please report bugs and feature requests in the project's issue queue.
- Include Backdrop core version, PHP version, and steps to reproduce.

## Current Maintainers

- [ericfoy](https://github.com/ericfoy)

## Credits

- This module was created for Backdrop by [ericfoy](https://github.com/ericfoy)
- Sponsored by [Perideo LLC](https://perideo.com).

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory for complete text.

