#include "hidapi.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#define MAXSCANLOG 100

typedef struct {
	int tag;
} SCANLOG;

typedef struct {
	unsigned char junk[7];
	unsigned char mfg[2];
	unsigned char junk1;
	unsigned char tag[2];
} EPC;

typedef struct {
	unsigned char reportID;
	unsigned char frameLength;
	unsigned char count;
	unsigned char RSSI;
	unsigned char freq[3];
	unsigned char EPCLength;
	unsigned char Reserved[2];
	EPC epc;
} RFID;

bool getCount(unsigned char *buf) {
	RFID *rfid = (RFID *) buf;
	if (rfid->count == 0)
		return false;
	return true;
}

void printbuf(unsigned char *buf, int length, int scanCount) {
	RFID *rfid = (RFID *) buf;
	if (rfid->count == 0)
		return;
	int mfg = ((int) rfid->epc.mfg[0]) << 8 | (int) rfid->epc.mfg[1];
	int tag = ((int) rfid->epc.tag[0]) << 8 | (int) rfid->epc.tag[1];
	fprintf(stdout, "%04d%04d\n", mfg, tag);
	fflush(stdout);
}

int main(int argc, char **argv) {
	int scanCount = 0;
	
	// 867500 = 0xD3CAC
	static unsigned char cmdBufferChangeFreq1[] = { 0x41, 0x08, 0x08, 0xac, 0x3c, 0x0d, 0xd8, 0x01 };
	// 902750 = 0xDC65E
	// Gotta keep it 'murican
	static unsigned char cmdBufferChangeFreq2[] = { 0x41, 0x08, 0x08, 0x5e, 0xc6, 0x0d, 0xd8, 0x01 };
	static unsigned char cmdBufferReadTags[] = { 0x43, 0x03, 0x01 };

	hid_device *connected_device = NULL;

	connected_device =  hid_open(0x1325, 0xc029,(wchar_t *) 0);

	if (!connected_device) {
		printf("-3");
		return -3;
	}

	unsigned char buf[65];
	for (int j = 0; j < 64; j++)
		buf[j] = 0;

	if (argc > 1) {
		switch(atoi(argv[1]))
		{
			case 1:
				memcpy(buf, cmdBufferChangeFreq1, sizeof(cmdBufferChangeFreq1));
				break;
			case 2:
				memcpy(buf, cmdBufferChangeFreq2, sizeof(cmdBufferChangeFreq2));
				break;
			default:
				break;
		}

		int res = hid_send_feature_report(connected_device,(unsigned char *)buf,64);
		if (res < 0) {
			printf ("-2");
			return -2;
		}
	}

	for (int l = 0; l < 64; l++)
			buf[l] = 0;

	memcpy(buf,cmdBufferReadTags,sizeof(cmdBufferReadTags));
	
	while (true) {

		int res = hid_send_feature_report(connected_device,(unsigned char *)buf,64);
		if (res < 0) {
			printf ("-3");
			return -3;
		}
		unsigned char buf1[65];

		do
		{
			for (int j = 0; j < 64; j++)
				buf1[j] = 0;

			res = hid_read(connected_device, buf1, 64);

			if (res < 0) {
				printf ("-3");
				return -3;
			}

			if(!getCount(buf1)) {
				scanCount++;
				printbuf(buf1,64,scanCount);
			} else {
				printbuf(buf1,64,scanCount);
			}

		} while ((int)buf1[2] > 1);
	}
 	hid_close(connected_device);

	return 0;
}
