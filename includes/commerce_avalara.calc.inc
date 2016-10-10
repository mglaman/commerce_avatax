<?php

/**
 * @file
 * AvaTax GetTax amount.
 */

/**
 * Gets the tax amount for the order based on the delivery address.
 *
 * @param object $order
 *   The order object.
 *
 * @return array
 *   An array containing the AvaTax request result
 *   or FALSE if the tax calculation failed.
 */
function commerce_avalara_create_transaction($order) {
  $company_code = commerce_avalara_company_code();

  // TODO: Fail silently or improve the error message?
  if (empty($company_code)) {
    drupal_set_message(t("The Avalara module is not properly configured, please configure the company code."), 'error');
  }
  $exemption_code = '';
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);

  // Get User name or e-mail address.
  if ($order->uid === 0) {
    if ($order->mail == '') {
      $customer_code = 'administrator';
    }
    else {
      $user_email = $order->mail;
      $customer_code = commerce_avalara_email_to_username($user_email);
    }
  }
  else {
    $user_wrapper = $order_wrapper->owner;
    $customer_code = $user_wrapper->name->value();

    // Check the Exemptions status.
    if (variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'exemptions_status', FALSE)) {
      if (isset($user_wrapper->avatax_exemption_code)) {
        $exemption_code = $user_wrapper->avatax_exemption_code->value();
      }
    }
  }

  // Prepare the Request Body.
  $request_body = array(
    // TODO: Investigate why the SalesOrder request fails.
    'type' => 'SalesInvoice',
    'companyCode' => $company_code,
    'date' => format_date(REQUEST_TIME, 'custom', 'c'),
    // TODO: Probably generate a unique code.
    'code' => 'DC-' . $order->order_id,
    'customerCode' => $customer_code,
    'currencyCode' => $order_wrapper->commerce_order_total->currency_code->value(),
    'addresses' => array(),
  );

  // If we have an exemption code.
  if (!empty($exemption_code)) {
    // TODO: Check if we need to use the exemptionNo instead here.
    $request_body['customerUsageType'] = $exemption_code;
  }

  // Prepare the Ships from address.
  $request_body['addresses']['ShipFrom'] = array(
    'line1' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_street1', ''),
    'line2' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_street2', ''),
    'city' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_city', ''),
    'region' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_state', ''),
    'country' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_country', ''),
    'postalCode' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_zip', ''),
  );

  $customer_profile_to_use = variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'commerce_avalara_tax_address', FALSE);
  // Retrieve the destination customer profile type to use for calculating the
  // Sales Tax.
  if (!$customer_profile_to_use) {
    $customer_profile_to_use = module_exists('commerce_shipping') ? 'shipping' : 'billing';
  }

  $customer_profile_field = 'commerce_customer_' . $customer_profile_to_use;
  // Retrieve the address from the configured customer profile type.
  if (!empty($order->{$customer_profile_field})) {
    if (isset($order_wrapper->{$customer_profile_field}->commerce_customer_address)) {
      $address = $order_wrapper->{$customer_profile_field}->commerce_customer_address->value();

      // Prepare the Ships from address.
      $request_body['addresses']['ShipTo'] = array(
        'line1' => $address['thoroughfare'],
        'line2' => $address['premise'],
        'city' => $address['locality'],
        'region' => $address['administrative_area'],
        'country' => $address['country'],
        'postalCode' => $address['postal_code'],
      );
    }
  }
  $product_version = commerce_avalara_product_version();
  $lines = array();

  // Loop over the line items.
  foreach ($order_wrapper->commerce_line_items as $delta => $line_item_wrapper) {
    // Ensure the line item still exists.
    if (!$line_item_wrapper->value()) {
      continue;
    }
    $line_item = $line_item_wrapper->value();

    // Handles products.
    if (in_array($line_item->type, commerce_product_line_item_types())) {
      // TODO: Defaults to a generic tax code? Or just skip the product if no
      // Tax code were set-up?
      $tax_code = '';

      // For the pro version, get the tax code from the Tax code term referenced
      // by the product.
      if ($product_version == COMMERCE_AVALARA_PRO_VERSION && isset($line_item_wrapper->commerce_product->avatax_code)) {
        if ($line_item_wrapper->commerce_product->avatax_code->value()) {
          $tax_code = $line_item_wrapper->commerce_product->avatax_code->name->value();
        }
      }
      $lines[] = array(
        'id' => $line_item->line_item_id,
        'number' => $delta + 1,
        'itemCode' => $line_item_wrapper->commerce_product->sku->value(),
        'description' => $line_item_wrapper->commerce_product->title->value(),
        'taxCode' => $tax_code,
        'quantity' => $line_item->quantity,
        'amount' => $line_item_wrapper->commerce_total->amount_decimal->value(),
        /*'Discounted' => 'true',
        'Ref1' => '',
        'Ref2' => '',
        'CustomerUsageType' => '',
        'OriginCode' => 0,
        'DestinationCode' => 1,*/
      );
    }
    elseif ($line_item->type === 'shipping') {
      $lines[] = array(
        'id' => $line_item->line_item_id,
        'number' => $delta + 1,
        'itemCode' => 'Shipping',
        'description' => 'Shipping',
        // Retrieve the configured Shipping tax code.
        'taxCode' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'shipcode', 'FR020100'),
        'quantity' => $line_item->quantity,
        'amount' => $line_item_wrapper->commerce_total->amount_decimal->value(),
        'discounted' => 'false',
        /*'Ref1' => '',
        'Ref2' => '',
        'CustomerUsageType' => '',
        'OriginCode' => 0,
        'DestinationCode' => 1,*/
      );
    }
    elseif ($line_item->type === 'commerce_coupon') {
      $lines[] = array(
        'id' => $line_item->line_item_id,
        'number' => $delta + 1,
        'itemCode' => 'Coupon',
        'description' => 'Coupon Amt',
        'taxCode' => '0D010000',
        'quantity' => $line_item->quantity,
        'amount' => $line_item_wrapper->commerce_total->amount_decimal->value(),
        'discounted' => 'false',
        /*'Ref1' => '',
        'Ref2' => '',
        'CustomerUsageType' => '',
        'OriginCode' => 0,
        'DestinationCode' => 1,*/
      );
    }
    elseif ($line_item->type === 'commerce_discount') {
      $request_body['discount'] = (($line_item_wrapper->commerce_unit_price->amount->value() / 100) * $line_item->quantity) * -1;
    }
  }
  $request_body['lines'] = $lines;

  // Get the library object.
  if ($avalara_object = commerce_avalara_object()) {
    $request = $avalara_object->transactionsCreate($company_code, array($request_body));

    // TODO: Map the error code to a meaningful message.
    if (empty($request['success'])) {
      drupal_set_message(t('AvaTax error: %msg', array(
        '%msg' => $request['result'],
      )), 'error');

      return FALSE;
    }

    return $request;
  }
  else {
    drupal_set_message(t("AvaTax did not get a response."), 'error');
    return FALSE;
  }
}
