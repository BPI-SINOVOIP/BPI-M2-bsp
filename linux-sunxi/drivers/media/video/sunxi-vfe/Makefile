
obj-$(CONFIG_CSI_VFE) += vfe_v4l2.o
obj-$(CONFIG_CSI_VFE) += vfe_os.o
obj-$(CONFIG_CSI_VFE) += vfe_subdev.o
obj-$(CONFIG_CSI_VFE) += device/
obj-$(CONFIG_CSI_VFE) += actuator/
#obj-$(CONFIG_CSI_VFE) += flash_light/
obj-$(CONFIG_CSI_VFE) += camera_detector/

vfe_v4l2-objs :=  mipi_csi/protocol/protocol_reg.o mipi_csi/dphy/dphy_reg.o \
									csi/csi_reg.o \
									bsp_csi.o bsp_mipi_csi.o bsp_common.o \
									config.o cfg_op.o \
									vfe.o lib/libisp
