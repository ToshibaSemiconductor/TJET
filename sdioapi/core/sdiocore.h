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

#define sdioapi_set(x) sdioapi_ddi_set(x)
#define sdioapi_unset(x) sdioapi_ddi_unset(x)
#define printf(x...) sdioapi_ddi_printf(x)
#define sdioapi_cmd52(x...) sdioapi_ddi_cmd52(x)
#define sdioapi_cmd53(x...) sdioapi_ddi_cmd53(x)

#define FALSE		0
#define TRUE		1

/* #define SD_DEBUG	(SD_DBG_CMD | SD_DBG_FUNC) */
#define SD_DEBUG	0x0

#define SD_DBG_WARN  0x01
#define SD_DBG_ERR   0x02
#define SD_DBG_FUNC  0x04
#define SD_DBG_CMD   0x08
#define SD_DBG_INFO  0x10
#define SD_DBG_STAT  0x20
#define SD_DBG_RW    0x40
#define SD_DBG_INT   0x80
#define SD_DBG_SG   0x100
#define SD_DBG_CLK  0x200

#define DPRINT(flag, x...) do { if ((flag) & SD_DEBUG) printf(x); } while (0)

