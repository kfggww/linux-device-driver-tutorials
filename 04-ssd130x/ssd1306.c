#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "ssd130x.h"

static int ssd1306_write_cmd(struct i2c_client *client, char cmd)
{
	int ret;
	char buf[2];
	buf[0] = 0x00;
	buf[1] = cmd;

	ret = i2c_master_send(client, buf, 2);
	if (ret != 2) {
		pr_err("%s: failed sending data to I2C device with addr=0x%x\n",
		       __func__, client->addr);
		return -EAGAIN;
	}

	return 0;
}

static int ssd1306_write_data(struct i2c_client *client, char data)
{
	int ret;
	char buf[2];
	buf[0] = 0x40;
	buf[1] = data;

	ret = i2c_master_send(client, buf, 2);
	if (ret != 2) {
		pr_err("%s: failed sending data to I2C device with addr=0x%x\n",
		       __func__, client->addr);
		return -EAGAIN;
	}

	return 0;
}

static int ssd1306_write_data_batch(struct i2c_client *cli,
				    const char __user *buf, size_t n)
{
	int ret = 0;
	char *buf_copy = kmalloc(n + 1, GFP_KERNEL);
	if (buf_copy == NULL) {
		return -ENOMEM;
	}

	buf_copy[0] = 0x40;
	ret = copy_from_user(buf_copy + 1, buf, n);
	i2c_master_send(cli, buf_copy, n + 1);

	kfree(buf_copy);
	return ret == 0 ? 0 : -EAGAIN;
}

static void ssd1306_init(struct i2c_client *cli)
{
	/* MUX ratio */
	ssd1306_write_cmd(cli, 0xA8);
	ssd1306_write_cmd(cli, 0x3F);

	/* Display offset */
	ssd1306_write_cmd(cli, 0xD3);
	ssd1306_write_cmd(cli, 0x00);

	/* Display start line */
	ssd1306_write_cmd(cli, 0x40);

	/* Segment re-map */
	ssd1306_write_cmd(cli, 0xA0);

	/* COM output scan direction */
	ssd1306_write_cmd(cli, 0xC0);

	/* COM pins hardware configuration */
	ssd1306_write_cmd(cli, 0xDA);
	ssd1306_write_cmd(cli, 0x02);

	/* Memory addressing mode: horizontal mode */
	ssd1306_write_cmd(cli, 0x20);
	ssd1306_write_cmd(cli, 0x00);

	/* Contrast control */
	ssd1306_write_cmd(cli, 0x81);
	ssd1306_write_cmd(cli, 0x7F);

	/* Disable entire display on */
	ssd1306_write_cmd(cli, 0xA4);

	/* Normal display */
	ssd1306_write_cmd(cli, 0xA6);

	/* Osc frequency */
	ssd1306_write_cmd(cli, 0xD5);
	ssd1306_write_cmd(cli, 0x80);

	/* Enable charge pump regulator */
	ssd1306_write_cmd(cli, 0x8D);
	ssd1306_write_cmd(cli, 0x14);

	/* Display on */
	ssd1306_write_cmd(cli, 0xAF);
}

static void ssd1306_deinit(struct i2c_client *cli)
{
	/* Display off */
	ssd1306_write_cmd(cli, 0xAE);
}

struct ssd130x_type ssd1306_type = {
	.init = ssd1306_init,
	.deinit = ssd1306_deinit,
	.write_cmd = ssd1306_write_cmd,
	.write_data = ssd1306_write_data,
	.write_data_batch = ssd1306_write_data_batch,
};