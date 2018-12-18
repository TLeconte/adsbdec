extern uint32_t CrcShort(const uint8_t *frame);
extern uint32_t CrcLong(const uint8_t *frame,uint32_t crcs);
extern uint32_t testFix(uint8_t *message, const uint32_t ecrc);
extern uint32_t testFix11(uint8_t *message, const uint32_t ecrc);
extern void fixChecksum(unsigned char *message, const unsigned int nb);

#define CrcEnd(frame,crc,n) ((crc) ^ (((uint32_t)frame[(n)-3]<<16)|((uint32_t)frame[(n)-2]<<8)|(uint32_t)frame[(n)-1]))

