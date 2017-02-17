<?php

namespace Drupal\Tests\commerce_avatax\Kernel;

use Drupal\KernelTests\KernelTestBase;

/**
 * Tests the API client factory.
 *
 * @group commerce_avatax
 */
class ClientFactoryTest extends KernelTestBase {

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'system',
    'commerce_avatax',
  ];

  /**
   * The config.
   *
   * @var \Drupal\Core\Config\Config
   */
  protected $config;

  /**
   * {@inheritdoc}
   */
  protected function setUp() {
    parent::setUp();

    $config_factory = $this->container->get('config.factory');
    $this->config = $config_factory->getEditable('commerce_avatax.settings');
    $this->config
      ->set('account_number', 'DUMMY ACCOUNT')
      ->set('license_key', 'DUMMY KEY')
      ->set('api_uid', '0x00001')
      ->set('api_mode', 'development')
      ->save();
  }

  /**
   * Test that the Guzzle client is properly configured.
   */
  public function testClientFactory() {
    $client_factory = $this->container->get('commerce_avatax.client_factory');

    $client = $client_factory->createInstance();

    $this->assertEquals('https://sandbox-rest.avatax.com/', $client->getConfig('base_uri'));
    $headers = $client->getConfig('headers');
    $this->assertEquals('Basic ' . base64_encode($this->config->get('account_number') . ':' . $this->config->get('license_key')), $headers['Authorization']);
    $this->assertEquals($this->config->get('api_uid'), $headers['x-Avalara-UID']);
    $server_machine_name = gethostname();
    $this->assertEquals("Drupal Commerce; Version [8.x-1.x]; REST; V2; [$server_machine_name]", $headers['x-Avalara-Client']);

    $client_with_options = $client_factory->createInstance([
      'headers' => ['x-Test-Header' => 'Testing'],
    ]);
    $headers = $client_with_options->getConfig('headers');
    $this->assertEquals('Testing', $headers['x-Test-Header']);

    $this->config->set('api_mode', 'production')->save();
    $production_client = $client_factory->createInstance();
    $this->assertEquals('https://rest.avatax.com/', $production_client->getConfig('base_uri'));
  }

}
