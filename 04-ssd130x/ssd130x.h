#ifndef SSD130x_H
#define SSD130x_H
#include <linux/i2c.h>

struct ssd130x_type {
	void (*init)(struct i2c_client *cli);
	void (*deinit)(struct i2c_client *cli);
	int (*write_cmd)(struct i2c_client *cli, const char cmd);
	int (*write_data)(struct i2c_client *cli, const char data);
	int (*write_data_batch)(struct i2c_client *cli, const char __user *buf,
				size_t n);
};

extern struct ssd130x_type ssd1306_type;

#endif