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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "chipid.h"
#include "comm.h"
#include "eefc.h"
#include "utils.h"

#define BUFFER_SIZE    8192
#define RESET_CR_ADDR  0x400e1800
#define RESET_KEY      0xA5000009
#define GPNVM_BIT1     0x00000001
#define EEFC_FCR_STUI  0x5A00000E
#define EEFC_FCR_ADDR  0x400E0C04
#define EEFC_FCR_SPUI  0x5A00000F
#define QSPI_MR_ADDR   0x4007C004
#define QSPI_MR_SERIAL 0x00000001
#define QSPI_ICR_ADDR  0x4007C034  //QSPI Instruction Code Register Address
#define QSPI_ICR_FREAD 0x00000005  //FAST READ Instruction
#define QSPI_IFR_ADDR  0x4007C038  //QSPI Instruction Frame Register Address
#define QSPI_ICR_START 0x01000096  //Value to Start transfer
#define QSPI_IMR_ADDR  0x4007C01C  //QSPI Interrupt Mask Register Address
#define QSPI_RDR_ADDR  0x4007C008  //QSPI Receive Data Register Address
#define QSPI_CR_ADDR   0x4007C000  //QSPI Control Register Address 24th bit for LASTXFER
#define QSPI_SR_ADDR   0x4007C010  //QSPI Status Register Address 10th bit for  INSTRE


//#define DEBUG

char information[65];

static bool get_file_size(const char* filename, uint32_t* size)
{
	struct stat st;

	if (stat(filename, &st) != 0) {
		fprintf(stderr, "Could not access '%s'\n", filename);
		return false;
	}

	*size = st.st_size;
	return true;
}

static bool read_flash(int fd, const struct _chip* chip, uint32_t addr, uint32_t size, const char* filename)
{
	FILE* file = fopen(filename, "wb");
	if (!file) {
		fprintf(stderr, "Could not open '%s' for writing\n", filename);
		return false;
	}

	uint8_t buffer[BUFFER_SIZE];
	uint32_t total = 0;
	while (total < size) {
		uint32_t count = MIN(BUFFER_SIZE, size - total);
		if (!eefc_read(fd, chip, buffer, addr, count)) {
			fprintf(stderr, "Error while reading from %#x\n", addr);
			fclose(file);
			return false;
		}

		if (fwrite(buffer, 1, count, file) != count) {
			fprintf(stderr, "Error while writing to '%s'", filename);
			fclose(file);
			return false;
		}

		total += count;
		addr += count;
	}

	fclose(file);
	return true;
}

static bool write_flash(int fd, const struct _chip* chip, const char* filename, uint32_t addr, uint32_t size)
{
	FILE* file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Could not open '%s' for reading\n", filename);
		return false;
	}

	uint8_t buffer[BUFFER_SIZE];
	uint32_t total = 0;
	while (total < size) {
		uint32_t count = MIN(BUFFER_SIZE, size - total);
		if (fread(buffer, 1, count, file) != count) {
			fprintf(stderr, "Error while reading from '%s'", filename);
			fclose(file);
			return false;
		}

		if (!eefc_write(fd, chip, buffer, addr, count)) {
			fclose(file);
			return false;
		}

		total += count;
		addr += count;
	}

	fclose(file);
	return true;
}

static bool verify_flash(int fd, const struct _chip* chip, const char* filename, uint32_t addr, uint32_t size)
{
	FILE* file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Could not open '%s' for reading\n", filename);
		return false;
	}

	uint8_t buffer1[BUFFER_SIZE];
	uint8_t buffer2[BUFFER_SIZE];
	uint32_t total = 0;
	while (total < size) {
		uint32_t count = MIN(BUFFER_SIZE, size - total);
		if (fread(buffer1, 1, count, file) != count) {
			fprintf(stderr, "Error while reading from '%s'", filename);
			fclose(file);
			return false;
		}

		if (!eefc_read(fd, chip, buffer2, addr, count)) {
			fclose(file);
			return false;
		}

		for (int i = 0; i < BUFFER_SIZE; i++) {
			if (buffer1[i] != buffer2[i]) {
				fprintf(stderr, "Verify failed, first difference at offset %d", addr + i);
				fclose(file);
				return false;
			}
		}

		total += count;
		addr += count;
	}

	return true;
	fclose(file);
}

void identify_uniqueID(int fd, const struct _chip* chip, char* filename, bool err)
{
         information[0] = 'D';
         information[1] = 'U';
         information[2] = 'M';
         information[3] = 'P';
         information[4] = '_';

         #if defined(DEBUG)
			printf("Identifying device by unique identifier\n");
			#endif
			if (samba_write_word(fd, EEFC_FCR_ADDR, EEFC_FCR_STUI)){
				err = false;
				#if defined(DEBUG)
				printf("STUI command sent successfully...\n");
				#endif
			}
			#if defined(DEBUG)
			printf("Reading %d bytes at 0x%08x to file '%s'\n", 1024, 0 , "uniqueIdentifier.bin");
			#endif
			if (read_flash(fd, chip, 0, 1024, "uniqueIdentifier.bin")) {
				err = false;
			}

			#if defined(DEBUG)
			printf("Identifying device by unique identifier\n");
			#endif
			if (samba_write_word(fd, EEFC_FCR_ADDR, EEFC_FCR_SPUI)){
				err = false;
				#if defined(DEBUG)
				printf("SPUI command sent successfully...\n");
				#endif
			}
         #if defined(DEBUG)
			printf("Converting Binaries to Readable...\n");
			#endif
            static const size_t BufferSize = 1024;
   		    int i;
            FILE *ptr;
            unsigned char buffer2[BufferSize];

            ptr = fopen("uniqueIdentifier.bin","rb");
            fread(buffer2, sizeof(unsigned char), BufferSize, ptr);
            #if defined(DEBUG)
            printf("Unique ID: ");
            #endif
            for(i = 1; i < 19; i++){
            	#if defined(DEBUG)
            	printf("%c", (int)buffer2[i]);
            	#endif
               if(!( '\0'==  buffer2[i]))
               information[i+4] =  buffer2[i];
               else 
               	information[i+4] =  ' ';
               }
            information[23] =  '_';
            information[24] =  '_';
            #if defined(DEBUG)
            printf("\nSerial Number:");
            #endif
            for(i = 119; i < 150; i++){
            	#if defined(DEBUG)
            	printf("%c", (int)buffer2[i]);
            	#endif
            	if(!( '\0'==  buffer2[i]))
               information[i-99] = buffer2[i];
               else 
               	information[i-99] =  ' ';
            }
            #if (defined(DEBUG))
            printf("\n");
            #endif
            
            fclose (ptr);
            information[64] = '\0'; 
            
}

static void usage(char* prog)
{
	printf("Usage: %s <port> (read|write|verify|erase-all|erase-pages|ext-read|ext-write|ext-erase-all|gpnvm|identify|reset|exit-samba) [args]*\n", prog);
	printf("\n");
	printf("- Reading Flash:\n");
	printf("    %s <port> read <filename> <start-address> <size>\n", prog);
	printf("    Note: you may forgo the filename and the program will automatically make its own filename with the Unique ID & Serial Number\n");
	printf("\n");
	printf("- Writing Flash:\n");
	printf("    %s <port> write <filename> <start-address>\n", prog);
	printf("\n");
	printf("- Verify Flash:\n");
	printf("    %s <port> verify <filename> <start-address>\n", prog);
	printf("\n");
	printf("- Erasing Flash:\n");
	printf("    %s <port> erase-all\n", prog);
	printf("\n");
   printf("- Erase 16 Pages:\n");
	printf("    %s <port> erase-pages <first-page>\n", prog);
	printf("\n");
   printf("- Reading External Flash:\n");
	printf("    %s <port> ext-read <filename> <start-address> <size>\n", prog);
	printf("\n");
	printf("- Writing External Flash:\n");
	printf("    %s <port> ext-write <filename> <start-address>\n", prog);
	printf("\n");
	printf("- Erasing External Flash:\n");
	printf("    %s <port> ext-erase-all\n", prog);
	printf("\n");
	printf("- Getting/Setting/Clearing GPNVM:\n");
	printf("    %s <port> gpnvm (get|set|clear) <gpnvm_number>\n", prog);
	printf("\n");
	printf("- Identify Chip's unique identifier code & serial number:\n");
	printf("    %s <port> identify\n", prog);
	printf("\n");
	printf("- Reset device:\n");
	printf("    %s <port> reset\n", prog);
	printf("\n");
	printf("- Exit Samba: \n");
	printf("    %s <port> exit-samba\n", prog);
	printf("\n");
	printf("for all commands:\n");
	printf("    <port> is the USB device node for the SAM-BA bootloader, for\n");
	printf("         example '/dev/ttyACM0'\n");
	printf("    <start-addres> and <size> can be specified in decimal, hexadecimal (if\n");
	printf("         prefixed by '0x') or octal (if prefixed by 0).\n");
}

enum {
	CMD_READ = 1,
	CMD_READ_CUSTOM_NAME = 2,
	CMD_WRITE = 3,
   CMD_VERIFY = 4,
	CMD_ERASE_ALL = 5,
   CMD_ERASE_PAGES = 6,
	CMD_READ_EXT_FLASH = 7,
	CMD_WRITE_EXT_FLASH = 8,
	CMD_ERASE_EXT_FLASH = 9,
	CMD_GPNVM_GET = 10,
	CMD_GPNVM_SET = 11,
	CMD_GPNVM_CLEAR = 12,
	CMD_IDENTIFY = 13,
	CMD_RESET = 14,
	CMD_EXIT_SAMBA = 15,
};

int main(int argc, char *argv[])
{
	int fd = -1;
	int command = 0;
	char* port = NULL;
	char* filename = NULL;
	uint32_t addr = 0;
	uint32_t size = 0;
	bool err = true;

	// parse command line
	if (argc < 3) {
		fprintf(stderr, "Error: not enough arguments\n");
		usage(argv[0]);
		return -1;
	}
	port = argv[1];
	char* cmd_text = argv[2];

	if (!strcmp(cmd_text, "read")) {
		if (argc == 6) {
			command = CMD_READ_CUSTOM_NAME;
			filename = argv[3];
			addr = strtol(argv[4], NULL, 0);
			size = strtol(argv[5], NULL, 0);
			err = false;
		}else if (argc == 5) {
			command = CMD_READ;
			addr = strtol(argv[3], NULL, 0);
			size = strtol(argv[4], NULL, 0);
			err = false;
		}  else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}

	} else if (!strcmp(cmd_text, "write")) {
		if (argc == 5) {
			command = CMD_WRITE;
			filename = argv[3];
			addr = strtol(argv[4], NULL, 0);
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}

	}else if (!strcmp(cmd_text, "ext-read")) {
		if (argc == 6) {
			command = CMD_READ_EXT_FLASH;
			filename = argv[3];
			addr = strtol(argv[4], NULL, 0);
			size = strtol(argv[5], NULL, 0);
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}

	} else if (!strcmp(cmd_text, "ext-write")) {
		if (argc == 5) {
			command = CMD_WRITE_EXT_FLASH;
			filename = argv[3];
			addr = strtol(argv[4], NULL, 0);
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}

	} else if (!strcmp(cmd_text, "ext-erase-all")) {
		if (argc == 3) {
			command = CMD_ERASE_EXT_FLASH;
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}

	} else if (!strcmp(cmd_text, "verify")) {
		if (argc == 5) {
			command = CMD_VERIFY;
			filename = argv[3];
			addr = strtol(argv[4], NULL, 0);
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}

	} else if (!strcmp(cmd_text, "erase-all")) {
		if (argc == 3) {
			command = CMD_ERASE_ALL;
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}

	}else if (!strcmp(cmd_text, "erase-pages")) {  
		if (argc == 4) {
			command = CMD_ERASE_PAGES;
			addr = strtol(argv[3], NULL, 0);
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}

	}else if (!strcmp(cmd_text, "identify")) {
		if (argc == 3) {
			command = CMD_IDENTIFY;
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}

	}  else if (!strcmp(cmd_text, "gpnvm")) {
		if (argc == 5) {
			if (!strcmp(argv[3], "get")) {
				command = CMD_GPNVM_GET;
				addr = strtol(argv[4], NULL, 0);
				err = false;
			} else if (!strcmp(argv[3], "set")) {
				command = CMD_GPNVM_SET;
				addr = strtol(argv[4], NULL, 0);
				err = false;
			} else if (!strcmp(argv[3], "clear")) {
				command = CMD_GPNVM_CLEAR;
				addr = strtol(argv[4], NULL, 0);
				err = false;
			} else {
				fprintf(stderr, "Error: unknown GPNVM command '%s'\n", argv[3]);
			}
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}

	}else if (!strcmp(cmd_text, "exit-samba")) {
		if (argc == 3) {
			command = CMD_EXIT_SAMBA;
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		} 

	}else if (!strcmp(cmd_text, "reset")) {
		if (argc == 3) {
			command = CMD_RESET;
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		} 

	}else {
		err = true;
		fprintf(stderr, "Error: unknown command '%s'\n", cmd_text);
	}
	if (err) {
		usage(argv[0]);
		return -1;
	}

	err = true;

	printf("Port: %s\n", port);
	fd = samba_open(port);
	if (fd < 0){
		printf(">>>failed to open port\n");
		fprintf(stderr, "Could not open port\n");
		return -1;
	} else {
		   #if defined(DEBUG)
			printf(">>>port is open\n");
			#endif
	}
	#if defined(DEBUG)
    perror("reading chip id");
    #endif
	//Identify chip
	const struct _chip* chip;
	if (!chipid_identity_serie(fd, &chip)) {
		fprintf(stderr, "Could not identify chip\n");
		goto exit;
	}
	#if defined(DEBUG)
	printf("Device: Atmel %s\n", chip->name);
	#endif
	// Read and check flash information
	struct _eefc_locks locks;
	if (!eefc_read_flash_info(fd, chip, &locks)) {
		fprintf(stderr, "Could not read flash information\n");
		goto exit;
	}
	#if defined(DEBUG)
	printf("Flash Size: %uKB\n", chip->flash_size);
	#endif

	// Execute command
	switch (command) {
		case CMD_READ_CUSTOM_NAME:
		{
			printf("CMD: READ WITH CUSTOM FILE NAME\n");
			printf("Reading %d bytes at 0x%08x to file '%s'\n", size, addr, filename );
			if (read_flash(fd, chip, addr, size, filename)) {
				err = false;
			}
			break;
		}

		case CMD_READ:
		{
			identify_uniqueID(fd, chip, filename, err);
         filename = information;
         strncat(filename, ".bin",4);
			printf("CMD: READ\n");
			printf("Reading %d bytes at 0x%08x to file '%s'\n", size, addr, filename );
			if (read_flash(fd, chip, addr, size, filename)) {
				err = false;
			}
			break;
		}


		case CMD_WRITE:
		{
			printf("CMD: WRITE\n");
			if (get_file_size(filename, &size)) {
				printf("Unlocking %d bytes at 0x%08x\n", size, addr);
				if (eefc_unlock(fd, chip, &locks, addr, size)) {
					printf("Writing %d bytes at 0x%08x from file '%s'\n", size, addr, filename);
					if (write_flash(fd, chip, filename, addr, size)) {
						err = false;
					}
				}
			}
			break;
		}

		case CMD_VERIFY:
		{
						printf("CMD: VERIFY\n");
			if (get_file_size(filename, &size)) {
				printf("Verifying %d bytes at 0x%08x with file '%s'\n", size, addr, filename);
				if (verify_flash(fd, chip, filename, addr, size)) {
					err = false;
				}
			}
			break;
		}

		case CMD_ERASE_ALL:
		{
			printf("CMD: ERASE_ALL\n");
			printf("Unlocking all pages\n");
			if (eefc_unlock(fd, chip, &locks, 0, chip->flash_size * 1024)) {
				printf("Erasing all pages\n");
				if (eefc_erase_all(fd, chip)) {
					err = false;
				}
			}
			break;
		}

		case CMD_ERASE_PAGES:
		{
			printf("Erasing 16 pages from %d\n", addr);
			if (eefc_erase_16pages(fd, chip, addr)) {
				err = false;
			}
			break;
		}
		case CMD_READ_EXT_FLASH:
		{
			#if defined(DEBUG)
			printf("Switching into QSPI Serial Mode\n");
			#endif
			if (samba_write_word(fd, QSPI_MR_ADDR, QSPI_MR_SERIAL)){
				err = false;
				#if defined(DEBUG)
				printf("Switch to QSPI Serial Mode was Successful...\n");
				#endif
			}
			#if defined(DEBUG)
			printf("Writing instruction to follow\n");
			#endif			
			if (samba_write_word(fd, QSPI_ICR_ADDR, QSPI_ICR_FREAD)){
				err = false;
				#if defined(DEBUG)
				printf("successfully wrote instruction in Register...\n");
				#endif
			}
			#if defined(DEBUG)
			printf("Begin Transfer of Data...\n");
			#endif			
			if (samba_write_word(fd, QSPI_IFR_ADDR, QSPI_ICR_START)){
				err = false;
				#if defined(DEBUG)
				printf(" Successfully Wrote instruction to Start Transfer of Data...\n");
				#endif
			}

			printf("CMD: READ\n");
			printf("Reading %d bytes at 0x%08x to file '%s'\n", size, addr, filename);
			if (read_flash(fd, chip, QSPI_RDR_ADDR, size, filename)) {
				err = false;
			}
			break;
		}

		case CMD_WRITE_EXT_FLASH:
		{
			printf("CMD: WRITE\n");
			if (get_file_size(filename, &size)) {
				printf("Unlocking %d bytes at 0x%08x\n", size, addr);
				if (eefc_unlock(fd, chip, &locks, addr, size)) {
					printf("Writing %d bytes at 0x%08x from file '%s'\n", size, addr, filename);
					if (write_flash(fd, chip, filename, addr, size)) {
						err = false;
					}
				}
			}
			break;
		}

		case CMD_EXIT_SAMBA:
		{
			printf("Exiting Samba...\n");
			printf("Setting GPNVM%d\n", GPNVM_BIT1);
			if (eefc_set_gpnvm(fd, chip, GPNVM_BIT1)) {
				err = false;
			}
		    printf("Reseting device...\n");
			if (samba_write_word(fd, RESET_CR_ADDR, RESET_KEY)){
				err = false;
			}
			break;
		}
		case CMD_GPNVM_GET:
		{
			printf("Getting GPNVM%d\n", addr);
			bool value;
			if (eefc_get_gpnvm(fd, chip, addr, &value)) {
				printf("GPNVM%d is %s\n", addr, value ? "set" : "clear");
				err = false;
			}
			break;
		}

		case CMD_GPNVM_SET:
		{
			if (!addr && !getenv("GPNVM0_CONFIRM")) {
				fprintf(stderr, "To avoid setting the security bit (GPNVM0) by mistake, an additional\n");
				fprintf(stderr, "confirmation is required: please add a 'GPNVM0_CONFIRM' environment\n");
				fprintf(stderr, "variable with any value and try again.\n");
			} else {
				printf("Setting GPNVM%d\n", addr);
				if (eefc_set_gpnvm(fd, chip, addr)) {
					err = false;
				}
			}
			break;
		}

		case CMD_GPNVM_CLEAR:
		{
			printf("Clearing GPNVM%d\n", addr);
			if (eefc_clear_gpnvm(fd, chip, addr)) {
				err = false;
			}
			break;
		}

		case CMD_RESET:
		{
			printf("Reseting device...\n");
			if (samba_write_word(fd, RESET_CR_ADDR, RESET_KEY)){
				err = false;
			}
			break;
		}

		case CMD_IDENTIFY:
		{
			

			printf("Identifying device by unique identifier\n");
			if (samba_write_word(fd, EEFC_FCR_ADDR, EEFC_FCR_STUI)){
				err = false;
				printf("STUI command sent successfully...\n");
			}
			printf("Reading %d bytes at 0x%08x to file '%s'\n", 1024, 0 , "uniqueIdentifier.bin");
			if (read_flash(fd, chip, 0, 1024, "uniqueIdentifier.bin")) {
				err = false;
			}
						printf("Identifying device by unique identifier\n");
			if (samba_write_word(fd, EEFC_FCR_ADDR, EEFC_FCR_SPUI)){
				err = false;
				printf("SPUI command sent successfully...\n");
			}

			printf("Converting Binaries to Readable...\n");
            static const size_t BufferSize = 1024;
   		    int i;
            FILE *ptr;
            unsigned char buffer2[BufferSize];

            ptr = fopen("uniqueIdentifier.bin","rb");
            const size_t fileSize = fread(buffer2, sizeof(unsigned char), BufferSize, ptr);
            printf("Unique ID: ");
            for(i = 0; i < 20; i++){
               printf("%c", (int)buffer2[i]);
               }

            printf("\nSerial Number:");
            for(i = 113; i < 150; i++){
            printf("%c", (int)buffer2[i]);
            }
            printf("\n");

            fclose (ptr);
            
            
			break;
		}

	}

exit:
	fflush(stdout);
	samba_close(fd);
	if (err) {
		fprintf(stderr, "Operation failed\n");
		return -1;
	} else {
        fprintf(stderr, "Operation was Successful\n");
		return 0;
	}
}
