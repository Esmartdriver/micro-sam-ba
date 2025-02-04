/*
 * Copyright (c) 2015-2016, Atmel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "comm.h"
#include "utils.h"

static bool configure_tty(int fd, int speed)
{
	struct termios tty;

	memset(&tty, 0, sizeof(tty));

	if (tcgetattr(fd, &tty) != 0) {
		perror("error from tcgetattr: ");
		return false;
	}

	if (speed) {
		cfsetospeed(&tty, speed);
		cfsetispeed(&tty, speed);
	}

	tty.c_cflag &= ~(CSIZE | PARENB | PARODD | CSTOPB | CRTSCTS);
	tty.c_cflag |= CS8 | CLOCAL | CREAD;
	tty.c_lflag = 0;
	tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 20;
	tty.c_iflag &= ~(ICRNL | IGNBRK | IXON | IXOFF | IXANY);

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		perror("error from tcsetattr: ");
		return false;
	}

	return true;
}

static int _read(int fd, void *buf, size_t count) {
	int ret = 0;
	uint8_t *p = (uint8_t *)buf;
	while (count>0) {
		int len;
		len = read(fd, p+ret, count);
		count -= len;
		ret += len;
	}
	return ret;
}

static bool switch_to_binary(int fd)
{
	char cmd[] = "N#";
	printf("start switch to binary\n");
	if (write(fd, cmd, strlen(cmd)) != strlen(cmd))
		return false;
	printf("write done\n");
	return _read(fd, cmd, 2) == 2;
}



int samba_open(const char* device)
{
	int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		perror("Could not open device");
		return -1;
	}
	printf("port open\n");
	if (!configure_tty(fd, B115200)) {
		perror("Could not configure");
		close(fd);
		return -1;
	}
	   	printf("switch to binary\n");	

	if (!switch_to_binary(fd)) {
		close(fd);
		return -1;
	}
	printf("finished port open\n");
	return fd;
}

void samba_close(int fd)
{
	close(fd);
}

bool samba_read_word(int fd, uint32_t addr, uint32_t* value)
{
	char cmd[12];
	snprintf(cmd, sizeof(cmd), "w%08x,#", addr);
	if (write(fd, cmd, strlen(cmd)) != strlen(cmd))
		return false;
	return _read(fd, value, 4) == 4;
}

bool samba_write_word(int fd, uint32_t addr, uint32_t value)
{
	char cmd[20];
	snprintf(cmd, sizeof(cmd), "W%08x,%08x#", addr, value);
	return write(fd, cmd, strlen(cmd)) == strlen(cmd);
}

bool samba_read(int fd, uint8_t* buffer, uint32_t addr, uint32_t size)
{
	char cmd[20];
	while (size > 0) {
		printf("reading addr %#x\n", addr);
		uint32_t count = MIN(size, 1024);
		printf("count is %d\n", count);
		// workaround for bug when size is exactly 512
		if (count == 512){
			printf("apply workaround \n");
			count = 1;
		}
		snprintf(cmd, sizeof(cmd), "R%08x,%08x#", addr, count);
				printf("write-cmd is %s \n", cmd);
		if (write(fd, cmd, strlen(cmd)) != strlen(cmd)){
    
			printf("cmd-write failed\n");
			return false;
		
		} else {
			printf("cmd-write done\n");
		}
        printf("read-cmd start...\n");
		if (read(fd, buffer, count) != count){
			printf("cmd-read failed\n");
			return false;
		} else {
			printf("cmd-read done\n");
		}
		addr += count;
		buffer += count;
		size -= count;
	}
	return true;
}

bool samba_write(int fd, uint8_t* buffer, uint32_t addr, uint32_t size)
{
	char cmd[20];
	while (size > 0) {
		uint32_t count = MIN(size, 1024);
		// workaround for bug when size is exactly 512
		if (count == 512)
			count = 1;
		snprintf(cmd, sizeof(cmd), "S%08x,%08x#", addr, count);
		if (write(fd, cmd, strlen(cmd)) != strlen(cmd))
			return false;
		if (write(fd, buffer, count) != count)
			return false;
		buffer += count;
		size -= count;
	}
	return true;
}
