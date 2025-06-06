# esp_mqtt_lights

The project focuses on enhancing the user experience and operational efficiency of an existing cleaning robot in an airport environment. By integrating LED lights and a dynamic LED panel face, the robot can visually communicate its operational status—such as emergency stop, charging, normal operating mode, and litter picking mode. This improves both the functionality and the robot's friendliness towards tourists and travelers. The ESP32 plays a central role in controlling both the LED strips and the LCD display, with communication facilitated through MQTT topics relayed from a Raspberry Pi.

The core of the solution involves the integration of a single LED strip and an LED panel to visually communicate the robot’s operational status, such as emergency stop, charging, and litter picking modes. The ESP32 uses RTOS utilizing the RMT (Remote Control) channel, enabling the control of two LED strips simultaneously. Communication between the ESP32 and the external components is facilitated through MQTT topics, which are filtered and relayed from ROS topics by a Raspberry Pi.

Code for the ESP32 (ESP-IDF Code) is located within the `/tcp` folder.<br/>
Code for the RPI (Rust Code) is located within the `/ros_mqtt_project` folder.<br/>
Instructions on how to assembly the hardware and setup the software is in the PDF(to be uploaded).
