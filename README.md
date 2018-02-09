# RFID-Skimmer
ESP8266 based RFID skimmer that can emulate EM4100 based cards (95% of the cheap RFID cards in the world)

All you need to do is connect a coil like [this](https://raw.githubusercontent.com/Crypter/RFID-Skimmer/master/images/rfid_coil.jpg) or like [this](https://raw.githubusercontent.com/Crypter/RFID-Skimmer/master/images/inductor.jpg) to any two pins on the ESP8266 board. That's all. This project is compatible with ESP-01 harware setup!

The device creates hotspot for initial setup, then you supply the RFID ID-s and the WiFi network you want it to connect to and you are good to go. You can change the ID emulated on the fly, or disable it. The device is turned on and off using the reset button as a toggle button.

LEGAL REMINDER: It's forbidden by law to abuse this for unauthorised access! Use at your own risk!
