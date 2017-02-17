<?php

namespace Drupal\commerce_avatax\Resolver;

use Drupal\commerce\PurchasableEntityInterface;

/**
 * Defines interface for tax code resolvers.
 */
interface TaxCodeResolverInterface {

  /**
   * Resolves the tax code of a given purchaseable entity.
   *
   * @param \Drupal\commerce\PurchasableEntityInterface $entity
   *   The purchaseable entity.
   *
   * @return string
   *   The tax code.
   */
  public function resolve(PurchasableEntityInterface $entity);

}
