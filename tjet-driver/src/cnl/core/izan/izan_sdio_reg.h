/*
    Copyright 2011-2014 Toshiba Corporation.
    All Rights Reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2 of the License only.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
/**
 *  @file     izan_sdio_reg.h
 *
 *  @brief    Describe IZAN register map for SDIO IF
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__IZAN_SDIO_REG_H__)
#define __IZAN_SDIO_REG_H__


/*-------------------------------------------------------------------
 * Macro Definitions
 *-----------------------------------------------------------------*/
/*******************************************************************/
/* below registers are Host IF register.                           */
/*******************************************************************/
//
// Power Management Unit(PMU) registers definition.
// address range : 0x00000 - 0x003FF
// 

// PMUMCHG register value.
#define REG_PMUMCHG                     0x00000
#define PMUMCHG_TO_DPSLP                   0x00
#define PMUMCHG_TO_SLP                     0x01
#define PMUMCHG_TO_AWK                     0x03

// PMUSTATE register value.
#define REG_PMUSTATE                    0x00010
#define PMUSTATE_DPSLP                     0x01
#define PMUSTATE_SLP                       0x02
#define PMUSTATE_DMT                       0x04
#define PMUSTATE_AWK                       0x08

// PMUSUBSTATE register value.
#define REG_PMUSUBSTATE                 0x00011
#define PMUSUBSTATE_FLD                    0x01
#define PMUSUBSTATE_INI                    0x02
#define PMUSUBSTATE_AWK2SLP                0x04
#define PMUSUBSTATE_SLP2AWK                0x08
#define PMUSUBSTATE_AWK2DMT                0x10
#define PMUSUBSTATE_DMT2AWK                0x20
#define PMUSUBSTATE_SLP2DPSLP              0x40

#define REG_SRCHDMTTIM0                 0x00018
#define REG_SRCHDMTTIM1                 0x00019
#define REG_HBNTDMTTIM0                 0x0001A
#define REG_HBNTDMTTIM1                 0x0001B

// DMTEIXT
#define REG_DMTEXIT                     0x0001C
#define DMTEXIT_ON                         0x01
#define DMTEXIT_OFF                        0x00

#define REG_LDOWUPTMG                   0x00020
#define REG_XOWUPTMG                    0x00021
#define REG_PLLWUPTMG                   0x00022
#define REG_LDOXOWUPTMG                 0x00023

//
// ClocK Generator(CKG) registers definition.
// address range : 0x00400 - 0x007FF
//
#define REG_CROSCEN                     0x00400
#define REG_CRCLKEN                     0x00401
#define REG_BUSCLKEN                    0x00402
#define REG_CRMSRCTL                    0x00410
#define REG_CRMSRSTAT                   0x00411
#define REG_CRMSRRSLT0                  0x00412
#define REG_CRMSRRSLT1                  0x00413

#define BUSCLKEN_SP                     0x01

//
// ReSet Generator(RSG) registers definition.
// address range : 0x00800 - 0x00BFF
//
#define REG_SWRST                       0x00800
#define REG_PMSWRST                     0x00801
#define REG_PMFUSESWRST                 0x00802

// SWRST register value.
#define SWRST_SP                           0x01
#define SWRST_RS                           0x02
#define SWRST_UR                           0x04
#define SWRST_CAG                          0x08
#define SWRST_CAM                          0x10
#define SWRST_CAD                          0x20
#define SWRST_CAJ                          0x40

// PMSWRST register value.
#define PMSWRST_PM                         0x01

// PMFUSESWRST register value.
#define PMFUSESWRST_PM_FUSE                0x01

//
// PLL registers definition.
// address range : 0x00C00 - 0x00FFF
//
#define REG_PLL_MONI                    0x00C00
#define REG_PLL_PRD                     0x00C01
#define REG_PLL_ND                      0x00C02
#define REG_PLL_FS                      0x00C03
#define REG_PLL_BWIS                    0x00C04
#define REG_PLL_EXBP                    0x00C05

//
// IO Mux(IOM) registers definition.
// address range : 0x01000 - 0x013FF 
//
#define REG_GPIOOE                      0x01000
#define REG_GPIODO                      0x01001
#define REG_GPIODI                      0x01002
#define REG_GPIOIE                      0x01003
#define REG_FUNC                        0x01004


//
// CNL registers definition.
// address range : 0x01400 - 0x017FF
//
#define REG_DMTOFF                      0x01400
#define DMT_ENABLE                         0x00                
#define DMT_DISABLE                        0x01

#define REG_CLMSTATE                    0x01401
#define CLMSTATE_CLOSE                     0x00
#define CLMSTATE_SEARCH                    0x01
#define CLMSTATE_CONNECTION_REQUEST        0x03
#define CLMSTATE_ACCEPT_WAITING            0x04
#define CLMSTATE_RESPONSE_WAITING          0x05
#define CLMSTATE_RESPONDER_RESPONSE        0x06
#define CLMSTATE_INIT_CONNECTED            0x07
#define CLMSTATE_INIT_TARGETSLEEP          0x08
#define CLMSTATE_INIT_LOCALHIBERNATE       0x09
#define CLMSTATE_RESP_CONNECTED            0x0B
#define CLMSTATE_RESP_TARGETSLEEP          0x0C
#define CLMSTATE_RESP_LOCALHIBERNATE       0x0D
#define CLMSTATE_DORMANT                   0x10
#define CLMSTATE_PLL_ON                    0x20

//
// EFuse Controller(EFC) registers definition.
// address range : 0x01800 - 0x01BFF
//
#define REG_RFFLDDONE                   0x01800
#define REG_CHIPVERSION                 0x01810

#define REG_ZA_0x01914                  0x01914 
#define REG_ZA_0x01915                  0x01915 
#define REG_ZA_0x01916                  0x01916 

#define RFFLDDONE                          0x01

//
// extra registers.
//
#define REG_TXRXFIFO                    0x0FC00
#define REG_INTST_REG0                  0x0FC10
#define REG_INTST_REG1                  0x0FC11
#define REG_INTST_REG2                  0x0FC12
#define REG_INTST_CLR_REG0              0x0FC14
#define REG_INTST_CLR_REG1              0x0FC15
#define REG_INTST_CLR_REG2              0x0FC16
#define REG_INTST_MASK_REG0             0x0FC18
#define REG_INTST_MASK_REG1             0x0FC19
#define REG_INTST_MASK_REG2             0x0FC1A
#define REG_RAW_INT_REG0                0x0FC1C
#define REG_RAW_INT_REG1                0x0FC1D
#define REG_RAW_INT_REG2                0x0FC1E
#define REG_CONF_REG1_0                 0x0FC20
#define REG_CONF_REG1_1                 0x0FC21
#define REG_CONF_REG1_2                 0x0FC22
#define REG_CONF_REG2                   0x0FC24
#define REG_CONF_REG3                   0x0FC28
#define REG_CARD_INT_REG1               0x0FC31
#define REG_CARD_INT_CLR_REG1           0x0FC35
#define REG_CARD_INT_MASK_REG1          0x0FC39
#define REG_CARD_RAW_INT_REG1           0x0FC3D
#define REG_TXFIFO_ADR_OFFSET_REG0      0x0FC40
#define REG_TXFIFO_ADR_OFFSET_REG1      0x0FC41
#define REG_TXFIFO_ADR_OFFSET_REG2      0x0FC42
#define REG_TXFIFO_ADR_OFFSET_REG3      0x0FC43
#define REG_TXFIFO_ADR_MASK_REG0        0x0FC44
#define REG_TXFIFO_ADR_MASK_REG1        0x0FC45
#define REG_TXFIFO_CUR_ADR_REG0         0x0FC48
#define REG_TXFIFO_CUR_ADR_REG1         0x0FC49
#define REG_RXFIFO_ADR_OFFSET_REG0      0x0FC50
#define REG_RXFIFO_ADR_OFFSET_REG1      0x0FC51
#define REG_RXFIFO_ADR_OFFSET_REG2      0x0FC52
#define REG_RXFIFO_ADR_OFFSET_REG3      0x0FC53
#define REG_RXFIFO_ADR_MASK_REG0        0x0FC54
#define REG_RXFIFO_ADR_MASK_REG1        0x0FC55
#define REG_RXFIFO_CUR_ADR_REG0         0x0FC58
#define REG_RXFIFO_CUR_ADR_REG1         0x0FC59
#define REG_SDAHB_ADR_OFFSET_REG0       0x0FC60
#define REG_SDAHB_ADR_OFFSET_REG1       0x0FC61
#define REG_SDAHB_ADR_OFFSET_REG2       0x0FC62
#define REG_SDAHB_ADR_OFFSET_REG3       0x0FC63
#define REG_SDAHB_ADR_MASK_REG0         0x0FC64
#define REG_SDAHB_ADR_MASK_REG1         0x0FC65
#define REG_REL_CARD_ADR_REG0           0x0FC70
#define REG_REL_CARD_ADR_REG1           0x0FC71
#define REG_WRITE_TR_UNIT_CNT_REG0      0x0FC74
#define REG_WRITE_TR_UNIT_CNT_REG1      0x0FC75
#define REG_READ_TR_UNIT_CNT_REG0       0x0FC76
#define REG_READ_TR_UNIT_CNT_REG1       0x0FC77



/*******************************************************************/
/* below registers are internal bus register(need clock).          */
/*******************************************************************/
//
// CNL registers definition. (default mapping)
// address range 0x10000 - 0x14FFF
//

// TX registers.
#define REG_TXDATA                      0x10000 // not use
#define REG_TXDATAINFO                  0x11000
#define REG_TXBANKSTA                   0x11100
#define REG_TXMNGBODY1                  0x11200
#define REG_TXMNGBODY2                  0x11204
#define REG_TXMNGBODY3                  0x11208
#define REG_TXMNGBODY4                  0x1120C
#define REG_TXMNGBODY5                  0x11210
#define REG_TXMNGBODY6                  0x11214
#define REG_TXMNGBODY7                  0x11218
#define REG_TXMNGBODY8                  0x1121C

#define REG_MFTXREQ                     0x11220
#define MFTXREQ_START                0x00000001

#define REG_TXMFSTOP                    0x11224
#define TXMFSTOP_ON                  0x00000001

// RX registers.
#define REG_RXDATA                      0x12000 // not use.
#define REG_RXDATAINFO                  0x13000
#define REG_RXBANKSTA                   0x13100
#define REG_READDONE                    0x13104
#define READDONE_ON                  0x00000001

#define REG_REWIND                      0x13108
#define REWIND_ON                    0x00000001

#define REG_RXMNGBODY1                  0x13200
#define REG_RXMNGBODY2                  0x13204
#define REG_RXMNGBODY3                  0x13208
#define REG_RXMNGBODY4                  0x1320C
#define REG_RXMNGBODY5                  0x13210
#define REG_RXMNGBODY6                  0x13214
#define REG_RXMNGBODY7                  0x13218
#define REG_RXMNGBODY8                  0x1321C

#define REG_MFDATAREADDONE              0x13220
#define MFDATAREADDONE_ON            0x00000001

#define REG_RXCRLSBODY1                 0x13230
#define REG_RXCRLSBODY2                 0x13234
#define REG_RXCRLSBODY3                 0x13238
#define REG_RXCRLSBODY4                 0x1323C
#define REG_RXCRLSBODY5                 0x13240
#define REG_RXCRLSBODY6                 0x13244
#define REG_RXCRLSBODY7                 0x13248
#define REG_RXCRLSBODY8                 0x1324C

// UID registers.
#define REG_OWNUID1                     0x14000
#define REG_OWNUID2                     0x14004
#define REG_TARGETUID1                  0x14008
#define REG_TARGETUID2                  0x1400C

// timer setting registers.
#define REG_TCONNECT                    0x14040
#define REG_TACCEPT                     0x14044
#define REG_TRETRY                      0x14048
#define REG_TRESEND                     0x1404C
#define REG_TKEEPALIVE                  0x14050
#define REG_TAS                         0x14054
#define REG_TAC                         0x1405C
#define REG_TSIFS                       0x14064
#define REG_TIIFS                       0x14068
#define REG_TRIFS                       0x1406C
#define REG_TBST                        0x14070
#define REG_TACKTIMEOUT                 0x14074
#define REG_TCREQINTVL                  0x14078
#define REG_TEDGEDETAWAKE               0x1407C
#define REG_TCREQIFS                    0x14080
#define REG_TCACCIFS                    0x14084
#define REG_TWDT                        0x14088

// timer control registers.
#define REG_TIMERCONTROL                0x14090
#define TIMERCTL_DISABLE_KEEPALIVE   0x00000008

// IIFS control registers.
#define REG_IIFSCHANGEMODE              0x140A0
#define IIFSCHANGEMODE_ENABLE        0x00000001
#define REG_IIFS2ND                     0x140A4
#define REG_IIFSPERIOD                  0x140A8
#define REG_IIFS2PERIOD                 0x140AC

// config registers.
#define REG_CONFIG                      0x140C0

#define CONFIG_TUID_AUTO_CLEAR       0x40000000
#define CONFIG_TUID_MANUAL_CLEAR     0x00000000
#define CONFIG_ACK_BUFFER_FULL       0x00000000
#define CONFIG_NO_ACK_BUFFER_FULL    0x00100000
#define CONFIG_NO_FILTER_TUID        0x00000000
#define CONFIG_FILTER_TUID           0x00010000


//
// rate register.
//
#define REG_RATECNTQ                    0x140D0
#define REG_RATECNTS                    0x140D4
#define REG_RATECTRLINIT                0x140D8
#define RATECTRLINIT_RESET           0x00000000
#define RATECTRLINIT_SET             0x00000001
#define REG_RATEOFF                     0x140DC
#define REG_RATE                        0x140E0
#define RATE_MANUAL                  0x00000000
#define RATE_LC_AUTO                 0x40000000
#define RATE_FIXED_VALUE             0x80000000
#define RATE_INIT_32                 0x01000000
#define RATE_INIT_65                 0x02000000
#define RATE_INIT_130                0x03000000
#define RATE_INIT_261                0x04000000
#define RATE_INIT_522                0x05000000
#define RATE_DOWN_ALG1               0x00000000
#define RATE_DOWN_ALG2               0x00008000
#define RATE_DOWN_1RANK              0x00000000
#define RATE_DOWN_2RANK              0x00001000
#define RATE_UP_ALG1                 0x00000000
#define RATE_UP_ALG2                 0x00000080

#define REG_RATE2                       0x140E4
#define REG_RATEOUT                     0x140E8
#define REG_VGAEVMCOEF                  0x140EC

// VGAEVMCOEF register value
// VGACOEF value bit[5:4]
#define VGAEVMCOEF_VGACOEF_00        0x00000000
#define VGAEVMCOEF_VGACOEF_01        0x00000010
#define VGAEVMCOEF_VGACOEF_10        0x00000020
#define VGAEVMCOEF_VGACOEF_11        0x00000030
#define VGAEVMCOEF_VGACOEF_MASK      0x00000030

//
// COMMAND register
//
#define REG_COMMAND                     0x14100
// store STMODE fieled CLMSTATE_XXXX
#define COMMAND_AUTO_CPROBE          0x00000000
#define COMMAND_MANUAL_CPROBE        0x00000100
#define COMMAND_RETXDATAREQ          0x00010000
#define COMMAND_ACK_DISABLE          0x00000000
#define COMMAND_ACK_ENABLE           0x01000000
#define COMMAND_CREQ_RCVDIS          0x10000000

//
// BANKCLR register
//
#define REG_BANKCLR                     0x14104
#define BANKCLR_TXBANK               0x80000000
#define BANKCLR_RXBANK               0x40000000
#define BANKCLR_TXDATASTOP           0x00010000
#define BANKCLR_TXENABLE             0x00000000
#define BANKCLR_TXDISABLE            0x00000001

#define REG_STATUS                      0x14108

// interrupt registers
#define REG_INT                         0x14200
#define REG_INTMASK                     0x14204

#define INT_CPROBETX                 0x00000001
#define INT_CSLEEPTX                 0x00000002
#define INT_CWAKETX                  0x00000004
#define INT_CACCTX                   0x00000008
#define INT_CPROBERCV                0x00000010
#define INT_CSLEEPRCV                0x00000020
#define INT_CWAKERCV                 0x00000040
#define INT_CRLSRCV                  0x00000080
#define INT_CACCRCV                  0x00000100
#define INT_CREQRCV                  0x00000200
#define INT_TXDATASTOPCONF           0x00000400
#define INT_TXMFSTOPCONF             0x00000800
#define INT_FTX                      0x00001000
#define INT_CRLSTX                   0x00002000
#define INT_LCFATALERR               0x00004000
#define INT_IRRDFSN                  0x00008000
#define INT_AWAKEPRIOD               0x00010000
#define INT_SLEEPTOAWAKE             0x00020000
#define INT_AWAKETOSLEEP             0x00040000
#define INT_CACCACKTX                0x00080000
#define INT_TKEEPATOUT               0x00100000
#define INT_TRESENDTOUT              0x00200000
#define INT_TRETRYTOUT               0x00400000
#define INT_TACCTOUT                 0x00800000
#define INT_TCONNTOUT                0x01000000
#define INT_RXBANKNOTEMPT            0x02000000
#define INT_TXDFRAME                 0x08000000
#define INT_TXBANKFULL               0x10000000
#define INT_FCFATALERR               0x20000000
#define INT_TXBANKONEEMPT            0x40000000
#define INT_TXBANKEMPT               0x80000000

// frame number registers.
#define REG_TXNUM                       0x14300
#define REG_TXRETNUM                    0x14304
#define REG_TXSDNUM                     0x14308
#define REG_TXMDNUM                     0x1430C
#define REG_TXMNNUM                     0x14310
#define REG_TXRETS32NUM                 0x14314
#define REG_TXRETS65NUM                 0x14318
#define REG_TXRET130NUM                 0x1431C
#define REG_TXRET261NUM                 0x14320
#define REG_TXRET522NUM                 0x14324
#define REG_RXPHYEDGEDETNUM             0x14328
#define REG_RXPHYHEADERRNUM             0x1432C
#define REG_RXPHYHEADDETNUM             0x14330
#define REG_RXCNLHEADERRNUM             0x14334
#define REG_RXCNLBODYERRNUM             0x14338
#define REG_RXACKOKNUM                  0x1433C
#define REG_RXSDOKNUM                   0x14340
#define REG_RXMD1OKNUM                  0x14344
#define REG_RXMD2OKNUM                  0x14348
#define REG_RXMNOKNUM                   0x1434C
#define REG_RX32OKNUM                   0x14350
#define REG_RX32NGNUM                   0x14354
#define REG_RX65OKNUM                   0x14358
#define REG_RX65NGNUM                   0x1435C
#define REG_RX130OKNUM                  0x14360
#define REG_RX130NGNUM                  0x14364
#define REG_RX261OKNUM                  0x14368
#define REG_RX261NGNUM                  0x1436C
#define REG_RX522OKNUM                  0x14370
#define REG_RX522NGNUM                  0x14374
#define REG_RXVGAGAIN                   0x14378
#define REG_RXEVM                       0x1437C
#define REG_TXSTATEN                    0x14380
#define REG_RXSTATEN                    0x14384

/* USE_RSSI */
#define RXSTAT_RXVGAGAIN_EN          0x00100000  // bit20 : 1 << 20
#define RXSTAT_RX522NGNUM_EN         0x00080000
#define RXSTAT_RX522OKNUM_EN         0x00040000
#define RXSTAT_RX261NGNUM_EN         0x00020000
#define RXSTAT_RX261OKNUM_EN         0x00010000
#define RXSTAT_RX130NGNUM_EN         0x00008000
#define RXSTAT_RX130OKNUM_EN         0x00004000
#define RXSTAT_RX65NGNUM_EN          0x00002000
#define RXSTAT_RX65OKNUM_EN          0x00001000
#define RXSTAT_RX32NGNUM_EN          0x00000800
#define RXSTAT_RX32OKNUM_EN          0x00000400

// TX read monitor registers.
#define REG_TXR32NUM                    0x14388
#define REG_TXRETR32NUM                 0x1438C
#define REG_TXR65NUM                    0x14390
#define REG_TXRETR65NUM                 0x14394
#define REG_TXR130NUM                   0x14398
#define REG_TXRETR130NUM                0x1439C
#define REG_TXR261NUM                   0x143A0
#define REG_TXRETR261NUM                0x143A4
#define REG_TXR522NUM                   0x143A8
#define REG_TXRETR522NUM                0x143AC

// RX read monitor registers.
#define REG_TXRATEDOWNNUM               0x143B0
#define REG_TXRATEUPNUM                 0x143B4

#define REG_ZA_0x14480                  0x14480
#define REG_ZA_0x14484                  0x14484
#define REG_ZA_0x14488                  0x14488
#define REG_ZA_0x1448C                  0x1448C
#define REG_ZA_0x14490                  0x14490
#define REG_ZA_0x14494                  0x14494
#define REG_ZA_0x14498                  0x14498
#define REG_ZA_0x1449C                  0x1449C
#define REG_ZA_0x144A0                  0x144A0
#define REG_ZA_0x144A4                  0x144A4

// mode setting registers.
#define REG_LCSTATE                     0x146A8
#define REG_TXCOMMONRSV                 0x146AC
#define REG_TXSUBSN                     0x146B0
#define REG_MONSEL                      0x146B4
#define REG_AHBTSTADDRES                0x146D0

// alias setting registers.
#define REG_ALIASSET                    0x14C00
#define REG_ALIAS                       0x14E00

//
// PHY registers definition
// address range :      0x15000 -      0x15FFF
//
#define REG_TESTMODULECLKGATE           0x15004
#define REG_CONTROLTESTMODE             0x15020

#define REG_PHY_RXCOUNT_CLEAR           0x15040

#define REG_PHYRFCTRL_DCAMP_EN          0x1527C
#define REG_PHYRX_EVM_ENABLE            0x15528
#define REG_PHYRXEQ_ELCOUNT_EN          0x1563C

#define REG_PHY_RXSYNC_MF_SETTING       0x15700
#define RXSYNC_MF_SETTING_512TAP        0x00280046

// rx info read monitor registers.
#define REG_PHY_RXCOUNT_EDGEDET         0x15044
#define REG_PHY_RXCOUNT_HEADDET         0x15048
#define REG_PHY_RXCOUNT_HEADERR         0x1504C
#define REG_PHY_RXCOUNT_PYLDERR         0x15050
#define REG_PHY_RXCOUNT_PYLDOK          0x15054
#define REG_PHY_RX_PYLDERR              0x15058
#define REG_PHYRX_HEAD_ERR              0x15524
#define REG_PHYRX_EVM_I                 0x15514
#define REG_PHYRX_EVM_Q                 0x15518
#define REG_PHYRX_HEAD_DATA             0x15520
#define REG_PHYRFCTRL_AGCRSSI_DB        0x15274
#define REG_PHYRFCTRL_DCOFSET_I         0x15294
#define REG_PHYRFCTRL_DCOFSET_Q         0x15298
#define REG_PHYRFCTRL_RXVGAGAIN         0x152B8
#define REG_PHYRXEQ_CALCULATED_PHASEOFST        0x15430
#define REG_PHYRXEQ_CALCULATED_ANGLERATE        0x15434
#define REG_PHYRXEQ_FCANCEL_POSITIVE_COUNTER    0x15438
#define REG_PHYRXEQ_FCANCEL_NEGATIVE_COUNTER    0x1543C
#define REG_PHYRXEQ_ELCOUNT                     0x15640

//
// ANA registers definition
// address range :      0x16000 -      0x163FF
//
#define REG_RFOWNUID1                   0x16060
#define REG_RFOWNUID2                   0x16064
#define REG_SYS_UID_A                   0x16180
#define REG_SYS_UID_B                   0x16184
#define REG_SYS_UID_C                   0x16188
#define REG_SYS_UID_D                   0x1618C

//
// RF regisgter definition
// add modified for evaluation board.
// 
#define REG_SYNS_TUNE                   0x160B4
#define REG_TX_POWER_SET                0x1614C
#define REG_TX_CARRIER_LEAK_TUNE        0x16288
#define REG_VCO_CAPSW_TUNE              0x160B4
#define REG_PAT_GC                      0x1614C
#define REG_IQDAC_TESTPO_INDIV          0x16384

//
// SPI(EEPROM) registers definition
// address range :      0x16400 -      0x167FF
//
#define REG_SPICONTROL                  0x16400
#define REG_SPITIMINGCONTROL            0x16404
#define REG_SPITXDATA                   0x16408
#define REG_SPIRXDATA                   0x1640C
#define REG_SPIINTCONTROL               0x16410
#define REG_SPIINTSTATUS                0x16414
#define REG_SPIDEBUG1                   0x16418
#define REG_SPIDEBUG2                   0x1641C
#define REG_SPITIMINGCONTROL2           0x16420

#define SPICTL_FSDOFLT_CMD              0x00800000
#define SPICTL_CSMODE_AUTO              0x00040000
#define SPICTL_RESERVED                 0x00000010

#define SPICTL_CFGMODE                  0x00000000
#define SPICTL_ACTMODE                  0x00000002

#define SPICTL_MODE_MASK                0x00000002
#define SPICTL_SPICS_ENABLE             0x00000100
#define SPICTL_SPICS_DISABLE            0x00000000
#define SPICTL_RXDMA_ENABLE             0x00010000

#define SPIINTST_TXFUL                  0x00000100
#define SPIINTST_STX                    0x00000001
#define SPIINTST_COMP_AHBTRANS          0x00000080
#define SPITIMCTL_BASE_SHIFT                     8

#define SPITXDATA_TXREADCMD             0x00000300
#define SPITXDATA_BYTECNT_2BYTE         0x00010000
#define SPITXDATA_CMD_READ              0x00040000
#define SPITXDATA_CONT_TRANS            0x00080000
#define SPITXDATA_BYTECNT_SHIFT                 16
#define SPITXDATA_LENGTH_SHIFT                  24


//
// UART registers definition
// address range :      0x16800 -      0x16BFF
//
#define REG_UARTCONTROL                 0x16800
#define REG_UARTBAUDRATE                0x16804
#define REG_UARTSTATUS                  0x16810


//
// RF SPI registers definition
// address range :      0x16C00 -      0x16FFF
//
#define REG_RSPCONTROL                  0x16C00
#define REG_RSPTXDATA                   0x16C08
#define REG_RSPRXDATA                   0x16C0C
#define REG_RSPINTCONTROL               0x16C10
#define REG_RSPINTSTATUS                0x16C14
#define REG_RSPDEBUG1                   0x16C18
#define REG_RSPDEBUG2                   0x16C1C
#define REG_RSPTXRXLENGTH               0x16C2C

//
// Calibration registers definition.
// address range : --- where to map ? ---
//
#define REG_BOOTREG                     0x00000
#define REG_HOSTREQUEST                 0x00100
#define REG_HOSTINT                     0x00104
#define REG_HOSTSTATUS                  0x00108
#define REG_HOSTMASK                    0x0010C
#define REG_HOSTFLAGSET0                0x00110
#define REG_HOSTFLAGSET1                0x00114
#define REG_HOSTFLAGSET2                0x00118
#define REG_HOSTFLAGSET3                0x0011C
#define REG_IZANREQUEST                 0x00120
#define REG_IZANINT                     0x00124
#define REG_IZANSTATUS                  0x00128
#define REG_IZANMASK                    0x0012C
#define REG_IZANFLAGSET0                0x00130
#define REG_IZANFLAGSET1                0x00134
#define REG_IZANFLAGSET2                0x00138
#define REG_IZANFLAGSET3                0x0013C

/*-------------------------------------------------------------------
 * Structure Definitions
 *-----------------------------------------------------------------*/


/*-------------------------------------------------------------------
 * Inline Functions
 *-----------------------------------------------------------------*/


#endif /* __IZAN_SDIO_REG_H_ */
