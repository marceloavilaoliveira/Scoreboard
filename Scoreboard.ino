//----------------------------------------------------------------------------//
// Filename    : Scoreboard.ino                                               //
// Description : Smart Basketball Scoreboard                                  //
// Version     : 1.1.0                                                        //
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
// CONSTANTS                                                                  //
//----------------------------------------------------------------------------//

// PINS
const int prox_pin = 2;
const int vibr_pin = 3;
const int led_r_pin = 4;
const int led_g_pin = 5;
const int led_b_pin = 6;

// TIME
const unsigned long wait_interval = 3000;

// MATH
const float percent_to_bright_factor = 100 * log10(2) / log10(255);

//----------------------------------------------------------------------------//
// VARIABLES                                                                  //
//----------------------------------------------------------------------------//

// TIME
unsigned long wait_time;

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
        if(digitalRead(prox_pin) == LOW) {
            #ifdef DEBUG_PROX
                Serial.println("Proximity detected");
                Serial.println();
            #endif

            prox = true;
            if (!vibr) {
                wait = true;
                wait_time = millis() + wait_interval;
            }
            set_shot(1);
        }
    }
}

void check_vibr() {
    if (!prox && !vibr) {
        if(digitalRead(vibr_pin) == HIGH) {
            #ifdef DEBUG_PROX
                Serial.println("Vibration detected");
                Serial.println();
            #endif

            vibr = true;
            wait = true;
            wait_time = millis() + wait_interval;
            set_led(1, 100);
        }
    }
}

void check_wait() {
    if (wait && millis() > wait_time) {
        if (!prox) {
            set_shot(0);
        }

        reset();
    }
}

//----------------------------------------------------------------------------//
// FUNCTIONS (MIS)                                                            //
//----------------------------------------------------------------------------//

void set_shot(int mode) {
    // MODE:
    // 0 = WRONG SHOT (MISS)
    // 1 = RIGHT SHOT (SCORE)

    if (mode == 0) {
        set_led(2, 100);
    } else {
        set_led(0, 100);
    }

    Serial1.print(mode);
    delay(1000);
}

void reset() {
    vibr = false;
    prox = false;
    wait = false;
    set_led(4, 100);
}

//----------------------------------------------------------------------------//
// MAIN                                                                       //
//----------------------------------------------------------------------------//

void loop() {
    check_prox();
    check_vibr();
    check_wait();
}
