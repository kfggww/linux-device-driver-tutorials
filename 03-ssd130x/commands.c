#include <linux/i2c.h>
#include <linux/printk.h>
#include "commands.h"

#define FONT_DATA(ch) ssd130x_font[(ch) - ' ']

extern const unsigned char ssd130x_font[][5];

static void write_cmd(struct i2c_client *client, char cmd)
{
	char buf[2];
	buf[0] = 0;
	buf[1] = cmd;
	int n = i2c_master_send(client, buf, 2);
	if (n < 0) {
		pr_err("%s: failed to send i2c cmd\n", __func__);
	}
}

static void write_data(struct i2c_client *client, char data)
{
	char buf[2];
	buf[0] = 0x40;
	buf[1] = data;
	int n = i2c_master_send(client, buf, 2);
	if (n < 0) {
		pr_err("%s: failed to send i2c data\n", __func__);
	}
}

void ssd130x_disp_print(struct i2c_client *client, char *str)
{
	while (*str != '\0') {
		const char *font_data = FONT_DATA(*str);
		int i = 0;
		for (; i < 5; i++) {
			write_data(client, font_data[i]);
		}
		str++;
	}
}

void ssd130x_disp_fill(struct i2c_client *client, char data)
{
	int total = 128 * 8;
	int i = 0;

	for (; i < total; i++) {
		write_data(client, data);
	}
}

void ssd130x_disp_setcursor(struct i2c_client *client, int lineno, int cursor)
{
	write_cmd(client, 0x21);
	write_cmd(client, cursor);
	write_cmd(client, 127);

	write_cmd(client, 0x22);
	write_cmd(client, lineno);
	write_cmd(client, 7);
}

void ssd130x_disp_init(struct i2c_client *client)
{
	if (client == NULL || client->addr == 0) {
		pr_err("%s: invalid i2c client\n", __func__);
		return;
	}

	write_cmd(client, 0x20); // Set memory addressing mode
	write_cmd(client, 0x00); // Horizontal addressing mode
	write_cmd(client, 0xAF); // Display ON in normal mode
	ssd130x_disp_fill(client, 0x00);
	ssd130x_disp_setcursor(client, 0, 0);

	pr_info("%s: done, addr=0x%x\n", __func__, client->addr);
}