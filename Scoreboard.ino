//----------------------------------------------------------------------------//
// Filename    : Scoreboard.ino                                               //
// Description : Smart Basketball Scoreboard                                  //
// Version     : 2.0.0                                                        //
// Author      : Marcelo Avila de Oliveira <marceloavilaoliveira@gmail.com>   //
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
// LIBRARIES                                                                  //
//----------------------------------------------------------------------------//

// TEMPERATURE & HUMIDITY LIBRARY
#include "DHT.h"
// MULTI SERIAL LIBRARY
#include "SoftwareSerial.h"

//----------------------------------------------------------------------------//
// DEFINITIONS                                                                //
//----------------------------------------------------------------------------//

// TURN ON DEBUG MODE
// #define DEBUG
// #define DEBUG_BLUE
// #define DEBUG_PROX
// #define DEBUG_VIBR
// #define DEBUG_DHT

// DHT SENSOR
#define DHTPIN 7
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// BLUETOOTH MODULE (RX, TX)
SoftwareSerial Bluetooth(10, 11);

//----------------------------------------------------------------------------//
// CONSTANTS                                                                  //
//----------------------------------------------------------------------------//

// PINS
const int prox_pin = 2;
const int vibr_pin = 3;
const int led_r_pin = 4;
const int led_g_pin = 5;
const int led_b_pin = 6;

// TIME
const unsigned long wait_interval = 2000;
const unsigned long dht_interval = 10000;

// MATH
const float percent_to_bright_factor = 100 * log10(2) / log10(255);

//----------------------------------------------------------------------------//
// VARIABLES                                                                  //
//----------------------------------------------------------------------------//

// TIME
unsigned long wait_time;
unsigned long dht_time;

// HUMIDITY & TEMPERATURE
float humi, temp, heat;

// STATUS
boolean prox = false;
boolean vibr = false;
boolean wait = false;

//----------------------------------------------------------------------------//
// FUNCTIONS (SETTINGS)                                                       //
//----------------------------------------------------------------------------//

void setup() {
    // INITIATE PINS
    pinMode(prox_pin, INPUT);
    pinMode(vibr_pin, INPUT);
    pinMode(led_r_pin, OUTPUT);
    pinMode(led_g_pin, OUTPUT);
    pinMode(led_b_pin, OUTPUT);

    // SET LED MAGENTA
    set_led(5, 100);

    // INITIATE SERIAL COMMUNICATION
    Serial.begin(9600);

    // INITIATE BLUETOOTH COMMUNICATION
    setup_bluetooth();

    // INITIATE DHT SENSOR
    dht.begin();

    // SET LED BLUE
    set_led(4, 100);

    #ifdef DEBUG
        Serial.println("Board is alive");
        Serial.println();
    #endif
}

void setup_bluetooth() {
    #ifdef DEBUG_BLUE
        Serial.println("Setting Bluetooth");
        Serial.println();
    #endif

    // SET BAUD RATE
    Bluetooth.begin(9600);
    // CLEAR ANY AVAILABLE DATA
    while (Bluetooth.available()) {
        delay(50);
        Bluetooth.read();
    }
}

//----------------------------------------------------------------------------//
// FUNCTIONS (LIGHT)                                                          //
//----------------------------------------------------------------------------//

int percent_to_bright(int percent) {
    // PERCENT:
    // 0..100
    // RETURN BRIGHT
    // 255..0

    return 256 - pow(2, percent / percent_to_bright_factor);
}

void set_led(int color, int bright) {
    // COLOR:
    // 0 = GREEN
    // 1 = YELLOW  
    // 2 = RED
    // 3 = CYAN
    // 4 = BLUE
    // 5 = MAGENTA
    // 6 = WHITE
    //
    // BRIGHT:
    // 0 = OFF
    // ..
    // 100 = MAX

    #ifdef DEBUG
        Serial.println("Setting LED");
        Serial.println();
    #endif

    if (color < 0 || color > 6 || bright < 0 || bright > 100) {
        return;
    }

    int led_r_bright = 255;
    int led_g_bright = 255;
    int led_b_bright = 255;
    int bright_aux = percent_to_bright(bright);

    switch (color) {
        case 0:
            // GREEN
            led_g_bright = bright_aux;
            break;
        case 1:
            // YELLOW
            led_r_bright = bright_aux;
            led_g_bright = bright_aux;
            break;
        case 2:
            // RED
            led_r_bright = bright_aux;
            break;
        case 3:
            // CYAN
            led_g_bright = bright_aux;
            led_b_bright = bright_aux;
            break;
        case 4:
            // BLUE
            led_b_bright = bright_aux;
            break;
        case 5:
            // MAGENTA
            led_r_bright = bright_aux;
            led_b_bright = bright_aux;
            break;
        case 6:
            // WHITE
            led_r_bright = bright_aux;
            led_g_bright = bright_aux;
            led_b_bright = bright_aux;
            break;
    }

    analogWrite(led_r_pin, led_r_bright);
    analogWrite(led_g_pin, led_g_bright);
    analogWrite(led_b_pin, led_b_bright);

    return;
}

//----------------------------------------------------------------------------//
// FUNCTIONS (CHECK)                                                          //
//----------------------------------------------------------------------------//

void check_prox() {
    if (!prox) {
        // CHECK PROXIMITY ONLY IF PROXIMITY WASN'T DETECTED

        if(digitalRead(prox_pin) == LOW) {
            #ifdef DEBUG_PROX
                Serial.println("Proximity detected");
                Serial.println();
            #endif

            // SET LED GREEN
            set_led(0, 100);
            send_data(2);

            prox = true;
            if (!vibr) {
                wait = true;
                wait_time = millis() + wait_interval;
            }
        }
    }
}

void check_vibr() {
    if (!prox && !vibr) {
        // CHECK VIBRATION ONLY IF PROXIMITY AND VIBRATION WEREN'T DETECTED

        if(digitalRead(vibr_pin) == HIGH) {
            #ifdef DEBUG_VIBR
                Serial.println("Vibration detected");
                Serial.println();
            #endif

            // SET LED YELLOW
            set_led(1, 100);

            vibr = true;
            wait = true;
            wait_time = millis() + wait_interval;
        }
    }
}

void check_wait() {
    if (wait && millis() > wait_time) {
        if (!prox) {
            // SET LED RED
            set_led(2, 100);
            send_data(1);
        }

        reset();
    }
}

void check_dht() {
    if (!prox && !vibr) {
        // CHECK DHT ONLY IF VIBRATION WASN'T DETECTED

        if (millis() > dht_time) {
            humi = dht.readHumidity();
            temp = dht.readTemperature();
            heat = dht.computeHeatIndex(temp, humi, false);

            #ifdef DEBUG_DHT
                Serial.print("Humidity   : ");
                Serial.print(humi);
                Serial.println("%");
                Serial.print("Temperature: ");
                Serial.print(temp);
                Serial.println("°C");
                Serial.print("Head Index : ");
                Serial.print(heat);
                Serial.println("°C");
                Serial.println("");
           #endif

            send_data(0);
            dht_time = millis() + dht_interval;
        }
    }
}

//----------------------------------------------------------------------------//
// FUNCTIONS (MIS)                                                            //
//----------------------------------------------------------------------------//

void send_data(int shot) {
    // SHOT:
    // 0 = NO DATA
    // 1 = WRONG SHOT (MISS)
    // 2 = RIGHT SHOT (SCORE)

    Bluetooth.print(humi);
    Bluetooth.print(temp);
    Bluetooth.print(heat);
    Bluetooth.print(shot);

    #ifdef DEBUG_BLUE
        Serial.println("Bluetooth sent");
        Serial.println();
    #endif
}

void reset() {
    vibr = false;
    prox = false;
    wait = false;

    delay(1000);

    // SET LED BLUE
    set_led(4, 100);
}

//----------------------------------------------------------------------------//
// MAIN                                                                       //
//----------------------------------------------------------------------------//

void loop() {
    check_prox();
    check_vibr();
    check_wait();
    check_dht();
}
