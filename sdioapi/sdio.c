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
 * SDIO Driver
 */

#include <linux/module.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include "sdio_if.h"

static struct sdioapi_callback *sdiocallp;
static struct sdio_func *sdiofunc;

#define DRIVER_VERSION "1.0.0"


#define SDIO_VENDOR_ID_TOSHIBA 0x0098
#define SDIO_BLOCK_SIZE 512

static const struct sdio_device_id sdioapi_ids[] = {
	{ SDIO_DEVICE(SDIO_VENDOR_ID_TOSHIBA, SDIO_ANY_ID),
	  .driver_data = (unsigned long) NULL},
	{ SDIO_DEVICE(0x02fe, 0x2128),
	  .driver_data = (unsigned long) NULL}, /* Test device */
	{ }	/* Terminating entry */
};

static void sdioapi_interrupt(struct sdio_func *func)
{
	sdio_release_host(func);

	if (sdiocallp != NULL && sdiocallp->interrupt_handler != NULL)
		sdiocallp->interrupt_handler(sdiocallp->args);

	sdio_claim_host(func);
}

static int sdioapi_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
	int ret;

	sdio_claim_host(func);

	ret = sdio_enable_func(func);
	if (ret) {
		pr_info("sdio_enable_func() failed.\n");
		ret = -EIO;
		goto release_host;
	}

	ret = sdio_claim_irq(func, sdioapi_interrupt);
	if (ret) {
		pr_info("sdio_claim_irq() failed.\n");
		ret = -EIO;
		goto disable_func;
	}

	ret = sdio_set_block_size(func, SDIO_BLOCK_SIZE);
	if (ret) {
		pr_info("cannot set SDIO block size\n");
		ret = -EIO;
		goto release_irq;
	}

	sdio_release_host(func);

	sdiofunc = func;
	if (sdiocallp != NULL && sdiocallp->probe != NULL)
		sdiocallp->probe((void *)NULL);

	return 0;

release_irq:
	sdio_release_irq(func);

disable_func:
	sdio_disable_func(func);

release_host:
	sdio_release_host(func);

	return ret;
}

static void sdioapi_remove(struct sdio_func *func)
{
	if (sdiocallp != NULL && sdiocallp->remove != NULL)
		sdiocallp->remove((void *)NULL);

	sdiofunc = NULL;

	sdio_claim_host(func);
	sdio_release_irq(func);
	sdio_disable_func(func);
	sdio_release_host(func);
}

static struct sdio_driver sdioapi = {
	.name		= "sdioapi",
	.id_table	= sdioapi_ids,
	.probe		= sdioapi_probe,
	.remove		= sdioapi_remove,
};

static int __init sdioapi_init(void)
{
	sdiocallp = NULL;

	pr_info("SDIOAPI driver: rev" DRIVER_VERSION);

	return 0;
}

static void __exit sdioapi_exit(void)
{
	sdiocallp = NULL;
}

module_init(sdioapi_init);
module_exit(sdioapi_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SDIO driver");
MODULE_VERSION(DRIVER_VERSION);

/* ddi_entrypoints */
int sdioapi_ddi_set(struct sdioapi_callback *c)
{
	int ret = 0;

	if (sdiocallp != NULL && c != NULL
	    && sdiocallp->probe == c->probe) {
		sdiocallp = c;
		return 0;
	}

	if (sdiocallp != NULL) {
		pr_err("SDIO Driver is already registered.\n");
		return -EBUSY;
	}

	sdiocallp = c;

	if (sdio_register_driver(&sdioapi) != 0) {
		pr_err("SDIO Driver registration failed.\n");
		return -ENODEV;
	}

	return ret;
}

void sdioapi_ddi_unset(void)
{
	sdio_unregister_driver(&sdioapi);
	sdiocallp = NULL;
}

int sdioapi_ddi_printf(const char *fmt, ...)
{
	va_list args;
	static char buf[512];
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	return printk("%s", buf);
}

int sdioapi_ddi_cmd52(unsigned int direction, unsigned int regaddr,
		      unsigned char *data)
{
	int err;

	if (sdiofunc == NULL)
		return -EBUSY;

	sdio_claim_host(sdiofunc);
	if (direction) {
		sdio_writeb(sdiofunc, *data, regaddr, &err);
	} else {
		*data = sdio_readb(sdiofunc, regaddr, &err);
	}
	sdio_release_host(sdiofunc);
	return err;
}

int sdioapi_ddi_cmd53(unsigned int direction, unsigned regaddr,
		      unsigned char *data, int size, unsigned int op)
{
	int err;

	if (sdiofunc == NULL)
		return -EBUSY;

	sdio_claim_host(sdiofunc);
	if (op) {
		if (direction) {
			err = sdio_memcpy_toio(sdiofunc, regaddr,
					       data, size);
		} else {
			err = sdio_memcpy_fromio(sdiofunc, data,
						 regaddr, size);
		}
	} else {
		if (direction) {
			err = sdio_writesb(sdiofunc, regaddr,
					   data, size);
		} else {
			err = sdio_readsb(sdiofunc, data,
					  regaddr, size);
		}
	}
	sdio_release_host(sdiofunc);
	return err;
}

EXPORT_SYMBOL(sdioapi_ddi_set);
EXPORT_SYMBOL(sdioapi_ddi_unset);
EXPORT_SYMBOL(sdioapi_ddi_printf);
EXPORT_SYMBOL(sdioapi_ddi_cmd52);
EXPORT_SYMBOL(sdioapi_ddi_cmd53);
