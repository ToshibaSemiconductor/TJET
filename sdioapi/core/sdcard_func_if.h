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

/* for sdcard func drivers */
struct sdcard_device {
	; /* dummy */
};

struct sdio_card_id {
	unsigned long CISTPL_MANFID_MANF; /* lower 16bit */
};

struct sdcard_driver {
	char *Name;
	struct sdio_card_id Id_table[16];
	void (*Probe)(struct sdcard_device *pdev);
	void (*Disconnect)(struct sdcard_device *pdev);
};

struct sdcard_cmd52 {
	unsigned int direction; /* 0: Rd(card->host), 1: Wr(host->card) */
	unsigned int raw; /* 0: Normal, 1: Read and Write */
	unsigned int regaddr; /* lower 17bit */
	unsigned int data; /* lower 8bit, read data or write data */
	unsigned int resp_flags; /* CMD response flags:
				    b0:Out of range, b1:Invalid funcno,
				    b2:RFU, b3:General Error,
				    b4:b5:IO state, b6: Illegal command,
				    b7:Command CRC Error */
};

struct sdcard_cmd53 {
	unsigned int direction; /* 0: Rd(card->host), 1: Wr(host->card) */
	unsigned int bm; /* 0: byte mode, 1:block mode*/
	unsigned int op; /* 0: fixed address, 1: incrementing address */
	unsigned int regaddr; /* lower 17bit */
	unsigned int bcount; /* read data or write data */
	unsigned char *dbuf; /* pointer to data buffer */
	unsigned int resp_flags; /* CMD response flags:
				    b0:Out of range, b1:Invalid funcno,
				    b2:RFU, b3:General Error,
				    b4:b5:IO state, b6: Illegal command,
				    b7:Command CRC Error */
};

/* definitions for sdcard func drivers */
struct sdcard_device *sdcard_register_driver(struct sdcard_driver *pdrv);
int sdcard_unregister_driver(struct sdcard_driver *pdrv);
int sdcard_register_irq_handler(void *irq_handler, void *ptr);
int sdcard_cmd52(struct sdcard_device *pdev, struct sdcard_cmd52 *cmd);
int sdcard_cmd53(struct sdcard_device *pdev, struct sdcard_cmd53 *cmd);

/* for debug */
int sdcard_cmd52_funcnum(struct sdcard_device *pdev, struct sdcard_cmd52 *cmd,
			 unsigned int funcnum);
int sdcard_cmd53_funcnum(struct sdcard_device *pdev, struct sdcard_cmd53 *cmd,
			 unsigned int funcnum);
