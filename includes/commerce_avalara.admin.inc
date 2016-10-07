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
    COMMERCE_AVALARA_ACCOUNT_NUMBER_FIELD => array(
      'title' => t('@mode account number', array('@mode' => $modes[$api_mode])),
      'description' => t('The Avalara account number that is used to authenticate against the API.'),
    ),
    COMMERCE_AVALARA_LICENSE_KEY_FIELD => array(
      'title' => t('@mode license key', array('@mode' => $modes[$api_mode])),
      'description' => t('The Avalara license key that is used to authenticate against the API.'),
    ),
  );

  $display_warning = FALSE;
  foreach ($credential_fields as $key => $conf) {
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
        '#title' => $conf['title'],
        '#description' => $conf['description'],
        '#markup' => check_plain($variable_value),
      );
    }
    else {
      $form['credentials'][$variable_name] = array(
        '#type' => 'textfield',
        '#title' => $conf['title'],
        '#description' => $conf['description'],
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
  // Avoid to repeat the message during when the credentials are validated.
  if ($display_warning && !isset($form_state['triggering_element'])) {
    $description = t('It is best practice to store API credentials outside of the database and your source code repository.');
    $description .= ' ' . t('Consider setting the credential variables as server environment variables and bringing them into your Drupal configuration via the $conf array in your settings.php file instead.');
    drupal_set_message($description, 'warning');
  }

  // Add a "Validate credentials" button.
  $form['credentials']['validate'] = array(
    '#value' => t('Validate credentials'),
    '#type' => 'submit',
    '#validate' => array('commerce_avalara_credentials_settings_validate'),
    '#ajax' => array(
      'callback' => 'commerce_avalara_ajax_mode_credentials',
      'wrapper' => 'commerce-avalara-credentials',
    ),
  );

  $form = system_settings_form($form);
  return $form;
}

/**
 * Validate handler for the "Validate credentials" button.
 */
function commerce_avalara_credentials_settings_validate($form, &$form_state) {
  $api_mode = $form_state['values']['commerce_avalara_api_mode'];
  $account_number_field = 'commerce_avalara_' . $api_mode . '_' . COMMERCE_AVALARA_ACCOUNT_NUMBER_FIELD;
  $license_key_field = 'commerce_avalara_' . $api_mode . '_' . COMMERCE_AVALARA_LICENSE_KEY_FIELD;
  $account_number = $form_state['values'][$account_number_field];
  $license_key = $form_state['values'][$license_key_field];
  $valid = FALSE;

  // Validate the credentials provided.
  if (!empty($account_number) && !empty($license_key)) {
    if ($avalara = commerce_avalara_object($account_number, $license_key, $api_mode)) {
      $ping_request = $avalara->ping();

      if ($ping_request['success'] && !empty($ping_request['result']['authenticated'])) {
        $valid = TRUE;
      }
    }

    if (!$valid) {
      form_set_error($account_number_field);
      form_set_error($license_key_field);
      drupal_set_message(t('Could not authenticate to the Avatax API.'), 'error');
    }
    else {
      $form_state['credentials_validated'] = TRUE;
      drupal_set_message(t('AvaTax response confirmed using the account and license key above.'));
    }
    $form_state['rebuild'] = TRUE;
  }
}

/**
 * Form validation handler for commerce_avalara_credentials_settings_form().
 */
function commerce_avalara_credentials_settings_form_validate($form, &$form_state) {
  if (empty($form_state['credentials_validated'])) {
    drupal_set_message(t('Please note that the credentials were not validated.'), 'warning');
  }
}

/**
 * Ajax: Returns container field for credential textfields.
 */
function commerce_avalara_ajax_mode_credentials($form, $form_state) {
  return $form['credentials'];
}
