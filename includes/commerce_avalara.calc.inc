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
 * @return array|bool
 *   An array containing the AvaTax request result
 *   or FALSE if the tax calculation failed.
 */
function commerce_avalara_create_transaction($order) {
  $company_code = commerce_avalara_company_code();

  // TODO: Fail silently or improve the error message?
  if (empty($company_code) || !$avalara_object = commerce_avalara_object()) {
    drupal_set_message(t("The Avalara module is not properly configured, please configure the company code."), 'error');
    return FALSE;
  }
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);

  // Prepare the Request Body.
  $request_body = array(
    // TODO: Investigate why the SalesOrder request fails.
    'type' => 'SalesInvoice',
    'companyCode' => $company_code,
    'date' => format_date(REQUEST_TIME, 'custom', 'c'),
    // TODO: Probably generate a unique code.
    'code' => 'DC-' . $order->order_id,
    'customerCode' => _commerce_avalara_transaction_get_customer_code($order),
    'currencyCode' => $order_wrapper->commerce_order_total->currency_code->value(),
    'addresses' => array(
      'shipFrom' => _commerce_avalara_transaction_get_ship_from(),
      'shipTo' => _commerce_avalara_transaction_get_ship_to($order),
    ),
  );

  // For non anonymous orders, send the exemption code if it exists.
  if ($order->uid > 0) {
    // Check the Exemptions status.
    if (variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'exemptions_status', FALSE)) {
      if (isset($order_wrapper->owner->avatax_exemption_code)) {
        $exemption_code = $order_wrapper->owner->avatax_exemption_code->value();

        // If we have an exemption code.
        if (!empty($exemption_code)) {
          // TODO: Check if we need to use the exemptionNo instead here.
          $request_body['customerUsageType'] = $exemption_code;
        }
      }
    }
  }
  _commerce_avalara_transaction_add_lines($request_body, $order_wrapper->commerce_line_items);

  // Create the transaction.
  $request = $avalara_object->transactionsCreate($company_code, array($request_body));

  // TODO: Map the error code to a meaningful message.
  if (empty($request['success'])) {
    drupal_set_message(t('Could not calculate the Tax'), 'error');
    return FALSE;
  }

  return $request;
}

/**
 * Returns the transaction "lines" that needs to be sent to the API.
 *
 * @param array $request_body
 *   The request body that needs to be altered.
 * @param array $line_items
 *   The line items that need to be added to the transaction.
 * @param
 */
function _commerce_avalara_transaction_add_lines(&$request_body, $line_items, $tax_type = 'sales_tax') {
  $lines = array();
  $product_version = commerce_avalara_product_version();

  foreach ($line_items as $delta => $line_item_wrapper) {
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

      // Get the total from the unit price if the tax type is VAT.
      if ($tax_type == 'vat') {
        $amount = $line_item_wrapper->commerce_unit_price->amount_decimal->value() * $line_item->quantity;
      }
      else {
        $amount = $line_item_wrapper->commerce_total->amount_decimal->value();
      }

      $lines[] = array(
        'id' => $line_item->line_item_id,
        'number' => $delta + 1,
        'itemCode' => $line_item_wrapper->commerce_product->sku->value(),
        'description' => $line_item_wrapper->commerce_product->title->value(),
        'taxCode' => $tax_code,
        'quantity' => $line_item->quantity,
        'amount' => $amount,
        // The discounted boolean needs to be set to TRUE, otherwise, discount
        // document level won't be applied.
        'discounted' => 'true',
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
      );
    }
    elseif ($line_item->type === 'commerce_discount') {
      $request_body['discount'] = $line_item_wrapper->commerce_total->amount_decimal->value() * -1;
    }
  }

  if ($lines) {
    $request_body['lines'] = $lines;
  }
}

/**
 * Helper function used to determine the customerCode sent.
 */
function _commerce_avalara_transaction_get_customer_code($order) {
  $customer_code = '';
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
  }

  return $customer_code;
}

/**
 * Returns the shipFrom address for a transaction.
 */
function _commerce_avalara_transaction_get_ship_from() {
  return array(
    'line1' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_street1', ''),
    'line2' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_street2', ''),
    'city' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_city', ''),
    'region' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_state', ''),
    'country' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_country', ''),
    'postalCode' => variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'primary_zip', ''),
  );
}

/**
 * Returns the shipTo address for a transaction.
 */
function _commerce_avalara_transaction_get_ship_to($order) {
  $ship_to = array();
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
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
      $ship_to = array(
        'line1' => $address['thoroughfare'],
        'line2' => $address['premise'],
        'city' => $address['locality'],
        'region' => $address['administrative_area'],
        'country' => $address['country'],
        'postalCode' => $address['postal_code'],
      );
    }
  }

  return $ship_to;
}

/**
 * Calculate the VAT for a given line item.
 *
 * @param object $line_item
 *   The line item object.
 *
 * @return array|bool
 *   An associative array of containing the vat 'amount' and 'currency_code' or
 *   FALSE if the amount could not be calculated.
 */
function commerce_avalara_calculate_vat_line_item($line_item) {
  $results = &drupal_static(__FUNCTION__, array());
  $company_code = commerce_avalara_company_code();

  // TODO: Fail silently or improve the error message?
  if (empty($company_code) || !$avalara_object = commerce_avalara_object()) {
    drupal_set_message(t("The Avalara module is not properly configured, please configure the company code."), 'error');
    return FALSE;
  }

  // Prevent calculating the VAT multiple times in the same request.
  if (!empty($line_item->commerce_product)) {
    $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
    $cid = 'sku_' . $line_item_wrapper->commerce_product->sku->value();
  }

  // If we couldn't generate a $cid, stop here.
  if (!isset($cid)) {
    return FALSE;
  }

  // Check if the result is statically cached.
  if (isset($results[$cid])) {
    return $results[$cid];
  }
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $order = $line_item_wrapper->order->value();

  // Prepare the transaction request body.
  $request_body = array(
    // TODO: Investigate why the SalesOrder request fails.
    'type' => 'SalesInvoice',
    'companyCode' => $company_code,
    'date' => format_date(REQUEST_TIME, 'custom', 'c'),
    'customerCode' => _commerce_avalara_transaction_get_customer_code($order),
    'addresses' => array(
      'shipFrom' => _commerce_avalara_transaction_get_ship_from(),
    ),
  );

  // Not sure which code we should generate here.
  $line_item_wrapper = entity_metadata_wrapper('commerce_line_item', $line_item);
  $code = 'SKU-' . $line_item_wrapper->commerce_product->sku->value();
  // Get the currency code from the line item.
  $request_body['currencyCode'] = $line_item_wrapper->commerce_unit_price->currency_code->value();
  $request_body['code'] = $code;

  // If the transaction lines couldn't be added to the request body.
  if (empty($request_body['lines'])) {
    _commerce_avalara_transaction_add_lines($request_body, array($line_item_wrapper), 'vat');
  }

  // Use the default Ship to country configured.
  if (empty($request_body['addresses']['shipTo'])) {
    $ship_to_country = variable_get(COMMERCE_AVALARA_VAR_PREFIX . 'vat_ship_to_country', '');

    if (!empty($ship_to_country)) {
      $request_body['addresses']['shipTo'] = array(
        'country' => $ship_to_country,
      );
    }
  }

  // Create the transaction.
  $request = $avalara_object->transactionsCreate($company_code, array($request_body));

  // TODO: Map the error code to a meaningful message.
  if (empty($request['success'])) {
    drupal_set_message(t('Could not calculate the VAT'), 'error');
    $results[$cid] = FALSE;
  }
  else {
    $results[$cid] = FALSE;

    // Get the total tax for this line item from the lines array.
    if (!empty($request['result'][0])) {
      $currency_code = $request['result'][0]['currencyCode'];
      foreach ($request['result'][0]['lines'] as $line) {
        // Skip the line if the tax is empty.
        if (empty($line['tax']) || !isset($line['itemCode'])) {
          continue;
        }

        $results[$cid][$line['itemCode']] = array(
          'currency_code' => $currency_code,
          'amount' => commerce_currency_decimal_to_amount($line['tax'], $currency_code),
        );
      }
    }
  }

  return $results[$cid];
}
