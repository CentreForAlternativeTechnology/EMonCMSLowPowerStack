#ifndef __EMONCMS_H__
#define __EMONCMS_H__

#ifdef LINUX
#include <time.h>
#include <cstring>
#include <stdint.h>
#else
#include "Arduino.h"
#endif

#define REGISTERREQUESTTIMEOUT 5000

/**
 * The is an enum to specify data formats to send over the
 * low power radio
 **/
enum dataTypes {
	STRING = 1,
	CHAR = 2,
	UCHAR = 3,
	SHORT = 4,
	USHORT = 5,
	INT = 6,
	UINT = 7,
	LONG = 8,
	ULONG = 9,
	FLOAT = 10
};

/**
 * Status codes from the OEMan Communications specification
 **/
enum Status {
	SUCCESS = 0x00,
	FAILURE = 0x01,
	UNSUPPORTED_ATTRIBUTE = 0x86,
	INVALID_VALUE = 0x87
};

/**
 * Radio packet types for request types
 **/
enum RequestType {
	NODE_REGISTER = 'R',
	ATTR_REGISTER = 'A',
	ATTR_POST = 'P',
	ATTR_POST_RESPONSE = 'p',
	ATTR_FAILURE
};

/**
 * Initial header of OEMan Low-power Radio spec packet
 **/
typedef struct {
	uint16_t dataSize; /** Size of the data items section in bytes **/
	uint8_t status; /** The status, SUCCESS in requests **/
	uint8_t dataCount; /** Number of data items **/
} HeaderInfo;

/**
 * A data item, consisting of a type and a pointer to data
 **/
typedef struct {
	uint8_t type;
	void *item;
} DataItem;

/**
 * Attributes are identified by the 3 values in this structure.
 **/
typedef struct {
	uint16_t groupID;
	uint16_t attributeID;
	uint16_t attributeNumber;
} AttributeIdentifier;

/**
 * NetworkSender implemented by the program to send data packets to the gateway
 * @param type Packet type
 * @param buffer data to send
 * @param length length of buffer
 * @return the length of the buffer on success
 **/
typedef uint16_t (*NetworkSender)(uint8_t type, uint8_t *buffer, uint16_t length);

/**
 * Function implemented by host program to retrieve the value of a piece of data.
 * It is expected that the pointer to the data set in the dataitem is a global
 * variable or static.
 * @param attr the identifier for the requested attribute reading
 * @param item the item that the implementation is expected to fill
 * @return 0 on success
 **/
typedef bool (*AttributeReader)(AttributeIdentifier *attr, DataItem *item);

/**
 * User implemented event which is triggered when the node ID is registered
 **/
typedef void (*NodeIDRegistered)(uint16_t emonNodeID);

/**
 * User implemented event which is triggered when a attribute is successfully registered.
 * @param attr the attribute identifier for the registered attribute
 **/
typedef void (*AttributeRegistered)(AttributeIdentifier *attr);

/**
 * Contains the information necessary to identifier, register and read
 * an attribute value.
 **/
typedef struct {
	AttributeIdentifier attr; /** The attribute identifier **/
	AttributeReader reader; /** the function to read the value for this attribute **/
	bool registered; /** user should set to false on creation **/
} AttributeValue;

class EMonCMS {
	public:
		/**
		 * @param values list of attributes which can be read from this node
		 * @param length length of attribute list
		 * @param sender NetworkSender for sending responses to incoming requests
		 * @param attrRegistered attribute registered callback
		 * @param node registered callback
		 * @param nodeID defaults to 0, node id for emoncms
		 **/
		EMonCMS(AttributeValue values[],
			int16_t length, 
			NetworkSender sender,
			AttributeRegistered attrRegistered = NULL,
			NodeIDRegistered nodeRegistered = NULL,
			uint16_t nodeID = 0
			);
		~EMonCMS();
		/* methods for receiving packets */
		/** Checks the incoming type to see if it is an accepted type
         * @param type type of incoming packet to check
         * @return true if it is an emon cms packet type
         **/
		bool isEMonCMSPacket(uint8_t type);
		/**
		 * Parses an incoming emon cms packet 
		 * @param header incomiing emon cms header
		 * @param type the type of the incoming packet
		 * @param buffer the raw unparsed data items
		 * @param items a list of data items the size of count in the header
		 * @return returns true if the function succeeded
		 **/
		bool parseEMonCMSPacket(HeaderInfo *header, uint8_t type, uint8_t *buffer, DataItem items[]);
		/* methods for sending packets */
		/**
		 * Calculates the buffer size for the buffer passed to attrBuilder
		 * @param type type of request to be sent
		 * @param item list of data items
		 * @param length length of list of data items
		 * @return the size of the buffer
		 **/
		uint16_t attrSize(RequestType type, DataItem *item, uint16_t length);
		/**
		 * Creates a packet into the given buffer containing the given data items.
		 * Items for each request type:
		 * 	NODE_REGISTER: None
		 * 	ATTR_REGISTER: Group ID, Attribute ID, Attribute Number, Attribure default
		 * 	ATTR_POST: Group ID, Attribute ID, Attribute Number, Attribute Value
		 * 	ATTR_FAILURE: Group ID, Attribute ID, Attribute Number
		 * @param type the type of the request to send
		 * @param items list of data items to send
		 * @param length length of list of data items to send
		 * @param buffer buffer to write packet into, including header
		 * @return the size of the buffer on success
		 **/
		uint16_t attrBuilder(RequestType type, DataItem *items, uint16_t length, uint8_t *buffer);
		/**
		 * Wraps attrBuilder and sends requests through the NetworkSender
		 * @param type type of request to send
		 * @param items list of items to attach
		 * @param length length of list of items to attach
		 * @return the size of the sent data on success
		 **/
		uint16_t attrSender(RequestType type, DataItem *items, uint16_t length);
		/**
		 * Converts and AttributeIdentifier to a list of DataItems.
		 * @param ident the incoming Attribute Identifier
		 * @param attrItems an array of DataItems of length 3
		 **/
		void attrIdentAsDataItems(AttributeIdentifier *ident, DataItem *attrItems);
		
		/* getters and setters */
		/**
		 * Returns the assigned node ID
		 * @return the node ID form emon cms
		 **/
		uint16_t getNodeID();
		/**
		 * Gets the data about an attribute including registration status
		 * and the function to get it from an Attribute Identifier.
		 * @param attr Attribute Identifier to get the data for.
		 * @return NULL on not found, otherwise Attribute Value struct.
		 **/
		AttributeValue *getAttribute(AttributeIdentifier *attr);
		/**
		 * called periodically to ensure all attributes are registered and
		 * the node ID is too
		 **/
		void registerNode();
		/**
		 * Compares 2 attribute identifiers
		 * @param a first attribute to compare
		 * @param b second attribute to compare
		 * @return 0 if they are the same
		 */
		int16_t compareAttribute(AttributeIdentifier *a, AttributeIdentifier *b);
		/**
		 * Reads an attribute value using it's reader and posts it.
		 * @param ident identifier of attribute to post
		 * @return the size of data sent on success
		 */
		uint16_t postAttribute(AttributeIdentifier *ident);
	protected:
		uint16_t nodeID; /** the EMonCMS node ID **/
		AttributeValue *attrValues; /** list of registered attributes on this node **/
		uint16_t attrValuesLength; /** length of list of registered attributes on this node **/
		uint32_t lastRegisterRequest; /** time of last sent register request **/
		NetworkSender networkSender; /** function to send data to the radios **/
		AttributeRegistered attrRegistered; /** attribute registered callback **/
		NodeIDRegistered nodeRegistered; /** node registered callback **/

		/**
		 * Gets the size of the item in a DataItem
		 * @param type the type of item
		 * @return the size of the given type
		 **/
		uint16_t getTypeSize(uint8_t type);
		/**
		 * Transfers a data item into a char array
		 * @param item item to put in char array
		 * @param buffer buffer to transfer to
		 * @return size of transferred item on success
		 **/
		uint16_t dataItemToBuffer(DataItem *item, uint8_t *buffer);
		/**
		 * Function to respond to a request for an attribute.
		 * Sends through the NetworkSender specified in constructor.
		 * @param header header of incoming request
		 * @param items item list containing attribute identifier
		 * @return true if building and sending succeeded
		 **/
		bool requestAttribute(HeaderInfo *header, DataItem items[]);
		
		#ifdef LINUX
		time_t start_time;
		unsigned long millis();
		#endif
};

#endif
