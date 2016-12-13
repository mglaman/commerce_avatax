<?php
/**
 * @file
 * AvaTax calculation/requests functions.
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
 *   An array containing the AvaTax request result, FALSE in case of failure.
 */
function commerce_avatax_create_transaction($order, $transaction_type = 'SalesOrder', $commit = FALSE) {
  $company_code = commerce_avatax_company_code();

  if (empty($company_code) || !$avatax_object = commerce_avatax_object()) {
    drupal_set_message(t("The Avatax module is not properly configured, please configure the company code."), 'error');
    return FALSE;
  }
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);

  // Return FALSE in case there are no line items.
  if ($order_wrapper->commerce_line_items->count() === 0) {
    return FALSE;
  }

  // Prepare the Request Body.
  $request_body = array(
    'type' => $transaction_type,
    'companyCode' => $company_code,
    'date' => format_date(REQUEST_TIME, 'custom', 'c'),
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

        if (!empty($exemption_code)) {
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
      $tax_included = variable_get(COMMERCE_AVATAX_VAR_PREFIX . 'vat_inclusive', TRUE);
    }
  }

  _commerce_avatax_transaction_add_lines($request_body, $order_wrapper->commerce_line_items, $tax_included);
  drupal_alter('commerce_avatax_create_transaction', $request_body, $order);

  return array(
    'request' => $request_body,
    'response' => $avatax_object->transactionsCreate($request_body),
  );
}

/**
 * Returns the transaction "lines" that needs to be sent to the API.
 *
 * @param array $request_body
 *   The request body that needs to be altered.
 * @param array $line_items
 *   An array of line items wrapper that need to be added to the transaction.
 * @param bool $tax_included
 *   Boolean indicating whether or not the tax is included.
 * @param
 */
function _commerce_avatax_transaction_add_lines(&$request_body, $line_items, $tax_included = FALSE) {
  $lines = array();
  $tax_included = $tax_included ? 'true' : 'false';
  $discounted_line_item_types = array();

  // Commerce discount 2.x integration.
  if (module_exists('commerce_discount')) {
    $discounts = array();
    $discount_line_items = array();

    // Search for discounts.
    foreach ($line_items as $line_item_wrapper) {
      // Skip non discount line items.
      if ($line_item_wrapper->type->value() != 'commerce_discount' || !$line_item_wrapper->value()) {
        continue;
      }
      $line_item = $line_item_wrapper->value();
      $discount_line_items[] = $line_item_wrapper;

      if (isset($line_item->data['discount_name'])) {
        $discounts[] = $line_item->data['discount_name'];
      }
    }

    // We need to load the discounts to see if "% offer types" are applied to
    // this order.
    if ($discounts) {
      // In case we have only have a "% discount offer type", we need to
      // properly tell AvaTax which line items are discounted.
      $discount_offer_types = array();

      // Load the discounts to see if we have any "%offer types".
      if ($discounts = entity_load_multiple_by_name('commerce_discount', $discounts)) {
        foreach ($discounts as $discount) {
          if (empty($discount->commerce_discount_offer)) {
            continue;
          }
          $discount_wrapper = entity_metadata_wrapper('commerce_discount', $discount);
          $offer_type = $discount_wrapper->commerce_discount_offer->type->value();
          $discount_offer_types[$offer_type] = $offer_type;
        }
      }

      // In case of percentage discount, retrieve the line item types the
      // discount were applied to.
      if (isset($discount_offer_types['percentage']) && count($discount_offer_types) === 1) {
        $discounted_line_item_types = variable_get('commerce_discount_line_item_types', array_diff(commerce_product_line_item_types(), array('product_discount')));
      }

      // Now calculate the actual discount amount.
      if ($discount_line_items) {
        $discount_total = commerce_line_items_total($discount_line_items);

        if ($discount_total['amount'] < 0) {
          $request_body['discount'] = commerce_currency_amount_to_decimal($discount_total['amount'], $discount_total['currency_code']) * -1;
        }
      }
    }
  }

  // If we found discounts, and we weren't able to specify which
  // line item types are discounted, assume all are.
  if (!empty($request_body['discount']) && !$discounted_line_item_types) {
    $line_item_types = commerce_line_item_types();
    $discounted_line_item_types = array_keys($line_item_types);
  }
  $discounted_line_item_types = array_filter($discounted_line_item_types);

  // Loop over the line items passed.
  foreach ($line_items as $delta => $line_item_wrapper) {
    // Ensure the line item still exists.
    if (!$line_item_wrapper->value()) {
      continue;
    }
    $line_item = $line_item_wrapper->value();

    $discounted = 'false';
    // Check if this line item type is discounted.
    if (!empty($request_body['discount']) && in_array($line_item->type, $discounted_line_item_types)) {
      $discounted = 'true';
    }

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
        'discounted' => $discounted,
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
        // Shipping shouldn't be discounted.
        'discounted' => 'false',
      );
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
    $customer_code = $order_wrapper->owner->name->value();
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
  $customer_profile_field = commerce_avatax_get_customer_profile_field();

  // Retrieve the address from the configured customer profile type.
  if (!empty($customer_profile_field) && !empty($order->{$customer_profile_field})) {
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
