# TankController
Arduino Mega and Raspberry Pi aquarium controller

Visit https://www.youtube.com/c/BlueBladeFish for video tutorials:

"DIY Arduino Digital Aquarium Controller" parts 1-15

------------------------------------------------

contents: 
(main tank conrtoller)

tank03142018_pub.tar or tank01092019_pub.zip - /app/tank* python web and PHP script content for controller

Main_tanl_routine_v07.zip - the Arduino sketch for use with Arduino Mega

R_Pi_serial_build.docx - configuration instructions for building raspberry Pi as SQL database, serial monitor, and website.

tank.sql - "tank" sql database for import

Nextion_display.zip - Nextion display files

supporting docs.zip - easyEda gerber files, shiled build doc, BOM, and pdf of plan drawings

Gerber_ControllerHatV1.3_2021-07-25_15-54-18.zip - latest update for main PCB. (assembly instructions in supporting docs.zip)

--------------------------------------------------
optional mods (main tank controller):

eMail_alerts.zip - includes instructions, test script, and main script to send email alerts (or email-to-cell text messages).
Alerts if temperature is too high/low, no top off has been performed, or if no recent updates have been received from arduino/time sync is off. 

--------------------------------------------------

Mini-controller (for additional controllers based on ESP8266 LoLin) code:

MinControllerV1_public.ino - Arduino code for a esp8266 controller

devices.sql - "devices" sql database for import

tank01092019_pub.zip - /app/tank/www/devices* web content for the esp8266 controller, along with a few other improvements to the website

Gerber_Mini_tank_board_1.1_2021-07-25_15-50-48.zip - Gerber filles for mini tank PCB

DS1307.zip- Arduino library for DS1307 to use with the desktop client

--------------------------------------------------
