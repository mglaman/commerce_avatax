<?php

namespace Drupal\commerce_avatax\Resolver;

use Drupal\commerce\PurchasableEntityInterface;
use Drupal\commerce_product\Entity\ProductVariationInterface;

class ProductVariationTaxCodeResolver implements TaxCodeResolverInterface {

  /**
   * {@inheritdoc}
   */
  public function resolve(PurchasableEntityInterface $entity) {
    if ($entity instanceof ProductVariationInterface) {
      if (!$entity->get('avatax_tax_code')->isEmpty()) {
        return $entity->avatax_tax_code->value;
      }
    }
  }
}
