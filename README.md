Feather9x_RX_Orlando are base general reciever code with minor tweaks most notably got rid of 1 second blink to save time and replaced with light up when recieving.
Feather9x_TX_IR_MOTION_SENSOR is a transiever modified code to use a motion detection sensor to send packets of data. The sensor has a large radius, and can be slightly sensitive. Will only light up when something is in front of sensor.
Feather9x_TX_Orlando needs to be re-made I accidentally relaced it with motion sensor code; it is no longer general use until fixed.
I2C_check will only check if something is attached to and sending data to the micro controller. Helpful to have when wanting to check if sensor or something else is being deteced properly.
