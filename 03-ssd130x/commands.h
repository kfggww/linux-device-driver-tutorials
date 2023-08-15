#ifndef COMMONDS_H
#define COMMONDS_H

void ssd130x_disp_init(struct i2c_client *client);
void ssd130x_disp_print(struct i2c_client *client, char *str);

#endif