<?php
/**
 * @file
 * Displays information about how frequently people register for events from
 * this source.
 */

$plugin = array(
  'single' => TRUE,
  'title' => t('Source Summary Frequency'),
  'description' => t('Displays a chart showing a frequency attendees registered for this event registered for others.'),
  'category' => t('CRM Core Event'),
  'edit form' => 'source_summary_frequency_form',
  'render callback' => 'source_summary_frequency_render',
  'admin info' => 'source_summary_frequency_admin_info',
  'defaults' => array(
    'height' => '',
    'color_series' => '',
    'display_labels' => 0,
    'include_legend' => 0,
		'include_tooltip' => 0,
  ),
  'all contexts' => TRUE,
);

/**
 * Admin info.
 */
function source_summary_frequency_admin_info($subtype, $conf, $contexts) {
  $block = new stdClass();
  $block->title = t('Source Summary Frequency');
  $block->content = t('Displays a chart showing a frequency attendees registered for this event registered for others.');

  return $block;
}

/**
 * Settings form.
 */
function source_summary_frequency_form($form, &$form_state) {
  $conf = $form_state['conf'];

  $form['include_legend'] = array(
    '#type' => 'checkbox',
    '#title' => t('Display a legend'),
    '#description' => t('Check this box to include a legend in your chart.'),
    '#default_value' => !empty($conf['include_legend']) ? $conf['include_legend'] : 0,
    '#return_value' => 1,
  );

  $form['include_tooltip'] = array(
    '#type' => 'checkbox',
    '#title' => t('Display tooltips'),
    '#description' => t('Check this box to display tooltips when hovering over a point in your chart.'),
    '#default_value' => !empty($conf['include_tooltip']) ? $conf['include_tooltip'] : 0,
    '#return_value' => 1,
  );
    
  // Display settings.
  $form['display_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Display settings'),
    '#weight' => 5,
    '#collapsible' => FALSE,
  );

  // Height.
  $form['display_settings']['height'] = array(
    '#type' => 'textfield',
    '#title' => t('Height'),
    '#description' => t('Please enter the height of the chart, as an integer.'),
    '#default_value' => !empty($conf['height']) ? $conf['height'] : '',
  );

  // Display labels.
  $form['display_labels'] = array(
    '#type' => 'checkbox',
    '#title' => t('Display labels'),
    '#description' => t('Check this box to display labels for every region on your chart.'),
    '#default_value' => !empty($conf['display_labels']) ? $conf['display_labels'] : 0,
    '#return_value' => 1,
  );

  // Color series.
  $form['display_settings']['color_series'] = array(
    '#type' => 'textarea',
    '#title' => t('Color series'),
    '#description' => t('Please enter the colors to use for the data series in valid #RRGGBB or rgba format. Please separate each value with a new line. Leave blank to use the default colors.'),
    '#default_value' => !empty($conf['color_series']) ? $conf['color_series'] : '',
  );

  return $form;
}

/**
 * Validate handler for settings form.
 */
function source_summary_frequency_form_validate($form, &$form_state) {

  // Height.
  if (!empty($form_state['values']['height']) && !is_numeric($form_state['values']['height'])) {
    form_set_error('height', t('The height of the chart must be entered as a number (i.e. 200).'));
  }
}

/**
 * Submit handler for settings form.
 */
function source_summary_frequency_form_submit($form, &$form_state) {
  // Legend.
  $form_state['conf']['include_legend'] = $form_state['values']['include_legend'];
  
  // display tooltip
  $form_state['conf']['include_tooltip'] = $form_state['values']['include_tooltip'];  

  // Height.
  $form_state['conf']['height'] = $form_state['values']['height'];

  // Colors.
  $form_state['conf']['color_series'] = $form_state['values']['color_series'];

  // Display labels.
  $form_state['conf']['display_labels'] = $form_state['values']['display_labels'];
}

/**
 * Render callback.
 */
function source_summary_frequency_render($subtype, $conf, $panel_args, $context = NULL) {

  // Set up a table for displaying raw data.
  $header = array('Status', 'Percentage');
  $rows = array();

  // Storing information that will go into the chart.
  $source_data = array();

  // Registered attendees.
  $registered = db_query("
  SELECT contact.field_activity_participants_target_id as contact_id
  FROM crm_core_activity AS registration
  LEFT JOIN field_data_field_activity_participants as contact ON contact.entity_id = registration.activity_id
  LEFT JOIN field_data_field_cmcev_source AS ers ON ers.entity_id = registration.activity_id
  WHERE registration.type = 'cmcev_event_registration'
  AND ers.field_cmcev_source_value = :source
    ", array(':source' => $panel_args[0]));
  $registered = array_unique($registered->fetchCol());

  if (!empty($registered)) {
    $items = db_query("
SELECT COUNT( DISTINCT event.field_cmcev_event_target_id) as events_count
  FROM crm_core_activity AS registration
  LEFT JOIN field_data_field_cmcev_event AS event ON event.entity_id = registration.activity_id
  LEFT JOIN field_data_field_activity_participants as contact ON contact.entity_id = registration.activity_id
  LEFT JOIN field_data_field_cmcev_source AS ers ON ers.entity_id = registration.activity_id
  WHERE (registration.type = 'cmcev_event_registration'
  AND (contact.field_activity_participants_target_id IN (:attendees))
  AND ers.field_cmcev_source_value = :source
  )
GROUP BY contact.field_activity_participants_target_id
  ", array(':source' => $panel_args[0], ':attendees' => $registered));

    // Building table.
    $table_data = array();
    foreach ($items as $item) {
      if (isset($table_data[$item->events_count])) {
        $table_data[$item->events_count]++;
      }
      else {
        $table_data[$item->events_count] = 1;
      }
    }

    // Set up the table.
    $not_first_timers = 0;
    foreach ($table_data as $events_count => $attendees) {
      $not_first_timers += $attendees;
      $percents = floor(($attendees / count($registered)) * 100);
      // Populate the chart.
      $temp = new flotData(array(array(0, $percents)));
      $temp->label = t('@count events', array(
        '@percents' => $percents,
        '@count' => $events_count,
      ));
      $source_data[] = $temp;

      // Populate the table.
      $rows[] = array(
        'data' => array(
          array(
            'data' => format_plural($events_count, '@count event', '@count events', array('@count' => $events_count)),
          	'class'=> 'table-field-registration-frequency'
          ),
          array(
            'data' => $percents . '%',
          	'class'=> 'table-field-registration-frequency-percent'
          ),
        ),
      );
    }

    // Configuration.
    // If setting not set for widget, we grabbing it from modules configuration.
    $height = empty($conf['height']) ? variable_get('crm_core_event_height', '200') : $conf['height'];
    $color_series = empty($conf['color_series']) ? variable_get('crm_core_event_series_colors', '') : $conf['color_series'];
    $display_labels = empty($conf['display_labels']) ? variable_get('crm_core_event_display_labels', 0) : $conf['display_labels'];
    $include_legend = empty($conf['include_legend']) ? variable_get('crm_core_event_include_legend', 0) : $conf['include_legend'];
    $include_tooltip = empty($conf['include_tooltip']) ? variable_get('crm_core_petition_include_tooltips', 0) : $conf['include_tooltip'];

    // Create a new pie chart.
    $options = new flotStylePie();

    // Create a radius, make it a donut chart.
    $options->series->pie->radius = 1;
    $options->series->pie->innerRadius = 0.6;
    $options->series->useLabel = true;
    $options->series->suffix = '%';
    
    // Add the color series.
    if (!empty($color_series)) {
      $options->colors = explode("\n", $color_series);
    }

    // Add the labels.
    if (!empty($display_labels)) {
      $options->series->pie->label->show = TRUE;
      $options->series->pie->label->radius = 2 / 3;
      $options->series->pie->label->threshold = 0.01;
      $options->series->pie->label->backgroundOpacity = 1;
    }
    else {
      $options->series->pie->label->show = FALSE;
    }
    
    // add the tooltips
    if (!empty($include_tooltip)) {
      $options->series->show_tooltip = TRUE;
    }
    else {
      $options->series->show_tooltip = FALSE;
    }

    // Display the chart.
    $content = theme('flot_graph', array(
      'data' => $source_data,
      'options' => $options,
      'element' => array(
        'style' => 'width: 100%; height: ' . (int) $height . 'px;',
      ),
      'legend' => ($include_legend === 1) ? TRUE : FALSE,
    ));

    // Output the table.
    // Adding wrapper block for fancy styling.
    $content .= '<div class="table-container">' . theme('table', array('rows' => $rows, 'header' => $header)) . '</div>';
  }
  else {
    $content = t('No one has registered through this source.');
  }

  // Return everything.
  $block = new stdClass();
  $block->title = t('Frequency');
  $block->content = $content;

  return $block;
}
