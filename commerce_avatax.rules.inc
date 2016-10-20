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
  );

  return $actions;
}
