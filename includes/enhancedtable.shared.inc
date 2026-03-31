<?php
/**
 * Shared helpers for EnhancedTable table styling.
 */

/**
 * Build a selector for this view + display wrapper.
 *
 * @param object $view
 *   The Views object.
 *
 * @return string
 *   CSS selector for the view wrapper.
 */
function enhancedtable_get_view_selector($view) {
  return '.view-id-' . $view->name . '.view-display-id-' . $view->current_display;
}

/**
 * Selectors for ONLY this display's own root table.
 *
 * This intentionally scopes to the direct root table of this view display.
 * Some Views displays render the table directly under .view-content, while
 * form-enabled displays render it under .view-content > .views-form > form.
 * Both paths are valid roots for this display and neither should affect any
 * nested/embedded subview tables rendered inside a parent row.
 *
 * @param object $view
 *   The Views object.
 *
 * @return array
 *   CSS selectors for the root table of this view display.
 */
function enhancedtable_get_root_table_selectors($view) {
  $base = enhancedtable_get_view_selector($view) . ' > .view-content';

  return array(
    // Plain Views table directly under .view-content.
    $base . ' > table.views-table',

    // Form-enabled Views that retain the form wrapper.
    $base . ' > .views-form > form > table.views-table',
    $base . ' > .views-form > form > div > table.views-table',

    // Embedded/nested form Views can lose the <form> wrapper in the final DOM
    // because nested forms are invalid HTML and the browser normalizes them.
    $base . ' > .views-form > div > table.views-table',
  );
}

/**
 * Selector string for this display's own root table(s).
 *
 * @param object $view
 *   The Views object.
 *
 * @return string
 *   Comma-separated CSS selectors for the root table(s) of this view display.
 */
function enhancedtable_get_root_table_selector($view) {
  return implode(', ', enhancedtable_get_root_table_selectors($view));
}

/**
 * Build inline CSS for EnhancedTable table options.
 *
 * @param array $options
 *   The style plugin options.
 * @param string $table_selector
 *   The CSS selector for the target table.
 *
 * @return string
 *   Inline CSS, or empty string.
 */
function enhancedtable_build_table_css(array $options, $table_selector) {
  $css = '';
  $table_selectors = preg_split('/\s*,\s*/', (string) $table_selector);
  $table_selectors = array_values(array_filter(array_map('trim', $table_selectors)));

  $build_selector = function (array $suffixes) use ($table_selectors) {
    $parts = array();

    foreach ($table_selectors as $root) {
      foreach ($suffixes as $suffix) {
        $parts[] = $root . $suffix;
      }
    }

    return implode(', ', $parts);
  };

  // Whenever any border style is enabled, force collapsed borders so cell borders
  // render consistently in both normal tables and subview tables.
  if (!empty($options['row_borders']) || !empty($options['column_borders']) || !empty($options['frame_border'])) {
    $css .= $build_selector(array(
      '',
    )) . " { border-collapse: collapse; border-spacing: 0; }\n";
  }

  // -------------------------------------------------------------------
  // Column widths: style_options[column_widths][FIELD] => CSS width.
  // -------------------------------------------------------------------
  if (!empty($options['column_widths']) && is_array($options['column_widths'])) {
    foreach ($options['column_widths'] as $field => $width) {
      $width = trim($width);
      if ($width === '') {
        continue;
      }

      // Conservative whitelist for width values:
      //   12, 12px, 10em, 8rem, 25%, 15ch, 10ex
      if (!preg_match('/^\d+(\.\d+)?(px|em|rem|%|ch|ex)?$/', $width)) {
        // Ignore anything outside this pattern.
        continue;
      }

      // Views table cells are "views-field-FIELDNAME".
      // Scope ONLY to the direct header/body cells of this table, so nested
      // subview tables are not affected.
      $field_class = backdrop_clean_css_identifier($field);

      $selector = $build_selector(array(
        ' > thead > tr > th.views-field-' . $field_class,
        ' > tbody > tr > td.views-field-' . $field_class,
      ));

      $css .= $selector . " { width: {$width}; }\n";
    }
  }

  // -------------------------------------------------------------------
  // Column alignment.
  // -------------------------------------------------------------------
  if (!empty($options['info']) && is_array($options['info'])) {
    foreach ($options['info'] as $field => $info) {
      if (empty($info['align'])) {
        continue;
      }

      $align_class = (string) $info['align'];
      $text_align = '';
      if (strpos($align_class, 'left') !== FALSE) {
        $text_align = 'left';
      }
      elseif (strpos($align_class, 'center') !== FALSE) {
        $text_align = 'center';
      }
      elseif (strpos($align_class, 'right') !== FALSE) {
        $text_align = 'right';
      }

      if ($text_align === '') {
        continue;
      }

      $field_class = backdrop_clean_css_identifier($field);
      $selector = $build_selector(array(
        ' > thead > tr > th.views-field-' . $field_class,
        ' > tbody > tr > td.views-field-' . $field_class,
      ));

      $css .= $selector . " { text-align: {$text_align}; }\n";
    }
  }

  // -------------------------------------------------------------------
  // Horizontal row borders.
  // -------------------------------------------------------------------
  if (!empty($options['row_borders'])) {
    // Thickness and color with simple validation/fallback.
    $thk = !empty($options['row_border_thickness'])
      ? trim($options['row_border_thickness'])
      : '1px';
    // Allow plain number to be treated as px.
    if (preg_match('/^\d+(\.\d+)?$/', $thk)) {
      $thk .= 'px';
    }
    // Accept standard CSS length units.
    if (!preg_match('/^\d+(\.\d+)?(px|em|rem|%|ch|ex)?$/', $thk)) {
      $thk = '1px';
    }

    $color = !empty($options['row_border_color'])
      ? trim($options['row_border_color'])
      : '#cccccc';

    $border = $thk . ' solid ' . $color;

    $css .= $build_selector(array(
      ' > tbody > tr + tr > td',
    )) . " { border-top: {$border}; }\n";
  }

  // -------------------------------------------------------------------
  // Vertical column borders.
  // -------------------------------------------------------------------
  if (!empty($options['column_borders'])) {
    $thk = !empty($options['column_border_thickness'])
      ? trim($options['column_border_thickness'])
      : '1px';
    if (preg_match('/^\d+(\.\d+)?$/', $thk)) {
      $thk .= 'px';
    }
    if (!preg_match('/^\d+(\.\d+)?(px|em|rem|%|ch|ex)?$/', $thk)) {
      $thk = '1px';
    }

    $color = !empty($options['column_border_color'])
      ? trim($options['column_border_color'])
      : '#cccccc';

    $border = $thk . ' solid ' . $color;

    $css .= $build_selector(array(
      ' > thead > tr > th + th',
      ' > tbody > tr > td + td',
    )) . " { border-left: {$border}; }\n";
  }

  // -------------------------------------------------------------------
  // Outer table frame border.
  // -------------------------------------------------------------------
  if (!empty($options['frame_border'])) {
    $thk = !empty($options['frame_border_thickness'])
      ? trim($options['frame_border_thickness'])
      : '1px';
    if (preg_match('/^\d+(\.\d+)?$/', $thk)) {
      $thk .= 'px';
    }
    if (!preg_match('/^\d+(\.\d+)?(px|em|rem|%|ch|ex)?$/', $thk)) {
      $thk = '1px';
    }

    $color = !empty($options['frame_border_color'])
      ? trim($options['frame_border_color'])
      : '#cccccc';

    $border = $thk . ' solid ' . $color;

    $css .= $build_selector(array(
      '',
    )) . " { border: {$border}; }\n";
      
  }

  return $css;
}
