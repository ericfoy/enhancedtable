# Views Enhanced Table

 **Views Enhanced Table** provides an enhanced **Table** style plugin for Backdrop Views.

It keeps the core table output and options, but adds a more convenient configuration UI and several additional display controls.

## Features
In the stock Views Table style, the fields (columns) are always rendered in the order in which they are defined at `Fields|Rearrange`. This module allows the user to reorder the rendering-only order, independent of the query order.

All of the stock Table styling options are available (some in a more intuitive UI), with the following additions:

- **Presentation-only column ordering** (left/right controls).
- **Inline titles for merged fields** (optional label prefix; respects the field’s “use colon after label” setting).
- **Per-column width** (CSS width value; validated).
- Optional borders with thickness and color controls:
  - **Horizontal row borders**
  - **Vertical column borders**
  - **Outer table frame**

## Requirements

- Backdrop CMS 1.x
- Views (core module)

## Installation
The easiest way to install the module on an existing Backdrop CMS site is to navigate to `admin/modules/install` and search for "views enhanced table". Otherwise,
- Install this module using the official Backdrop CMS instructions at
  https://docs.backdropcms.org/documentation/extend-with-modules.
- Enable the module at:  
   **Administration → Functionality → List modules**

## Usage

1. Edit a View.
2. In the display’s **Format** section, choose:
   - **Show:** Table
   - **Style plugin:** **Table (Enhanced)**
3. Click **Settings** next to the style plugin.
4. Use the **Columns** fieldset to reorder, merge, set separators, alignment, widths, and click-sorting.
5. Use **Styling** for borders/striping and row-class options.

## Configuration storage

Enhanced Table does not create a separate, module-level configuration object (with the exception noted below).

All settings are stored per View display, inside the View’s configuration (`display_options`), primarily in `style_plugin` and `style_options`.

## Disabling / uninstalling

Enhanced Table includes a safety mechanism: when the module is **disabled**, it automatically switches any displays using the Enhanced Table style back to the core **Table** style so the View continues to render (albit, with some styling no longer working), and it will create a module-level configuration object for storage of a list of views formerly using its styles. When the module is **re-enabled**, it will attempt to restore those displays to Enhanced Table (best effort).

If you plan to uninstall the module permanently, verify affected Views are using the core **Table** style before uninstalling.

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

