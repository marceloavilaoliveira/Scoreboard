//----------------------------------------------------------------------------//
// Filename    : Scoreboard.ino                                               //
// Description : Smart Basketball Scoreboard                                  //
// Version     : 1.0.0                                                        //
// Author      : Marcelo Avila de Oliveira <marceloavilaoliveira@gmail.com>   //
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
// DEFINITIONS                                                                //
//----------------------------------------------------------------------------//

// TURN ON DEBUG MODE
// #define DEBUG
// #define DEBUG_PROX
// #define DEBUG_VIBR

//----------------------------------------------------------------------------//
// LIBRARIES                                                                  //
//----------------------------------------------------------------------------//

// SOUND LIBRARY
#include <Pitches.h>

//----------------------------------------------------------------------------//
// CONSTANTS                                                                  //
//----------------------------------------------------------------------------//

// PINS
const int prox_pin = 2;
const int vibr_pin = 3;
const int led_r_pin = 4;
const int led_g_pin = 5;
const int led_b_pin = 6;
const int speaker_pin = 7;

// TIME
unsigned long current_time = millis();
unsigned long score_interval = 3000;
unsigned long vibr_interval = 3000;
unsigned long score_time;
unsigned long vibr_time;

// MATH
float percent_to_bright_factor = 100 * log10(2) / log10(255);

//----------------------------------------------------------------------------//
// VARIABLES                                                                  //
//----------------------------------------------------------------------------//

// STATUS
boolean score = false;
boolean vibr = false;

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
    pinMode(speaker_pin, OUTPUT);

    set_led(5, 100);

    // INITIATE SERIAL COMMUNICATION
    Serial.begin(9600);

    // INITIATE BLUETOOTH COMMUNICATION
    setup_bluetooth();

    set_led(4, 100);

    #ifdef DEBUG
        Serial.println("Board is alive");
        Serial.println();
    #endif
}

void setup_bluetooth() {
    #ifdef DEBUG
        Serial.println("Setting Bluetooth");
        Serial.println();
    #endif

    Serial1.begin(38400);                   // Set baud rate
    Serial1.print("\r\n+STWMOD=0\r\n");     // Set to work in slave mode
    Serial1.print("\r\n+STNA=Arduino\r\n"); // Set name
    Serial1.print("\r\n+STOAUT=1\r\n");     // Permit Paired device to connect me
    Serial1.print("\r\n+STAUTO=0\r\n");     // Auto-connection should be forbidden here
    delay(2000);                            // This delay is required.
    Serial1.print("\r\n+INQ=1\r\n");        // Make the slave inquirable 
    delay(2000);                            // This delay is required.
    while (Serial1.available()) {           // Clear data
        delay(50);
        Serial1.read();
    }
}

//----------------------------------------------------------------------------//
// FUNCTIONS (SOUND)                                                          //
//----------------------------------------------------------------------------//

void play_tone(int note, int duration) {
    // NOTE:
    // 0 = BASS
    // 1 = MID-BASS
    // 2 = MID-TREBLE
    // 3 = TREBLE
    //
    // DURATION:
    // 0 = SHORT
    // 1 = MID-SHORT
    // 2 = MID-LONG
    // 3 = LONG

    #ifdef DEBUG
        Serial.println("Playing sound");
        Serial.println();
    #endif

    if (note < 0 || note > 3 || duration < 0 || duration > 3) {
        return;
    }

    switch (note) {
        case 0:
            note = NOTE_FS3;
            break;
        case 1:
            note = NOTE_C5;
            break;
        case 2:
            note = NOTE_FS6;
            break;
        case 3:
            note = NOTE_C8;
            break;
    }

    switch (duration) {
        case 0:
            duration = 100;
            break;
        case 1:
            duration = 200;
            break;
        case 2:
            duration = 400;
            break;
        case 3:
            duration = 800;
            break;
    }

    tone(speaker_pin, note, duration);
    delay(duration);
    noTone(speaker_pin);

    return;
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
    if (!score) {
        if(digitalRead(prox_pin) == LOW) {
            #ifdef DEBUG_PROX
                Serial.println("Proximity detected");
                Serial.println();
            #endif
            board(1);
        }
    }
}

void check_vibr() {
    if (!score) {
        if (vibr) {
            if (millis() > vibr_time) {
                board(0);
            }
        } else {
            if(digitalRead(vibr_pin) == HIGH) {
                #ifdef DEBUG_PROX
                    Serial.println("Vibration detected");
                    Serial.println();
                #endif
                vibr = true;
                vibr_time = millis() + vibr_interval;
                set_led(1, 100);
            }
        }
    }
}

void check_score() {
    if (score && millis() > score_time) {
        reset();
    }
}

//----------------------------------------------------------------------------//
// FUNCTIONS (MIS)                                                            //
//----------------------------------------------------------------------------//

void board(int mode) {
    // MODE:
    // 0 = WRONG
    // 1 = RIGHT

    score = true;
    score_time = millis() + score_interval;

    if (mode == 0) {
        Serial1.print(0);
        set_led(2, 100);
        play_tone(0, 3);
    } else {
        Serial1.print(1);
        set_led(0, 100);
        play_tone(3, 3);
    }
}

void reset() {
    vibr = false;
    score = false;
    set_led(4, 100);
}

//----------------------------------------------------------------------------//
// MAIN                                                                       //
//----------------------------------------------------------------------------//

void loop() {
    check_prox();
    check_vibr();
    check_score();
}
