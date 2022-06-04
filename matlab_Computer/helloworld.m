clc;
clear all;

%instrhwinfo('Bluetooth','SmartD')
b = Bluetooth('SmartD',1)
fopen(b);

data=[]
for i=1:100
    data=[data;fgets(b)];
end