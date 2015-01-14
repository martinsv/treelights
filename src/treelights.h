#ifndef TREELIGHTS_H_
#define TREELIGHTS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

void cleanup(int sig);
void nsleep(unsigned long nsecs);
void _usleep(unsigned long usecs);

int do_exit;
char buf[255];

int  m_mfd;
volatile unsigned long *m_base_addr;

int gpioSetup();
void gpioDirection(int gpio, int direction);
int gpioRead(int gpio);
void gpioSet(int gpio, int value);

#endif