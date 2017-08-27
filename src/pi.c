/** @file pi.c
 * @brief Communicates with Raspberry Pi via serial port
 */

#include <API.h>
#include <string.h>
#include "pi.h"

#define B(value) ((value) ? '1' : '0')

static void piRead(char *str, int num, FILE *uart);
static void piLoop(void *params);
static void piHid(FILE *uart);
static void piMotors(FILE *uart);
static void piPins(FILE *uart);

void piInit(FILE *uart) {
    usartInit(uart, 19200, SERIAL_8N1);
    taskCreate(piLoop, TASK_DEFAULT_STACK_SIZE, uart, 1);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
static void piLoop(void *params) {
    FILE *uart = (FILE *) params;

    // Let the robot get into a steady state
    delay(1000);

/*
    // Simple loop that echos back plus copies to the debug serial port
    fputs("echo", uart);
    int ch;
    while ((ch = fgetc(uart)) != -1 && ch != '\n' && ch != '\r') {
        fputc(ch, uart);
        putchar(ch);
    }
    fprint("\r\n", uart);
    printf("\r\n");
*/

    // Command request/response loop
    for (;;) {
        char cmd[15];
        fputs("ready", uart);
        piRead(cmd, sizeof(cmd), uart);
        printf("received %s\n", cmd);  // to debug, not back to the pi uart

        // TODO: ultrasonic
        // TODO: motor encoders
        // TODO: ime

        bool all = strncmp(cmd, "all", sizeof(cmd)) == 0;
        if (all || strncmp(cmd, "time", sizeof(cmd)) == 0) {
            fprintf(uart, "time %ld %ld\n", millis(), micros());
        }
        if (all || strncmp(cmd, "battery", sizeof(cmd)) == 0) {
            fprintf(uart, "battery %d %d\n", powerLevelMain(), powerLevelBackup());
        }
        if (all || strncmp(cmd, "competition", sizeof(cmd)) == 0) {
            fprintf(uart, "competition %c %c %c\n", B(isOnline()), B(isEnabled()), B(isAutonomous()));
        }
        if (all || strncmp(cmd, "hid", sizeof(cmd)) == 0) {
            piHid(uart);
        }
        if (all || strncmp(cmd, "motor", sizeof(cmd)) == 0) {
            piMotors(uart);
        }
        if (all || strncmp(cmd, "pin", sizeof(cmd)) == 0) {
            piPins(uart);
        }

        // Blank line indicates end of response
        fputc('\n', uart);
    }
}
#pragma clang diagnostic pop

void piRead(char *str, int num, FILE *uart) {
    char *ptr = str;
    for (int ch; num > 1 && (ch = fgetc(uart)) != EOF && ch != '\n' && ch != '\r'; num--) {
        *ptr++ = ch;
    }
    *ptr = '\0';
}

static void piHid(FILE *uart) {
    // Analog inputs including accel
    fprint("hid J1 A", uart);
    for (int axis = 1; axis <= 6; axis++) {
        fprintf(uart, " %d", joystickGetAnalog(1, axis));
    }
    fputc('\n', uart);

    // Digital inputs
    fprint("hid J1 D", uart);
    for (int group = 5; group <= 8; group++) {
        fputc(' ', uart);
        fputc(B(joystickGetDigital(1, group, JOY_DOWN)), uart);
        fputc(B(joystickGetDigital(1, group, JOY_UP)), uart);
        if (group >= 7) {
            fputc(B(joystickGetDigital(1, group, JOY_LEFT)), uart);
            fputc(B(joystickGetDigital(1, group, JOY_RIGHT)), uart);
        }
    }
    fputc('\n', uart);
}

static void piMotors(FILE *uart) {
    fprint("motor ", uart);
    for (int motor = 1; motor <= 10; motor++) {
        fprintf(uart, " %d", motorGet(motor));
    }
    fputc('\n', uart);
}

static void piPins(FILE *uart) {
    fprint("pin A", uart);
    for (int pin = 1; pin <= 8; pin++) {
        fprintf(uart, " %d", analogRead(pin));
    }
    fputc('\n', uart);

    fprint("pin D ", uart);
    for (int pin = 1; pin <= 26; pin++) {
        fputc(B(digitalRead(pin)), uart);
    }
    fputc('\n', uart);
}
