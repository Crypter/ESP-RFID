# ESP-RFID
ESP8266 based RFID emulator for EM4100 based cards (95% of the cheap RFID cards in the world)

#### How do I make it work?
Download and install the [ArduinoJson library](https://github.com/bblanchon/ArduinoJson), and flash this sketch onto your ESP8266 based device (TODO: ESP32). From hardware perspective you need to connect a coil like [this](https://raw.githubusercontent.com/Crypter/ESP-RFID/master/images/rfid_coil.jpg) or [this](https://raw.githubusercontent.com/Crypter/ESP-RFID/master/images/inductor.jpg) between any GPIO pin on the ESP8266 board and ground. That's all. This project is compatible even with ESP-01 boards.

#### Basic usage:
The device creates hotspot for initial setup and provides Web interface at 192.168.4.1. You supply the RFID ID-s and the WiFi network you want it to connect in future (can be ommited) and you are good to go. You can change the ID emulated on the fly, or disable it. The device is turned on and off using the reset button as a toggle button. It starts RFID-ing immediately after boot, and will try to connect to a WiFi network in mean time. After 15 seconds of failing to do so it starts the hotspot.

#### How does it work?
RFID cards use current to communicate with the readers. They have a coil through which they receive their power (think wireless charging), and they use this coil to modulate the data. High current consumption (shorted coil) and low current consumption (open coil) make the one and zeros. Most of the time they communicate by Manchester encoding the data stream at ~2KHz (64 RF cycles per bit)
In case of EM4100 there are 64 bits on the card that are transmitted in a loop once the card is powered by the reader. These 64 bits include preamble, vendor ID data, UniqueID data, parity data and stop bit. You can read more about this [here](http://www.priority1design.com.au/em4100_protocol.html). The UID data most of the time is printed on the card itself.  
We achieve this on ESP8266 by toggling the dedicated GPIO pin between Input and Output. When in Output mode the pin provides connection to ground, effectively shorting the coil. And when in Input mode the pin has so big impedance which makes it act as an open circuit.  
TCP is very error resilient protocol, and so is RFID. This allows us to abuse their characteristics and do both of them ~at the same time~. TCP will resend requests that were dropped when the device is RFIDing, and the RFID reader will tolerate a few missed readings while we do TCP stuff. That way we can have our cake and eat it too!

#### What's the big dial?  
-It wasn't possible to do this on the fly-untill now! This was previously done with Attiny85, but it took programming harware and the ID could not be changed unless you reprogram the chip using dedicated programmer. What makes this bigger deal is the fact that the ID of the skimmed cards is printed on them most of the time, you don't even need a reader.

#### But there's Vendor ID that it's not imprinted on the cards!  
-Unfortunately, it doesn't matter because 90% of the readers ignore that part to achieve compatibility with the [Wiegand interface](https://en.wikipedia.org/wiki/Wiegand_interface). To make them easier to implement they use only the ID data. You can replace this vendor ID with zeros most of the time.

#### Can I have one?  
-**No.** I'm doing this as a proof of concept. I'm not selling this. I'm not going to build one for you. I did this as an exercise for my penetration testing skils. It's public because I want to share my knowledge.

**LEGAL REMINDER: It's forbidden by law to abuse this for unauthorised access! Don't get yourself in trouble because you wanted to show-off in front of your friends.**
