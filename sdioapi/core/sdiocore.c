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

/*
 * SDIO Card Driver
 */

#include <linux/module.h>

#include "sdcard_func_if.h"
#include "sdiocore.h"
#include "../sdio_if.h"

/*-------------------------------------------------------------------
 * Macro definition
 *-----------------------------------------------------------------*/
/**
 * @brief module version informations
 */
#define DRIVER_VERSION "1.0.1";

#define NULL ((void *)0)

static struct sdcard_device pdev;

static struct sdioapi_callback c = {
	NULL, NULL, NULL, NULL
};

struct sdcard_device *
sdcard_register_driver(struct sdcard_driver *pdrv)
{
	DPRINT(SD_DBG_FUNC, "%s: enter\n", __func__);
	c.probe = (void(*)(struct dummy *)) pdrv->Probe;
	c.remove = (void(*)(struct dummy *)) pdrv->Disconnect;
	sdioapi_set(&c);

	return &pdev;
}

int
sdcard_unregister_driver(struct sdcard_driver *pdrv)
{
	DPRINT(SD_DBG_FUNC, "%s: enter\n", __func__);
	sdioapi_ddi_unset();

	return TRUE;
}

int
sdcard_register_irq_handler(void *irq_handler, void *ptr)
{
	DPRINT(SD_DBG_FUNC, "%s: enter\n", __func__);
	c.interrupt_handler = irq_handler;
	c.args = ptr;
	sdioapi_set(&c);

	return TRUE;
}

int
sdcard_cmd52(struct sdcard_device *pdev, struct sdcard_cmd52 *cmd)
{
	int ret;

	DPRINT(SD_DBG_CMD, "%s: %s, addr=%08x, data=%04x\n",
	       __func__, cmd->direction ? "write" : "read ",
	       cmd->regaddr, cmd->data);

	ret = sdioapi_cmd52(cmd->direction, cmd->regaddr,
			    (unsigned char *)&cmd->data);
	if (ret)
		return FALSE;

	cmd->resp_flags = 0;
	return TRUE;
}

int
sdcard_cmd53(struct sdcard_device *pdev, struct sdcard_cmd53 *cmd)
{
	int size;
	int ret;

	size = cmd->bm ? cmd->bcount * 512 : cmd->bcount;

	DPRINT(SD_DBG_CMD, "%s: %s, addr=%08x, size=%04x\n",
	       __func__, cmd->direction ? "write" : "read ",
	       cmd->regaddr, size);

	ret = sdioapi_cmd53(cmd->direction, cmd->regaddr, cmd->dbuf,
			    size, cmd->op);
	if (ret)
		return FALSE;

	cmd->resp_flags = 0;
	return TRUE;
}

EXPORT_SYMBOL(sdcard_register_driver);
EXPORT_SYMBOL(sdcard_unregister_driver);
EXPORT_SYMBOL(sdcard_register_irq_handler);
EXPORT_SYMBOL(sdcard_cmd52);
EXPORT_SYMBOL(sdcard_cmd53);

int 
sdcard_cmd52_funcnum(struct sdcard_device *pdev, struct sdcard_cmd52 *cmd,
		     unsigned int funcnum)
{
	DPRINT(SD_DBG_CMD, "%s: %s, addr=%08x, data=%04x, fn=%d\n",
	       __func__, cmd->direction ? "write" : "read ",
	       cmd->regaddr, cmd->data, funcnum);
	cmd->resp_flags = 0;
	return TRUE;
}

int 
sdcard_cmd53_funcnum(struct sdcard_device *pdev, struct sdcard_cmd53 *cmd,
		     unsigned int funcnum)
{
	int size = cmd->bm ? cmd->bcount * 512 : cmd->bcount;
	DPRINT(SD_DBG_CMD, "%s: %s, addr=%08x, size=%04x, fn=%d\n",
	       __func__, cmd->direction ? "write" : "read ",
	       cmd->regaddr, size, funcnum);
	cmd->resp_flags = 0;
	return TRUE;
}

EXPORT_SYMBOL(sdcard_cmd52_funcnum);
EXPORT_SYMBOL(sdcard_cmd53_funcnum);

MODULE_LICENSE("GPL v2");
MODULE_VERSION( DRIVER_VERSION );
