<?php

namespace Drupal\commerce_avatax\Resolver;

/**
 * Defines a chain tax code resolver.
 */
interface ChainTaxCodeResolverInterface extends TaxCodeResolverInterface {

  /**
   * Adds a resolver.
   *
   * @param \Drupal\commerce_avatax\Resolver\TaxCodeResolverInterface $resolver
   *   The resolver.
   */
  public function addResolver(TaxCodeResolverInterface $resolver);

  /**
   * Gets all added resolvers.
   *
   * @return \Drupal\commerce_avatax\Resolver\TaxCodeResolverInterface[]
   *   The resolvers.
   */
  public function getResolvers();

}
