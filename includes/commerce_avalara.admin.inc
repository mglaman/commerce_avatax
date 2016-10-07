<?php
/**
 * @file
 * Administrative callbacks for the Commerce Avalara module.
 */

/**
 * Builds the Avalara credentials settings form.
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
      'id' => 'commerce-avalara-credentials-wrapper',
    ),
    '#description' => t('Please note that only the account number and the license key are validated.')
  );

  $form['credentials']['commerce_avalara_api_mode'] = array(
    '#title' => t('API Mode'),
    '#description' => t('Only select Production after completing the GO LIVE process with Avalara'),
    '#type' => 'select',
    '#options' => $modes,
    '#default_value' => $api_mode,
    '#ajax' => array(
      'callback' => 'commerce_avalara_ajax_mode_credentials',
      'wrapper' => 'commerce-avalara-credentials-wrapper',
    ),
  );

  // Allow to set different credentials for each mode (dev|prod).
  $fields = array(
    COMMERCE_AVALARA_COMPANY_CODE_FIELD => array(
      'title' => t('@mode company code', array('@mode' => $modes[$api_mode])),
      'description' => t('The Avalara company code.'),
      'required' => FALSE,
    ),
    COMMERCE_AVALARA_ACCOUNT_NUMBER_FIELD => array(
      'title' => t('@mode account number', array('@mode' => $modes[$api_mode])),
      'description' => t('The Avalara account number that is used to authenticate against the API.'),
      'required' => TRUE,
    ),
    COMMERCE_AVALARA_LICENSE_KEY_FIELD => array(
      'title' => t('@mode license key', array('@mode' => $modes[$api_mode])),
      'description' => t('The Avalara license key that is used to authenticate against the API.'),
      'required' => TRUE,
    ),
  );

  $display_warning = FALSE;
  foreach ($fields as $key => $conf) {
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
        '#required' => $conf['required'],
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
      'wrapper' => 'commerce-avalara-credentials-wrapper',
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
    drupal_set_message(t('Please note that you have to click on the "Validate credentials" button in order to test the connectivity to the API.'), 'warning');
  }
}

/**
 * Ajax: Returns container field for credential textfields.
 */
function commerce_avalara_ajax_mode_credentials($form, $form_state) {
  return $form['credentials'];
}

/**
 * Builds the Avalara address settings form.
 */
function commerce_avalara_address_settings_form($form, &$form_state) {
  $form['address'] = array(
    '#title' => t('Address validation'),
    '#type' => 'fieldset',
    '#attributes' => array(
      'id' => 'commerce-avalara-address-wrapper'
    ),
  );

  $form['address']['commerce_avalara_validate_address'] = array(
    '#type' => 'checkbox',
    '#title' => t('Validate shipping address during checkout'),
    '#default_value' => variable_get('commerce_avatax_validate_address', TRUE),
  );
  $validate_address = variable_get('commerce_avalara_validate_address', TRUE);
  if (isset($form_state['values']['commerce_avalara_validate_address'])) {
    $validate_address = $form_state['values']['commerce_avalara_validate_address'];
  }
  if ($validate_address) {
    $countries = variable_get('commerce_avalara_address_validate_countries', array('US'));
    if (!is_array($countries)) {
      $countries = array($countries);
    }
    $form['address']['commerce_avalara_address_validate_countries'] = array(
      '#type' => 'select',
      '#title' => t('Countries'),
      '#description' => t('Countries to validate addresses?'),
      '#options' => array(
        'US' => t('United States'),
      ),
      '#default_value' => $countries,
      '#required' => TRUE,
    );
    $form['address']['commerce_avalara_address_postal_code'] = array(
      '#type' => 'checkbox',
      '#title' => t('Match on postal code'),
      '#description' => t('Postal codes are 9 digits, but most people enter the first 5 digits, do you want AvaTax to match all 9 digits?'),
      '#default_value' => variable_get('commerce_avalara_address_postal_code', TRUE),
    );
    $form['address']['commerce_avalara_autocomplete_postal_code'] = array(
      '#type' => 'checkbox',
      '#title' => t('Auto complete 5 digit postal code to 9 digits'),
      '#description' => t('Automatically insert the 9 digit postal code provided by AvaTax'),
      '#default_value' => variable_get('commerce_avalara_autocomplete_postal_code', TRUE),
    );
  }

  return system_settings_form($form);
}
