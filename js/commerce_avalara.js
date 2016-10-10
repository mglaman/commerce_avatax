/**
 * @file
 * Handles AJAX submission for AvaTax address validation.
 */

(function($) {

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
            console.log(button);
            console.log($(this));
            switch (button.code) {
              case 'invalid':
                $(this).dialog('close');
                break;

              case 'recommended':
                $(this).dialog("close");
                $('form.commerce-checkout-form').submit();
                break;

              case 'keep_address':
                $(this).dialog("close");
                $('form.commerce-checkout-form').submit();
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

  $(function() {
    Drupal.ajax.prototype.commands.address_modal_display = Drupal.CommerceAvalara.Modal.modal_display;
    Drupal.ajax.prototype.commands.address_modal_dismiss = Drupal.CommerceAvalara.Modal.modal_dismiss;
  });

}(jQuery));
