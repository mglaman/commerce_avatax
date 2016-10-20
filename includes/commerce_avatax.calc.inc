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
 * @param string $transaction_type
 *   The transaction type (e.g SalesOrder|SalesInvoice).
 * @param bool $commit
 *   Boolean indicating whether or not to commit the transaction.
 *
 * @return array|bool
 *   An array containing the AvaTax request result
 *   or FALSE if the tax calculation failed.
 */
function commerce_avatax_create_transaction($order, $transaction_type = 'SalesOrder', $commit = FALSE) {
  $company_code = commerce_avatax_company_code();

  // TODO: Fail silently or improve the error message?
  if (empty($company_code) || !$avatax_object = commerce_avatax_object()) {
    drupal_set_message(t("The Avatax module is not properly configured, please configure the company code."), 'error');
    return FALSE;
  }
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);

  // Prepare the Request Body.
  $request_body = array(
    // TODO: Investigate why the SalesOrder request fails.
    'type' => $transaction_type,
    'companyCode' => $company_code,
    'date' => format_date(REQUEST_TIME, 'custom', 'c'),
    // TODO: Probably generate a unique code.
    'code' => 'DC-' . $order->order_id,
    'customerCode' => _commerce_avatax_transaction_get_customer_code($order),
    'currencyCode' => $order_wrapper->commerce_order_total->currency_code->value(),
    'addresses' => array(
      'shipFrom' => _commerce_avatax_transaction_get_ship_from(),
      'shipTo' => _commerce_avatax_transaction_get_ship_to($order),
    ),
  );

  if (!empty($commit)) {
    $request_body['commit'] = TRUE;
  }

  // For non anonymous orders, send the exemption code if it exists.
  if ($order->uid > 0) {
    // Check the Exemptions status.
    if (variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'exemptions_status', FALSE)) {
      if (isset($order_wrapper->owner->{COMMERCE_AVATAX_EXEMPTION_CODE_FIELD})) {
        $exemption_code = $order_wrapper->owner->{COMMERCE_AVATAX_EXEMPTION_CODE_FIELD}->value();

        // If we have an exemption code.
        if (!empty($exemption_code)) {
          // TODO: Check if we need to use the exemptionNo instead here.
          $request_body['customerUsageType'] = $exemption_code;
        }
      }
    }

    // Check if the user has a VAT ID field.
    if (variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'add_vat_field', FALSE)) {
      if (isset($order_wrapper->owner->{COMMERCE_AVATAX_VAT_ID_FIELD})) {
        $vat_id = $order_wrapper->owner->{COMMERCE_AVATAX_VAT_ID_FIELD}->value();
        if (!empty($vat_id)) {
          $request_body['businessIdentificationNo'] = $vat_id;
        }
      }
    }
  }

  $tax_included = FALSE;
  // If the ship from country is not in the US|CA, assume the price entered
  // is tax inclusive.
  if (isset($request_body['addresses']['shipFrom']['country'])) {
    if (!in_array($request_body['addresses']['shipFrom']['country'], array('CA', 'US'))) {
      $tax_included = TRUE;
    }
  }

  _commerce_avatax_transaction_add_lines($request_body, $order_wrapper->commerce_line_items, $tax_included);
  drupal_alter('commerce_avatax_create_transaction', $request_body, $order);

  // Create the transaction.
  $request = $avatax_object->transactionsCreate($request_body);

  return $request;
}

/**
 * Returns the transaction "lines" that needs to be sent to the API.
 *
 * @param array $request_body
 *   The request body that needs to be altered.
 * @param array $line_items
 *   The line items that need to be added to the transaction.
 * @param bool $tax_included
 *   Boolean indicating whether or not the tax is included.
 * @param
 */
function _commerce_avatax_transaction_add_lines(&$request_body, $line_items, $tax_included = FALSE) {
  $lines = array();
  $tax_included = $tax_included ? 'true' : 'false';

  foreach ($line_items as $delta => $line_item_wrapper) {
    // Ensure the line item still exists.
    if (!$line_item_wrapper->value()) {
      continue;
    }
    $line_item = $line_item_wrapper->value();

    // Handles products.
    if (in_array($line_item->type, commerce_product_line_item_types())) {
      $tax_code = '';

      // Get the tax code from the "Tax code" term referenced by the product.
      if (isset($line_item_wrapper->commerce_product->commerce_avatax_code)) {
        if ($line_item_wrapper->commerce_product->commerce_avatax_code->value()) {
          $tax_code = $line_item_wrapper->commerce_product->commerce_avatax_code->name->value();
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
        // The discounted boolean needs to be set to TRUE, otherwise, discount
        // document level won't be applied.
        'discounted' => 'true',
        'taxIncluded' => $tax_included,
      );
    }
    elseif ($line_item->type === 'shipping') {
      $lines[] = array(
        'id' => $line_item->line_item_id,
        'number' => $delta + 1,
        'itemCode' => 'Shipping',
        'description' => 'Shipping',
        // Retrieve the configured Shipping tax code.
        'taxCode' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'shipcode', 'FR020100'),
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
    // Add a document level discount if there's a discount applied to the order.
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
function _commerce_avatax_transaction_get_customer_code($order) {
  $customer_code = '';
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);

  // Get User name or e-mail address.
  if ($order->uid === 0) {
    if ($order->mail == '') {
      $customer_code = 'administrator';
    }
    else {
      $user_email = $order->mail;
      $customer_code = commerce_avatax_email_to_username($user_email);
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
function _commerce_avatax_transaction_get_ship_from() {
  return array(
    'line1' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'primary_street1', ''),
    'line2' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'primary_street2', ''),
    'city' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'primary_city', ''),
    'region' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'primary_state', ''),
    'country' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'primary_country', ''),
    'postalCode' => variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'primary_zip', ''),
  );
}

/**
 * Returns the shipTo address for a transaction.
 */
function _commerce_avatax_transaction_get_ship_to($order) {
  $ship_to = array();
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  $customer_profile_to_use = variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'tax_address', FALSE);
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
