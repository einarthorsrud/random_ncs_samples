Bluetooth: Peripheral HIDS keyboard with button to disconnect oldest connection
###############################################################################

Overview
********

This sample implements a Bluetooth Low Energy (LE) Human Interface Device (HID) keyboard peripheral that can connect to multiple central devices simultaneously.

See the documentation for the sample in the SDK: https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/samples/bluetooth/peripheral_hids_keyboard/README.html

The User interface for this modifie sample is slightly different, where ther eis no Caps Lock LED as instead two LEDs are used to indicate number of connections. Moreover, a button is used to disconnect the oldest connection. If only one connection is active, that is disconnected.


User interface
**************

nRF52 and nRF53 DKs
-------------------

      Button 1:
         Sends one character of the predefined input ("hello\\n") to the computer.

         When pairing, press this button to confirm the passkey value that is printed on the COM listener to pair with the other device.

      Button 2:
         Simulates the Shift key.

         When pairing, press this button to reject the passkey value which is printed on the COM listener to prevent pairing with the other device.

      LED 1:
         Blinks with a period of two seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.

      LED 2:
         Lit when at least one device is connected.

      LED 3:
         Lit when at least two devices are connected.

      If the `NFC_OOB_PAIRING` feature is enabled:

      Button 3:
         Disconnect oldest connection.

      Button 4:
         Starts advertising.

      LED 4:
         Indicates if an NFC field is present.

nRF54 DKs
---------

      Button 0:
         Sends one character of the predefined input ("hello\\n") to the computer.

         When pairing, press this button to confirm the passkey value that is printed on the COM listener to pair with the other device.

      Button 1:
         Simulates the Shift key.

         When pairing, press this button to reject the passkey value which is printed on the COM listener to prevent pairing with the other device.

      LED 0:
         Blinks with a period of two seconds with the duty cycle set to 50% when the main loop is running and the device is advertising.

      LED 1:
         Lit when at least one device is connected.

      LED 2:
         Lit when at least two devices are connected.

      If the `NFC_OOB_PAIRING` feature is enabled:

      Button 2:
         Disconnect oldest connection.

      Button 3:
         Starts advertising.

      LED 3:
         Indicates if an NFC field is present.
