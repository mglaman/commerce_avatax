<?php

namespace Drupal\commerce_avatax\Resolver;

use Drupal\commerce\PurchasableEntityInterface;

/**
 * Provides the chain tax code resolver.
 */
class ChainTaxCodeResolver implements ChainTaxCodeResolverInterface {

  /**
   * The tax code resolvers.
   *
   * @var \Drupal\commerce_avatax\Resolver\TaxCodeResolverInterface[]
   */
  protected $resolvers;

  /**
   * Constructs a new ChainTaxCodeResolver object.
   *
   * @param \Drupal\commerce_avatax\Resolver\TaxCodeResolverInterface[] $resolvers
   *   The tax code resolvers.
   */
  public function __construct(array $resolvers = []) {
    $this->resolvers = $resolvers;
  }

  /**
   * {@inheritdoc}
   */
  public function addResolver(TaxCodeResolverInterface $resolver) {
    $this->resolvers[] = $resolver;
  }

  /**
   * {@inheritdoc}
   */
  public function getResolvers() {
    return $this->resolvers;
  }

  /**
   * {@inheritdoc}
   */
  public function resolve(PurchasableEntityInterface $entity) {
    foreach ($this->resolvers as $resolver) {
      $result = $resolver->resolve($entity);
      if ($result) {
        return $result;
      }
    }
  }

}
