/**
 * @file
 * Handles AJAX submission for AvaTax address validation.
 */

(function($) {

  var checkoutFormSelector = 'form[id^=commerce-checkout]';
  var continueButtonsSelector = 'input.checkout-continue';
  var suggestionInputSelector = 'input[name="use_suggested_address"]';
  // Make sure our objects are defined.
  Drupal.CommerceAvalara = Drupal.CommerceAvalara || {};
  Drupal.CommerceAvalara.Modal = Drupal.CommerceAvalara.Modal || {};

  /**
   * AJAX responder command to place HTML within the modal.
   */
  Drupal.CommerceAvalara.Modal.modal_display = function(ajax, response, status) {
    var buttons = [];

    if (response.buttons) {
      $.each(response.buttons, function(delta, button) {
        buttons[delta] = {
          text:button.text,
          click: function() {
            switch (button.code) {
              case 'invalid':
                $(this).dialog('close');
                $('span.checkout-processing').addClass('element-invisible');

                // Remove the additional continue button added by
                // commerce_checkout.
                if ($(continueButtonsSelector).length > 1) {
                  $(continueButtonsSelector).get(1).remove();
                  $(continueButtonsSelector).show();
                }
                $(checkoutFormSelector).removeClass('avalara-processed');
                break;

              case 'recommended':
                $(this).dialog("close");
                // Store the suggestion delta in a hidden input field located
                // in the main checkout form.
                if ($('input[name="addresses"]').length > 0) {
                  if ($(suggestionInputSelector).length > 0) {
                    $(suggestionInputSelector).val("1");
                  }
                }
                $.fn.commerceAvalaraUnblockCheckout();
                break;

              case 'keep_address':
                $(this).dialog("close");
                $.fn.commerceAvalaraUnblockCheckout();
                break;
            }
          }
        }
      });
    }

    $(response.selector).dialog({
        height: 500,
        width: 800,
        modal: true,
        title: Drupal.t('Confirm your address'),
        resizable: false,
        draggable: false,
        buttons: buttons,
        dialogClass: 'no-close',
        closeOnEscape: false
    });
    $(response.selector).html(response.html);
    $(response.selector).dialog('open');
  }

  Drupal.CommerceAvalara.Modal.modal_dismiss = function(ajax, response, status) {
    $(response.selector).dialog.close();
  }

  Drupal.behaviors.commerceAvalara = {
    attach: function(context, settings) {
      // Catch the submit, click our custom "Validate address" button.
      $(checkoutFormSelector).submit(function(event) {
        if (!$(this).hasClass('avalara-processed')) {
          $('#commerce-avalara-address-validate-btn').trigger('mousedown');
          event.preventDefault();
          return false;
        }
      });
    }
  };

  $(function() {
    Drupal.ajax.prototype.commands.address_modal_display = Drupal.CommerceAvalara.Modal.modal_display;
    Drupal.ajax.prototype.commands.address_modal_dismiss = Drupal.CommerceAvalara.Modal.modal_dismiss;
  });

  /**
   * Unblock the checkout form submission.
   */
  $.fn.commerceAvalaraUnblockCheckout = function() {
    $(checkoutFormSelector).addClass('avalara-processed');
    $(continueButtonsSelector).click();
  }

}(jQuery));
