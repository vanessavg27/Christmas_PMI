#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpiops.h"
#include "xgpio.h"
#include "xscugic.h"
#include "sleep.h"
#include "a4988_zynq.h"

// Definiciones de GPIO EMIO (Motor)
#define GPIO_DEVICE_ID      XPAR_XGPIOPS_0_DEVICE_ID
#define EMIO_PIN_ENABLE     54  // EMIO[0]
#define EMIO_PIN_STEP       55  // EMIO[1]
#define EMIO_PIN_DIR        56  // EMIO[2]
#define EMIO_PIN_MS1        57  // EMIO[3]
#define EMIO_PIN_MS2        58  // EMIO[4]
#define EMIO_PIN_MS3        59  // EMIO[5]

// Definiciones del botón (AXI GPIO)
#define BTN_GPIO_DEVICE_ID  XPAR_AXI_GPIO_0_DEVICE_ID
#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID
#define BTN_INTERRUPT_ID    XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define BUTTON_CHANNEL      1

// Configuración del motor NEMA 17
#define NEMA17_STEPS        200

// Instancias globales
XGpioPs GpioPs;      // GPIO para motor (EMIO)
XGpio BtnGpio;       // GPIO para botón (AXI)
XScuGic Intc;        // Controlador de interrupciones
A4988_Motor motor;

// Variables de control
volatile int motor_running = 0;  // 0 = detenido, 1 = en funcionamiento
volatile int stop_requested = 0; // Bandera para detener el motor

// Prototipos de funciones
int initGPIO();
int SetupInterruptSystem(XScuGic *IntcInstancePtr, XGpio *GpioInstancePtr, u16 GpioIntrId);
void ButtonHandler(void *CallbackRef);
void MotorSequence();

/**
 * Inicializar GPIO para motor (EMIO)
 */
int initGPIO() {
    XGpioPs_Config *ConfigPtr;
    int Status;

    ConfigPtr = XGpioPs_LookupConfig(GPIO_DEVICE_ID);
    if (ConfigPtr == NULL) {
        xil_printf("ERROR: GPIO Lookup failed\r\n");
        return XST_FAILURE;
    }

    Status = XGpioPs_CfgInitialize(&GpioPs, ConfigPtr, ConfigPtr->BaseAddr);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: GPIO Init failed\r\n");
        return XST_FAILURE;
    }

    xil_printf("GPIO initialized successfully\r\n");
    return XST_SUCCESS;
}

/**
 * Manejador de interrupción del botón
 */
void ButtonHandler(void *CallbackRef) {
    XGpio *GpioPtr = (XGpio *)CallbackRef;
    u32 button_value;

    // Leer el estado del botón
    button_value = XGpio_DiscreteRead(GpioPtr, BUTTON_CHANNEL);

    // Si el botón está presionado
    if (button_value & 0x01) {
        if (motor_running == 0) {
            // Motor detenido -> Iniciar
            motor_running = 1;
            stop_requested = 0;
            xil_printf("\r\n*** BOTON PRESIONADO - INICIANDO MOTOR ***\r\n\r\n");
        } else {
            // Motor en funcionamiento -> Detener
            motor_running = 0;
            stop_requested = 1;
            xil_printf("\r\n*** BOTON PRESIONADO - DETENIENDO MOTOR ***\r\n\r\n");
        }
    }

    // Limpiar la interrupción
    XGpio_InterruptClear(GpioPtr, XGPIO_IR_CH1_MASK);
}

/**
 * Configurar el sistema de interrupciones
 */
int SetupInterruptSystem(XScuGic *IntcInstancePtr, XGpio *GpioInstancePtr, u16 GpioIntrId) {
    int Status;
    XScuGic_Config *IntcConfig;

    // Inicializar el controlador de interrupciones (GIC)
    IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
    if (IntcConfig == NULL) {
        return XST_FAILURE;
    }

    Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Configurar el manejador de excepciones
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                  (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                  IntcInstancePtr);

    // Conectar el manejador del botón a la interrupción
    Status = XScuGic_Connect(IntcInstancePtr, GpioIntrId,
                            (Xil_ExceptionHandler)ButtonHandler,
                            (void *)GpioInstancePtr);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Habilitar la interrupción del GPIO en el GIC
    XScuGic_Enable(IntcInstancePtr, GpioIntrId);

    // Habilitar interrupciones del GPIO
    XGpio_InterruptEnable(GpioInstancePtr, XGPIO_IR_CH1_MASK);
    XGpio_InterruptGlobalEnable(GpioInstancePtr);

    // Habilitar excepciones
    Xil_ExceptionEnable();

    return XST_SUCCESS;
}

/**
 * Secuencia de movimiento del motor
 */
void MotorSequence() {
    // Habilitar motor
    A4988_Enable(&motor);
    xil_printf("Motor enabled - Starting sequence...\r\n\r\n");
    sleep(1);

    while (!stop_requested) {  // Bucle infinito hasta que se presione el botón
        // ========== ESTADO 1: 5 vueltas CW ==========
        if (stop_requested) break;
        xil_printf("========================================\r\n");
        xil_printf("STATE 1: 5 revolutions CW (60 RPM)\r\n");
        xil_printf("========================================\r\n");
        A4988_SetSpeed(&motor, 60);
        A4988_RotateRevolutions(&motor, 5, DIR_CW);
        if (stop_requested) break;
        xil_printf("State 1 complete\r\n\r\n");
        sleep(2);

        // ========== ESTADO 2: 5 vueltas CCW (retorno) ==========
        if (stop_requested) break;
        xil_printf("========================================\r\n");
        xil_printf("STATE 2: 5 revolutions CCW - Return (60 RPM)\r\n");
        xil_printf("========================================\r\n");
        A4988_RotateRevolutions(&motor, 5, DIR_CCW);
        if (stop_requested) break;
        xil_printf("State 2 complete\r\n\r\n");
        sleep(2);

        // ========== ESTADO 3: 3 vueltas CW ==========
        if (stop_requested) break;
        xil_printf("========================================\r\n");
        xil_printf("STATE 3: 3 revolutions CW (40 RPM)\r\n");
        xil_printf("========================================\r\n");
        A4988_SetSpeed(&motor, 40);
        A4988_RotateRevolutions(&motor, 3, DIR_CW);
        if (stop_requested) break;
        xil_printf("State 3 complete\r\n\r\n");
        sleep(2);

        // ========== ESTADO 4: 3 vueltas CCW (retorno) ==========
        if (stop_requested) break;
        xil_printf("========================================\r\n");
        xil_printf("STATE 4: 3 revolutions CCW - Return (40 RPM)\r\n");
        xil_printf("========================================\r\n");
        A4988_RotateRevolutions(&motor, 3, DIR_CCW);
        if (stop_requested) break;
        xil_printf("State 4 complete\r\n\r\n");
        sleep(2);

        // ========== ESTADO 5: 6 vueltas CW ==========
        if (stop_requested) break;
        xil_printf("========================================\r\n");
        xil_printf("STATE 5: 6 revolutions CW (20 RPM)\r\n");
        xil_printf("========================================\r\n");
        A4988_SetSpeed(&motor, 20);
        A4988_RotateRevolutions(&motor, 6, DIR_CW);
        if (stop_requested) break;
        xil_printf("State 5 complete\r\n\r\n");
        sleep(2);

        // ========== ESTADO 6: 6 vueltas CCW (retorno) ==========
        if (stop_requested) break;
        xil_printf("========================================\r\n");
        xil_printf("STATE 6: 6 revolutions CCW - Return (20 RPM)\r\n");
        xil_printf("========================================\r\n");
        A4988_RotateRevolutions(&motor, 6, DIR_CCW);
        if (stop_requested) break;
        xil_printf("State 6 complete\r\n\r\n");
        sleep(2);

        xil_printf("========================================\r\n");
        xil_printf("  Cycle Complete! Restarting...\r\n");
        xil_printf("========================================\r\n\r\n");
        sleep(1);
    }

    // Deshabilitar motor cuando se detiene
    A4988_Disable(&motor);
    motor_running = 0;

    xil_printf("========================================\r\n");
    xil_printf("  Motor stopped by user\r\n");
    xil_printf("========================================\r\n\r\n");
    stop_requested = 0;
}

int main() {
    int Status;

    xil_printf("\r\n");
    xil_printf("================================================\r\n");
    xil_printf("  NEMA 17 Motor Control with A4988\r\n");
    xil_printf("  Button Interrupt Control via AXI GPIO\r\n");
    xil_printf("================================================\r\n\r\n");

    // Inicializar GPIO para motor
    if (initGPIO() != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Inicializar GPIO para botón
    Status = XGpio_Initialize(&BtnGpio, BTN_GPIO_DEVICE_ID);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: Button GPIO Init failed\r\n");
        return XST_FAILURE;
    }

    // Configurar dirección del botón como entrada
    XGpio_SetDataDirection(&BtnGpio, BUTTON_CHANNEL, 0xFFFFFFFF);
    xil_printf("Button GPIO initialized successfully\r\n");

    // Configurar sistema de interrupciones
    Status = SetupInterruptSystem(&Intc, &BtnGpio, BTN_INTERRUPT_ID);
    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: Interrupt setup failed\r\n");
        return XST_FAILURE;
    }
    xil_printf("Interrupt system configured successfully\r\n\r\n");

    // Inicializar motor A4988
    A4988_Init(&motor, &GpioPs,
               EMIO_PIN_ENABLE, EMIO_PIN_STEP, EMIO_PIN_DIR,
               EMIO_PIN_MS1, EMIO_PIN_MS2, EMIO_PIN_MS3,
               NEMA17_STEPS);

    // Configurar microstepping a 1/16
    A4988_SetMicrostepping(&motor, SIXTEENTH_STEP);

    // Mostrar información del motor
    A4988_PrintInfo(&motor);

    xil_printf("\r\n");
    xil_printf("========================================\r\n");
    xil_printf("  System Ready!\r\n");
    xil_printf("  Press BTN_0 to start/stop motor\r\n");
    xil_printf("========================================\r\n\r\n");

    // Loop principal
    while (1) {
        if (motor_running == 1 && stop_requested == 0) {
            // Ejecutar secuencia del motor
            MotorSequence();
        }

        // Pequeña pausa para no saturar el CPU
        usleep(100000);  // 100ms
    }

    return 0;
}
