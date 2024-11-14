# **Project Overview ESP32 Project RC CAR**

In this project the motion of the car is controlled (by controlling motors) remotely using webserver created within esp32 which have
different paths (for example forward , right etc), Based this paths it(esp32) takes different actions which is triggered using the UI or
Frontend Created in React js . It can also be created in QT or any GUI development Toolkit . So That is it .

# **Development Overview**

 Pretty Simple . First let discuss why motor driver is used . It is used because one cannot get the current and voltage required from
esp32 directly . It may damaged the esp32 and few other reasons. So used motor driver which get power from the cells you see. It got
two ouputs , one for motor1 and other for motor2 . so it is easy right furthermore this motors connected to motor-driver are
controlled using pwm signals from esp32 , the esp32 pwm signals from pins are connected to motordriver . so it get’s signals(pwm)
from esp32 and esp32 is controlled using webserver however can be further improved used pid_controlers(control systems) with
additional setup . that is it . there is some other wires connected to motor-driver such as GND etc, The esp32 takes power from
motor-driver as well . from VCC pin of motor-driver. motor-driver have features like it can reverse motors direction etc.

# **Short Overview of The Code**

What the code does is , It first connects to the wifi then setup the server , uses LEDC library from esp-idf , setup pwm pins and finally
starts listening for any tcp/ip requests , Base on the path of the request it performs actions like , go forward , go right etc .

From the code this is how the paths setup for the server looks like and some constants
You can further Add OTA updates and whatever .

#define PIN_motor_1 5
#define PIN_motor_2 18
httpd_register_uri_handler(server, &forward);
httpd_register_uri_handler(server, &stop);
httpd_register_uri_handler(server, &right);
httpd_register_uri_handler(server, &left);
httpd_register_uri_handler(server, &down);
httpd_register_uri_handler(server, &forward_options);


