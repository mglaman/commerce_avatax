<?php

/**
 * @file
 * Default Rules for commerce_avalara.
 */

/**
 * Implements hook_default_rules_configuration().
 */
function commerce_avalara_default_rules_configuration() {
  $rule = rules_reaction_rule();
  $rule->label = 'Calculate Sales Tax using AvaTax';
  $rule->active = TRUE;
  $rule->event('commerce_order_presave');
  $rule->condition(rules_condition('data_is_empty', array(
    'data:select' => 'commerce-order-unchanged',
  ))->negate());
  $rule->condition(rules_condition('data_is', array(
    'data:select' => 'commerce-order-unchanged:status',
    'op' => 'IN',
    'value' => array(
      'checkout_review' => 'checkout_review',
    ),
  ))->negate());
  $rule->condition(rules_condition('data_is', array(
    'data:select' => 'commerce-order:status',
    'op' => 'IN',
    'value' => array(
      'checkout_review' => 'checkout_review',
    ),
  )));
  $rule->action(rules_action('commerce_avalara_calculate_sales_tax', array(
    'commerce_order:select' => 'commerce-order',
  )));
  $rules['commerce_avalara_calculate_sales_tax'] = $rule;

  $rule = rules_reaction_rule();
  $rule->label = 'Delete AvaTax line items from order';
  $rule->active = TRUE;
  $rule->event('commerce_order_presave');
  $rule->condition(rules_condition('data_is', array(
    'data:select' => 'commerce-order:status',
    'op' => 'IN',
    'value' => array(
      'checkout_review' => 'cart',
    ),
  )));
  $rule->condition(rules_condition('data_is_empty', array(
    'data:select' => 'commerce-order-unchanged',
  ))->negate());
  $rule->condition(rules_condition('data_is', array(
    'data:select' => 'commerce-order-unchanged:commerce-order-total:amount',
    'op' => '==',
    'value' => 'commerce-order:commerce-order-total:amount',
  ))->negate());
  $rule->action(rules_action('commerce_avalara_delete_sales_tax_line_items', array(
    'commerce_order:select' => 'commerce-order',
  )));
  $rules['commerce_avalara_delete_sales_tax_line_items'] = $rule;

  return $rules;
}