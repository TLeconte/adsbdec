#ifdef WITH_RTL
#define PULSEW 6
#else
#define PULSEW 5
#endif

#define DECOFFSET (240*PULSEW)

extern int deqframe(const float *ampbuff, const int len);
extern void handlerExit(int sig);

