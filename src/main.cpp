
#include "main.hpp"
#include <Arduino.h>
#include <FastLED.h>
#include <nvs_flash.h>
#include <WiFi.h>

#define WIFI_SSID "MCH2022-fieldday-insecure"
#define WIFI_USER "hacker"
#define WIFI_PASS "hacker"

#define BUTTON_THRESH 200

uint8_t packetBuffer[2048] = {0};

// Pattern variables.
CRGB leds   [N_LEDS];
CRGB led_alt[N_LEDS];
int  led_ctr[N_LEDS];

int n_rain = 0;
rain_t *rain = NULL;

unsigned long lastLedTime = 0;
unsigned long lastRainTime = 0;
unsigned long lastPixelflutTime = 0;
unsigned long lastCoinTime = 0;
int coinPulses = 0;

int coin_value[7] = {
	200,
	100,
	50,
	20,
	10,
	5,
	0
};

// How much MONEY has been added as of yet.
int n_coin[7];
int n_money;
int n_money_dropped = 0;

nvs_handle handle;

WiFiUDP Udp;
WiFiServer server;
bool isPixelflutMode = 0;
static int n_pixelflut_workers = 0;

void setup() {
	Serial.begin(115200);
	// Pin settings.
	pinMode(BRIGHTNESS_PIN, INPUT);
	pinMode(COIN_PIN,       INPUT_PULLUP);
	//pinMode(BUTTON_PIN,     INPUT_PULLUP);
	pinMode(DEBUG_PIN,      INPUT_PULLDOWN);
	// Reset some arrays.
	for (int i = 0; i < N_LEDS; i++) {
		leds[i] = CRGB(0, 0, 0);
		led_alt[i] = CRGB(0, 0, 0);
		led_ctr[i] = 0;
	}
	//randomPattern();
	// Add this to the fastest LED.
	FastLED.addLeds<STRIP_TYPE,STRIP_PIN,STRIP_ORDER>(leds, N_LEDS).setCorrection(TypicalLEDStrip);
	FastLED.setBrightness(64);
	// Read coin value bits.
	initNvs();
	// Pixelfluts.
	WiFi.begin(WIFI_SSID);
	server.setNoDelay(1);
	server.begin(1234);

	Udp.begin(1234);

	// Interrupt handlers.
	attachInterrupt(digitalPinToInterrupt(COIN_PIN),   coinHandler,   FALLING);
	//attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonHandler, CHANGE);

	while (!WiFi.isConnected()) {

	}
	IPAddress ip = WiFi.localIP();
	printf("IP: %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);
}

// Time for everything inside and called from loop.
unsigned long now;

static bool reset_coin_prompted;

//static bool lastButtonTime;
static bool lastButtonLevel;

void loop() {
	// Do the button.
	bool buttonLevel = analogReadMilliVolts(BUTTON_PIN) <= BUTTON_THRESH;
	if (buttonLevel && !lastButtonLevel) {
		isPixelflutMode = 0;
		nextPattern();
	}
	lastButtonLevel = buttonLevel;

	if (Serial.available()) {
		char c = Serial.read();
		if (reset_coin_prompted && (c == 'Y' || c == 'y')) {
			resetMoney();
			printf("Coin count reset.\r\n");
			reset_coin_prompted = false;
		} else if (reset_coin_prompted) {
			printf("Coin count kept.\r\n");
			reset_coin_prompted = false;
		} else if (c == 'R') {
			printf("Reset coin count? (Y/N)\r\n");
			reset_coin_prompted = true;
		} else if (c == 'c') {
			showMoney();
		}
	}

	if (server.hasClient() && n_pixelflut_workers < PIXELFLUT_MAX_WORKERS) {
		// Accept client.
		//WiFiClient *client = new WiFiClient(server.accept());
		// Start task.
		//handleClientLoop(client);
		TaskHandle_t handle;
		//printf("Starting worker: %p, %p\r\n", client, &handle);
		xTaskCreate((TaskFunction_t) handleClientTask, "pixelflut_worker",
					4096, NULL, tskIDLE_PRIORITY, &handle);
	}

	if (Udp.parsePacket()) {
		int len = Udp.read(packetBuffer, 2047);
		packetBuffer[len] = 0;
		handle_input((const char*) packetBuffer, NULL);
	}

	now = millis();
	if (lastCoinTime && now > lastCoinTime + COIN_END_DELAY) {
		coinHasArrived();
		nextPattern();
	}
	
	if (isPixelflutMode && now > lastPixelflutTime + PIXELFLUT_TIME) {
		isPixelflutMode = false;
	}

	if (now > lastRainTime + 150) {
		FastLED.setBrightness(digitalRead(BRIGHTNESS_PIN) ? 255 : 64);
		lastRainTime = now;
		// Time to let the rain fall.
		rain_t *current = rain;
		while (current) {
			current->led -= LEDS_PER_REV;
			if (current->led < 0) {
				// Remove from list.
				if (current->prev) current->prev->next = current->next;
				else rain = current->next;
				if (current->next) current->next->prev = current->prev;
				// Next element and free memory.
				void *mem = current;
				current = current->next;
				free(mem);
				n_rain --;
			} else {
				// Next element.
				current = current->next;
			}
		}
		// Add another raindrop or something.
		int max = min(random(MAX_FALL) + 1, (n_money - n_money_dropped) / 5 + 1L);
		int offs = random(LEDS_PER_REV / max);
		for (int i = 0; i < max && n_rain < MAX_RAIN && n_money_dropped < n_money; i++) {
			n_money_dropped ++;
			rain_t *add = (rain_t *) malloc(sizeof(rain_t));
			*add = (rain_t) {
				.next  = rain,
				.prev  = NULL,
				.led   = N_LEDS - i * LEDS_PER_REV / max - offs,
				.color = CRGB(255, 192, 0)
			};
			if (add->next) add->next->prev = add;
			rain = add;
			n_rain ++;
		}
	}

	if (now > lastLedTime + 10) {
		lastLedTime = now;
		if (!SHOW_DEBUG_PATTERN && !isPixelflutMode) {
			// Pattern.
			drawPattern();
		} else if (SHOW_DEBUG_PATTERN) {
			// Debug pattern.
			for (int i = 0; i < N_LEDS; i ++) {
				leds[i] = CRGB(0, 0, 0);
			}
			for (int i = 0; i < N_LEDS; i += LEDS_PER_REV / 3) {
				leds[i] = CRGB(255, 0, 0);
			}
		}
		// Show rain.
		rain_t *current = rain;
		while (current) {
			current->hiding = leds[current->led];
			leds[current->led] = current->color;
			current = current->next;
		}
		FastLED.show();
		// Hide rain.
		current = rain;
		while (current) {
			leds[current->led] = current->hiding;
			current = current->next;
		}
	}
}

CRGB fade(int factor, CRGB from, CRGB to) {
	float f = factor / 255.0;
	return CRGB(
		from.r + (to.r - from.r) * f,
		from.g + (to.g - from.g) * f,
		from.b + (to.b - from.b) * f
	);
}

void initNvs() {
	// Initialize NVS.
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		// NVS partition was truncated and needs to be erased.
		// Retry nvs_flash_init.
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	// Open some storage.
	err = nvs_open("storage", NVS_READWRITE, &handle);
	if (err != ESP_OK) {
		printf("Error opening NVS flash: %s (%d)\r\n", esp_err_to_name(err), err);
		n_money = 0;
		for (int i = 0; i < 7; i ++) {
			n_coin[i] = 0;
		}
	} else {
		n_money = 0;
		char buf[6] = "coin0";
		for (int i = 0; i < 7; i++) {
			// Try to read coin values from NVS flash.
			buf[4] = '0' + i;
			// Default to 0 for uninitialised value.
			n_coin[i] = 0;
			err = nvs_get_i32(handle, buf, n_coin + i);
			n_money += n_coin[i] * coin_value[i];
		}
	}
	n_money_dropped = n_money;
}

// Handles all requests of a single client in asynchronous task form.
// Args: Two pointers: Task handle, Client handle.
void handleClientTask(void *args) {
	static int next_id = 0;
	n_pixelflut_workers ++;
	// Clean up the args we don't need anymore.
	free(args);
	// Run the wrapped function.
	int id = next_id ++;
	printf("PixelFlut worker %04x started.\r\n", id);
	WiFiClient client = server.accept();
	if (client) handleClientLoop(&client);
	printf("PixelFlut worker %04x stopped.\r\n", id);
	n_pixelflut_workers --;
	// Delete this task.
	vTaskDelete(NULL);
}

// Handles all requests of a single client.
void handleClientLoop(WiFiClient *client) {
	unsigned long now = millis();
	unsigned long timeout = now + PIXELFLUT_TIMEOUT;
	lastPixelflutTime = now;
	client->setTimeout(2);
	while (millis() < timeout && client->connected() || client->available()) {
		handleClient(client, timeout);
		lastPixelflutTime = millis();
		// Give up some CPU time.
		taskYIELD();
	}
	lastPixelflutTime = millis();
	client->flush();
	client->stop();
	//puts("done");
}

// Decodes pixelflut.
void handle_input(const char *str, WiFiClient *client) {
	const char *ptr = str;
	//puts(str);
	int split = strchr(str, ' ') - str;
	if (split < 0) {
		split = strlen(str);
	}
	if (!strncasecmp(str, "help", min(4, split))) {
		// Show help.
		if (client) client->write("HELP\r\n");
		//printf("help\r\n");
	} else if (!strncasecmp(str, "size", min(4, split))) {
		// Return size.
		char buf[64];
		sprintf(buf, "SIZE %d 1\r\n", N_LEDS);
		if (client) client->write(buf);
		//printf("size\r\n");
	} else if (!strncasecmp(str, "px", min(2, split))) {
		if (*(str + split) == 0) return;//{printf("px abort 0\r\n"); return;}
		// Pixel action.
		ptr = str + split + 1;
		int split = strchr(ptr, ' ') - ptr;
		//puts(ptr);
		if (split <= 0) return;//{printf("px abort 1\r\n"); return;}
		// Get X.
		char *tmp = strndup(ptr, split);
		int x = atoi(tmp);
		free(tmp);
		// Split more.
		ptr = ptr + split + 1;
		//puts(ptr);
		split = strchr(ptr, ' ') - ptr;
		bool do_read = false;
		if (split <= 0) {
			do_read = true;
			split = strlen(ptr);
		}
		// Get Y.
		tmp = strndup(ptr, split);
		int y = atoi(tmp);
		free(tmp);
		// Split more.
		ptr = ptr + split + 1;
		//puts(ptr);
		if (do_read) {
			// We are reading.
			char buf[64];
			int col = 0;
			if (x < N_LEDS && x >= 0 && y == 0) {
				CRGB led = leds[x];
				col = (led.r << 16) | (led.g << 8) | led.b;
			}
			if (client) sprintf(buf, "PX %d %d %06x\r\n", x, y, col);
			client->write(buf);
			//printf("read\r\n");
		} else {
			// We are writing.
			int col = strtol(ptr, NULL, 16);
			if (x < N_LEDS && x >= 0 && y == 0) {
				leds[x] = CRGB(col);
			}
			isPixelflutMode = 1;
			//printf("write\r\n");
		}
	} else {
		//printf("dunno\r\n");
	}
}

// Handles a single client synchronously.
void handleClient(WiFiClient *client, unsigned long timeoutTime) {
	unsigned long start = millis();
	String raw = "";
	while (1) {
		unsigned long now = millis();
		if (client->available()) {
			char c = client->read();
			if (c == '\r' || c == '\n' && raw.length() == 0) {
				// Ignore.
			} else if (c == '\r' || c == '\n' || now > start + 500) {
				// End.
				break;
			} else {
				// Append.
				raw += c;
			}
		} else {
			// Give up some CPU time.
			taskYIELD();
		}
		if (now > timeoutTime) return;
	}
	// Strip '\r' if any.
	if (raw.endsWith("\r")) raw = raw.substring(0, raw.length() - 1);
	const char *str = raw.c_str();
	handle_input(str, client);
}

// Button interrupt handler. Currently unused.
void buttonHandler() {
	static unsigned long lastButtonTime = 0;
	static bool lastState = false;
	// Simple debounce.
	unsigned long now = millis();
	if (now < lastButtonTime + 50) return;
	lastButtonTime = now;

	bool state = digitalRead(BUTTON_PIN);
	if (!state && lastState) {
		// Button pressed just now.
		nextPattern();
	}

	lastState = state;
}

// Coint interrupt handler.
void coinHandler() {
	unsigned long now = millis();
	if (lastCoinTime && now < lastCoinTime + COIN_DEBOUNCE_DELAY) return;
	coinPulses ++;
	lastCoinTime = now;
}

// Called when a coin has arrived.
void coinHasArrived() {
	coin_type_t type = COIN_TYPE_UNKNOWN;
	switch (coinPulses) {
		case COIN_200:
			type = COIN_TYPE_200;
			break;
		case COIN_100:
			type = COIN_TYPE_100;
			break;
		case COIN_050:
			type = COIN_TYPE_050;
			break;
		case COIN_020:
			type = COIN_TYPE_020;
			break;
		case COIN_010:
			type = COIN_TYPE_010;
			break;
		case COIN_005:
			type = COIN_TYPE_005;
			break;
	}
	int value = coin_value[type];
	printf("€%d,%02d coin inserted: %d\r\n", value/100, value%100, coinPulses);
	coinPulses = 0;
	lastCoinTime = 0;
	countMoney(type);
}

// Count tha money.
void countMoney(coin_type_t type) {
	// Increment number.
	n_coin[type] ++;
	n_money += coin_value[type];
	// Write NVS flash.
	char buf[6] = "coin0";
	buf[4] = '0' + type;
	nvs_set_i32(handle, buf, n_coin[type]);
	showMoney();
}

// Shows how much money has been counted.
void showMoney() {
	// Show count.
	for (int i = 0; i < 7; i++) {
		int value = coin_value[i];
		printf("€%d,%02d: %dx   ", value/100, value%100, n_coin[i]);
	}
	printf("total €%d,%d\r\n", n_money/100, n_money%100);
}

// Reset the coin count.
void resetMoney() {
	char buf[6] = "coin0";
	for (int i = 0; i < 7; i++) {
		// Reset count.
		n_coin[i] = 0;
		// Reset NVS.
		buf[4] = '0' + i;
		nvs_set_i32(handle, buf, 0);
	}
	n_money = 0;
	n_money_dropped = 0;
}
