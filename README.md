# RFID-Skimmer
ESP8266 based RFID skimmer that can emulate EM4100 based cards (95% of the cheap RFID cards in the world)

All you need to do is connect a coil like [this](https://raw.githubusercontent.com/Crypter/RFID-Skimmer/master/images/rfid_coil.jpg) or like [this](https://raw.githubusercontent.com/Crypter/RFID-Skimmer/master/images/inductor.jpg) to any two pins on the ESP8266 board. That's all. This project is compatible with ESP-01 harware setup!

The device creates hotspot for initial setup and provides Web interface at 192.168.4.1. You supply the RFID ID-s and the WiFi network you want it to connect to and you are good to go. You can change the ID emulated on the fly, or disable it. The device is turned on and off using the reset button as a toggle button.

Why is this a big dial?__
-It wasn't possible to do this on the fly untill now! What makes this bigger deal is the fact that the ID of the skimmed cards is inprinted on them most of the time, you don't even need a reader.

But there's Vendor ID that it's not imprinted on the cards!__
-...And it doesn't matter because 90% of the readers ignore that part. So I can just make it all zeros.

Can I have one? For educational purposes...__
-No. I'm doing this as a proof of concept. I'm not selling this, I did this as an exercise for my penetration testing skils. It's public because I want to share knowledge.

LEGAL REMINDER: It's forbidden by law to abuse this for unauthorised access! Use at your own risk!
