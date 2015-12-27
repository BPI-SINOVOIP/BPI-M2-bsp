For 1-Wire w1-gpio driver use w1-sunxi module.

You need to add the following section to your sys_config.fex
Then rebuild it with u-boot and flash to the device.
You may need to change the w1_gpio port definition according to your need.

[w1_para]
w1_used              = 1
; This is 1W pin for Banana Pi M2 located on 40 pins connector 
w1_gpio              = port:PM02<1><default><default><default>
