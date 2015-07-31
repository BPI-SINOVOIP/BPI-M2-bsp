/*
 * sunxi mipi csi bsp header file
 * Author:raymonxiu
*/

#ifndef __MIPI__CSI__H__
#define __MIPI__CSI__H__

#include "mipi_csi/dphy/dphy.h"
#include "mipi_csi/dphy/dphy_reg.h"
#include "mipi_csi/protocol/protocol.h"
#include "mipi_csi/protocol/protocol_reg.h"
#include "bsp_common.h"

#define MAX_MIPI  1
#define MAX_MIPI_CH 4

struct mipi_para {
  unsigned int        auto_check_bps;
  unsigned int        bps;
  unsigned int        dphy_freq;
  unsigned int        lane_num;
  unsigned int        total_rx_ch;
};

struct mipi_fmt {
  enum field          field[MAX_MIPI_CH];
  enum pkt_fmt        packet_fmt[MAX_MIPI_CH];
  unsigned int        vc[MAX_MIPI_CH];
};

extern int  bsp_mipi_csi_set_base_addr(unsigned int sel, unsigned int addr_base);
extern int  bsp_mipi_dphy_set_base_addr(unsigned int sel, unsigned int addr_base);
extern void bsp_mipi_csi_dphy_init(unsigned int sel);
extern void bsp_mipi_csi_dphy_exit(unsigned int sel);
extern void bsp_mipi_csi_dphy_enable(unsigned int sel);
extern void bsp_mipi_csi_dphy_disable(unsigned int sel);
extern void bsp_mipi_csi_protocol_enable(unsigned int sel);
extern void bsp_mipi_csi_protocol_disable(unsigned int sel);
extern void bsp_mipi_csi_det_mipi_clk(unsigned int sel, unsigned int *mipi_bps,unsigned int dphy_clk);
extern void bsp_mipi_csi_set_rx_dly(unsigned int sel, unsigned int mipi_bps,unsigned int dphy_clk);
extern void bsp_mipi_csi_set_lprst_dly(unsigned int sel, unsigned int mipi_bps,unsigned int dphy_clk);
extern void bsp_mipi_csi_set_lp_ulps_wp(unsigned int sel, unsigned int lp_ulps_wp_ms,unsigned int lp_clk);
extern void bsp_mipi_csi_set_dphy_timing(unsigned int sel, unsigned int *mipi_bps,unsigned int dphy_clk, unsigned int mode);
extern void bsp_mipi_csi_set_lane(unsigned int sel, unsigned char lane_num);
extern void bsp_mipi_csi_set_total_ch(unsigned int sel, unsigned char ch_num);
extern void bsp_mipi_csi_set_pkt_header(unsigned int sel, unsigned char ch,unsigned char vc,enum pkt_fmt mipi_pkt_fmt);
extern void bsp_mipi_csi_set_src_type(unsigned int sel, unsigned char ch,enum source_type src_type);
extern void bsp_mipi_csi_set_para(unsigned int sel, struct mipi_para *para);
extern void bsp_mipi_csi_set_fmt(unsigned int sel, unsigned int total_rx_ch, struct mipi_fmt *fmt);

#endif  //__MIPI__CSI__H__