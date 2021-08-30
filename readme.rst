=========================================
IoTaWatt WiFi Electricity Monitor Project
=========================================

IoTaWatt is an open-source, open hardware project to produce an accurate, 
inexpensive and easy to use energy monitor.  

This fork adds temperature sensor support (DS18B20) on ports 13 and 14. Two temperature sensors are supported per port.

Make the following hardware changes to the IotaWatt circuit board:
Connect ports 13,14 to the NodeMCU D3 and D4 instead of the ADC (cutting a few traces and adding a few wires). 

.. figure:: Docs/pics/readme/iotawatt-hardware-modifications.png
    :scale: 40 %
    :align: center
    :alt: **IoTaWatt Circuit board modifications**

    Circuit board modifications to change ports 13,14 to temperature ports

The modified user interface:

.. figure:: Docs/pics/readme/iotawatt-example-temperature-monitoring.png
    :scale: 40 %
    :align: center
    :alt: **IoTaWatt User Interface**

    Example user interface 







