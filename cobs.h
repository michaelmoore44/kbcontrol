
#ifndef COBS_H
#define COBS_H

/*
   [OHB]                                : Overhead byte (Start of frame)
     3+ -------------->|                : Points to relative location of first zero symbol
                       2+-------->|     : Is a zero data byte, pointing to next zero symbol
                                  [EOP] : Location of end-of-packet zero symbol.
     0     1     2     3     4    5     : Byte Position
     03    11    22    02    33   00    : COBS Data Frame
           11    22    00    33         : Extracted Data
     
OHB = Overhead Byte (Points to next zero symbol)
EOP = End Of Packet
*/

/** COBS encode data to buffer
	@param data Pointer to input data to encode
	@param length Number of bytes to encode
	@param buffer Pointer to encoded output buffer
	@return Encoded buffer length in bytes
	@note Does not output delimiter byte
*/
uint8_t cobsEncode(uint8_t *data, uint8_t length, uint8_t *buffer);

/** COBS decode data from buffer
	@param buffer Pointer to encoded input bytes
	@param length Number of bytes to decode
	@param data Pointer to decoded output data
	@return Number of bytes successfully decoded
	@note Stops decoding if delimiter byte is found
*/
uint8_t cobsDecode(uint8_t* buffer, uint8_t length, uint8_t* data);

#endif //end COBS_H
