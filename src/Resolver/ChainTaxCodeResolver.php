<?php

namespace Drupal\commerce_avatax\Resolver;

use Drupal\commerce\PurchasableEntityInterface;

class ChainTaxCodeResolver implements ChainTaxCodeResolverInterface {

  /**
   * @var \Drupal\commerce_avatax\Resolver\TaxCodeResolverInterface[]
   */
  protected $resolvers;

  public function __construct(array $resolvers =[]) {
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
      if ($result ) {
        return $result;
      }
    }
  }

}
