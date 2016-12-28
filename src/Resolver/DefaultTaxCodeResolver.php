<?php

namespace Drupal\commerce_avatax\Resolver;

use Drupal\commerce\PurchasableEntityInterface;

class DefaultTaxCodeResolver implements TaxCodeResolverInterface {

  /**
   * {@inheritdoc}
   */
  public function resolve(PurchasableEntityInterface $entity) {
    $config = \Drupal::configFactory()->get('commerce_avatax.settings');
    return $config->get('tax_code');
  }

}
