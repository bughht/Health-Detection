clc;
clear all;

instrhwinfo('Bluetooth','SmartD')
b = Bluetooth('SmartD',1)
fopen(b);
