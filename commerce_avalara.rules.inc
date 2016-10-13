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
      'group' => t('Commerce Avalara'),
      'parameter' => $parameter,
    ),
    'commerce_avalara_delete_sales_tax_line_items' => array(
      'label' => t('Delete Avalara Sales Tax line items'),
      'group' => t('Commerce Avalara'),
      'parameter' => $parameter,
    ),
    'commerce_avalara_calculate_vat' => array(
      'label' => t('Calculate VAT for a line item'),
      'group' => t('Commerce AvaTax'),
      'parameter' => array(
        'commerce_line_item' => array(
          'type' => 'commerce_line_item',
          'label' => t('Line item'),
        ),
      ),
    ),
  );

  return $actions;
}
