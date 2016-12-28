<?php

namespace Drupal\commerce_avatax\Form;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Routing\RequestContext;
use Symfony\Component\DependencyInjection\ContainerInterface;

class ConfigSettingsForm extends ConfigFormBase {

  /**
   * The request context.
   *
   * @var \Drupal\Core\Routing\RequestContext
   */
  protected $requestContext;

  /**
   * Constructs a ConfigSettingsForm object.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The factory for configuration objects.
   * @param \Drupal\Core\Routing\RequestContext $request_context
   *   The request context.
   */
  public function __construct(ConfigFactoryInterface $config_factory, RequestContext $request_context) {
    parent::__construct($config_factory);

    $this->requestContext = $request_context;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('config.factory'),
      $container->get('router.request_context')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'commerce_avatax_config_settings';
  }

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return ['commerce_avatax.settings'];
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $config = $this->config('commerce_avatax.settings');

    $form['configuration'] = [
      '#type' => 'details',
      '#title' => t('Configuration'),
      '#open' => TRUE,
    ];
    $form['configuration']['api_mode'] = [
      '#type' => 'radios',
      '#title' => t('API mode:'),
      '#default_value' => $config->get('api_mode'),
      '#options' => [
        'sandbox' => $this->t('Sandbox'),
        'production' => $this->t('Production')
      ],
      '#required' => TRUE,
      '#description' => $this->t('The mode to use when calculating taxes.'),
    ];
    $form['configuration']['api_key'] = [
      '#type' => 'textfield',
      '#title' => t('API key:'),
      '#default_value' => $config->get('api_key'),
      '#required' => TRUE,
      '#description' => $this->t('The API key to send to Avatax when calculating taxes.'),
    ];
    $form['configuration']['api_uid'] = [
      '#type' => 'textfield',
      '#title' => t('API UID:'),
      '#default_value' => $config->get('api_uid'),
      '#required' => TRUE,
      '#description' => $this->t('The API user id to send to Avatax when calculating taxes.'),
    ];
    $form['configuration']['license_key'] = [
      '#type' => 'textfield',
      '#title' => t('License key:'),
      '#default_value' => $config->get('license_key'),
      '#required' => TRUE,
      '#description' => $this->t('The license key to send to Avatax when calculating taxes.'),
    ];
    $form['configuration']['company_code'] = [
      '#type' => 'textfield',
      '#title' => t('Company code:'),
      '#default_value' => $config->get('company_code'),
      '#required' => TRUE,
      '#description' => $this->t('The default company code to send to Avatax when calculating taxes, if company code is not set on the store of a given order.'),
    ];
    $form['configuration']['tax_code'] = [
      '#type' => 'textfield',
      '#title' => t('Default tax code:'),
      '#default_value' => $config->get('tax_code'),
      '#required' => TRUE,
      '#description' => $this->t('The default tax code to send to Avatax when calculating taxes, if company code is not set on the purchased entity of a given order item.'),
    ];

    return parent::buildForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    parent::validateForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $this->config('commerce_avatax.settings')
      ->set('api_mode', $form_state->getValue('api_mode'))
      ->set('api_key', $form_state->getValue('api_key'))
      ->set('api_uid', $form_state->getValue('api_uid'))
      ->set('license_key', $form_state->getValue('license_key'))
      ->set('company_code', $form_state->getValue('company_code'))
      ->set('tax_code', $form_state->getValue('tax_code'))
      ->save();

    parent::submitForm($form, $form_state);
  }

}
