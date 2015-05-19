#ifdef LINUX

#include "Debug.h"
#include "EMonCMS.h"

#include <iostream>
#include <fstream>
#include <cstring>

#define TEST(x) if(x()) { \
		passCount++; \
		std::cout << #x ": PASS\n"; \
	} else { \
		std::cout << #x ": FAIL\n"; \
	} \
	total++;

#define TMP_BUFFER_SIZE 128
int globalFakeReading = 234234;
int bufferSize = TMP_BUFFER_SIZE;
unsigned char tmpBuffer[TMP_BUFFER_SIZE];

bool testNodeRegisterRequest() {
	EMonCMS emon(NULL, 0, NULL, NULL, NULL, 2);
	int regSize = 0;
	if((regSize = emon.attrSize(NODE_REGISTER, NULL, 0)) != 4) {
		std::cout << "ERR: Size of register should be 4, is " << regSize << "\n";
		return false;
	}
	unsigned char c[] = { 0x00, 0x00, 0x00, 0x00 };
	unsigned char buffer[regSize];
	emon.attrBuilder(NODE_REGISTER, NULL, 0, buffer);
	if(memcmp(c, buffer, 4) != 0) {
		std::cout << "ERR: wrong packet generated for node register\n";
		return false;
	}
	
	return true;
}

bool testAttributeRegisterRequest() {
	EMonCMS emon(NULL, 0, NULL, NULL, NULL, 2);
	int regSize = 0;
	DataItem regItems[4];
	AttributeIdentifier regIdent;
	regIdent.groupID = 0x6453;
	regIdent.attributeID = 0x2321;
	regIdent.attributeNumber = 0x0321;
	emon.attrIdentAsDataItems(&regIdent, regItems);

	unsigned long fakeDefault = 200200;
	regItems[3].type = ULONG;
	regItems[3].item = &fakeDefault;

	regSize = 0;
	if((regSize = emon.attrSize(ATTR_REGISTER, regItems, 4)) != 25) {
		std::cout << "ERR: wrong packet size for attribute register " << regSize << "\n";
		return false;
	}
	
	unsigned char regBuffer[regSize];
	if(regSize != emon.attrBuilder(ATTR_REGISTER, regItems, 4, regBuffer)) {
		std::cout << "ERR: estimated size does not equal actual size\n";
		return false;
	}

	unsigned char regCmp[] = { 0x15, 0x0, 0x0, 0x5, 0x5, 0x2, 0x0, 0x5, 0x53, 0x64,
					0x5, 0x21, 0x23, 0x5, 0x21, 0x3, 0x9, 0x8, 0xe, 0x3, 0x0, 0x0, 0x0, 0x0, 0x0 };
	if(memcmp(regBuffer, regCmp, regSize) != 0) {
		std::cout << "ERR: attr register output does not match expected output";
		return false;
	}
	
	return true;
}

bool testAttributePostRequest() {
	EMonCMS emon(NULL, 0, NULL, NULL, NULL, 2);
	DataItem postItems[4];
	AttributeIdentifier postIdent;
	postIdent.groupID = 0x6453;
	postIdent.attributeID = 0x2321;
	postIdent.attributeNumber = 0x0321;
	emon.attrIdentAsDataItems(&postIdent, postItems);

	unsigned long fakeValue = 2052066570;
	postItems[3].type = ULONG;
	postItems[3].item = &fakeValue;
	int postSize = 0;
	if((postSize = emon.attrSize(ATTR_POST, postItems, 4)) != 25) {
		std::cout << "ERR: wrong packet size for attribute register " << postSize << "\n";
		return false;
	}

	unsigned char postBuffer[postSize];
	if(postSize != emon.attrBuilder(ATTR_POST, postItems, 4, postBuffer)) {
		std::cout << "ERR: attr post estimated size does not equal actual size expected " << postSize << "\n";
		return false;
	}

	unsigned char postCmp[] = { 0x15, 0x0, 0x0, 0x5, 0x5, 0x2, 0x0, 0x5, 0x53, 0x64, 0x5, 0x21, 0x23, 0x5,
				0x21, 0x3, 0x9, 0xa, 0xd, 0x50, 0x7a, 0x0, 0x0, 0x0, 0x0 };
	if(memcmp(postBuffer, postCmp, postSize) != 0) {
		std::cout << "ERR: attr post output does not match expected output";
		return false;
	}
	
	return true;
}

bool testSetNodeID() {
	EMonCMS emon(NULL, 0, NULL);
	unsigned char setterPacket[] = { 0x00, 0x03, 0x00, 0x01, 0x05, 0x12, 0x53 };
	if(!emon.isEMonCMSPacket('r')) {
		LOG("ERR: Packet not identified as register\n");
		return false;
	}
	
	HeaderInfo *header = (HeaderInfo *)setterPacket;
	DataItem items[header->dataCount];
	if(!emon.parseEMonCMSPacket(header, 'r', &(setterPacket[4]), items)) {
		LOG("ERR: Parse emon cms packet could not parse setting node ID\n");
		return false;
	}
	
	if(emon.getNodeID() != 0x5312) {
		LOG("ERR: Parse emon misinterpreted node id setting\n");
		return false;
	}
	
	return true;
	
}

bool fakeAttributeReader(AttributeIdentifier *attr, DataItem *item) {
	item->type = INT;
	item->item = &globalFakeReading;
	return true;
}

uint16_t fakeNetworkSender(uint8_t type, uint8_t *buffer, uint16_t length) {
	memcpy(tmpBuffer, buffer, length);
	bufferSize = length;
}

bool testAttributePostResponse() {
	/* Build an attribute to be registered */
	AttributeValue attrVal;
	attrVal.attr.groupID = 10;
	attrVal.attr.attributeID = 20;
	attrVal.attr.attributeNumber = 40;
	attrVal.reader = fakeAttributeReader;
	attrVal.registered = false;
	
	EMonCMS emon(&attrVal, 1, fakeNetworkSender, NULL, NULL, 2);
	
	/* Create a fake request for the specified attribute */
	HeaderInfo header;
	header.status = SUCCESS;
	header.dataCount = 4;
	header.dataSize = 12;
	unsigned char testBuffer[12] = { USHORT, 0x00, 0x00, USHORT, 0x0a, 0x00, USHORT, 0x14, 0x00, USHORT, 0x28, 0x00 };
	DataItem items[4]; 
	if(!emon.parseEMonCMSPacket(&header, 'P', testBuffer, items)) {
		LOG("Error parsing request packing\n");
		return false;
	}
	
	unsigned char cmpBuffer[] = { 0x11, 0x0, 0x0, 0x5, 0x5, 0x2, 0x0, 0x5, 0x0, 0x0,
		0x5, 0xa, 0x0, 0x5, 0x14, 0x0, 0x6, 0xfa, 0x92, 0x3, 0x0 };
	if(memcmp(cmpBuffer, tmpBuffer, 21) != 0) {
		LOG("Error: expected output of testing fetch attribute request fails\n");
		return false;
	}
	
	
	return true;
}

bool testBuildAttributeRegister() {
	AttributeValue attrVal;
	attrVal.attr.groupID = 10;
	attrVal.attr.attributeID = 20;
	attrVal.attr.attributeNumber = 40;
	attrVal.reader = fakeAttributeReader;
	attrVal.registered = false;
	
	EMonCMS emon(&attrVal, 1, fakeNetworkSender, NULL, NULL, 2);
	
	DataItem regItems[4];
	emon.attrIdentAsDataItems(&(attrVal.attr), regItems);
	
	fakeAttributeReader(&(attrVal.attr), &(regItems[3]));
	
	emon.attrSender(ATTR_REGISTER, regItems, 4);
	
	unsigned char cmpBuffer[] = { 0x11, 0x0, 0x0, 0x5, 0x5, 0x2, 0x0, 0x5,
		0xa, 0x0, 0x5, 0x14, 0x0, 0x5, 0x28, 0x0, 0x6, 0xfa, 0x92, 0x3, 0x0 };
	
	if(memcmp(tmpBuffer, cmpBuffer, bufferSize) != 0) {
		for(int i = 0; i < bufferSize; i++) {
			printf("0x%x, ", tmpBuffer[i]);
		}
		printf("\n");
		return false;
	}
	
	return true;
}

int main(int argc, char *args[]) {
	int total = 0;
	int passCount = 0;

	TEST(testNodeRegisterRequest);
	TEST(testAttributeRegisterRequest);
	TEST(testAttributePostRequest);
	TEST(testSetNodeID);
	TEST(testAttributePostResponse);
	TEST(testBuildAttributeRegister);
	
	std::cout << passCount << " pass of " << total << "\n";
	
	
	return 0;
}

#endif
