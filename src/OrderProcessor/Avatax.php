<?php

namespace Drupal\commerce_avatax\OrderProcessor;

use Drupal\commerce_avatax\ClientFactory;
use Drupal\commerce_avatax\Resolver\ChainTaxCodeResolverInterface;
use Drupal\commerce_order\Adjustment;
use Drupal\commerce_order\Entity\OrderInterface;
use Drupal\commerce_order\OrderProcessorInterface;
use Drupal\commerce_price\Price;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Datetime\DateFormatterInterface;
use Drupal\Core\Extension\ModuleHandlerInterface;
use Drupal\Core\Logger\LoggerChannelFactoryInterface;
use GuzzleHttp\Exception\ClientException;

class Avatax implements OrderProcessorInterface {

  protected $config;

  protected $client;

  protected $dateFormatter;

  protected $chainTaxCodeResolver;

  protected $moduleHandler;

  protected $logger;

  public function __construct(ConfigFactoryInterface $config_factory, ClientFactory $client_factory, DateFormatterInterface $date_formatter, ChainTaxCodeResolverInterface $chain_tax_code_resolver, ModuleHandlerInterface $module_handler, LoggerChannelFactoryInterface $logger_factory) {
    $this->config = $config_factory->get('commerce_avatax.settings');
    $this->client = $client_factory->createInstance();
    $this->dateFormatter = $date_formatter;
    $this->chainTaxCodeResolver = $chain_tax_code_resolver;
    $this->moduleHandler = $module_handler;
    $this->logger = $logger_factory->get('commerce_avatax');
  }

  /**
   * {@inheritdoc}
   */
  public function process(OrderInterface $order) {
    $config = $this->config;

    // Attempt to get company code for specific store.
    $store = $order->getStore();
    if ($store->get('avatax_company_code')->isEmpty()) {
      $company_code = $config->get('company_code');
    }
    else {
      $company_code = $store->avatax_company_code->value;
    }

    $request_body = [
      'type' => 'SalesInvoice',
      'companyCode' => $company_code,
      'date' => $this->dateFormatter->format(REQUEST_TIME, 'custom', 'c'),
      'code' => 'DC-' . $order->id(),
      'customerCode' => $order->getEmail(),
      'currencyCode' => $order->getTotalPrice()->getCurrencyCode(),
      'addresses' => [],
      'lines' => []
    ];

    $addresses = [];
    if ($order->getBillingProfile()) {
      /** @var \Drupal\address\Plugin\Field\FieldType\AddressItem $address */
      $address = $order->getBillingProfile()->get('address')->first();
      $addresses = [
        'singleLocation' => [
          'line1' => $address->getAddressLine1(),
          'line2' => $address->getAddressLine2(),
          'city' => $address->getLocality(),
          'region' => $address->getAdministrativeArea(),
          'country' => $address->getCountryCode(),
          'postalCode' => $address->getPostalCode(),
        ],
      ];
    }
    $this->moduleHandler->alter('commerce_avatax_order_addresses', $addresses, $order);

    if (empty($addresses)) {
      return;
    }

    $request_body['addresses'] = $addresses;

    foreach ($order->getItems() as $item) {
      $request_body['lines'][] = [
        'number' => $item->id(),
        'quantity' => $item->getQuantity(),
        'amount' => $item->getTotalPrice()->getNumber(),
        'taxCode' => $this->chainTaxCodeResolver->resolve($item->getPurchasedEntity()),
      ];
    }

    $this->moduleHandler->alter('commerce_avatax_order_request', $request_body, $order);

    try {
      $response = $this->client->post('/api/v2/transactions/create', [
        'json' => $request_body,
      ]);

      $body = json_decode($response->getBody()->getContents(), TRUE);
      $order->addAdjustment(new Adjustment([
        'type' => 'sales_tax',
        'label' => 'Sales tax',
        'amount' => new Price((string) $body['totalTax'], $order->getTotalPrice()->getCurrencyCode())
      ]));
    }
    catch (ClientException $e) {
      // @todo port error handling from D7.
      $this->logger->error($e->getResponse()->getBody()->getContents());
    }
    catch (\Exception $e) {
      $this->logger->error($e->getMessage());
    }
  }

}
