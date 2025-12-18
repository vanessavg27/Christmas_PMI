/*
 * a4988_zynq.c
 * A4988 Stepper Motor Driver Library for Xilinx Zynq
 */

#include "a4988_zynq.h"

/**
 * Inicializar el motor A4988
 */
void A4988_Init(A4988_Motor *motor, XGpioPs *gpio,
                int pin_enable, int pin_step, int pin_dir,
                int pin_ms1, int pin_ms2, int pin_ms3,
                int motor_steps) {

    motor->gpio = gpio;
    motor->pin_enable = pin_enable;
    motor->pin_step = pin_step;
    motor->pin_dir = pin_dir;
    motor->pin_ms1 = pin_ms1;
    motor->pin_ms2 = pin_ms2;
    motor->pin_ms3 = pin_ms3;
    motor->motor_steps = motor_steps;
    motor->microstep = FULL_STEP;
    motor->steps_per_rev = motor_steps;
    motor->step_delay_us = 1000;
    motor->current_dir = DIR_CW;
    motor->is_enabled = 0;

    // Configurar todos los pines como OUTPUT
    XGpioPs_SetDirectionPin(gpio, pin_enable, 1);
    XGpioPs_SetDirectionPin(gpio, pin_step, 1);
    XGpioPs_SetDirectionPin(gpio, pin_dir, 1);
    XGpioPs_SetDirectionPin(gpio, pin_ms1, 1);
    XGpioPs_SetDirectionPin(gpio, pin_ms2, 1);
    XGpioPs_SetDirectionPin(gpio, pin_ms3, 1);

    // Habilitar output
    XGpioPs_SetOutputEnablePin(gpio, pin_enable, 1);
    XGpioPs_SetOutputEnablePin(gpio, pin_step, 1);
    XGpioPs_SetOutputEnablePin(gpio, pin_dir, 1);
    XGpioPs_SetOutputEnablePin(gpio, pin_ms1, 1);
    XGpioPs_SetOutputEnablePin(gpio, pin_ms2, 1);
    XGpioPs_SetOutputEnablePin(gpio, pin_ms3, 1);

    // Estado inicial
    XGpioPs_WritePin(gpio, pin_enable, 1);  // Disabled
    XGpioPs_WritePin(gpio, pin_step, 0);
    XGpioPs_WritePin(gpio, pin_dir, 0);
    XGpioPs_WritePin(gpio, pin_ms1, 0);     // Full step por defecto
    XGpioPs_WritePin(gpio, pin_ms2, 0);
    XGpioPs_WritePin(gpio, pin_ms3, 0);

    xil_printf("A4988 Motor initialized\r\n");
}

/**
 * Habilitar el motor
 */
void A4988_Enable(A4988_Motor *motor) {
    XGpioPs_WritePin(motor->gpio, motor->pin_enable, 0);  // Activo bajo
    motor->is_enabled = 1;
    usleep(1000);
}

/**
 * Deshabilitar el motor
 */
void A4988_Disable(A4988_Motor *motor) {
    XGpioPs_WritePin(motor->gpio, motor->pin_enable, 1);  // Activo bajo
    motor->is_enabled = 0;
    usleep(1000);
}

/**
 * Establecer direcci n del motor
 */
void A4988_SetDirection(A4988_Motor *motor, Direction dir) {
    motor->current_dir = dir;
    XGpioPs_WritePin(motor->gpio, motor->pin_dir, dir);
    usleep(5);
}

/**
 * Configurar microstepping
 * Tabla A4988:
 * MS1  MS2  MS3  | Microstep Resolution
 * L    L    L    | Full step
 * H    L    L    | Half step
 * L    H    L    | Quarter step
 * H    H    L    | Eighth step
 * H    H    H    | Sixteenth step
 */
void A4988_SetMicrostepping(A4988_Motor *motor, MicrostepMode mode) {
    motor->microstep = mode;
    motor->steps_per_rev = motor->motor_steps * mode;

    switch(mode) {
        case FULL_STEP:  // 1
            XGpioPs_WritePin(motor->gpio, motor->pin_ms1, 0);
            XGpioPs_WritePin(motor->gpio, motor->pin_ms2, 0);
            XGpioPs_WritePin(motor->gpio, motor->pin_ms3, 0);
            xil_printf("Microstepping set to FULL STEP (1)\r\n");
            break;

        case HALF_STEP:  // 2
            XGpioPs_WritePin(motor->gpio, motor->pin_ms1, 1);
            XGpioPs_WritePin(motor->gpio, motor->pin_ms2, 0);
            XGpioPs_WritePin(motor->gpio, motor->pin_ms3, 0);
            xil_printf("Microstepping set to HALF STEP (1/2)\r\n");
            break;

        case QUARTER_STEP:  // 4
            XGpioPs_WritePin(motor->gpio, motor->pin_ms1, 0);
            XGpioPs_WritePin(motor->gpio, motor->pin_ms2, 1);
            XGpioPs_WritePin(motor->gpio, motor->pin_ms3, 0);
            xil_printf("Microstepping set to QUARTER STEP (1/4)\r\n");
            break;

        case EIGHTH_STEP:  // 8
            XGpioPs_WritePin(motor->gpio, motor->pin_ms1, 1);
            XGpioPs_WritePin(motor->gpio, motor->pin_ms2, 1);
            XGpioPs_WritePin(motor->gpio, motor->pin_ms3, 0);
            xil_printf("Microstepping set to EIGHTH STEP (1/8)\r\n");
            break;

        case SIXTEENTH_STEP:  // 16
            XGpioPs_WritePin(motor->gpio, motor->pin_ms1, 1);
            XGpioPs_WritePin(motor->gpio, motor->pin_ms2, 1);
            XGpioPs_WritePin(motor->gpio, motor->pin_ms3, 1);
            xil_printf("Microstepping set to SIXTEENTH STEP (1/16)\r\n");
            break;

        default:
            xil_printf("ERROR: Invalid microstepping mode\r\n");
            return;
    }

    xil_printf("  Steps per revolution: %d\r\n", motor->steps_per_rev);
    usleep(1000);  // Tiempo para que el A4988 actualice configuraci n
}

/**
 * Establecer velocidad en RPM
 */
void A4988_SetSpeed(A4988_Motor *motor, int rpm) {
    // delay_us = (60,000,000) / (rpm * steps_per_rev * 2)
    motor->step_delay_us = (60000000) / (rpm * motor->steps_per_rev * 2);
    xil_printf("Speed set to %d RPM (delay: %d us)\r\n", rpm, motor->step_delay_us);
}

/**
 * Establecer delay manual entre pasos
 */
void A4988_SetStepDelay(A4988_Motor *motor, int delay_us) {
    motor->step_delay_us = delay_us;
}

/**
 * Ejecutar un n mero de pasos
 */
void A4988_Step(A4988_Motor *motor, int steps) {
    Direction direction = (steps >= 0) ? DIR_CW : DIR_CCW;
    int abs_steps = (steps >= 0) ? steps : -steps;

    A4988_SetDirection(motor, direction);

    for (int i = 0; i < abs_steps; i++) {
        XGpioPs_WritePin(motor->gpio, motor->pin_step, 1);
        usleep(motor->step_delay_us);
        XGpioPs_WritePin(motor->gpio, motor->pin_step, 0);
        usleep(motor->step_delay_us);
    }
}

/**
 * Rotar grados espec ficos
 */
void A4988_Rotate(A4988_Motor *motor, float degrees) {
    int steps = (int)((degrees / 360.0) * motor->steps_per_rev);
    A4988_Step(motor, steps);
}

/**
 * Rotar revoluciones completas
 */
void A4988_RotateRevolutions(A4988_Motor *motor, int revolutions, Direction dir) {
    int total_steps = revolutions * motor->steps_per_rev;

    xil_printf("Rotating %d revolutions %s (%d steps)\r\n",
               revolutions,
               (dir == DIR_CW) ? "CW" : "CCW",
               total_steps);

    A4988_SetDirection(motor, dir);

    for (int i = 0; i < total_steps; i++) {
        XGpioPs_WritePin(motor->gpio, motor->pin_step, 1);
        usleep(motor->step_delay_us);
        XGpioPs_WritePin(motor->gpio, motor->pin_step, 0);
        usleep(motor->step_delay_us);

        // Mostrar progreso cada revoluci n
        if ((i + 1) % motor->steps_per_rev == 0) {
            xil_printf("  Revolution %d/%d complete\r\n",
                      (i + 1) / motor->steps_per_rev,
                      revolutions);
        }
    }
}

/**
 * Imprimir informaci n del motor
 */
void A4988_PrintInfo(A4988_Motor *motor) {
    xil_printf("\r\n=== A4988 Motor Info ===\r\n");
    xil_printf("Motor steps: %d\r\n", motor->motor_steps);
    xil_printf("Microstepping: 1/%d\r\n", motor->microstep);
    xil_printf("Steps per revolution: %d\r\n", motor->steps_per_rev);
    xil_printf("Step delay: %d us\r\n", motor->step_delay_us);
    xil_printf("Current direction: %s\r\n",
               (motor->current_dir == DIR_CW) ? "CW" : "CCW");
    xil_printf("Motor %s\r\n",
               motor->is_enabled ? "ENABLED" : "DISABLED");
    xil_printf("========================\r\n\r\n");
}
