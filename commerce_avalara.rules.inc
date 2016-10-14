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
    'commerce_avalara_calculate_tax' => array(
      'label' => t('Calculate sales tax for an order'),
      'group' => t('Commerce Avalara'),
      'parameter' => $parameter,
    ),
    'commerce_avalara_delete_sales_tax_line_items' => array(
      'label' => t('Delete Avalara Sales Tax line items'),
      'group' => t('Commerce Avalara'),
      'parameter' => $parameter,
    ),
  );

  return $actions;
}
