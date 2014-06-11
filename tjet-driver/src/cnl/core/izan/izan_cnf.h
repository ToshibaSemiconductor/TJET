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
 *  @file     izan_cnf.h
 *
 *  @brief    IZAN depended header file.
 *
 *
 *  @note
 */
/*=================================================================*/

#if !defined(__IZAN_CNF_H__)
#define __IZAN_CNF_H__


/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
/**
 * Bank Number configurations.
 * TX and RX bank number.
 * 
 * valid range is
 * IZAN_TX_CSDU_NUM (1-15)
 * IZAN_RX_CSDU_NUM (16 - IZAN_TX_CSDU_NUM)
 *
 */
#define IZAN_TX_CSDU_NUM         8
#define IZAN_RX_CSDU_NUM         8

/**
 * CNL timer configurations.
 * set value as `usec' order
 */
#define IZAN_TCONNECT_TIMER   5000000 // default 5sec      REG_TCONNECT  0x14040
#define IZAN_TACCEPT_TIMER    1000000 // default 1sec      REG_TACCEPT   0x14044
#define IZAN_TRETRY_TIMER     2000000 // default 2sec      REG_TRETRY    0x14048
#define IZAN_TRESEND_TIMER     100000 // default 100msec   REG_TRESEND   0x1404C
#define IZAN_TKEEPALIVE_TIMER  500000 // default 500msec   REG_TKEEPALIVE0x14050
#define IZAN_TAS_TIMER            140 // default 140usec   REG_TAS       0x14054
#define IZAN_TAC_TIMER            100 // default 100usec   REG_TAC       0x1405C
#define IZAN_SRCHDMT_TIMER     500000 // default 500msec   TDS0(REG_SRCHDMTTIM0),TDS1(REG_SRCHDMTTIM1)
#define IZAN_HBNTDMT_TIMER     500000 // default 500msec   TDC0(REG_HBNTDMTTIM0),TDC1(REG_HBNTDMTTIM1)

/* CNL Timer configurations. */
#define IZAN_TSIFS          0x0000018A  // REG_TSIFS          0x14064 (header only)
#define IZAN_TIIFS          0x0000018A  // REG_TIIFS          0x14068 (header only)
#define IZAN_TRIFS          0x000003EE  // REG_TRIFS          0x1406C (header only)
#define IZAN_TCREQIFS       0x000005EA  // REG_TCREQIFS       0x14080 (header only)
#define IZAN_TCACCIFS       0x000006CB  // REG_TCACCIFS       0x14084 (header only)

/* CNL IIFS configurations */
#define IZAN_IIFSCHANGEMODE 0x00000000  // REG_IIFSCHANGEMODE 0x140A0 (header only)
#define IZAN_IIFS2ND        0x00000532  // REG_IIFS2ND        0x140A4 (header only)
#define IZAN_IIFSPERIOD     0x000122C6  // REG_IIFSPERIOD     0x140A8 (header only)
#define IZAN_IIFS2PERIOD    0x000122C6  // REG_IIFS2PERIOD    0x140AC (header only)

/* CNL others reg configurations */
#define IZAN_FUNC           0x00000000  // REG_FUNC           0x01004 (header only)
#define IZAN_MONSEL         0x00000000  // REG_MONSEL         0x146B4

/* CNL Rate configurations */
#define IZAN_RATECNTQ       0x02040003  // REG_RATECNTQ       0x140D0
#define IZAN_RATECNTS       0x06100020  // REG_RATECNTS       0x140D4
#define IZAN_RATEOFF        0x00000000  // REG_RATEOFF        0x140DC
#define IZAN_RATE           0x45000000  // REG_RATE           0x140E0

#define IZAN_OWNUID1        0x00000000  // REG_OWNUID1        0x14000
#define IZAN_OWNUID2        0x00000000  // REG_OWNUID2        0x14004

/*
 * RF configurations.
 */
#define IZAN_VCO_CAPSW_TUNE     0x0000000E  // REG_VCO_CAPSW_TUNE        0x160B4
#define IZAN_PAT_GC             0x00000034  // REG_PAT_GC                0x1614C
#define IZAN_IQDAC_TESTPO_INDIV 0x00000024  // REG_IQDAC_TESTPO_INDIV    0x16384

#define IZAN_CRMSRSTAT_STANDARD 0x36B0  // REG_CRMSRRSLT0/1        0x00411
#define IZAN_MAX_SRCHDMNTTIM    0x2346
#define IZAN_MAX_HBNTDMTTIM     0x2346
#define IZAN_MAX_TKEEPALIVE     0x2346

#endif /* __IZAN_CNF_H__ */
