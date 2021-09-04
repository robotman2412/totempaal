
#ifndef MAIN_HPP
#define MAIN_HPP

#include <Arduino.h>
#include <FastLED.h>
#include "patterns.hpp"
#include "WiFi.h"

extern int coin_value[7];

typedef enum coin_type {
    COIN_TYPE_200 = 0,
    COIN_TYPE_100 = 1,
    COIN_TYPE_050 = 2,
    COIN_TYPE_020 = 3,
    COIN_TYPE_010 = 4,
    COIN_TYPE_005 = 5,
    COIN_TYPE_UNKNOWN = 6
} coin_type_t;

struct rain;
typedef struct rain rain_t;
struct rain {
    rain_t *next;
    rain_t *prev;
    int led;
    CRGB color;
    CRGB hiding;
};

#define BRIGHTNESS_PIN 18
#define DEBUG_PIN      5
#define BUTTON_PIN     33
#define COIN_PIN       22

#define STRIP_PIN   12
#define STRIP_TYPE  WS2811
#define STRIP_ORDER GRB
//WS2811

#define LEDS_PER_REV 24
#define CIRCULAR_OFFS 9

#define N_LEDS 458

#define SPARKLE_MAX_ACTIVE (N_LEDS / 2)
#define SPARKLE_INCREMENT 7

#define COIN_200 18
#define COIN_100 15
#define COIN_050 12
#define COIN_020  9
#define COIN_010  6
#define COIN_005  3

#define COIN_END_DELAY 250
#define COIN_DEBOUNCE_DELAY 75

#define SHOW_DEBUG_PATTERN (digitalRead(DEBUG_PIN))

// 25% rain particles at most.
#define MAX_RAIN (N_LEDS / 4)
// 5 particles per iteration at most.
#define MAX_FALL 8

#define PIXELFLUT_PORT 1234
// After 5 minutes, revert to normal.
#define PIXELFLUT_TIME 300000
// Timeout after which the pixelflut connection is broken.
#define PIXELFLUT_TIMEOUT 1500
// Maximum number of pixelflut workers.
#define PIXELFLUT_MAX_WORKERS 10

extern CRGB leds   [N_LEDS];
extern CRGB led_alt[N_LEDS];
extern int  led_ctr[N_LEDS];

// Time for everything inside and called from loop.
extern unsigned long now;
// Fade between two colors.
CRGB fade(int factor, CRGB from, CRGB to);

// Initialise NVS flash and such.
void initNvs();
// Handles all requests of a single client in asynchronous task form.
// Args: Two pointers: Task handle, Client handle.
void handleClientTask(void *args);
// Handles all requests of a single client.
void handleClientLoop(WiFiClient *client);
// Handles a single client synchronously.
void handleClient(WiFiClient *client, unsigned long timeoutTime);

// Button interrupt handler.
void buttonHandler();
// Coin interrupt handler.
void coinHandler();

// Called when a coin has arrived.
void coinHasArrived();
// Count tha money.
void countMoney(coin_type_t coin);
// Shows how much money has been counted.
void showMoney();
// Reset the coin count.
void resetMoney();

#endif //MAIN_HPP
