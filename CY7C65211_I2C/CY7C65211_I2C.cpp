// CY7C65211_I2C.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "cmdline.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include "cyusb_i2c.h"

#define I2C_TIMEOUT (1000)
#define MAX_DEVICE_NUM (16)
#define DEFAULT_VID 0x04B4
#define DEFAULT_PID 0x0004
#define DEFAULT_I2C_ADDR 0x3c
#define DEFAULT_I2C_FREQ (100000)
#define QUOTE_STR(x) #x
#define INDENT_STR "        "
#define SHOW_HEX4(str, x) std::cout \
<< str\
<< "0x"\
<< std::setfill('0')\
<< std::setw(4)\
<< std::hex\
<< x\
<< std::endl;
#define SHOW_HEX2(str, x) std::cout \
<< str\
<< "0x"\
<< std::setfill('0')\
<< std::setw(2)\
<< std::hex\
<< x;
#define SHOW_NUM(str, x) std::cout \
<< str\
<< std::dec\
<< x\
<< std::endl;
#define SHOW_STR(str, x) std::cout \
<< str\
<< x\
<< std::endl;

const char cyDeviceTypeStr[6][10] =
{
	"DISABLED",
	"UART",
	"SPI",
	"I2C",
	"JTAG",
	"MFG"
};

const char cyDeviceSerialBlockStr[3][5] =
{
	"SCB0",
	"SCB1",
	"MFG"
};

struct cyusb_t {
	struct param_t {
		CY_VID_PID cyVidPid;
		uint8_t i2c_addr;
		int verbose;
		bool nak;
		bool stop;
		bool clock_stretch;
		bool read;
		int usb_num;
		int freq;
	} params;

	int nr_dev_found, nr_dev_match;

	struct {
		int devnum, ifnum;
	} selected;

	CY_HANDLE cyHandle;
	CY_I2C_CONFIG cyConfig;
	CY_I2C_DATA_CONFIG cyDataConfig;
	CY_RETURN_STATUS cyReturnStatus;
	CY_DEVICE_INFO cyDeviceInfo, cyDeviceInfoList[MAX_DEVICE_NUM];
	uint8_t cyDeviceId[MAX_DEVICE_NUM];
	uint8_t cyNumDevices;
};

int findDevice(cyusb_t& cy)
{
	cy.cyReturnStatus = CyGetDeviceInfoVidPid(cy.params.cyVidPid, cy.cyDeviceId, (PCY_DEVICE_INFO)&cy.cyDeviceInfoList, &cy.cyNumDevices, MAX_DEVICE_NUM);

	int deviceIndex = -1;
	for (int index = 0; index < cy.cyNumDevices; index++) {
		std::cout	
			<< "Number of interfaces: "
			<< std::dec
			<< (int)cy.cyDeviceInfoList[index].numInterfaces 
			<< std::endl;
		SHOW_HEX4("Vid: ", cy.cyDeviceInfoList[index].vidPid.vid);
		SHOW_HEX4("Pid: ", cy.cyDeviceInfoList[index].vidPid.pid);
		SHOW_NUM("Serial num: ", cy.cyDeviceInfoList[index].serialNum);
		SHOW_STR("Manufacturer name: ", cy.cyDeviceInfoList[index].manufacturerName);
		SHOW_STR("Product name: ", cy.cyDeviceInfoList[index].productName);
		SHOW_STR("SCB Number: ", cyDeviceSerialBlockStr[cy.cyDeviceInfoList[index].deviceBlock]);
		SHOW_STR("Device Type: ", cyDeviceTypeStr[cy.cyDeviceInfoList[index].deviceType[0]]);
		SHOW_NUM("Device Class: ", cy.cyDeviceInfoList[index].deviceClass[0]);
		SHOW_STR("", "");

		if (cy.cyDeviceInfoList[index].deviceBlock == SerialBlock_SCB0)
		{
			deviceIndex = index;
		}
	}
	return deviceIndex;
}

void print_params(const cyusb_t & cy)
{
	std::cout << "Params" << std::endl;
	SHOW_NUM(INDENT_STR "verbose: ", (int)cy.params.verbose);
	SHOW_HEX4(INDENT_STR "Pid: ", cy.params.cyVidPid.pid);
	SHOW_HEX4(INDENT_STR "Vid: ", cy.params.cyVidPid.vid);
	SHOW_HEX4(INDENT_STR "i2c addr: ", (int)cy.params.i2c_addr);
	SHOW_NUM(INDENT_STR "frequency: ", (int)cy.params.freq);
	SHOW_NUM(INDENT_STR "nak bit: ", (int)cy.params.nak);
	SHOW_NUM(INDENT_STR "stop bit: ", (int)cy.params.stop);
	SHOW_NUM(INDENT_STR "clock stretch: ", (int)cy.params.clock_stretch);
	SHOW_NUM(INDENT_STR "device num: ", (int)cy.params.usb_num);
	SHOW_STR(INDENT_STR "read/write: ", (cy.params.read? "read":"write"));
	SHOW_STR("", "");
}

void init_params(cyusb_t & cy)
{
	cy.params.cyVidPid.vid = DEFAULT_VID;
	cy.params.cyVidPid.pid = DEFAULT_PID;
	cy.params.verbose = 0;
	cy.params.i2c_addr = DEFAULT_I2C_ADDR;
	cy.params.nak = false;
	cy.params.stop = false;
	cy.params.clock_stretch = false;
	cy.params.usb_num = 1;
	cy.params.read = false;
	cy.params.freq = DEFAULT_I2C_FREQ;
}

void init_cmdline(cmdline::parser & a)
{
	a.add("verbose", 'v', "verbose output");
	a.add<std::string>("addr", 'a', "I2C slave addr", false, QUOTE_STR(0x3c));
	a.add<std::string>("device", 'd', "<vid>:<pid> USB vendor and product ID", false, "0x04B4" ":" "0x0004");
	a.add<int>("freq", 'f', "I2C Speed", false, DEFAULT_I2C_FREQ, cmdline::range(DEFAULT_I2C_FREQ / 2, DEFAULT_I2C_FREQ * 4));
	a.add<int>("unum", 'u', "select <u>th usb device[1..16]", false, 1, cmdline::range(1, 16));
	a.add("clockstretch", 'c', "use clock stretch");
	a.add("nakbit", 'n', "send Nak bit");
	a.add("stopbit", 's', "send Stop bit");
	a.add("help", 'h', "help");
	a.footer("[RW] byte...");
}

int main(int argc, char *argv[])
{
	cyusb_t cy;
	cmdline::parser a;
	std::vector<uint8_t> buff;

	init_params(cy);
	init_cmdline(a);
	bool ok = a.parse(argc, argv);

	if (argc == 5 || a.exist("help")) {
		std::cerr << a.usage();
		return 0;
	}
	
	if (a.exist("addr")) {
		uint8_t addr;
		addr = std::stoi(a.get<std::string>("addr"), 0, 0);
		cy.params.i2c_addr = addr;
	}

	if (a.exist("device")) {
		std::string s;
		std::istringstream ss(a.get<std::string>("device"));
		size_t pos;
		if (std::getline(ss, s, ':'))
		{
			cy.params.cyVidPid.vid = std::stoi(s, &pos, 0);
		}
		if (std::getline(ss, s, ':'))
		{
			cy.params.cyVidPid.pid = std::stoi(s, &pos, 0);
		}
		printf("vid = $%04x, pid = $%04x\n", cy.params.cyVidPid.vid, cy.params.cyVidPid.pid);
	}

	cy.params.nak = a.exist("nakbit");
	cy.params.stop = a.exist("stopbit");
	cy.params.clock_stretch = a.exist("clockstretch");
	if (a.exist("unum")) {
		cy.params.usb_num = a.get<int>("unum");
	}
	if (a.exist("freq"))
	{
		cy.params.freq = a.get<int>("freq");
	}

	if (a.rest().size() > 0) {
		for (auto& val : a.rest()) {
			std::cout << "- " << val << std::endl;
			if (val == "r" || val == "R")
				cy.params.read = true;
			else if (val == "w" || val == "W")
				cy.params.read = false;
			else
				buff.push_back(std::stoi(val, nullptr, 0));
		}
	}
	
	print_params(cy);

	int deviceIndex = findDevice(cy);
	if (deviceIndex >= 0)
	{
		deviceIndex++;
		if (deviceIndex >= cy.params.usb_num)
		{
			CY_RETURN_STATUS retStat;
			retStat = CyOpen(cy.params.usb_num, 0, &cy.cyHandle);
			if (retStat != CY_SUCCESS)
			{
				return -1;
			}
			
			{
				cy.cyConfig.frequency = cy.params.freq;
				cy.cyConfig.isClockStretch = cy.params.clock_stretch;
				cy.cyConfig.isMaster = true;
				cy.cyConfig.slaveAddress = cy.params.i2c_addr;
				retStat = CySetI2cConfig(cy.cyHandle, &cy.cyConfig);
				if (retStat != CY_SUCCESS)
				{
					return -1;
				}
			}

			{
				cy.cyDataConfig.isNakBit = cy.params.nak;
				cy.cyDataConfig.isStopBit = cy.params.stop;
				cy.cyDataConfig.slaveAddress = cy.params.i2c_addr;
			}

			if (cy.params.read)
			{ // read
				uint8_t readBuff[256] = {0};
				CY_DATA_BUFFER db = 
				{
					readBuff,
					sizeof(readBuff),
					0
				};
				db.length = buff[0];
				retStat = CyI2cRead(cy.cyHandle, &cy.cyDataConfig, &db, I2C_TIMEOUT);
				if (retStat != CY_SUCCESS)
				{
					return -1;
				}
				for (uint32_t i = 0; i < db.transferCount; i++) {
					SHOW_HEX2(" ", db.buffer[i]);
				}
				
			}
			else
			{ // write
				CY_DATA_BUFFER db = { 0 };
				db.buffer = &buff[0];
				db.length = buff.size();
				cy.cyDataConfig.isNakBit = false; // ignore
				retStat = CyI2cWrite(cy.cyHandle, &cy.cyDataConfig, &db, I2C_TIMEOUT);
				if (retStat != CY_SUCCESS)
				{
					return -1;
				}
				
			}
			retStat = CyClose(cy.cyHandle);
		}
	}
	return 0;

}

