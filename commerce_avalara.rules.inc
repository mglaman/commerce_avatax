<?php

/**
 * @file
 * Rules supporting AvaTax Order Processing.
 */

/**
 * Implements hook_rules_action_info().
 */
function commerce_avalara_rules_action_info() {
  $parameter = array(
    'commerce_order' => array(
      'type' => 'commerce_order',
      'label' => t('Commerce Order'),
    ),
  );
  $actions = array(
    'commerce_avalara_calculate_sales_tax' => array(
      'label' => t('Calculate sales tax for an order'),
      'group' => t('Commerce AvaTax'),
      'parameter' => $parameter,
    ),
    'commerce_avalara_delete_sales_tax_line_items' => array(
      'label' => t('Delete AvaTax line items'),
      'group' => t('Commerce AvaTax'),
      'parameter' => $parameter,
    ),
    'commerce_avalara_commit_transaction' => array(
      'label' => t('Change status of sales tax to COMMITTED in AvaTax'),
      'group' => t('Commerce AvaTax'),
      'parameter' => $parameter,
    ),
    'commerce_avalara_void_transaction' => array(
      'label' => t('Change status of sales tax to VOIDED in AvaTax'),
      'group' => t('Commerce AvaTax'),
      'parameter' => $parameter,
    ),
  );

  return $actions;
}

/**
 * COMMIT AvaTax transaction for a given $order.
 */
function commerce_avalara_commit_transaction($order) {
}

/**
 * VOID AvaTax transaction for $order.
 */
function commerce_avalara_void_transaction($order) {
}
