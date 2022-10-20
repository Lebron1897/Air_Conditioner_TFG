# Air Conditioner TFG

These are the files that are part of my final thesis. In them I make reference to all the codes and programs that I have used to develop it. There is also a file where all the technical aspects are explained in more detail.

This project refers to a product generated for a customer, who needed an automated control of an air conditioning system from an external device, other than a remote control. It was also intended to control different aspects of the room, such as temperature and humidity, the movement inside the room and the condition of the windows and doors

The following is a summary of the project and the technolgies used in it.
Air conditioning systems are increasingly used by society, with a wide variety of devices with different technologies.
This project aims to make more efficient use of air-conditioning equipment and, in addition, to extend as far as possible the average life of use of an equipment. This control of the operation will be carried out with a main module, which will be responsible for sending the commands that indicate the action to be performed. It also has two others modules that are composed of sensors that send the information collected by these to the main module, through ESP-NOW.
On the other hand, we are faced with the transmisión of messages from a user interface to the main module by the protocol MQTT, which is responsible for exchanging messages over the internet network, which means that we will always need our devices to be connected to the internet, the main module and the one containing the user interface, in order to be able to send the messages between them.
