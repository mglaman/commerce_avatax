<?php

/**
 * @file
 * Default Rules for commerce_avatax.
 */

/**
 * Implements hook_default_rules_configuration().
 */
function commerce_avatax_default_rules_configuration() {
  $rules = array();
  $cart_statuses = commerce_order_statuses(array('cart' => TRUE));
  $statuses = array();

  // Delete the Avatax line items on all cart statuses with a weight lower
  // than the checkout_review status.
  foreach ($cart_statuses as $key => $status) {
    // Break as soon as we hit the checkout_review status.
    if ($key == 'checkout_review') {
      break;
    }
    $statuses[$key] = $key;
  }
  $rule = rules_reaction_rule();
  $rule->label = 'Delete AvaTax line items from order';
  $rule->tags = array('Commerce Avatax');
  $rule->active = TRUE;
  $rule->weight = -1;
  $rule->event('commerce_order_update');
  $rule->condition(rules_condition('data_is', array(
    'data:select' => 'commerce-order:status',
    'op' => 'IN',
    'value' => $statuses,
  )));
  $rule->condition(rules_condition('data_is_empty', array(
    'data:select' => 'commerce-order-unchanged',
  ))->negate());
  $rule->condition(rules_condition('data_is', array(
    'data:select' => 'commerce-order-unchanged:status',
    'op' => '==',
    'value:select' => 'commerce-order:status',
  ))->negate());
  $rule->action(rules_action('commerce_avatax_delete_sales_tax_line_items', array(
    'commerce_order:select' => 'commerce-order',
  )));
  $rules['commerce_avatax_delete_sales_tax_line_items'] = $rule;

  $rule = rules_reaction_rule();
  $rule->label = 'Calculate Sales Tax using AvaTax';
  $rule->active = TRUE;
  $rule->tags = array('Commerce Avatax');
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
  $rule->action(rules_action('commerce_avatax_calculate_tax', array(
    'commerce_order:select' => 'commerce-order',
  )));
  $rules['commerce_avatax_calculate_tax'] = $rule;

  $rule = rules_reaction_rule();
  $rule->label = 'Void AvaTax transaction on order cancellation';
  $rule->active = TRUE;
  $rule->event('commerce_order_update');
  $rule->condition(rules_condition('data_is', array(
    'data:select' => 'commerce-order:status',
    'op' => '==',
    'value:select' => 'commerce-order-unchanged:status',
  ))->negate());
  $rule->condition(rules_condition('data_is', array(
    'data:select' => 'commerce-order:status',
    'op' => '==',
    'value' => 'canceled',
  )));
  $rule->action(rules_action('commerce_avatax_void_transaction', array(
    'order:select' => 'commerce-order',
    'code' => 'DocVoided',
  )));
  $rules['commerce_avatax_void_transaction_on_order_cancellation'] = $rule;

  return $rules;
}
