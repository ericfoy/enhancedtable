<?php
/**
 * File: enhancedtable/views/enhancedtable_plugin_style_table.inc
 */

class enhancedtable_plugin_style_table extends views_plugin_style_table {

  /**
   * Whether inline-title prefixes have already been applied to rendered fields.
   *
   * Views may call render_fields() multiple times during a single build.
   * Since Views caches rendered output in $this->rendered_fields, we must
   * ensure we only prefix once to avoid duplicate titles.
   *
   * @var bool
   */
  protected $enhancedtable_inline_titles_applied = FALSE;

  /**
   * Inherit the standard table options.
   */
  public function option_definition() {
    $options = parent::option_definition();

    // Presentation-only field order (left-to-right). Values are numeric weights
    // keyed by field ID.
    $options['field_order'] = array(
      'default' => array(),
    );

    // Presentation-only "merge into the column to the left" flags.
    // Keyed by field ID.
    $options['merge_left'] = array(
      'default' => array(),
    );

    // Presentation-only: when a field is merged into the column to its left,
    // optionally prefix that field's output with its label ("Inline title").
    // Keyed by field ID.
    $options['inline_title'] = array(
      'default' => array(),
    );

    // New: per-column width settings.
    $options['column_widths'] = array(
      'default' => array(),  // keyed by field ID, e.g. ['title' => '10em'].
    );

    // New: global border toggles.
    $options['row_borders'] = array(
      'default' => 0,
    );
    $options['column_borders'] = array(
      'default' => 0,
    );
    $options['frame_border'] = array(
      'default' => 0,
    );
    // NEW: border thickness & color (with conservative defaults).
    $options['row_border_thickness'] = array(
      'default' => '1px',
    );
    $options['row_border_color'] = array(
      'default' => '#cccccc',
    );

    $options['column_border_thickness'] = array(
      'default' => '1px',
    );
    $options['column_border_color'] = array(
      'default' => '#cccccc',
    );

    $options['frame_border_thickness'] = array(
      'default' => '1px',
    );
    $options['frame_border_color'] = array(
      'default' => '#cccccc',
    );
    return $options;
  }

  /**
   * Determine if a rendered field output should be treated as empty.
   *
   * @param mixed $value
   *   A rendered field value (usually a string, but may be an array).
   *
   * @return bool
   *   TRUE if the output is effectively empty.
   */
  protected function enhancedtable_is_effectively_empty($value) {
    // Preserve numeric zero.
    if ($value === 0 || $value === '0') {
      return FALSE;
    }

    if (is_array($value)) {
      $value = implode('', $value);
    }

    $value = (string) $value;
    // Treat markup-only whitespace as empty.
    $stripped = trim(strip_tags($value));
    return ($stripped === '');
  }

  public function render_fields($result) {
    // Render fields via Views core first.
    $rendered = parent::render_fields($result);

    // If we've already applied prefixes, return the cached/rendered output.
    if (!empty($this->enhancedtable_inline_titles_applied)) {
      return $rendered;
    }

    if (empty($rendered) || !is_array($rendered)) {
      return $rendered;
    }

    $inline = (!empty($this->options['inline_title']) && is_array($this->options['inline_title']))
      ? $this->options['inline_title']
      : array();
    if (empty($inline)) {
      return $rendered;
    }

    $merge_left = (!empty($this->options['merge_left']) && is_array($this->options['merge_left']))
      ? $this->options['merge_left']
      : array();

    if (empty($this->view) || empty($this->view->field) || !is_array($this->view->field)) {
      return $rendered;
    }

    // Walk fields left-to-right to determine merge groups.
    $root = NULL;
    foreach (array_keys($this->view->field) as $fid) {
      $want_merge = !empty($merge_left[$fid]);
      if (!$want_merge || !$root) {
        $root = $fid;
        continue;
      }

      if (empty($inline[$fid])) {
        continue;
      }

      $handler = isset($this->view->field[$fid]) ? $this->view->field[$fid] : NULL;
      if (!$handler) {
        continue;
      }

      // Use the field label as configured in the Views field settings.
      $label = '';
      if (method_exists($handler, 'label')) {
        $label = $handler->label();
      }
      elseif (!empty($handler->options['label'])) {
        $label = $handler->options['label'];
      }
      if ($label === '') {
        continue;
      }

      // Borrow punctuation from core Views field setting: "use colon after
      // label" (element_label_colon).
      $use_colon = TRUE;
      if (isset($handler->options) && is_array($handler->options) && array_key_exists('element_label_colon', $handler->options)) {
        $use_colon = !empty($handler->options['element_label_colon']);
      }

      $prefix = '<span class="enhancedtable-inline-title">' . check_plain($label) . '</span>';
      if ($use_colon) {
        $prefix .= '<span class="enhancedtable-inline-title-punct">:</span> ';
      }
      else {
        $prefix .= ' ';
      }

      foreach ($rendered as $row_index => &$row_fields) {
        if (!is_array($row_fields) || !array_key_exists($fid, $row_fields)) {
          continue;
        }

        // Avoid emitting a dangling label if the value is empty.
        if ($this->enhancedtable_is_effectively_empty($row_fields[$fid])) {
          continue;
        }

        $row_fields[$fid] = $prefix . $row_fields[$fid];
      }
      unset($row_fields);
    }

    // Keep Views' internal cache in sync.
    $this->rendered_fields = $rendered;

    // Mark as applied so subsequent render_fields() calls don't double-prefix.
    $this->enhancedtable_inline_titles_applied = TRUE;

    return $rendered;
  }


  /**
   * Apply EnhancedTable presentation order before rendering.
   *
   * This does not change the underlying Views query; it only influences the
   * order in which fields are rendered (and therefore the table column order).
   */
  public function pre_render($result) {
    // Match core signature: views_plugin_style::pre_render($result)
    parent::pre_render($result);

    if (empty($this->view) || empty($this->view->field) || empty($this->options['field_order'])) {
      return;
    }

    $weights = $this->options['field_order'];
    if (!is_array($weights) || empty($weights)) {
      return;
    }

    // Build the weighted list via foreach (also silences some static analyzers).
    $original = (array) $this->view->field;

    $weighted = array();
    $delta = 0;
    foreach ($original as $field_id => $handler) {
      $w = isset($weights[$field_id]) ? (int) $weights[$field_id] : ($delta * 10);
      $weighted[] = array(
        'field' => $field_id,
        'weight' => $w,
        'delta' => $delta,
      );
      $delta++;
    }

    // Avoid the spaceship operator for broader PHP compatibility.
    usort($weighted, function($a, $b) {
      if ($a['weight'] == $b['weight']) {
        if ($a['delta'] == $b['delta']) {
          return 0;
        }
        return ($a['delta'] < $b['delta']) ? -1 : 1;
      }
      return ($a['weight'] < $b['weight']) ? -1 : 1;
    });

    $reordered = array();
    foreach ($weighted as $item) {
      $fid = $item['field'];
      if (isset($original[$fid])) {
        $reordered[$fid] = $original[$fid];
      }
    }

    if (!empty($reordered)) {
      $this->view->field = $reordered;
    }
  }

  /**
   * Pass-through submit handler (keep for debugging if needed).
   */
  public function options_submit(&$form, &$form_state) {
    // Translate EnhancedTable's intuitive layout controls into the stock Views
    // table-style "columns" settings.
    if (!empty($form_state['values']['style_options']) && is_array($form_state['values']['style_options'])) {
      $so =& $form_state['values']['style_options'];

      // Determine field order from submitted weights.
      $order_weights = !empty($so['field_order']) && is_array($so['field_order']) ? $so['field_order'] : array();
      $order = array();

      if (!empty($order_weights)) {
        $tmp = array();
        foreach ($order_weights as $fid => $w) {
          $tmp[] = array('field' => $fid, 'weight' => (int) $w);
        }
        // Avoid the spaceship operator for broader PHP compatibility.
        usort($tmp, function($a, $b) {
          if ($a['weight'] == $b['weight']) {
            return strcmp($a['field'], $b['field']);
          }
          return ($a['weight'] < $b['weight']) ? -1 : 1;
        });
        foreach ($tmp as $item) {
          $order[] = $item['field'];
        }
      }

      // Fallback to view handler order if weights are missing.
      if (empty($order) && !empty($this->display) && !empty($this->display->handler)) {
        $handlers = $this->display->handler->get_handlers('field');
        if (!empty($handlers)) {
          $order = array_keys($handlers);
        }
      }

      $merge_left = !empty($so['merge_left']) && is_array($so['merge_left']) ? $so['merge_left'] : array();

      // Preserve existing mapping for fields that are not in our UI.
      $existing_columns = array();
      if (!empty($so['columns']) && is_array($so['columns'])) {
        $existing_columns = $so['columns'];
      }
      elseif (!empty($this->options['columns']) && is_array($this->options['columns'])) {
        $existing_columns = $this->options['columns'];
      }

      $new_columns = $existing_columns;

      // Compute merge-left.
      //
      // Stock Views table UI allows multiple fields to be placed into the same
      // column, but only the *parent column field* is a real column. Child
      // fields must be mapped directly to that parent.
      //
      // EnhancedTable's UI expresses this as "Merge left" on each field.
      // Therefore we map each merged field to the last non-merged field to its
      // left (the parent column).
      $root = NULL;
      foreach ($order as $fid) {
        $want_merge = !empty($merge_left[$fid]);
        if ($want_merge && $root) {
          $new_columns[$fid] = $root;
        }
        else {
          $new_columns[$fid] = $fid;
          $root = $fid;
        }
      }

      $so['columns'] = $new_columns;

      // Separator mapping.
      //
      // Stock Views table UI allows only one separator per combined column. It
      // is owned by the *parent column field* (the left-most field in that
      // column group).
      //
      // Older EnhancedTable versions collected the separator on the child being
      // merged.
      //
      // Copy (only the first child) separator onto its parent so Views uses it
      // at render-time.
      //
      // IMPORTANT: Do NOT delete child separators.
      // Views will ignore separators on non-parent fields when they are in the
      // same combined column, but keeping the values allows the user to unmerge
      // later and recover their previous separator settings.
      if (!empty($so['info']) && is_array($so['info'])) {
        $info =& $so['info'];
        $root = NULL;
        $first_child = TRUE;
        foreach ($order as $fid) {
          $want_merge = !empty($merge_left[$fid]);
          if ($want_merge && $root) {
            // Only the first merged child can contribute a separator.
            if ($first_child && array_key_exists('separator', $info[$fid])) {
              if (!isset($info[$root]) || !is_array($info[$root])) {
                $info[$root] = array();
              }
              // Always overwrite the parent separator with the first child's
              // submitted value (mirrors stock table behavior: only one
              // separator is used per combined column).
              $info[$root]['separator'] = $info[$fid]['separator'];
            }
            $first_child = FALSE;
          }
          else {
            // New parent column.
            $root = $fid;
            $first_child = TRUE;
          }
        }
      }

      // Click-sorting cleanup.
      // If a column is not sortable, ensure it cannot remain the default
      // click-sort column (which can leave an 'active' header class behind).
      $sortable_fields = array();
      if (!empty($so['info']) && is_array($so['info'])) {
        foreach ($so['info'] as $fid => &$info) {
          $is_sortable = !empty($info['sortable']);
          if ($is_sortable) {
            $sortable_fields[$fid] = TRUE;
          }
          else {
            // These options only make sense when sortable.
            if (isset($info['default_sort_order'])) {
              unset($info['default_sort_order']);
            }
          }
        }
        unset($info);
      }

      // If the chosen default is no longer sortable, fall back to "no default".
      if (isset($so['default'])) {
        $def = $so['default'];
        if ($def !== -1 && $def !== '-1' && $def !== '' && empty($sortable_fields[$def])) {
          $so['default'] = -1;
        }
        // If nothing is sortable, force no default.
        if (empty($sortable_fields)) {
          $so['default'] = -1;
        }
      }
    }

    parent::options_submit($form, $form_state);
    // Uncomment while debugging if you want to inspect saved options:
    // watchdog('enhancedtable', 'opts:<pre>@o</pre>', ['@o' => print_r($this->options, TRUE)], WATCHDOG_DEBUG);
  }

  // TEMPORARILY DISABLE THE PREVIEW

  //// OPTIONS FORM ////
  public function options_form(&$form, &$form_state) {
    // Let the parent build the normal options form first.
    parent::options_form($form, $form_state);

    // Attach EnhancedTable admin assets (JS/CSS) + jQuery UI sortable.
    $form['#attached']['library'][] = array('enhancedtable', 'enhancedtable.admin');

    // Backdrop's sortable library name has varied across releases.
    // Prefer whichever is available to avoid hard failures in the Views modal.
    $sortable_attached = FALSE;
    if (function_exists('backdrop_get_library')) {
      $lib = backdrop_get_library('system', 'ui.sortable');
      if (!empty($lib)) {
        $form['#attached']['library'][] = array('system', 'ui.sortable');
        $sortable_attached = TRUE;
      }
      else {
        $lib = backdrop_get_library('system', 'jquery.ui.sortable');
        if (!empty($lib)) {
          $form['#attached']['library'][] = array('system', 'jquery.ui.sortable');
          $sortable_attached = TRUE;
        }
      }
    }
    if (!$sortable_attached) {
      // Best-effort default.
      $form['#attached']['library'][] = array('system', 'ui.sortable');
    }

    // IMPORTANT:
    // Views UI will theme the table style plugin settings with
    // theme_views_ui_style_plugin_table(), which assumes $form['columns'],
    // $form['info'], and $form['default'] exist.
    // EnhancedTable uses a custom theme for this form so we can present a
    // different UI (cards) without breaking Views UI.

    // Capture and remove the stock table widgets; we rebuild the UI as cards.
    $columns_el = isset($form['columns']) ? $form['columns'] : array();
    $info_el = isset($form['info']) ? $form['info'] : array();
    $default_el = isset($form['default']) ? $form['default'] : array();
    unset($form['columns'], $form['info'], $form['default']);

    // Use EnhancedTable theme function (prevents Views UI from trying to render
    // the stock table matrix, and avoids fatal errors if columns/info/default
    // are absent).
    $form['#theme'] = array('enhancedtable_views_ui_style_plugin_table');
/*
    // Mark this as a EnhancedTable style-options form so we can scope CSS tightly.
    // -This was implemented to eliminate the pesky horizontal scroll bar.
    // -It allows us to clamp the width of the fieldsets.
    if (!isset($form['#attributes']['class'])) {
      $form['#attributes']['class'] = array();
    }
    $form['#attributes']['class'][] = 'enhancedtable-style-options-form';
*/
    // If the parent didn't build the field matrix, just bail.
    if (empty($this->display) || empty($this->display->handler)) {
      return;
    }
    
    $handlers = $this->display->handler->get_handlers('field');
    if (empty($handlers)) {
      // Let the parent error message stand as-is.
      return;
    }

    // Visible fields (not excluded).
    $field_labels = $this->display->handler->get_field_labels();
    $visible_fields = array();
    foreach ($handlers as $field_id => $handler) {
      if (!empty($handler->options['exclude'])) {
        continue;
      }
      $visible_fields[] = $field_id;
    }

    // Query order (as determined by Views "Fields | Rearrange").
    // We store this in the DOM so the UI can reset back to it.
    $bt_query_index = array();
    $q = 0;
    foreach ($visible_fields as $fid) {
      $bt_query_index[$fid] = $q;
      $q++;
    }

    // Normalize field order defaults.
    if (empty($this->options['field_order']) || !is_array($this->options['field_order'])) {
      $this->options['field_order'] = array();
    }
    $weight = 0;
    foreach ($visible_fields as $fid) {
      if (!isset($this->options['field_order'][$fid])) {
        $this->options['field_order'][$fid] = $weight;
      }
      $weight += 10;
    }

    // Derive merge-left defaults from columns when merge_left is not yet set.
    if (empty($this->options['merge_left']) || !is_array($this->options['merge_left'])) {
      $this->options['merge_left'] = array();
    }
    $columns_current = array();
    if (!empty($this->options['columns']) && is_array($this->options['columns'])) {
      $columns_current = $this->sanitize_columns($this->options['columns'], $handlers);
    }
    foreach ($visible_fields as $fid) {
      if (!isset($this->options['merge_left'][$fid])) {
        $is_merged = (!empty($columns_current[$fid]) && $columns_current[$fid] !== $fid);
        $this->options['merge_left'][$fid] = $is_merged ? 1 : 0;
      }
    }

    // Compute ordered field list.
    $ordered = $visible_fields;
    $weights = $this->options['field_order'];
    // Avoid the spaceship operator for broader PHP compatibility.
    usort($ordered, function($a, $b) use ($weights) {
      $wa = isset($weights[$a]) ? (int) $weights[$a] : 0;
      $wb = isset($weights[$b]) ? (int) $weights[$b] : 0;
      if ($wa == $wb) {
        return strcmp($a, $b);
      }
      return ($wa < $wb) ? -1 : 1;
    });

    // Precompute merge groups for UI behavior.
    // - $bt_root_of[FIELD] gives the parent column field for that field.
    // - $bt_is_first_child[FIELD] is TRUE only for the first merged child in
    //   a group (the only place we allow a separator input).
    $bt_root_of = array();
    $bt_is_first_child = array();
    $bt_root = NULL;
    $bt_seen_child = FALSE;
    foreach ($ordered as $fid) {
      $want_merge = !empty($this->options['merge_left'][$fid]);
      if (!$want_merge || !$bt_root) {
        $bt_root = $fid;
        $bt_seen_child = FALSE;
        $bt_root_of[$fid] = $fid;
        continue;
      }
      $bt_root_of[$fid] = $bt_root;
      $bt_is_first_child[$fid] = !$bt_seen_child;
      $bt_seen_child = TRUE;
    }

    // EnhancedTable Layout (cards): order + merge-left + per-field controls.
    $form['bt_layout'] = array(
      '#type' => 'fieldset',
      '#title' => t('Columns'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#weight' => -10,
      '#description' => t('Use the left/right arrows to reorder columns. Use “Merge left” to combine fields into the column to the left. Only one separator is used per combined column (shown on the first merged field).'),
    );

    // Global "No default" radio (if present).
    if (isset($default_el[-1])) {
      $default_el[-1]['#title'] = t('No default sort');
      $default_el[-1]['#title_display'] = 'after';
      // Radios share one #parents array.
      $default_el[-1]['#parents'] = array('style_options', 'default');
      $form['bt_layout']['no_default_sort'] = array(
        '#type' => 'container',
        '#attributes' => array('class' => array('bt-no-default-sort')),
        '#weight' => 1,
      );
      $form['bt_layout']['no_default_sort']['label'] = array(
        '#markup' => '<div class="description">' /*. check_plain(t('Default click-sorting:')) */. '</div>',
      );
      $form['bt_layout']['no_default_sort']['radio'] = $default_el[-1];
    }

    $form['bt_layout']['cards'] = array(
      '#type' => 'container',
      '#attributes' => array('class' => array('bt-cards')),
      '#weight' => 0,
    );

    // Add a reset-order control above the cards, then a centering wrapper
    // div around .bt-cards.
    $reset_label = check_plain(t('Reset field order'));
    $reset_help = check_plain(t('Reset the field order to match the query order (determined on previous page under "Fields | Rearrange").'));
    $form['bt_layout']['cards']['#prefix'] = '<div class="bt-reset-order">'
      . '<button type="button" class="bt-reset-field-order">' . $reset_label . '</button>'
      . '<div class="description">' . $reset_help . '</div>'
      . '</div>'
      . '<div class="bt-cards-wrap">';
    $form['bt_layout']['cards']['#suffix'] = '</div>';

    $first_fid = reset($ordered);
    foreach ($ordered as $fid) {
      if (empty($handlers[$fid]) || !empty($handlers[$fid]->options['exclude'])) {
        continue;
      }

      $label = isset($field_labels[$fid]) ? $field_labels[$fid] : $fid;

      $form['bt_layout']['cards'][$fid] = array(
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('bt-card'),
          'data-bt-field' => $fid,
          'data-bt-query-index' => isset($bt_query_index[$fid]) ? (string) $bt_query_index[$fid] : '0',
        ),
      );

      // Card header.
      $form['bt_layout']['cards'][$fid]['header'] = array(
        '#type' => 'container',
        '#attributes' => array('class' => array('bt-card-header')),
      );

      // Row 1: arrows.
      $form['bt_layout']['cards'][$fid]['header']['nav'] = array(
        '#type' => 'container',
        '#attributes' => array('class' => array('bt-card-nav')),
      );

      $form['bt_layout']['cards'][$fid]['header']['nav']['move_left'] = array(
        '#type' => 'markup',
        //'#markup' => '<button type="button" class="bt-move-left" title="' . check_plain(t('Move left')) . '">◀</button>',
        '#markup' => '<button type="button" class="bt-move-left" title="' 
                     . check_plain(t('Move left')) . '">' 
                     . icon('arrow-fat-left', array(
                         'attributes' => array('class' => array('bt-nav-icon'))))
                     . '</button>',
      );

      $form['bt_layout']['cards'][$fid]['header']['nav']['move_right'] = array(
        '#type' => 'markup',
        //'#markup' => '<button type="button" class="bt-move-right" title="' . check_plain(t('Move right')) . '">▶</button>',
        '#markup' => '<button type="button" class="bt-move-right" title="' 
                     . check_plain(t('Move right')) . '">' 
                     . icon('arrow-fat-right', array(
                         'attributes' => array('class' => array('bt-nav-icon'))))
                     . '</button>',
      );

      // Row 2: title (full width).
      $form['bt_layout']['cards'][$fid]['header']['title'] = array(
        '#type' => 'markup',
        '#markup' => '<div class="bt-card-title">' . check_plain($label) . '</div>',
      );

      // Hidden order weight (updated by JS sortable).
      $form['bt_layout']['cards'][$fid]['field_order'] = array(
        '#type' => 'hidden',
        '#default_value' => isset($this->options['field_order'][$fid]) ? (int) $this->options['field_order'][$fid] : 0,
        '#parents' => array('style_options', 'field_order', $fid),
        '#attributes' => array('class' => array('bt-field-weight')),
      );

      // Merge-left.
      $form['bt_layout']['cards'][$fid]['merge_left'] = array(
        '#type' => 'checkbox',
        '#title' => t('Merge left'),
        '#default_value' => (!empty($this->options['merge_left'][$fid]) ? 1 : 0),
        '#parents' => array('style_options', 'merge_left', $fid),
        '#attributes' => array('class' => array('bt-merge-left')),
      );

      // Disable merge-left on the left-most column, but keep it visible.
      if ($fid === $first_fid) {
        $form['bt_layout']['cards'][$fid]['merge_left']['#disabled'] = TRUE;
        $form['bt_layout']['cards'][$fid]['merge_left']['#default_value'] = 0;
      }

      // When a field is merged into the column to its left, most per-column
      // settings no longer apply to the child field. Hide those controls to
      // avoid confusion.
      $bt_states_not_merged = array(
        'visible' => array(
          ':input[name="style_options[merge_left][' . $fid . ']"]' => array('checked' => FALSE),
        ),
      );

      // Separator.
      // Stock Views behavior: only one separator is used per combined column,
      // and it's owned by the parent column field. EnhancedTable collects that
      // separator on the first merged child field and moves it to the parent
      // on submit.
      $sep_default = '';
      $want_merge_default = !empty($this->options['merge_left'][$fid]);
      if ($want_merge_default) {
        // Prefer the child's stored value so it is preserved and recoverable
        // if the field is later unmerged.
        if (isset($this->options['info'][$fid]['separator'])) {
          $sep_default = $this->options['info'][$fid]['separator'];
        }

        // Fallback to the parent column field's separator (effective value).
        if ($sep_default === '') {
          $root_fid = isset($bt_root_of[$fid]) ? $bt_root_of[$fid] : NULL;
          if (!empty($root_fid) && isset($this->options['info'][$root_fid]['separator'])) {
            $sep_default = $this->options['info'][$root_fid]['separator'];
          }
          elseif (!empty($root_fid) && isset($info_el[$root_fid]['separator']['#default_value'])) {
            $sep_default = $info_el[$root_fid]['separator']['#default_value'];
          }
        }

        // Final fallback: whatever Views built for this field.
        if ($sep_default === '' && isset($info_el[$fid]['separator']['#default_value'])) {
          $sep_default = $info_el[$fid]['separator']['#default_value'];
        }
      }

      $form['bt_layout']['cards'][$fid]['separator'] = array(
        '#type' => 'textfield',
        '#title' => t('Separator'),
        '#size' => 8,
        '#maxlength' => 64,
        '#default_value' => $sep_default,
        '#parents' => array('style_options', 'info', $fid, 'separator'),
        '#attributes' => array('class' => array('bt-separator-input')),
        '#states' => array(
          // Only show on merged fields; JS further restricts this to the first
          // merged child in a group.
          'visible' => array(
            ':input[name="style_options[merge_left][' . $fid . ']"]' => array('checked' => TRUE),
          ),
          // Ensure non-merged fields do not submit separator values.
          'disabled' => array(
            ':input[name="style_options[merge_left][' . $fid . ']"]' => array('checked' => FALSE),
          ),
        ),
      );

      // Inline title (prefix the merged field output with its label).
      // Visibility mirrors the Separator control (shown only when merged),
      // but unlike the Separator, this remains visible for all merged children.
      $inline_default = !empty($this->options['inline_title'][$fid]) ? 1 : 0;
      $form['bt_layout']['cards'][$fid]['inline_title'] = array(
        '#type' => 'checkbox',
        '#title' => t('Inline title'),
        '#default_value' => $inline_default,
        '#parents' => array('style_options', 'inline_title', $fid),
        '#attributes' => array('class' => array('bt-inline-title')),
      //'#description' => t('Prefix this merged field with its field label.'),
        '#states' => array(
          'visible' => array(
            ':input[name="style_options[merge_left][' . $fid . ']"]' => array('checked' => TRUE),
          ),
          'disabled' => array(
            ':input[name="style_options[merge_left][' . $fid . ']"]' => array('checked' => FALSE),
          ),
        ),
      );

      // Alignment.
      if (isset($info_el[$fid]['align'])) {
        $align = $info_el[$fid]['align'];
        $align['#title'] = t('Alignment');
        // Ensure this stores back into style_options[info][FIELD][align].
        $align['#parents'] = array('style_options', 'info', $fid, 'align');
        $align['#states'] = $bt_states_not_merged;
        $form['bt_layout']['cards'][$fid]['align'] = $align;
      }

      // Width (EnhancedTable extra).
      $form['bt_layout']['cards'][$fid]['width'] = array(
        '#type' => 'textfield',
        '#title' => t('Width'),
        '#size' => 8,
        '#maxlength' => 32,
        '#default_value' => isset($this->options['column_widths'][$fid]) ? $this->options['column_widths'][$fid] : '',
        '#parents' => array('style_options', 'column_widths', $fid),
        '#attributes' => array(
          'placeholder' => t('%, px, em, rem'),
        ),
        '#states' => $bt_states_not_merged,
//        '#description' => t('CSS width (e.g. 8em, 120px, 20%).'),
      );

      // Hide-if-empty.
      if (isset($info_el[$fid]['empty_column'])) {
        $empty = $info_el[$fid]['empty_column'];
        $empty['#title'] = t('Hide if empty');
        $empty['#title_display'] = 'after';
        $empty['#parents'] = array('style_options', 'info', $fid, 'empty_column');
        $empty['#states'] = $bt_states_not_merged;
        $form['bt_layout']['cards'][$fid]['empty_column'] = $empty;
      }

      // Click-sorting controls.
      if (isset($info_el[$fid]['sortable']) || isset($default_el[$fid]) || isset($info_el[$fid]['default_sort_order'])) {

        // No enclosing fieldset; always exposed.
        $form['bt_layout']['cards'][$fid]['sorting'] = array(
          '#type' => 'container',
          '#attributes' => array('class' => array('bt-sorting')),
          '#states' => $bt_states_not_merged,
        );

        if (isset($info_el[$fid]['sortable'])) {
          $sort = $info_el[$fid]['sortable'];
          $sort['#title'] = t('Click-sort');
          $sort['#title_display'] = 'after';
          $sort['#parents'] = array('style_options', 'info', $fid, 'sortable');
          $form['bt_layout']['cards'][$fid]['sorting']['sortable'] = $sort;
        }

        if (isset($info_el[$fid]['default_sort_order'])) {
          $ord = $info_el[$fid]['default_sort_order'];
          $ord['#title'] = t('Initial order');
          $ord['#parents'] = array('style_options', 'info', $fid, 'default_sort_order');
          // Only show when click-sorting is enabled.
          $ord['#states'] = array(
            'visible' => array(
              ':input[name="style_options[info][' . $fid . '][sortable]"]' => array('checked' => TRUE),
            ),
          );
          $form['bt_layout']['cards'][$fid]['sorting']['default_sort_order'] = $ord;
        }

        if (isset($default_el[$fid])) {
          $def = $default_el[$fid];
          $def['#title'] = t('Default sort');
          $def['#title_display'] = 'after';
          // Radios share one #parents array.
          $def['#parents'] = array('style_options', 'default');
          // Only show when click-sorting is enabled.
          $def['#states'] = array(
            'visible' => array(
              ':input[name="style_options[info][' . $fid . '][sortable]"]' => array('checked' => TRUE),
            ),
          );
          $form['bt_layout']['cards'][$fid]['sorting']['default'] = $def;
        }
      }

      // Update previous field tracker.
    }

    // ------------------------------------------------------------------
    // Column setting help text (appears just under the flipped table).
    // ------------------------------------------------------------------

    $form['bt_help'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Help'),
      '#collapsible' => TRUE,
      '#collapsed'   => TRUE,
      '#weight'      => 0,
    );

    // ------------------------------------------------------------------
    // Set up our help text from an external HTML file.
    // ------------------------------------------------------------------
    $help_file = backdrop_get_path('module', 'enhancedtable') . '/help/enhancedtable-settings-help.html';
    $help_text = '';

    if (file_exists($help_file)) {
      $help_text = file_get_contents($help_file);
      // Optional: sanitize if you’re not fully trusting this file.
      // $help_text = filter_xss_admin($markup);
    }

    $form['bt_help']['column_help'] = array(
      '#type'   => 'markup',
      '#markup' => $help_text,
      '#weight' => 0,
    );

    // Set the weight of Grouping.
    if (isset($form['grouping'])) {
      $form['grouping']['#weight'] = 30;
    }

    // ------------------------------------------------------------------
    // Styling fieldset:
    //   - Add striping...
    //   - Add horizontal row borders
    //   - Add vertical column borders
    //   - Add a table frame border
    //   - Add views row classes
    //   - Row class (text)
    // ------------------------------------------------------------------

    $form['bt_styling'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Styling'),
      '#collapsible' => TRUE,
      '#collapsed'   => FALSE,
      '#weight'      => 20,
    );

  // 1) Striping checkbox (row_class_special) into Styling.
    if (isset($form['row_class_special'])) {
      $form['bt_styling']['row_class_special'] = $form['row_class_special'];
      $form['bt_styling']['row_class_special']['#weight'] = 0;
      // Preserve original option mapping.
      $form['bt_styling']['row_class_special']['#parents'] = array('style_options', 'row_class_special');
      unset($form['row_class_special']);
    }

    /**
     * Row borders row: checkbox + thickness + color inline.
     */
    $form['bt_styling']['row_borders_row'] = array(
      '#type'       => 'container',
      '#attributes' => array('class' => array('bt-border-row', 'bt-row-borders-row')),
      '#weight'     => 10,
    );

    // Checkbox: "Add horizontal row borders".
    $form['bt_styling']['row_borders_row']['row_borders'] = array(
      '#type'          => 'checkbox',
      '#title'         => t('<span class="btz-brdr">Add horizontal row borders</span>'),
      '#default_value' => !empty($this->options['row_borders']),
      '#parents'       => array('style_options', 'row_borders'),
      '#title_display' => 'after',
    );

    // Thickness.
    $form['bt_styling']['row_borders_row']['row_border_thickness'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Thickness'),
      '#size'          => 6,
      '#maxlength'     => 16,
      '#default_value' => !empty($this->options['row_border_thickness'])
        ? $this->options['row_border_thickness']
        : '1px',
      '#parents'       => array('style_options', 'row_border_thickness'),
      '#title_display' => 'before',
      '#attributes'    => array('class' => array('bt-border-thickness')),
      '#states'        => array(
        'visible' => array(
          'input[name="style_options[row_borders]"]' => array('checked' => TRUE),
        ),
      ),
    );

    // Color.
    $form['bt_styling']['row_borders_row']['row_border_color'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Color'),
      '#size'          => 8,
      '#maxlength'     => 32,
      '#default_value' => !empty($this->options['row_border_color'])
        ? $this->options['row_border_color']
        : '#cccccc',
      '#parents'       => array('style_options', 'row_border_color'),
      '#title_display' => 'before',
      '#attributes'    => array('class' => array('bt-color-input', 'bt-border-color')),
      '#states'        => array(
        'visible' => array(
          'input[name="style_options[row_borders]"]' => array('checked' => TRUE),
        ),
      ),
    );

    /**
     * Column borders row.
     */
    $form['bt_styling']['column_borders_row'] = array(
      '#type'       => 'container',
      '#attributes' => array('class' => array('bt-border-row', 'bt-column-borders-row')),
      '#weight'     => 20,
    );

    // Checkbox: "Add vertical column borders".
    $form['bt_styling']['column_borders_row']['column_borders'] = array(
      '#type'          => 'checkbox',
      '#title'         => t('<span class="btz-brdr">Add vertical column borders</span>'),
      '#default_value' => !empty($this->options['column_borders']),
      '#parents'       => array('style_options', 'column_borders'),
      '#title_display' => 'after',
    );

    // Thickness.
    $form['bt_styling']['column_borders_row']['column_border_thickness'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Thickness'),
      '#size'          => 6,
      '#maxlength'     => 16,
      '#default_value' => !empty($this->options['column_border_thickness'])
        ? $this->options['column_border_thickness']
        : '1px',
      '#parents'       => array('style_options', 'column_border_thickness'),
      '#title_display' => 'before',
      '#attributes'    => array('class' => array('bt-border-thickness')),
      '#states'        => array(
        'visible' => array(
          'input[name="style_options[column_borders]"]' => array('checked' => TRUE),
        ),
      ),
    );

    // Color.
    $form['bt_styling']['column_borders_row']['column_border_color'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Color'),
      '#size'          => 8,
      '#maxlength'     => 32,
      '#default_value' => !empty($this->options['column_border_color'])
        ? $this->options['column_border_color']
        : '#cccccc',
      '#parents'       => array('style_options', 'column_border_color'),
      '#title_display' => 'before',
      '#attributes'    => array('class' => array('bt-color-input', 'bt-border-color')),
      '#states'        => array(
        'visible' => array(
          'input[name="style_options[column_borders]"]' => array('checked' => TRUE),
        ),
      ),
    );

    /**
     * Frame border row.
     */
    $form['bt_styling']['frame_border_row'] = array(
      '#type'       => 'container',
      '#attributes' => array('class' => array('bt-border-row', 'bt-frame-border-row')),
      '#weight'     => 30,
    );

    // Checkbox: "Add a table frame border".
    $form['bt_styling']['frame_border_row']['frame_border'] = array(
      '#type'          => 'checkbox',
      '#title'         => t('<span class="btz-brdr">Add a table frame border</span>'),
      '#default_value' => !empty($this->options['frame_border']),
      '#parents'       => array('style_options', 'frame_border'),
      '#title_display' => 'after',
    );

    // Thickness.
    $form['bt_styling']['frame_border_row']['frame_border_thickness'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Thickness'),
      '#size'          => 6,
      '#maxlength'     => 16,
      '#default_value' => !empty($this->options['frame_border_thickness'])
        ? $this->options['frame_border_thickness']
        : '1px',
      '#parents'       => array('style_options', 'frame_border_thickness'),
      '#title_display' => 'before',
      '#attributes'    => array('class' => array('bt-border-thickness')),
      '#states'        => array(
        'visible' => array(
          'input[name="style_options[frame_border]"]' => array('checked' => TRUE),
        ),
      ),
    );

    // Color.
    $form['bt_styling']['frame_border_row']['frame_border_color'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Color'),
      '#size'          => 8,
      '#maxlength'     => 32,
      '#default_value' => !empty($this->options['frame_border_color'])
        ? $this->options['frame_border_color']
        : '#cccccc',
      '#parents'       => array('style_options', 'frame_border_color'),
      '#title_display' => 'before',
      '#attributes'    => array('class' => array('bt-color-input', 'bt-border-color')),
      '#states'        => array(
        'visible' => array(
          'input[name="style_options[frame_border]"]' => array('checked' => TRUE),
        ),
      ),
    );

    // "Add views row classes".
    if (isset($form['default_row_class'])) {
      $form['bt_styling']['default_row_class'] = $form['default_row_class'];
      $form['bt_styling']['default_row_class']['#weight'] = 40;
      // Preserve original option mapping.
      $form['bt_styling']['default_row_class']['#parents'] = array('style_options', 'default_row_class');
      unset($form['default_row_class']);
    }

    // "Row class" text field.
    if (isset($form['row_class'])) {
      $form['bt_styling']['row_class'] = $form['row_class'];
      $form['bt_styling']['row_class']['#weight'] = 50;
      // Preserve original option mapping.
      $form['bt_styling']['row_class']['#parents'] = array('style_options', 'row_class');
      unset($form['row_class']);
    }

    // ------------------------------------------------------------------
    // Mechanics fieldset:
    //   - Override normal sorting...
    //   - Enable sticky table headers
    //   - Show the empty text in the table
    // ------------------------------------------------------------------

    $form['bt_mechanics'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Mechanics'),
      '#collapsible' => TRUE,
      '#collapsed'   => FALSE,
      '#weight'      => 40,
    );

    $mechanics_order = array('override', 'sticky', 'empty_table');
    $w = 0;
    foreach ($mechanics_order as $key) {
      if (isset($form[$key])) {
        $form['bt_mechanics'][$key] = $form[$key];
        $form['bt_mechanics'][$key]['#weight'] = $w;
        // Preserve original option mapping.
        $form['bt_mechanics'][$key]['#parents'] = array('style_options', $key);
        unset($form[$key]);
        $w += 10;
      }
    }

    // ------------------------------------------------------------------
    // Admin fieldset:
    //   - Short description of table (caption)
    //   - Table summary
    // ------------------------------------------------------------------

    $form['bt_admin'] = array(
      '#type'        => 'fieldset',
      '#title'       => t('Admin'),
      '#collapsible' => TRUE,
      '#collapsed'   => TRUE,
      '#weight'      => 50,
    );

    $admin_order = array('caption', 'summary');
    $w = 0;
    foreach ($admin_order as $key) {
      if (isset($form[$key])) {
        $form['bt_admin'][$key] = $form[$key];
        $form['bt_admin'][$key]['#weight'] = $w;
        // Preserve original option mapping.
        $form['bt_admin'][$key]['#parents'] = array('style_options', $key);
        unset($form[$key]);
        $w += 10;
      }
    }

    // EnhancedTable no longer uses the flipped matrix theme; the form is built
    // directly using "cards" containers.
  }

}
