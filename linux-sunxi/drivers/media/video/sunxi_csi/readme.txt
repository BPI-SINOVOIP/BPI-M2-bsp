===========================================

Version: V1_21

Author:  raymonxiu

Date:     2013-1-17 15:39:42

Description:

newest module list:(X = 0 or 1)

insmod sun4i_csiX.ko ccm="gc0307" i2c_addr=0x42
insmod sun4i_csiX.ko ccm="gc0308" i2c_addr=0x42
insmod sun4i_csiX.ko ccm="gt2005" i2c_addr=0x78
insmod sun4i_csiX.ko ccm="gc2035" i2c_addr=0x78
insmod sun4i_csiX.ko ccm="hi253" i2c_addr=0x40
insmod sun4i_csiX.ko ccm="ov5640" i2c_addr=0x78
insmod sun4i_csiX.ko ccm="s5k4ec" i2c_addr=0x5a


V1_00
CSI:Initial version for linux 3.3
1) Support DVP CSI and MIPI CSI

V1_10
CSI: Fix OV5640/GC0307 bugs; Optimizing standby and probe
1)Add sensor write try count to 3 in ov5640
2)Delete sensor set ae delayed work in ov5640
3)Move power ldo request and power on sequency to work queue in probe
4)Add semaphore in resume and open to ensure open being called after resume
5)Optimize power on timming in gc0307

V1_11
CSI: Add unlock when csi driver is opened twice; Optimizing ov5640 and gc2035
1) Add unlock when csi driver is opened twice
2) Force regulator disable when release
3) Optimizing GC2035 power on sequency
4) OV5640 disable internal LDO when initial
5) OV5640 recheck when af fw download is failed
6) OV5640 add IO oe disable before S_FMT and IO OE enable after S_FMT

V1_12
CSI: Modify probe; Move standby to early standby; OV5640 add 1080p@15fps
1) Modify probe handle insure that the video node and platform suspend/resume function
   is not registered when the module is installed failed.
2) Modify the suspend/resume to early suspend/late resume
3) Modify ov5640 1080p to 15fps

V1_20
CSI: Optimizing ov5640 and add preview mode(s5k4ec)
1) Modify ov5640 to auto fps
2) Reduce denoise level and enhance sharpness in ov5640
3) Add preview mode in ov5640 and s5k4ec

V1_21
CSI: Optimizing ov5640/gc2035 and fix bugs
1) Optimizing lens and cmx parameter for LA ov5640 module
2) Disable ov5640 oe before s_fmt and enable oe after s_fmt
3) Fix af firmware download error
4) Fix fps setting bug on test app
5) Optimizing gc2035 power timing
