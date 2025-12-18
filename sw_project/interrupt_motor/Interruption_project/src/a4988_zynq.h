/*
 * a4988_zynq.h
 * A4988 Stepper Motor Driver Library for Xilinx Zynq
 * Full control including microstepping (MS1, MS2, MS3)
 *
 * Based on frostybeard's a4988_stepper_library
 * Adapted for Zynq EMIO GPIO
 */

#ifndef A4988_ZYNQ_H
#define A4988_ZYNQ_H

#include "xgpiops.h"
#include "sleep.h"
#include "xil_printf.h"

// Direcciones del motor
typedef enum {
    DIR_CW = 0,     // Clockwise (sentido horario)
    DIR_CCW = 1     // Counter-clockwise (sentido antihorario)
} Direction;

// Modos de microstepping
typedef enum {
    FULL_STEP = 1,
    HALF_STEP = 2,
    QUARTER_STEP = 4,
    EIGHTH_STEP = 8,
    SIXTEENTH_STEP = 16
} MicrostepMode;

// Estructura del motor A4988
typedef struct {
    XGpioPs *gpio;          // Puntero al driver GPIO
    int pin_enable;         // Pin ENABLE
    int pin_step;           // Pin STEP
    int pin_dir;            // Pin DIR
    int pin_ms1;            // Pin MS1
    int pin_ms2;            // Pin MS2
    int pin_ms3;            // Pin MS3
    int motor_steps;        // Pasos por revolucion del motor (ej: 200 para NEMA17)
    MicrostepMode microstep;// Microstepping actual (1, 2, 4, 8, 16)
    int steps_per_rev;      // Pasos totales por revolucion
    int step_delay_us;      // Delay entre pulsos (controla velocidad)
    Direction current_dir;  // Direccion actual
    int is_enabled;         // Estado del motor
} A4988_Motor;

// Funciones de la librer a
void A4988_Init(A4988_Motor *motor, XGpioPs *gpio,
                int pin_enable, int pin_step, int pin_dir,
                int pin_ms1, int pin_ms2, int pin_ms3,
                int motor_steps);

void A4988_Enable(A4988_Motor *motor);
void A4988_Disable(A4988_Motor *motor);

void A4988_SetDirection(A4988_Motor *motor, Direction dir);
void A4988_SetMicrostepping(A4988_Motor *motor, MicrostepMode mode);
void A4988_SetSpeed(A4988_Motor *motor, int rpm);
void A4988_SetStepDelay(A4988_Motor *motor, int delay_us);

void A4988_Step(A4988_Motor *motor, int steps);
void A4988_Rotate(A4988_Motor *motor, float degrees);
void A4988_RotateRevolutions(A4988_Motor *motor, int revolutions, Direction dir);

void A4988_PrintInfo(A4988_Motor *motor);

#endif // A4988_ZYNQ_H
