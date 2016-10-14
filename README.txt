INTRODUCTION
============
The Drupal Commerce Connector for Avalara is a Drupal compliant module that
integrates the Drupal Commerce check-out process with AvaTax from Avalara, Inc.
and is used for Tax calculations and Tax compliance.

AvaTax reduces the audit risk to a company with a cloud-based sales tax
services that makes it simple to do rate calculation while managing exemption
certificates, filing forms and remitting payments.

The module supports two modes - Basic - Pro - which correspond to
the levels of service available for AvaTax.

The AvaTax Basic service offers limited use of the AvaTax sales tax codes, in
particular P0000000 for Personal Taxable goods, Shipping tax codes and NT for
Not Taxable goods.

The AvaTax Pro service offers full use of the AvaTax sales tax codes. The Pro
service is required for states like New York were sales tax rates can be based
on the type of product, the delivery address, and the time of year.

The Tax is calculated based on the delivery address, the sales tax codes
assigned to line item in the order, and the sales tax rules applicable to the
states in which Nexus has been configured.

Access to a full development account can be requested by contacting
Avalara, Inc.

REQUIREMENTS
============
a) The service uses the AvaTax Rest api v2 for processing transactions.
b) The server PHP configuration must support cURL


NEW INSTALLATION
=================
Installing the module is done as for any custom Drupal Commerce module

a) Unzip & copy the folder "commerce_avalara" to the location shown below,
or in accordance with your Drupal Commerce configuration.

yoursite/sites/all/modules/commerce_avalara


CONFIGURATION
=============
Select Store -> Configuration -> Avalara

Complete the information requested.
