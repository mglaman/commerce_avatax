<?php

/**
 * Validate the shipping address entered on checkout form.
 */
function commerce_avalara_validate_address($address) {
  $parameters = array();
  if ($address['line1']) {
    $parameters['Line1'] = $address['line1'];
  }
  if ($address['line2']) {
    $parameters['Line2'] = $address['line2'];
  }
  if ($address['city']) {
    $parameters['City'] = $address['city'];
  }
  if ($address['state']) {
    $parameters['Region'] = $address['state'];
  }
  if ($address['postal_code']) {
    $parameters['PostalCode'] = $address['postal_code'];
  }

  $result = commerce_avalara_v1_address_request('address/validate', $parameters);
  if (empty($result['body'])) {
    watchdog('commerce_avalara', 'Could not connect to AvaTax for address validation.');
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
function commerce_avalara_address_compare($original, $validated_address) {
  $result = array(
    'result' => '',
    'msg' => '',
    'suggestions' => array(),
  );
  $correct_address = array(
    'line1' => isset($validated_address['Line1']) ? $validated_address['Line1'] : '',
    'line2' => isset($validated_address['Line2']) ? $validated_address['Line2'] : '',
    'city' => isset($validated_address['City']) ? $validated_address['City'] : '',
    'state' => isset($validated_address['Region']) ? $validated_address['Region'] : '',
    'country' => isset($validated_address['Country']) ? $validated_address['Country'] : '',
    'postal_code' => isset($validated_address['PostalCode']) ? $validated_address['PostalCode'] : '',
  );
  if (!$validated_address) {
    $result['result'] = 'invalid';
    $result['msg'] = '<p>' . t('We could not validate the shipping address entered. Please check that you have entered the correct address.') . '</p>';
    $result['msg'] .= '<p>' . t('Entered address is:') . '</p>' . theme('commerce_avalara_address', array('address' => $original));
    return $result;
  }

  $line1 = $correct_address['line1'] == $original['line1'];
  $line2 = $correct_address['line2'] == $original['line2'];
  $city = $correct_address['city'] == $original['city'];
  $state = $correct_address['state'] == $original['state'];
  $country = $correct_address['country'] == $original['country'];

  $post_code_full_validation = variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'address_postal_code', TRUE);
  $autocomplete_post_code = variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'autocomplete_postal_code', TRUE);
  $validated_postal_code = $correct_address['postal_code'];
  $original_postal_code = $original['postal_code'];
  if (!$post_code_full_validation || $autocomplete_post_code) {
    $validated_postal_code = substr($validated_postal_code, 0, 5);
    $original_postal_code = substr($original_postal_code, 0, 5);
  }
  $postal_code = $validated_postal_code == $original_postal_code;

  if (!$line1 || !$line2 || !$city || !$state || !$country || !$postal_code) {
    $result['result'] = 'needs_correction';
    $form = drupal_get_form('commerce_avalara_address_suggestion_form', $original, array($correct_address));
    $result['msg'] = drupal_render($form);
    $result['suggestions'] = array($correct_address);
  }
  else {
    $result['result'] = 'valid';
    $result['msg'] = '';
    $result['suggestions'] = array($correct_address);
  }

  return $result;
}
