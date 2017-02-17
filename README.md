# Drupal Commerce Connector for Avatax.


The [Drupal Commerce Connector for Avatax] is a Drupal compliant module that
integrates the Drupal Commerce with [AvaTax from Avalara, Inc.] and is used for
Tax calculations and Tax compliance.

AvaTax reduces the audit risk to a company with a cloud-based sales tax
services that makes it simple to do rate calculation while managing exemption
certificates, filing forms and remitting payments.


The Tax is calculated based on the delivery address, the sales tax codes
assigned to line item in the order, and the sales tax rules applicable to the
states in which Nexus has been configured.

Access to a full development account can be requested by contacting Avalara, Inc.

The service uses the AvaTax Rest api v2 for processing transactions.


## Installation

As with Drupal Commerce, and its contributed modules, you must installed with Composer.

```bash
composer require drupal/commerce_avatax:~1
```


[Drupal Commerce Connector for Avatax]: https://www.drupal.org/project/commerce_avatax
[AvaTax from Avalara, Inc.]: https://www.avalara.com/products/sales-and-use-tax/avatax-2
