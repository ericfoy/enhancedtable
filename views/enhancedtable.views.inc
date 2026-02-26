<?php

function enhancedtable_views_plugins() {
  return array(
    'style' => array(
      'enhancedtable' => array(
        'title'   => t('Table (Enhanced)'),
        'help'    => t('Table with enhanced controls.'),
        'handler' => 'enhancedtable_plugin_style_table',
        'theme'   => 'views_view_table', // stop auto hook !! IMPORTANT to avoid recursion
        'uses fields' => TRUE,
        'uses row class' => TRUE,
        'uses row plugin' => FALSE,
        'uses options' => TRUE,
        'type' => 'normal', // verify that this should not be 'table'...
        // Important! No parent key here. Otherwise... recursive death.
      ),
    ),
  );
}
