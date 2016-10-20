<?php

/**
 * @file
 * This file contains no working PHP code; it exists to provide additional
 * documentation for doxygen as well as to document hooks in the standard
 * Drupal manner.
 */

/**
 * Allows modules to alter the create transaction request before its sent to the
 * Avatax API.
 *
 * @param array $request_body
 *   The request body array.
 * @param object $order
 *   The order object.
 *
 * @see commerce_avatax_create_transaction().
 */
function hook_commerce_avatax_create_transaction_alter(&$request_body, $order) {

}
