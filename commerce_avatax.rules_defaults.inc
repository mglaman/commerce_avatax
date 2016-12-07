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

  $rule = rules_reaction_rule();
  $rule->label = 'Void Avatax transaction on order deletion';
  $rule->active = TRUE;
  $rule->event('commerce_order_delete');
  $rule->action(rules_action('commerce_avatax_void_transaction', array(
    'order:select' => 'commerce-order',
    'code' => 'DocDeleted',
  )));
  $rules['commerce_avatax_void_transaction_on_order_deletion'] = $rule;

  return $rules;
}
