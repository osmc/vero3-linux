/*
 * include/linux/amlogic/media/registers/regs/dmc_regs.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef DMC_REGS_HEADERS__
#define DMC_REGS_HEADERS__


#define TM2_REVB
#ifdef TM2_REVB

#define TM2_REVB_DMC_REQ_CTRL                               0x0000
#define TM2_REVB_DMC_SOFT_RST                               0x0001
#define TM2_REVB_DMC_SOFT_RST1                              0x0002
#define TM2_REVB_DMC_SOFT_RST2                              0x0003
#define TM2_REVB_DMC_RST_STS1                               0x0004
#define TM2_REVB_DMC_VERSION                                0x0005
#define TM2_REVB_DMC_CLKG_CTRL0                             0x0006
#define TM2_REVB_DMC_CLKG_CTRL1                             0x0007
#define TM2_REVB_DMC_CLKG_CTRL2                             0x0008
#define TM2_REVB_DMC_CLKG_CTRL3                             0x0009
#define TM2_REVB_DC_CAV_LUT_DATAL                           0x0012
#define TM2_REVB_DC_CAV_LUT_DATAH                           0x0013
#define TM2_REVB_DC_CAV_LUT_ADDR                            0x0014
#define TM2_REVB_DC_CAV_LUT_RDATAL                          0x0015
#define TM2_REVB_DC_CAV_LUT_RDATAH                          0x0016
#define TM2_REVB_DC_CAV_BLK_CTRL0                           0x0018
#define TM2_REVB_DC_CAV_BLK_CTRL1                           0x0019
#define TM2_REVB_DC_CAV_BLK_CTRL2                           0x001a
#define TM2_REVB_DC_CAV_BLK_CTRL3                           0x001b
#define TM2_REVB_DC_CAV_BLK_CTRL4                           0x001c
#define TM2_REVB_DC_CAV_BLK_CTRL5                           0x001d
#define TM2_REVB_DC_CAV_BLK_CTRL6                           0x001e
#define TM2_REVB_DC_CAV_BLK_CTRL7                           0x001f
#define TM2_REVB_DMC_MON_CTRL0                              0x0020
#define TM2_REVB_DMC_MON_CTRL1                              0x0021
#define TM2_REVB_DMC_MON_CTRL2                              0x0022
#define TM2_REVB_DMC_MON_CTRL3                              0x0023
#define TM2_REVB_DMC_MON_CTRL4                              0x0024
#define TM2_REVB_DMC_MON_CTRL5                              0x0025
#define TM2_REVB_DMC_MON_CTRL6                              0x0026
#define TM2_REVB_DMC_MON_CTRL7                              0x0027
#define TM2_REVB_DMC_MON_CTRL8                              0x0028
#define TM2_REVB_DMC_MON_ALL_REQ_CNT                        0x0029
#define TM2_REVB_DMC_MON_ALL_GRANT_CNT                      0x002a
#define TM2_REVB_DMC_MON_ONE_GRANT_CNT                      0x002b
#define TM2_REVB_DMC_MON_SEC_GRANT_CNT                      0x002c
#define TM2_REVB_DMC_MON_THD_GRANT_CNT                      0x002d
#define TM2_REVB_DMC_MON_FOR_GRANT_CNT                      0x002e
#define TM2_REVB_DMC_MON_TIMER                              0x002f
#define TM2_REVB_DMC_PROT0_RANGE                            0x0030
#define TM2_REVB_DMC_PROT0_CTRL                             0x0031
#define TM2_REVB_DMC_PROT0_CTRL1                            0x0032
#define TM2_REVB_DMC_PROT1_RANGE                            0x0033
#define TM2_REVB_DMC_PROT1_CTRL                             0x0034
#define TM2_REVB_DMC_PROT1_CTRL1                            0x0035
#define TM2_REVB_DMC_PROT_VIO_0                             0x0036
#define TM2_REVB_DMC_PROT_VIO_1                             0x0037
#define TM2_REVB_DMC_PROT_VIO_2                             0x0038
#define TM2_REVB_DMC_PROT_VIO_3                             0x0039
#define TM2_REVB_DMC_PROT_IRQ_CTRL                          0x003a
#define TM2_REVB_DMC_IRQ_STS                                0x003b
/*idle*/
#define TM2_REVB_DMC_CHAN_STS                               0x003c
#define TM2_REVB_DMC_2ARB_CTRL                              0x003d
#define TM2_REVB_DMC_CMD_FILTER_CTRL1                       0x0040
#define TM2_REVB_DMC_CMD_FILTER_CTRL2                       0x0041
#define TM2_REVB_DMC_CMD_FILTER_CTRL3                       0x0042
#define TM2_REVB_DMC_CMD_FILTER_CTRL4                       0x0043
#define TM2_REVB_DMC_CMD_FILTER_CTRL5                       0x0044
#define TM2_REVB_DMC_CMD_FILTER_CTRL6                       0x0045
#define TM2_REVB_DMC_CMD_FILTER_CTRL7                       0x0046
#define TM2_REVB_DMC_CMD_BUFFER_CTRL                        0x0047
#define TM2_REVB_DMC_CMD_BUFFER_CTRL1                       0x0048
#define TM2_REVB_DMC_AM0_CHAN_CTRL                          0x0060
#define TM2_REVB_DMC_AM0_HOLD_CTRL                          0x0061
#define TM2_REVB_DMC_AM0_CHAN_CTRL1                         0x0062
#define TM2_REVB_DMC_AM0_CHAN_CTRL2                         0x0063
#define TM2_REVB_DMC_AM1_CHAN_CTRL                          0x0064
#define TM2_REVB_DMC_AM1_HOLD_CTRL                          0x0065
#define TM2_REVB_DMC_AM1_CHAN_CTRL1                         0x0066
#define TM2_REVB_DMC_AM1_CHAN_CTRL2                         0x0067
#define TM2_REVB_DMC_AM2_CHAN_CTRL                          0x0068
#define TM2_REVB_DMC_AM2_HOLD_CTRL                          0x0069
#define TM2_REVB_DMC_AM2_CHAN_CTRL1                         0x006a
#define TM2_REVB_DMC_AM2_CHAN_CTRL2                         0x006b
#define TM2_REVB_DMC_AM3_CHAN_CTRL                          0x006c
#define TM2_REVB_DMC_AM3_HOLD_CTRL                          0x006d
#define TM2_REVB_DMC_AM3_CHAN_CTRL1                         0x006e
#define TM2_REVB_DMC_AM3_CHAN_CTRL2                         0x006f
#define TM2_REVB_DMC_AM4_CHAN_CTRL                          0x0070
#define TM2_REVB_DMC_AM4_HOLD_CTRL                          0x0071
#define TM2_REVB_DMC_AM4_CHAN_CTRL1                         0x0072
#define TM2_REVB_DMC_AM4_CHAN_CTRL2                         0x0073
/*vdec urgent*/
#define TM2_REVB_DMC_AM5_CHAN_CTRL                          0x0074
#define TM2_REVB_DMC_AM5_HOLD_CTRL                          0x0075
#define TM2_REVB_DMC_AM6_CHAN_CTRL                          0x0078
#define TM2_REVB_DMC_AM6_HOLD_CTRL                          0x0079
#define TM2_REVB_DMC_AM7_CHAN_CTRL                          0x007c
#define TM2_REVB_DMC_AM7_HOLD_CTRL                          0x007d
#define TM2_REVB_DMC_AXI0_CHAN_CTRL                         0x0080
#define TM2_REVB_DMC_AXI0_HOLD_CTRL                         0x0081
#define TM2_REVB_DMC_AXI0_CHAN_CTRL1                        0x0082
#define TM2_REVB_DMC_AXI1_CHAN_CTRL                         0x0084
#define TM2_REVB_DMC_AXI1_HOLD_CTRL                         0x0085
#define TM2_REVB_DMC_AXI1_CHAN_CTRL1                        0x0086
#define TM2_REVB_DMC_AXI2_CHAN_CTRL                         0x0088
#define TM2_REVB_DMC_AXI2_HOLD_CTRL                         0x0089
#define TM2_REVB_DMC_AXI3_CHAN_CTRL                         0x008c
#define TM2_REVB_DMC_AXI3_HOLD_CTRL                         0x008d
#define TM2_REVB_DMC_AXI4_CHAN_CTRL                         0x0090
#define TM2_REVB_DMC_AXI4_HOLD_CTRL                         0x0091
#define TM2_REVB_DMC_AXI5_CHAN_CTRL                         0x0094
#define TM2_REVB_DMC_AXI5_HOLD_CTRL                         0x0095
#define TM2_REVB_DMC_AXI6_CHAN_CTRL                         0x0098
#define TM2_REVB_DMC_AXI6_HOLD_CTRL                         0x0099
#define TM2_REVB_DMC_AXI7_CHAN_CTRL                         0x009c
#define TM2_REVB_DMC_AXI7_HOLD_CTRL                         0x009d
#define TM2_REVB_DMC_AXI8_CHAN_CTRL                         0x00a0
#define TM2_REVB_DMC_AXI8_HOLD_CTRL                         0x00a1
#define TM2_REVB_DMC_AXI9_CHAN_CTRL                         0x00a4
#define TM2_REVB_DMC_AXI9_HOLD_CTRL                         0x00a5
#define TM2_REVB_DMC_AXI10_CHAN_CTRL                        0x00a8
#define TM2_REVB_DMC_AXI10_HOLD_CTRL                        0x00a9
#define TM2_REVB_DMC_AXI10_CHAN_CTRL1                       0x00aa
#define TM2_REVB_DMC_AXI11_CHAN_CTRL                        0x00ac
#define TM2_REVB_DMC_AXI11_HOLD_CTRL                        0x00ad
#define TM2_REVB_DMC_AXI12_CHAN_CTRL                        0x00b0
#define TM2_REVB_DMC_AXI12_HOLD_CTRL                        0x00b1
#define TM2_REVB_DMC_AXI13_CHAN_CTRL                        0x00b4
#define TM2_REVB_DMC_AXI13_HOLD_CTRL                        0x00b5
#endif



#define DMC_REQ_CTRL 0x00
#define DMC_SOFT_RST 0x01
#define DMC_SOFT_RST1 0x02
#define DMC_RST_STS 0x03
#define DMC_RST_STS1 0x04
#define DMC_VERSION 0x05
#define DMC_RAM_PD 0x11
#define DC_CAV_LUT_DATAL 0x12
#define DC_CAV_LUT_DATAH 0x13
#define DC_CAV_LUT_ADDR 0x14
#define DC_CAV_LUT_RDATAL 0x15
#define DC_CAV_LUT_RDATAH 0x16
#define DMC_2ARB_CTRL 0x20
#define DMC_REFR_CTRL1 0x23
#define DMC_REFR_CTRL2 0x24
#define DMC_PARB_CTRL 0x25
#define DMC_MON_CTRL2 0x26
#define DMC_MON_CTRL3 0x27
#define DMC_MON_ALL_REQ_CNT 0x28
#define DMC_MON_ALL_GRANT_CNT 0x29
#define DMC_MON_ONE_GRANT_CNT 0x2a
#define DMC_CLKG_CTRL0 0x30
#define DMC_CLKG_CTRL1 0x31
#define DMC_CHAN_STS 0x32
#define DMC_CMD_FILTER_CTRL1 0x40
#define DMC_CMD_FILTER_CTRL2 0x41
#define DMC_CMD_FILTER_CTRL3 0x42
#define DMC_CMD_FILTER_CTRL4 0x43
#define DMC_CMD_BUFFER_CTRL 0x44
#define DMC_AM0_CHAN_CTRL 0x60
#define DMC_AM0_HOLD_CTRL 0x61
#define DMC_AM0_QOS_INC 0x62
#define DMC_AM0_QOS_INCBK 0x63
#define DMC_AM0_QOS_DEC 0x64
#define DMC_AM0_QOS_DECBK 0x65
#define DMC_AM0_QOS_DIS 0x66
#define DMC_AM0_QOS_DISBK 0x67
#define DMC_AM0_QOS_CTRL0 0x68
#define DMC_AM0_QOS_CTRL1 0x69
#define DMC_AM1_CHAN_CTRL 0x6a
#define DMC_AM1_HOLD_CTRL 0x6b
#define DMC_AM1_QOS_INC 0x6c
#define DMC_AM1_QOS_INCBK 0x6d
#define DMC_AM1_QOS_DEC 0x6e
#define DMC_AM1_QOS_DECBK 0x6f
#define DMC_AM1_QOS_DIS 0x70
#define DMC_AM1_QOS_DISBK 0x71
#define DMC_AM1_QOS_CTRL0 0x72
#define DMC_AM1_QOS_CTRL1 0x73
#define DMC_AM2_CHAN_CTRL 0x74
#define DMC_AM2_HOLD_CTRL 0x75
#define DMC_AM2_QOS_INC 0x76
#define DMC_AM2_QOS_INCBK 0x77
#define DMC_AM2_QOS_DEC 0x78
#define DMC_AM2_QOS_DECBK 0x79
#define DMC_AM2_QOS_DIS 0x7a
#define DMC_AM2_QOS_DISBK 0x7b
#define DMC_AM2_QOS_CTRL0 0x7c
#define DMC_AM2_QOS_CTRL1 0x7d
#define DMC_AM3_CHAN_CTRL 0x7e
#define DMC_AM3_HOLD_CTRL 0x7f
#define DMC_AM3_QOS_INC 0x80
#define DMC_AM3_QOS_INCBK 0x81
#define DMC_AM3_QOS_DEC 0x82
#define DMC_AM3_QOS_DECBK 0x83
#define DMC_AM3_QOS_DIS 0x84
#define DMC_AM3_QOS_DISBK 0x85
#define DMC_AM3_QOS_CTRL0 0x86
#define DMC_AM3_QOS_CTRL1 0x87
#define DMC_AM4_CHAN_CTRL 0x88
#define DMC_AM4_HOLD_CTRL 0x89
#define DMC_AM4_QOS_INC 0x8a
#define DMC_AM4_QOS_INCBK 0x8b
#define DMC_AM4_QOS_DEC 0x8c
#define DMC_AM4_QOS_DECBK 0x8d
#define DMC_AM4_QOS_DIS 0x8e
#define DMC_AM4_QOS_DISBK 0x8f
#define DMC_AM4_QOS_CTRL0 0x90
#define DMC_AM4_QOS_CTRL1 0x91
#define DMC_AM5_CHAN_CTRL 0x92
#define DMC_AM5_HOLD_CTRL 0x93
#define DMC_AM5_QOS_INC 0x94
#define DMC_AM5_QOS_INCBK 0x95
#define DMC_AM5_QOS_DEC 0x96
#define DMC_AM5_QOS_DECBK 0x97
#define DMC_AM5_QOS_DIS 0x98
#define DMC_AM5_QOS_DISBK 0x99
#define DMC_AM5_QOS_CTRL0 0x9a
#define DMC_AM5_QOS_CTRL1 0x9b
#define DMC_AM6_CHAN_CTRL 0x9c
#define DMC_AM6_HOLD_CTRL 0x9d
#define DMC_AM6_QOS_INC 0x9e
#define DMC_AM6_QOS_INCBK 0x9f
#define DMC_AM6_QOS_DEC 0xa0
#define DMC_AM6_QOS_DECBK 0xa1
#define DMC_AM6_QOS_DIS 0xa2
#define DMC_AM6_QOS_DISBK 0xa3
#define DMC_AM6_QOS_CTRL0 0xa4
#define DMC_AM6_QOS_CTRL1 0xa5
#define DMC_AM7_CHAN_CTRL 0xa6
#define DMC_AM7_HOLD_CTRL 0xa7
#define DMC_AM7_QOS_INC 0xa8
#define DMC_AM7_QOS_INCBK 0xa9
#define DMC_AM7_QOS_DEC 0xaa
#define DMC_AM7_QOS_DECBK 0xab
#define DMC_AM7_QOS_DIS 0xac
#define DMC_AM7_QOS_DISBK 0xad
#define DMC_AM7_QOS_CTRL0 0xae
#define DMC_AM7_QOS_CTRL1 0xaf
#define DMC_AXI0_CHAN_CTRL 0xb0
#define DMC_AXI0_HOLD_CTRL 0xb1
#define DMC_AXI0_QOS_INC 0xb2
#define DMC_AXI0_QOS_INCBK 0xb3
#define DMC_AXI0_QOS_DEC 0xb4
#define DMC_AXI0_QOS_DECBK 0xb5
#define DMC_AXI0_QOS_DIS 0xb6
#define DMC_AXI0_QOS_DISBK 0xb7
#define DMC_AXI0_QOS_CTRL0 0xb8
#define DMC_AXI0_QOS_CTRL1 0xb9
#define DMC_AXI1_CHAN_CTRL 0xba
#define DMC_AXI1_HOLD_CTRL 0xbb
#define DMC_AXI1_QOS_INC 0xbc
#define DMC_AXI1_QOS_INCBK 0xbd
#define DMC_AXI1_QOS_DEC 0xbe
#define DMC_AXI1_QOS_DECBK 0xbf
#define DMC_AXI1_QOS_DIS 0xc0
#define DMC_AXI1_QOS_DISBK 0xc1
#define DMC_AXI1_QOS_CTRL0 0xc2
#define DMC_AXI1_QOS_CTRL1 0xc3
#define DMC_AXI2_CHAN_CTRL 0xc4
#define DMC_AXI2_HOLD_CTRL 0xc5
#define DMC_AXI2_QOS_INC 0xc6
#define DMC_AXI2_QOS_INCBK 0xc7
#define DMC_AXI2_QOS_DEC 0xc8
#define DMC_AXI2_QOS_DECBK 0xc9
#define DMC_AXI2_QOS_DIS 0xca
#define DMC_AXI2_QOS_DISBK 0xcb
#define DMC_AXI2_QOS_CTRL0 0xcc
#define DMC_AXI2_QOS_CTRL1 0xcd
#define DMC_AXI3_CHAN_CTRL 0xce
#define DMC_AXI3_HOLD_CTRL 0xcf
#define DMC_AXI3_QOS_INC 0xd0
#define DMC_AXI3_QOS_INCBK 0xd1
#define DMC_AXI3_QOS_DEC 0xd2
#define DMC_AXI3_QOS_DECBK 0xd3
#define DMC_AXI3_QOS_DIS 0xd4
#define DMC_AXI3_QOS_DISBK 0xd5
#define DMC_AXI3_QOS_CTRL0 0xd6
#define DMC_AXI3_QOS_CTRL1 0xd7
#define DMC_AXI4_CHAN_CTRL 0xd8
#define DMC_AXI4_HOLD_CTRL 0xd9
#define DMC_AXI4_QOS_INC 0xda
#define DMC_AXI4_QOS_INCBK 0xdb
#define DMC_AXI4_QOS_DEC 0xdc
#define DMC_AXI4_QOS_DECBK 0xdd
#define DMC_AXI4_QOS_DIS 0xde
#define DMC_AXI4_QOS_DISBK 0xdf
#define DMC_AXI4_QOS_CTRL0 0xe0
#define DMC_AXI4_QOS_CTRL1 0xe1
#define DMC_AXI5_CHAN_CTRL 0xe2
#define DMC_AXI5_HOLD_CTRL 0xe3
#define DMC_AXI5_QOS_INC 0xe4
#define DMC_AXI5_QOS_INCBK 0xe5
#define DMC_AXI5_QOS_DEC 0xe6
#define DMC_AXI5_QOS_DECBK 0xe7
#define DMC_AXI5_QOS_DIS 0xe8
#define DMC_AXI5_QOS_DISBK 0xe9
#define DMC_AXI5_QOS_CTRL0 0xea
#define DMC_AXI5_QOS_CTRL1 0xeb
#define DMC_AXI6_CHAN_CTRL 0xec
#define DMC_AXI6_HOLD_CTRL 0xed
#define DMC_AXI6_QOS_INC 0xee
#define DMC_AXI6_QOS_INCBK 0xef
#define DMC_AXI6_QOS_DEC 0xf0
#define DMC_AXI6_QOS_DECBK 0xf1
#define DMC_AXI6_QOS_DIS 0xf2
#define DMC_AXI6_QOS_DISBK 0xf3
#define DMC_AXI6_QOS_CTRL0 0xf4
#define DMC_AXI6_QOS_CTRL1 0xf5
#define DMC_AXI7_CHAN_CTRL 0xf6
#define DMC_AXI7_HOLD_CTRL 0xf7
#define DMC_AXI7_QOS_INC 0xf8
#define DMC_AXI7_QOS_INCBK 0xf9
#define DMC_AXI7_QOS_DEC 0xfa
#define DMC_AXI7_QOS_DECBK 0xfb
#define DMC_AXI7_QOS_DIS 0xfc
#define DMC_AXI7_QOS_DISBK 0xfd
#define DMC_AXI7_QOS_CTRL0 0xfe
#define DMC_AXI7_QOS_CTRL1 0xff

#endif









