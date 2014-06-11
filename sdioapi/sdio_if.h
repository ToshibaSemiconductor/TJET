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

struct dummy {
	;
};

struct sdioapi_callback {
	void (*probe)(struct dummy *p);
	void (*remove)(struct dummy *p);
	void (*interrupt_handler)(void *p);
	void *args;
};

int sdioapi_ddi_set(struct sdioapi_callback *c);
void sdioapi_ddi_unset(void);
int sdioapi_ddi_printf(const char *fmt, ...);
int sdioapi_ddi_cmd52(unsigned int direction, unsigned int regaddr,
		      unsigned char *data);
int sdioapi_ddi_cmd53(unsigned int direction, unsigned regaddr,
		      unsigned char *data, int size, unsigned int op);

