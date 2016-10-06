<?php

/**
 * @file
 * Administrative callbacks for the Commerce Avalara module.
 */

/**
 * Builds the Avalara settings form.
 */
function commerce_avalara_credentials_settings_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'commerce_avalara', 'includes/commerce_avalara.admin');
  $modes = array(
    COMMERCE_AVALARA_DEVELOPMENT_MODE => t('Development'),
    COMMERCE_AVALARA_PRODUCTION_MODE => t('Production'),
  );
  $api_mode = commerce_avalara_api_mode();

  if (!empty($form_state['values']['commerce_avalara_api_mode'])) {
    $api_mode = $form_state['values']['commerce_avalara_api_mode'];
  }

  $form['credentials'] = array(
    '#title' => t('API Credentials'),
    '#type' => 'fieldset',
    '#attributes' => array(
      'id' => 'commerce-avalara-credentials',
    ),
  );

  $form['credentials']['commerce_avalara_api_mode'] = array(
    '#title' => t('API Mode'),
    '#description' => t('Only select Production after completing the GO LIVE process with Avalara'),
    '#type' => 'select',
    '#options' => $modes,
    '#default_value' => $api_mode,
    '#ajax' => array(
      'callback' => 'commerce_avalara_ajax_mode_credentials',
      'wrapper' => 'commerce-avalara-credentials',
    ),
  );

  // Allow to set different credentials for each mode (dev|prod).
  $credential_fields = array(
    'username' => t('@mode account Username', array('@mode' => $modes[$api_mode])),
    'password' => t('@mode account Password', array('@mode' => $modes[$api_mode])),
  );

  $display_warning = FALSE;
  foreach ($credential_fields as $key => $label) {
    $function = 'commerce_avalara_' . $key;
    $variable_value = $function($api_mode);
    $variable_name = 'commerce_avalara_' . $api_mode . '_' . $key;
    // Determine if the settings are in the database; if they are not but have
    // values set, we disable the form fields so the values will not be saved to
    // the database on submission.
    $variable_in_db = unserialize(db_query("SELECT value FROM {variable} WHERE name = :name", array(':name' => $variable_name))->fetchField());

    if (!empty($variable_value) && empty($variable_in_db)) {
      $form['credentials'][$variable_name] = array(
        '#type' => 'item',
        '#title' => $label,
        '#markup' => check_plain($variable_value),
      );
    }
    else {
      $form['credentials'][$variable_name] = array(
        '#type' => 'textfield',
        '#title' => $label,
        '#default_value' => $variable_value,
        '#required' => TRUE,
      );
    }

    // Show a message informing the user of best practices to not store API
    // credentials in the database if necessary.
    if (empty($variable_value) || (!empty($variable_value) && $variable_in_db)) {
      $display_warning = TRUE;
    }
  }

  // Show a message informing the user of best practices to not store API
  // credentials in the database if necessary.
  if ($display_warning) {
    $description = t('It is best practice to store API credentials outside of the database and your source code repository.');
    $description .= ' ' . t('Consider setting the credential variables as server environment variables and bringing them into your Drupal configuration via the $conf array in your settings.php file instead.');
    $form['credentials']['#description'] = '<div class="messages warning">' . $description . '</div>';
  }

  $form = system_settings_form($form);
  $form['actions']['submit']['#value'] = t('Validate credentials');
  return $form;
}

/**
 * Form validation handler for commerce_avalara_credentials_settings_form().
 */
function commerce_avalara_credentials_settings_form_validate($form, &$form_state) {
  $authenticated = FALSE;
  // The variables are not yet saved to the database, so we need to manually
  // commerce_avalara_object() won't be able to correctly encode the
  // authorization key, we need to do that here.
  $username = $form_state['values']['commerce_avalara_' . $form_state['values']['commerce_avalara_api_mode'] . '_username'];
  $password = $form_state['values']['commerce_avalara_' . $form_state['values']['commerce_avalara_api_mode'] . '_password'];

  // Perform a PING request to test if the authentification worked.
  if (!empty($username) && !empty($password)) {
    if ($avalara = commerce_avalara_object(base64_encode("$username:$password"))) {
      $ping_request = $avalara->ping();

      if ($ping_request['success'] && !empty($ping_request['result']['authenticated'])) {
        $authenticated = TRUE;
      }
    }
  }
  if (!$authenticated) {
    form_set_error('credentials', t('Could not authenticate to the Avatax API'));
  }
}

/**
 * Ajax: Returns container field for credential textfields.
 */
function commerce_avalara_ajax_mode_credentials($form, $form_state) {
  return $form['credentials'];
}
