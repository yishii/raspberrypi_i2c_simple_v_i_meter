/*

  Simple Volt/Current meter with Raspberry Pi



  with Strawberry-linux's i2c lcd control sample
  LCD : http://strawberry-linux.com/catalog/items?code=27030

  Coded by Yasuhiro ISHII,24 Mar,2013
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

#define I2C_SLAVEADDR_LCD 0x3e
#define I2C_SLAVEADDR_INA 0x40

int lcd_sendInit(int fd);
int lcd_sendString(int fd,char* str);
int lcd_sendCursorLocate(int fd,int row,int col);
int ina_getDieId(int fd,unsigned short* id);
int ina_getVoltage(int fd,int* vol);
int ina_getCurrent(int fd,int* curr);
static int ina_getValue(int fd,int addr,int* val);

int main(void)
{
  int fd;
  unsigned short id;
  signed int v;
  signed int c;
  char buff[20];

  fd = open("/dev/i2c-1",O_RDWR);
  if(fd < 0){
    printf("i2c open error\n");
    exit(1);
  }
    
  lcd_sendInit(fd);

  while(1){
    ina_getVoltage(fd,&v);
    ina_getCurrent(fd,&c);

    sprintf(buff,"%6dmV",v);
    lcd_sendCursorLocate(fd,0,0);
    lcd_sendString(fd,buff);

    sprintf(buff,"%6dmA",c);
    lcd_sendCursorLocate(fd,0,1);
    lcd_sendString(fd,buff);

    usleep(10000);//wait 10[ms]
  }

  close(fd);
}

int lcd_sendInit(int fd)
{
  unsigned char init_1[] = { 0x3e,0x39,0x14,0x75,0x5e,0x6c };
  unsigned char init_2[] = { 0x0c,0x01,0x06 };
  unsigned char buff[2];
  int i;
  int result;

  ioctl(fd,I2C_SLAVE,I2C_SLAVEADDR_LCD);
  
  buff[0] = 0;
  for(i=0;i<sizeof(init_1);i++){
    buff[1] = init_1[i];
    result = write(fd,buff,2);
    usleep(30);
  }
  
  usleep(200000);

  for(i=0;i<sizeof(init_2);i++){
    buff[1] = init_2[i];
    result = write(fd,buff,2);
    usleep(30);
  }

  return(0);
}

int lcd_sendString(int fd,char* str)
{
  int i;
  unsigned char buff[2];
  int result;

  ioctl(fd,I2C_SLAVE,I2C_SLAVEADDR_LCD);

  buff[0] = 0x40;
  while(*str){
    buff[1] = *str;
    result = write(fd,buff,2);
    str++;
  }

  return(0);
}

int lcd_sendCursorLocate(int fd,int row,int col)
{
  unsigned char buff[2];
  int result;

  ioctl(fd,I2C_SLAVE,I2C_SLAVEADDR_LCD);

  buff[0] = 0x00;
  buff[1] = 0x80;

  if(col != 0){
    buff[1] += 0x40;
  }
  buff[1] += row;

  result = write(fd,buff,2);

  return(0);
}

int ina_getDieId(int fd,unsigned short* id)
{
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg[2];
  int result;
  unsigned char buff[2] = { 0 };
  unsigned char buff2[2] = { 0 };

  memset((void*)&data,0,sizeof(struct i2c_rdwr_ioctl_data));
  memset((void*)msg,0,sizeof(msg));

  buff[0] = 0x00;

  msg[0].addr = I2C_SLAVEADDR_INA;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = buff;

  msg[1].addr = msg[0].addr;
  msg[1].flags = I2C_M_RD;
  msg[1].len = 2;
  msg[1].buf = buff2;

  data.msgs = msg;
  data.nmsgs = 2;

  result = ioctl(fd,I2C_RDWR,&data);

  return(0);
}

int ina_getVoltage(int fd,int* vol)
{
  int result;

  result = ina_getValue(fd,2,vol);
  *vol += *vol >> 2;

  return(result);
}

int ina_getCurrent(int fd,int* curr)
{
  int result;
  
  result = ina_getValue(fd,4,curr);

  return(result);
}

static int ina_getValue(int fd,int addr,int* val)
{
  struct i2c_rdwr_ioctl_data data;
  struct i2c_msg msg[2];
  int result;
  unsigned char buff[2] = { 0 };
  unsigned char buff2[2] = { 0 };

  memset((void*)&data,0,sizeof(struct i2c_rdwr_ioctl_data));
  memset((void*)msg,0,sizeof(msg));

  buff[0] = addr;

  msg[0].addr = I2C_SLAVEADDR_INA;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = buff;

  msg[1].addr = msg[0].addr;
  msg[1].flags = I2C_M_RD;
  msg[1].len = 2;
  msg[1].buf = buff2;

  data.msgs = msg;
  data.nmsgs = 2;

  result = ioctl(fd,I2C_RDWR,&data);

  *val = buff2[0]<<8 | buff2[1];

  return(0);
}
