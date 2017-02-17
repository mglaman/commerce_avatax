<?php

namespace Drupal\commerce_avatax\Resolver;

use Drupal\commerce\PurchasableEntityInterface;
use Drupal\Core\Config\ConfigFactoryInterface;

/**
 * Resolves tax code based on global settings.
 */
class DefaultTaxCodeResolver implements TaxCodeResolverInterface {

  /**
   * The configuration.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   */
  protected $config;

  /**
   * Constructs a new DefaultTaxCodeResolver object.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory.
   */
  public function __construct(ConfigFactoryInterface $config_factory) {
    $this->config = $config_factory->get('commerce_avatax.settings');
  }

  /**
   * {@inheritdoc}
   */
  public function resolve(PurchasableEntityInterface $entity) {
    return $this->config->get('tax_code');
  }

}
