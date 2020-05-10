#include <stdio.h>
#include <stdlib.h>

#include "misc.h"
#include "init.h"

#include <libusb.h>

#define DEV_VID 0x1c7a
#define DEV_PID 0x0570
#define DEV_CONF 0x1
#define DEV_INTF 0x0

#define DEV_EPOUT 0x04
#define DEV_EPIN 0x83

int _num = 0;

void printData(unsigned char * data, int length)
{
	for (int i = 0; i < length; ++i)
		printf("%#04x ", data[i]);
	printf("\n");
}

void finger_status(unsigned char * data)
{

}

void writeRaw(const char * filename, unsigned char * data, int length)
{
	FILE * fp = fopen(filename, "w");
	if (fp == NULL)
	{
		perror("Error opening file");
		return;
	}
	fwrite(data, sizeof(unsigned char), length, fp);

	fclose(fp);
}

void writeImg(const char * filename, unsigned char * data, int width, int height)
{
	const char format_mark[] = "P5";

	FILE * fp = fopen(filename, "w");
	if (fp == NULL)
	{
		perror("Error opening file");
		return;
	}

	fprintf(fp, "%s\n%d %d\n%d\n", format_mark, width, height, 255);

	int i = 0;
	for (int x = 0; x < height; ++x)
	{
		for (int y = 0; y < width; ++y, ++i)
		{
			fputc(data[i], fp);
		}
	}

	fclose(fp);
}

void imgInfo(unsigned char * data, int length)
{
	int res = 0;
	int min = data[0], max = data[0];
	for (int i = 0; i < length; ++i)
	{
		res += data[i];
		if (data[i] < min)
			min = data[i];
		if (data[i] > max)
			max = data[i];
	}
	printf("Image intensity stats: min: %d | max: %d | avg: %d\n", min, max, res / length);
}

void printDataRange(unsigned char * data, int length, int beginByte, int endByte)
{
	for (int i = beginByte; i < endByte; i++)
	{
		printf("%#04x ", data[i]);
	}
	printf("\n");
}

int main(int argc, char * argv[])
{
	libusb_context * cntx = NULL;
	if (libusb_init(&cntx) != 0)
		perror_exit("Could not init context");

	//libusb_set_debug(cntx, LIBUSB_LOG_LEVEL_DEBUG);

	libusb_device_handle * handle = libusb_open_device_with_vid_pid(cntx, DEV_VID, DEV_PID);
	if (handle == NULL)
		perror_exit("Device not opened");

	if (libusb_kernel_driver_active(handle, DEV_INTF) == 1)
		libusb_detach_kernel_driver(handle, DEV_INTF);

	if (libusb_set_configuration(handle, DEV_CONF) != 0)
		perror_exit("Could not set configuration");

	if (libusb_claim_interface(handle, DEV_INTF) != 0)
		perror_exit("Could not claim device interface");

	libusb_reset_device(handle);

	// TRANSFER

	int initLen = sizeof(init) / sizeof(init[0]);
	int initPktSize = sizeof(init[0]);
	unsigned char data[32512];
	int length = sizeof(data);
	int transferred;
	printf("Init packet length: %d | Init packets num: %d | Answer data length: %d\n", initPktSize, initLen, length);

	for (int i = 0; i < initLen; ++i)
	{
		printf("Step %d: ", i+1);
		if (libusb_bulk_transfer(handle, DEV_EPOUT, init[i], initPktSize, &transferred, 0) != 0)
		{
			printf("write %d bytes\n", transferred);
			perror_exit("'Out' transfer error");
		}

		printf("write %d bytes, ", transferred);

		if (libusb_bulk_transfer(handle, DEV_EPIN, data, length, &transferred, 0) != 0)
		{
			printf("read %d bytes\n", transferred);
			perror_exit("'In' transfer error");
		}
		printf("read %d bytes\n", transferred);
	}

	writeRaw("scans/raw.bin", data, transferred);

	// The data can be separated to 5 images of size 114*57
	writeImg("scans/114x285.pgm", data, 114, 285);
	writeImg("scans/114x57.pgm", data, 114, 57);

	imgInfo(data, 114*57);

	// 32512 - 114*285 = 22: Let's try to figure out is there any meaning in the rest of bytes. Seems that there is no regularity.
	printDataRange(data, transferred, transferred-22, transferred);
	printDataRange(data, transferred, 0, 22);

	libusb_release_interface(handle, DEV_INTF);
	libusb_attach_kernel_driver(handle, DEV_INTF);
	libusb_close(handle);
	libusb_exit(cntx);
	return 0;
}
