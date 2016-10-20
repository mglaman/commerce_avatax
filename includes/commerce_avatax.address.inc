<?php

/**
 * Validate the shipping address entered on checkout form.
 */
function commerce_avatax_validate_address($address) {
  $parameters = array();
  $address_mapping = commerce_avatax_address_fields_mapping();

  // Prepare the address to be validated.
  foreach ($address as $key => $value) {
    if (!isset($address_mapping[$key])) {
      continue;
    }
    $parameters[$address_mapping[$key]] = $value;
  }

  $result = commerce_avatax_v1_address_request('address/validate', $parameters);
  if (empty($result['body'])) {
    watchdog('commerce_avatax', 'Could not connect to AvaTax for address validation.');
    return NULL;
  }
  elseif ($result['body']['ResultCode'] != 'Success') {
    return array();
  }
  return $result['body']['Address'];
}

/**
 * Compare entered address and the address returned by AvaTax.
 */
function commerce_avatax_address_compare($original, $validated_address) {
  $result = array(
    'result' => '',
    'msg' => '',
    'suggestions' => array(),
  );

  // If the address couldn't be validated, stop there.
  if (!$validated_address) {
    $result['result'] = 'invalid';
    $result['msg'] = '<p>' . t('We could not validate the shipping address entered. Please check that you have entered the correct address.') . '</p>';
    $result['msg'] .= '<p>' . t('Entered address is:') . '</p>' . theme('commerce_avatax_address', array('address' => $original));
    return $result;
  }

  // Map the response back to Addressfield names.
  $address_mapping = commerce_avatax_address_fields_mapping(TRUE);
  $correct_address = array();
  $address_mismatch = FALSE;

  // Create an array of field keys to compare.
  $keys_to_compare = drupal_map_assoc(array(
    'thoroughfare',
    'premise',
    'locality',
    'administrative_area',
    'country',
  ));

  foreach ($validated_address as $key => $value) {
    if (!isset($address_mapping[$key])) {
      continue;
    }
    $correct_key = $address_mapping[$key];
    $correct_address[$correct_key] = $value;

    // Compare the user entered address & the validated address.
    if (isset($keys_to_compare[$correct_key]) && $correct_address[$correct_key] != $original[$correct_key]) {
      $address_mismatch = TRUE;
    }
  }

  // Validate the postal code.
  $post_code_full_validation = variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'address_postal_code', TRUE);
  $autocomplete_post_code = variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'autocomplete_postal_code', TRUE);
  $validated_postal_code = $correct_address['postal_code'];
  $original_postal_code = $original['postal_code'];
  if (!$post_code_full_validation || $autocomplete_post_code) {
    $validated_postal_code = substr($validated_postal_code, 0, 5);
    $original_postal_code = substr($original_postal_code, 0, 5);
  }
  $postal_code = $validated_postal_code == $original_postal_code;
  $suggestions = array($correct_address);

  if ($address_mismatch || !$postal_code) {
    $result['result'] = 'needs_correction';
    $form = drupal_get_form('commerce_avatax_address_suggestion_form', $original, $suggestions);
    $result['msg'] = drupal_render($form);
    $result['suggestions'] = $suggestions;
  }
  else {
    $result['result'] = 'valid';
    $result['msg'] = '';
    $result['suggestions'] = $suggestions;
  }

  return $result;
}
