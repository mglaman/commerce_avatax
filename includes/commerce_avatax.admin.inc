<?php
/**
 * @file
 * Administrative callbacks for the Commerce Avatax module.
 */

/**
 * Builds the AvaTax credentials settings form.
 */
function commerce_avatax_credentials_settings_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'commerce_avatax', 'includes/commerce_avatax.admin');
  $var_prefix = COMMERCE_AVATAX_VAR_PREFIX;
  $modes = array(
    COMMERCE_AVATAX_DEVELOPMENT_MODE => t('Development'),
    COMMERCE_AVATAX_PRODUCTION_MODE => t('Production'),
  );
  $api_mode = commerce_avatax_api_mode();

  if (!empty($form_state['values'][$var_prefix . 'api_mode'])) {
    $api_mode = $form_state['values'][$var_prefix . 'api_mode'];
  }

  $form['credentials'] = array(
    '#title' => t('API Credentials'),
    '#type' => 'fieldset',
    '#attributes' => array(
      'id' => 'commerce-avatax-credentials-wrapper',
    ),
    '#description' => t('Please note that only the account number and the license key are validated.')
  );

  $form['credentials'][$var_prefix . 'api_mode'] = array(
    '#title' => t('API Mode'),
    '#description' => t('Only select Production after completing the GO LIVE process with Avatax'),
    '#type' => 'select',
    '#options' => $modes,
    '#default_value' => $api_mode,
    '#ajax' => array(
      'callback' => 'commerce_avatax_ajax_mode_credentials',
      'wrapper' => 'commerce-avatax-credentials-wrapper',
    ),
  );

  // Allow to set different credentials for each mode (dev|prod).
  $fields = array(
    COMMERCE_AVATAX_COMPANY_CODE => array(
      'title' => t('@mode company code', array('@mode' => $modes[$api_mode])),
      'description' => t('The Avatax company code.'),
      'required' => FALSE,
    ),
    COMMERCE_AVATAX_ACCOUNT_NUMBER => array(
      'title' => t('@mode account number', array('@mode' => $modes[$api_mode])),
      'description' => t('The Avatax account number that is used to authenticate against the API.'),
      'required' => TRUE,
    ),
    COMMERCE_AVATAX_LICENSE_KEY => array(
      'title' => t('@mode license key', array('@mode' => $modes[$api_mode])),
      'description' => t('The Avatax license key that is used to authenticate against the API.'),
      'required' => TRUE,
    ),
  );

  $display_warning = FALSE;
  foreach ($fields as $key => $conf) {
    $variable_name = $var_prefix . $api_mode . '_' . $key;
    $variable_value = variable_get($variable_name, '');
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
    '#validate' => array('commerce_avatax_credentials_settings_validate'),
    '#ajax' => array(
      'callback' => 'commerce_avatax_ajax_mode_credentials',
      'wrapper' => 'commerce-avatax-credentials-wrapper',
    ),
  );

  $form = system_settings_form($form);
  return $form;
}

/**
 * Validate handler for the "Validate credentials" button.
 */
function commerce_avatax_credentials_settings_validate($form, &$form_state) {
  $var_prefix = COMMERCE_AVATAX_VAR_PREFIX;
  $api_mode = $form_state['values'][$var_prefix . 'api_mode'];
  $account_number_field = $var_prefix . $api_mode . '_' . COMMERCE_AVATAX_ACCOUNT_NUMBER;
  $license_key_field = $var_prefix . $api_mode . '_' . COMMERCE_AVATAX_LICENSE_KEY;
  $account_number = $form_state['values'][$account_number_field];
  $license_key = $form_state['values'][$license_key_field];
  $valid = FALSE;

  // Validate the credentials provided.
  if (!empty($account_number) && !empty($license_key)) {
    $api_key = base64_encode("$account_number:$license_key");
    if ($avatax = commerce_avatax_object($api_key, $api_mode)) {
      $ping_request = $avatax->ping();

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
 * Form validation handler for commerce_avatax_credentials_settings_form().
 */
function commerce_avatax_credentials_settings_form_validate($form, &$form_state) {
  if (empty($form_state['credentials_validated'])) {
    drupal_set_message(t('Please note that you have to click on the "Validate credentials" button in order to test the connectivity to the API.'), 'warning');
  }
}

/**
 * Ajax: Returns container field for credential textfields.
 */
function commerce_avatax_ajax_mode_credentials($form, $form_state) {
  return $form['credentials'];
}

/**
 * Builds the AvaTax address settings form.
 */
function commerce_avatax_address_settings_form($form, &$form_state) {
  $form['address'] = array(
    '#title' => t('Address validation'),
    '#type' => 'fieldset',
    '#attributes' => array(
      'id' => 'commerce-avatax-address-wrapper'
    ),
  );
  $var_prefix = COMMERCE_AVATAX_VAR_PREFIX;

  $form['address'][$var_prefix . 'validate_address'] = array(
    '#type' => 'checkbox',
    '#title' => t('Validate addresses during checkout'),
    '#description' => t('If checked, addresses will be validated during checkout.'),
    '#default_value' => variable_get($var_prefix . 'validate_address', TRUE),
  );
  $countries = variable_get($var_prefix . 'address_validate_countries', array('US'));
  if (!is_array($countries)) {
    $countries = array($countries);
  }
  $form['address']['options'] = array(
    '#type' => 'container',
    '#states' => array(
      'visible' => array(
        ':input[name="' . $var_prefix . 'validate_address"]' => array('checked' => TRUE),
      ),
    ),
  );
  $form['address']['options'][$var_prefix . 'address_validate_countries'] = array(
    '#type' => 'select',
    '#title' => t('Countries'),
    '#description' => t('Countries to validate addresses?'),
    '#options' => array(
      'CA' => t('Canada'),
      'US' => t('United States'),
    ),
    '#multiple' => TRUE,
    '#default_value' => $countries,
    '#required' => TRUE,
  );
  $form['address']['options'][$var_prefix . 'address_postal_code'] = array(
    '#type' => 'checkbox',
    '#title' => t('Match on postal code'),
    '#description' => t('Postal codes are 9 digits, but most people enter the first 5 digits, do you want AvaTax to match all 9 digits?'),
    '#default_value' => variable_get($var_prefix . 'address_postal_code', TRUE),
  );
  $form['address']['options'][$var_prefix . 'autocomplete_postal_code'] = array(
    '#type' => 'checkbox',
    '#title' => t('Auto complete 5 digit postal code to 9 digits'),
    '#description' => t('Automatically insert the 9 digit postal code provided by AvaTax.'),
    '#default_value' => variable_get($var_prefix . 'autocomplete_postal_code', TRUE),
  );

  return system_settings_form($form);
}

/**
 * Builds the AvaTax general settings form.
 */
function commerce_avatax_general_settings_form($form, &$form_state) {
  $form['general'] = array(
    '#title' => t('General settings'),
    '#type' => 'fieldset',
  );
  $form['general'][COMMERCE_AVATAX_VAR_PREFIX . 'product_types'] = array(
    '#title' => t('Product types'),
    '#type' => 'select',
    '#multiple' => TRUE,
    '#options' => commerce_product_type_get_name(),
    '#default_value' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'product_types', array()),
    '#description' => t('Select the product types for which you want to customize tax codes that are sent to Avatax, otherwise the generic code P0000000 is going to be used.'),
  );
  $form['general'][COMMERCE_AVATAX_VAR_PREFIX . 'exemptions_status'] = array(
    '#title' => t('Administer Sales Tax Exemptions'),
    '#description' => t('Registered users with the "configure avatax exemptions" permission will be able to specify a tax exemption code.'),
    '#type' => 'checkbox',
    '#default_value' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'exemptions_status', 0),
  );
  $form['general'][COMMERCE_AVATAX_VAR_PREFIX . 'tax_calculation_enabled'] = array(
    '#title' => t('Enable tax calculation?'),
    '#description' => t('Uncheck this to disable tax calculation.'),
    '#type' => 'checkbox',
    '#default_value' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'tax_calculation_enabled', TRUE),
  );
  $form['general'][COMMERCE_AVATAX_VAR_PREFIX . 'enable_logging'] = array(
    '#title' => t('Enable logging'),
    '#type' => 'checkbox',
    '#description' => t('Enables detailed AvaTax transaction logging.'),
    '#default_value' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'enable_logging', FALSE),
  );
  $form['general'][COMMERCE_AVATAX_VAR_PREFIX . 'disable_document_committing'] = array(
    '#title' => t('Disable document committing'),
    '#type' => 'checkbox',
    '#default_value' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'disable_document_committing', FALSE),
  );
  $form = system_settings_form($form);
  $form['#submit'][] = 'commerce_avatax_general_settings_form_submit';

  return $form;
}

/**
 * Call the field refresh function.
 */
function commerce_avatax_general_settings_form_submit($form, &$form_state) {
  commerce_avatax_flush_caches();
}

/**
 * Builds the AvaTax Shipping settings form.
 */
function commerce_avatax_shipping_settings_form($form, &$form_state) {
  include_once DRUPAL_ROOT . '/includes/locale.inc';
  $var_prefix = COMMERCE_AVATAX_VAR_PREFIX;
  $form['shipping'] = array(
    '#title' => t('Shipping settings'),
    '#type' => 'fieldset',
  );
  $states = array(
    'AL' => t('Alabama'),
    'AK' => t('Alaska'),
    'AZ' => t('Arizona'),
    'AR' => t('Arkansas'),
    'CA' => t('California'),
    'CO' => t('Colorado'),
    'CT' => t('Connecticut'),
    'DE' => t('Delaware'),
    'DC' => t('District Of Columbia'),
    'FL' => t('Florida'),
    'GA' => t('Georgia'),
    'HI' => t('Hawaii'),
    'ID' => t('Idaho'),
    'IL' => t('Illinois'),
    'IN' => t('Indiana'),
    'IA' => t('Iowa'),
    'KS' => t('Kansas'),
    'KY' => t('Kentucky'),
    'LA' => t('Louisiana'),
    'ME' => t('Maine'),
    'MD' => t('Maryland'),
    'MA' => t('Massachusetts'),
    'MI' => t('Michigan'),
    'MN' => t('Minnesota'),
    'MS' => t('Mississippi'),
    'MO' => t('Missouri'),
    'MT' => t('Montana'),
    'NE' => t('Nebraska'),
    'NV' => t('Nevada'),
    'NH' => t('New Hampshire'),
    'NJ' => t('New Jersey'),
    'NM' => t('New Mexico'),
    'NY' => t('New York'),
    'NC' => t('North Carolina'),
    'ND' => t('North Dakota'),
    'OH' => t('Ohio'),
    'OK' => t('Oklahoma'),
    'OR' => t('Oregon'),
    'PA' => t('Pennsylvania'),
    'RI' => t('Rhode Island'),
    'SC' => t('South Carolina'),
    'SD' => t('South Dakota'),
    'TN' => t('Tennessee'),
    'TX' => t('Texas'),
    'UT' => t('Utah'),
    'VT' => t('Vermont'),
    'VA' => t('Virginia'),
    'WA' => t('Washington'),
    'WV' => t('West Virginia'),
    'WI' => t('Wisconsin'),
    'WY' => t('Wyoming'),
    'AA' => t('Armed Forces (Americas)'),
    'AE' => t('Armed Forces (Europe, Canada, Middle East, Africa)'),
    'AP' => t('Armed Forces (Pacific)'),
    'AS' => t('American Samoa'),
    'FM' => t('Federated States of Micronesia'),
    'GU' => t('Guam'),
    'MH' => t('Marshall Islands'),
    'MP' => t('Northern Mariana Islands'),
    'PW' => t('Palau'),
    'PR' => t('Puerto Rico'),
    'VI' => t('Virgin Islands'),
  );

  $form['shipping'][$var_prefix . 'primary_country'] = array(
    '#title' => t('Primary Business Country'),
    '#description' => t('The primary country your business is located in.'),
    '#type' => 'select',
    '#required' => TRUE,
    '#options' => country_get_list(),
    '#default_value' => variable_get($var_prefix . 'primary_country', 'US'),
  );

  // Select the states that will use AvaTax to calculate the sales tax amount.
  $form['shipping'][$var_prefix . 'select_states'] = array(
    '#title' => t('AvaTax States'),
    '#description' => t('Select States for which to enable Sales tax calculation - Leave blank for all states.'),
    '#type' => 'select',
    '#multiple' => TRUE,
    '#options' => $states,
    '#default_value' => variable_get($var_prefix . 'select_states', array()),
    '#states' => array(
      'visible' => array(
        ':input[name="' . $var_prefix  . 'primary_country"]' => array('value' => 'US'),
      ),
    ),
  );

  // Set Shipping Tax code to be used by AvaTax.
  $form['shipping'][$var_prefix . 'shipcode'] = array(
    '#title' => t('Shipping Tax Code'),
    '#description' => t('The Sales Tax code to be used for Shipping.'),
    '#type' => 'textfield',
    '#default_value' => variable_get($var_prefix . 'shipcode', 'FR020100'),
  );

  // Configure address to use for sales tax.
  $form['shipping'][$var_prefix . 'tax_address'] = array(
    '#title' => t('Destination Address to use for Sales Tax calculation'),
    '#description' => t('Select "Shipping information" if Commerce Shipping is installed.'),
    '#type' => 'select',
    '#options' => commerce_customer_profile_type_options_list(),
    '#default_value' => variable_get($var_prefix . 'tax_address', 'shipping'),
  );

  // Set Street, City, State and Zip for Primary Business Office Location.
  $form['shipping'][$var_prefix . 'account_name'] = array(
    '#title' => t('Business or Company Name'),
    '#description' => t('Registered Name of Company or Business.'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#default_value' => variable_get($var_prefix . 'account_name', ''),
  );
  $form['shipping'][$var_prefix . 'primary_street1'] = array(
    '#title' => t('Primary Business Street 1'),
    '#description' => t('The Primary Street 1 your business is located in.'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#default_value' => variable_get($var_prefix . 'primary_street1', ''),
  );
  $form['shipping'][$var_prefix . 'primary_street2'] = array(
    '#title' => t('Primary Business Street 2'),
    '#description' => t('The Primary Street 2 your business is located in.'),
    '#type' => 'textfield',
    '#default_value' => variable_get($var_prefix . 'primary_street2', ''),
  );
  $form['shipping'][$var_prefix . 'primary_city'] = array(
    '#title' => t('Primary Business City'),
    '#description' => t('The Primary City your business is located in.'),
    '#required' => TRUE,
    '#type' => 'textfield',
    '#default_value' => variable_get($var_prefix . 'primary_city', ''),
  );
  $form['shipping'][$var_prefix . 'primary_state'] = array(
    '#title' => t('Primary Business State'),
    '#description' => t('The Primary State your business is located in.'),
    '#type' => 'select',
    '#options' => $states,
    '#default_value' => variable_get($var_prefix . 'primary_state', ''),
    '#states' => array(
      'visible' => array(
        ':input[name="' . $var_prefix  . 'primary_country"]' => array('value' => 'US'),
      ),
    ),
  );
  $form['shipping'][$var_prefix . 'primary_zip'] = array(
    '#title' => t('Primary Business Zip'),
    '#description' => t('The Primary Zip Code your business is located in. NB - Must be a Valid 5 digit zip.'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#default_value' => variable_get($var_prefix . 'primary_zip', ''),
  );

  return system_settings_form($form);
}

/**
 * Form callback: confirmation form for manually calculating Taxes of an order.
 *
 * @param $order
 *   The order object to apply the Taxes to.
 *
 * @return
 *   The form array to confirm the calculation.
 *
 * @see confirm_form()
 */
function commerce_avatax_order_admin_calculate_tax_form($form, &$form_state, $order) {
  $form['order_id'] = array(
    '#type' => 'value',
    '#value' => $order->order_id,
  );

  // Build a description of what the user may expect to occur on submission.
  $items = array(
    t('The AvaTax Service needs a valid postal address.'),
    t('Please note that a SalesOrder transaction is going to be created, the committed SalesInvoice transaction will be created once the order is paid in full.'),
  );

  $form = confirm_form($form,
    t('Are you sure you want to apply Taxes to order @number?', array('@number' => $order->order_number)),
    'admin/commerce/orders/' . $order->order_id . '/edit',
    '<p>' . theme('item_list', array('items' => $items)) . '</p>',
    t('Calculate Taxes'),
    t('Cancel')
  );

  return $form;
}

/**
 * Form submit callback for commerce_avatax_order_admin_calculate_tax_form().
 */
function commerce_avatax_order_admin_calculate_tax_form_submit($form, &$form_state) {
  if ($order = commerce_order_load($form_state['values']['order_id'])) {
    // Check if the request didn't fail.
    if ((!$request = commerce_avatax_calculate_tax($order)) || empty($request['response'])) {
      drupal_set_message(t('The taxes could not be calculated for this order.'), 'error');
      $form_state['redirect'] = 'admin/commerce/orders/' . $order->order_id . '/edit';
      return;
    }
    $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
    $tax_line_item_found = FALSE;

    // Look for an AvaTax line item.
    foreach ($order_wrapper->commerce_line_items as $line_item_wrapper) {
      if ($line_item_wrapper->type->value() == 'avatax') {
        $tax_line_item_found = TRUE;
        break;
      }
    }

    $response = $request['response'];
    // Save the order if the tax calculation was performed.
    if (!empty($response['success']) && $tax_line_item_found) {
      commerce_order_save($order);
      drupal_set_message(t('Taxes have been applied and the order updated.'));
    }
    else {
      $error = t('Unknown error');
      // Display a detailed error message if available.
      if (!empty($response['error']) && isset($response['error']['message'])) {
        $error = $response['error']['message'];
      }
      drupal_set_message(t('The taxes could not be calculated for this order (@error)', array('@error' => $error)), 'error');
    }
    $form_state['redirect'] = 'admin/commerce/orders/' . $order->order_id . '/edit';
  }
  else {
    drupal_set_message(t('Order not found.'), 'error');
    $form_state['redirect'] = 'admin/commerce/orders';
  }
}

/**
 * Builds the AvaTax Global VAT settings form.
 */
function commerce_avatax_global_vat_settings_form($form, &$form_state) {
  $form['global-vat'][COMMERCE_AVATAX_VAR_PREFIX . 'add_vat_field'] = array(
    '#title' => t('Allow customers to supply a VAT ID for tax-exemptions'),
    '#description' => t('Customers will be able to supply a VAT ID that will be submitted as their business identification number.'),
    '#type' => 'checkbox',
    '#default_value' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'add_vat_field', FALSE),
  );
  $form['global-vat'][COMMERCE_AVATAX_VAR_PREFIX . 'vat_inclusive'] = array(
    '#title' => t('Prices entered are VAT-inclusive.'),
    '#type' => 'checkbox',
    '#default_value' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'vat_inclusive', TRUE),
  );
  $form = system_settings_form($form);
  // This will call the install helper in order to refresh the fields.
  $form['#submit'][] = 'commerce_avatax_general_settings_form_submit';
  return $form;
}
