
#ifndef PATTERNS_HPP
#define PATTERNS_HPP

#include <FastLED.h>
#include <Arduino.h>

typedef void(*funcptr)(void);

#define N_PATTERNS 3
extern funcptr patterns     [N_PATTERNS];
extern funcptr pattern_inits[N_PATTERNS];

// Select random pattern.
void randomPattern();
// Next pattern.
void nextPattern();
// Specific pattern.
void selectPattern(int index);

// Draw the current pattern.
void drawPattern();

// Rainbow pattern (more vertical).
void patt_rainbow_h();
void init_rainbow_h();
// Rainbow pattern (more horizontal).
void patt_rainbow_v();
void init_rainbow_v();
// Sparkly pattern.
void patt_sparkle();
void init_sparkle();

#endif //PATTERNS_HPP
