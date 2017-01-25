<?php

namespace Drupal\commerce_avatax;

use Drupal\Component\Utility\NestedArray;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Http\ClientFactory as CoreClientFactory;

class ClientFactory {

  protected $config;
  protected $clientFactory;

  /**
   * Constructs a new AvataxClient object.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory.
   * @param \Drupal\Core\Http\ClientFactory $client_factory
   *   The client factory.
   */
  public function __construct(ConfigFactoryInterface $config_factory, CoreClientFactory $client_factory) {
    $this->config = $config_factory->get('commerce_avatax.settings');
    $this->clientFactory = $client_factory;
  }

  /**
   * Gets an API client instance.
   *
   * @param array $config
   *   Additional config for the client.
   *
   * @return \GuzzleHttp\Client
   *   The API client.
   */
  public function createInstance(array $config = []) {
    switch ($this->config->get('api_mode')) {
      case 'production':
        $base_uri = 'https://rest.avatax.com/';
        break;
      case 'development':
      default:
        $base_uri = 'https://sandbox-rest.avatax.com/';
        break;
    }
    $default_config = [
      'base_uri' => $base_uri,
      'headers' => [
        'Authorization' => 'Basic ' . base64_encode($this->config->get('api_key') . ':' . $this->config->get('license_key')),
        'Content-Type' => 'application/json',
        'x-Avalara-UID' => $this->config->get('api_uid'),
      ],
    ];
    $config = NestedArray::mergeDeep($default_config, $config);
    return $this->clientFactory->fromOptions($config);
  }

}
