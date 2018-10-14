extern uint32_t Checksum(const uint8_t *message, const int n);
extern uint32_t testFix(uint8_t *message, const uint32_t ecrc);
extern uint32_t testFix11(uint8_t *message, const uint32_t ecrc);
extern void fixChecksum(unsigned char *message, const unsigned int nb);

