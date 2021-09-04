
#include "patterns.hpp"
#include "main.hpp"

static int current_patt = 0;

funcptr patterns[N_PATTERNS] = {
	patt_rainbow_h,
	patt_rainbow_v,
	patt_sparkle
};

funcptr pattern_inits[N_PATTERNS] = {
	init_rainbow_h,
	init_rainbow_v,
	init_sparkle
};

// Select random pattern.
void randomPattern() {
	selectPattern(random(N_PATTERNS));
}

// Next pattern.
void nextPattern() {
	selectPattern(current_patt + 1);
}

// Specific pattern.
void selectPattern(int index) {
	index %= N_PATTERNS;
	(pattern_inits[index])();
	current_patt = index;
}

// Draw the current pattern.
void drawPattern() {
	(patterns[current_patt])();
}

static int rainbow_fade = 0;
// Rainbow pattern (more vertical).
void patt_rainbow_h() {
	// Pattern: rainbows (horizontal).
	for (int i = 0; i < N_LEDS; i++) {
		leds[i] = CHSV(i * 255 / LEDS_PER_REV + now / 10, 255, 255);
	}
	if (rainbow_fade != 256) {
		// Merge with old pattern.
		for (int i = 0; i < N_LEDS; i++) {
			leds[i] = fade(rainbow_fade, led_alt[i], leds[i]);
		}
		rainbow_fade += 16;
	}
}

void init_rainbow_h() {
	rainbow_fade = 0;
	memcpy(led_alt, leds, sizeof(leds));
}

// Rainbow pattern (more horizontal).
void patt_rainbow_v() {
	// Pattern: rainbows (vertical).
	for (int i = 0; i < N_LEDS; i++) {
		leds[i] = CHSV(i * 512 / N_LEDS + now / 10, 255, 255);
	}
	if (rainbow_fade != 256) {
		// Merge with old pattern.
		for (int i = 0; i < N_LEDS; i++) {
			leds[i] = fade(rainbow_fade, led_alt[i], leds[i]);
		}
		rainbow_fade += 16;
	}
}

void init_rainbow_v() {
	rainbow_fade = 0;
	memcpy(led_alt, leds, sizeof(leds));
}

static int sparkle_n_active = 0;
// Sparkly pattern.
void patt_sparkle() {
	// Pattern: sparkle.
	for (int i = 0; i < 20; i++) if (random(SPARKLE_MAX_ACTIVE) >= sparkle_n_active) {
		// Add a random LED.
		int led = random(N_LEDS);
		if (!led_ctr[led]) {
			// If it's not already active.
			if (leds[led].r | leds[led].g | leds[led].b) {
				// Turn off remnant of other pattern.
				led_ctr[led] = 256;
				led_alt[led] = leds[led];
			} else {
				// Turn on new LED.
				led_ctr[led] = 1;
				led_alt[led] = CHSV(led * 7 + now / 20, 255, 255);
			}
			sparkle_n_active ++;
		}
	}
	for (int i = 0; i < N_LEDS; i++) {
		if (led_ctr[i]) {
			// Fade colors.
			int fac = led_ctr[i];
			if (led_ctr[i] > 255) {
				fac = 512 - led_ctr[i];
			}
			leds[i] = fade(fac, CRGB(0), led_alt[i]);
			// Up the factor.
			led_ctr[i] += SPARKLE_INCREMENT;
			if (led_ctr[i] > 512) {
				// Finish when the thingy ends.
				sparkle_n_active --;
				led_ctr[i] = 0;
			}
		}
	}
}

void init_sparkle() {
	sparkle_n_active = 0;
	for (int i = 0; i < N_LEDS; i++) {
		led_ctr[i] = 0;
	}
}
