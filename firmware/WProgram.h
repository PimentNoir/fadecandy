#ifndef WProgram_h
#define WProgram_h

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "wiring.h"

#define DMAMEM __attribute__ ((section(".dmabuffers"), used))

// Enforce inlining, so we can take advantage of inter-procedural optimization
#define ALWAYS_INLINE __attribute__ ((always_inline))

#ifdef __cplusplus

uint16_t makeWord(uint16_t w);
uint16_t makeWord(byte h, byte l);

#define word(...) makeWord(__VA_ARGS__)

unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout = 1000000L);

void tone(uint8_t pin, uint16_t frequency, uint32_t duration = 0);
void noTone(uint8_t pin);

// WMath prototypes
uint32_t random(void);
uint32_t random(uint32_t howbig);
int32_t random(int32_t howsmall, int32_t howbig);
void randomSeed(uint32_t newseed);
void srandom(uint32_t newseed);
long map(long, long, long, long, long);

#include "pins_arduino.h"

#endif // __cplusplus

#endif // WProgram_h
