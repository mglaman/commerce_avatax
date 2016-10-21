<?php

/**
 * @file
 * Rules supporting AvaTax Order Processing.
 */

/**
 * Implements hook_rules_action_info().
 */
function commerce_avatax_rules_action_info() {
  $parameter = array(
    'commerce_order' => array(
      'type' => 'commerce_order',
      'label' => t('Commerce Order'),
    ),
  );
  $actions = array(
    'commerce_avatax_calculate_tax' => array(
      'label' => t('Calculate sales tax for an order'),
      'group' => t('Commerce Avatax'),
      'parameter' => $parameter,
    ),
    'commerce_avatax_delete_sales_tax_line_items' => array(
      'label' => t('Delete Avatax line items'),
      'group' => t('Commerce Avatax'),
      'parameter' => $parameter,
    ),
    'commerce_avatax_commit_transaction' => array(
      'label' => t('Commit a SalesOrder transaction'),
      'group' => t('Commerce AvaTax'),
      'parameter' => $parameter,
    ),
    'commerce_avatax_void_transaction' => array(
      'label' => t('Void a transaction in Avatax'),
      'group' => t('Commerce AvaTax'),
      'parameter' => $parameter + array(
        'code' => array(
          'type' => 'token',
          'label' => t('Void code'),
          'description' => t('The reason for voiding or cancelling this transaction'),
          'options list' => 'commerce_avatax_void_codes_list',
          'optional' => TRUE,
          'default value' => 'DocDeleted',
        ),
      ),
    ),
  );

  return $actions;
}
