/*
 * uartecho.c
 *
 * Christian J. Maldonado
 *
 */

/* Driver libraries */
#include <stdint.h>                                                                         // Including integer-size C control
#include <stddef.h>                                                                         // Memory arrangement / Arithmetical pointer management
#include <string.h>                                                                         // Including string handling functions
#include <stdio.h>                                                                          // Including I/O for sprintf() functions
#include <stdlib.h>                                                                         // Including strtoul() functions
#include <stdbool.h>                                                                        // Including boolean operations
#include <math.h>                                                                           // Including mathematical operations

/* Driver-Header files */
#include <ti/drivers/GPIO.h>                                                                // GPIO API header file controls
#include <ti/drivers/UART.h>                                                                // UART API header file controls
#include <ti/drivers/Timer.h>                                                               // Timer API header file controls
#include <ti/sysbios/BIOS.h>                                                                // BIOS API header file controls
#include <ti/sysbios/knl/Task.h>                                                            // Task API header file controls
#include <ti/sysbios/knl/Semaphore.h>                                                       // Semaphore API header file controls
#include <ti/sysbios/knl/Queue.h>                                                           // Queue API header file controls
#include <ti/drivers/SPI.h>                                                                 // SPI API header file controls

/* Device-GPIO maps */
#include <ti/devices/msp432e4/driverlib/gpio.h>                                             // Definitions for GPIO_PORTL_BASE and GPIO_PIN_X
#include <ti/devices/msp432e4/driverlib/pin_map.h>                                          // Definitions for GPIO pin mapping
#include <ti/devices/msp432e4/driverlib/sysctl.h>                                           // Definitions for SysCtlPeripheralEnable/Ready

/* Driver configuration */
#include "ti_drivers_config.h"                                                              // Hardware mapping peripherals
#include "tony.h"                                                                           // Header variable definitions
#include "sine.h"                                                                           // Sine table header excel values

/* Global variables */
char command_history[HISTORY_SIZE][BUFFER_SIZE];                                            // Declaring command history variable
int history_count = 0;                                                                      // Declaring count history variable
int current_history_index = -1;                                                             // Declaring command history cycle variable
UART_Handle uart7;                                                                          // Declaring global UART7 handle
Task_Handle uart7TaskHandle;                                                                // Declaring global UART7 task handle
static uint8_t uart7TaskStack[4096];                                                        // Declaring stack for UART7 task
static UART_Handle main_uart;                                                               // Declaring global UART handle

/* History function(s) */
    // Add to history buffer
void add_to_history(const char* command) {
    int i;                                                                                  // Declare i for loops
    if (history_count < HISTORY_SIZE) {                                                     // Checking for full history
        strcpy(command_history[history_count++], command);                                  // Add to history if not full
    } else {
        for (i = 0; i < HISTORY_SIZE - 1; i++) {                                            // If history is full
            strcpy(command_history[i], command_history[i + 1]);                             // shift commands one position, discard oldest
        }
        strcpy(command_history[HISTORY_SIZE - 1], command);                                 // Adding new command to end of history
    }
    current_history_index = history_count;                                                  // Updating history index
}
    // Clear the input line
void clear_input_line(UART_Handle uart, char* input, size_t* index) {
    while (*index > 0) {                                                                    // While characters are present in terminal
        UART_write(uart, "\b \b", 3);                                                       // Backspace, space, backspace (erase last character)
        (*index)--;                                                                         // Decrementing index
    }
    memset(input, 0, BUFFER_SIZE);                                                          // Clearing input buffer (fill with 0s)
}

/* UART7 function(s) */
    // UART7 message safety
bool isMessageTooLong(const char *message) {
    return (message && strlen(message) > (USER_PROMPT_SIZE - 100));                         // Allow space below USER_PROMPT_SIZE
}
    // UART7 task handler
void uart7Task(UArg arg0, UArg arg1) {
    char input[BUFFER_SIZE];                                                                // Declaring input array
    size_t index = 0;                                                                       // Declaring index variable
    bool overflow = false;                                                                  // Declaring overflow flag
    while (1) {                                                                             // Infinite processing loop
        char c;                                                                             // Declaring char scanner
        UART_read(uart7, &c, 1);                                                            // Reading each char in UART7
        if (c == '\r' || c == '\n') {                                                       // If break or tab detected
            if (index > 0 && !overflow) {                                                   // If index full present and overflow inactive
                input[index] = '\0';                                                        // Process tabs to input
                processUart7Input(uart7, input);                                            // Declaring input process to UART7
                index = 0;                                                                  // Declaring index as 0
                overflow = false;                                                           // Declaring overflow flag as flase
                Task_sleep(20);                                                             // Adding delay between commands
            }
            memset(input, 0, BUFFER_SIZE);                                                  // Clearing inputs from buffer size
        }
        else if (index < BUFFER_SIZE - 2) {                                                 // For valid index against buffer size
            input[index++] = c;                                                             // Incrementing each char from input
        }
        else {                                                                              // For overflow detected
            overflow = true;                                                                // Declaring overflow flag as true
            increment_error(ERR_UART);                                                      // Incrementing UART7 error
            const char *OFmsg = "\r\n\033[31mError\033[0m: Input too long, command will be ignored.\r\n";
            UART_write(main_uart, OFmsg, strlen(OFmsg));                                    // Writing overflow prompt to main UART
        }
        Task_sleep(10);                                                                     // Giving parse small breaks to avoid collision
    }
}
    // UART7 input processer
void processUart7Input(UART_Handle uart, char *command) {
    if (command == NULL || strlen(command) == 0) {                                          // For empty commands
        return;                                                                             // Return from loop
    }
    char cmd_copy[BUFFER_SIZE];                                                             // Creating a copy to prevent modification of original
    memset(cmd_copy, 0, BUFFER_SIZE);                                                       // Ensuring buffer is clear
    strncpy(cmd_copy, command, BUFFER_SIZE - 1);                                            // Copying command to buffer format
    cmd_copy[BUFFER_SIZE - 1] = '\0';                                                       // Copying breaks in tabs
    if (strcmp(cmd_copy, "-help") == 0 || strcmp(cmd_copy, "-about") == 0) {                // For commands that generate large output, use a delay
        valid_cmd(main_uart, cmd_copy);                                                     // Execute command with main UART
        Task_sleep(100);                                                                    // Giving system time to process large output
        return;                                                                             // Return from loop
    }
    if (strncmp(cmd_copy, "-print", 6) == 0) {                                              // Special handling for print command
        char *substring = (strlen(cmd_copy) > 7) ? cmd_copy + 7 : NULL;                     // Declaring substring pointer to parse options
        print_cmd(main_uart, substring);                                                    // Pointing print_cmd variables
        UART_write(main_uart, user_prompt, strlen(user_prompt));                            // Writing to main uart
    } else {                                                                                // For all other commands
        valid_cmd(main_uart, cmd_copy);                                                     // Processing normally
    }
}

/* mainThread */
void *mainThread(void *arg0)                                                                // Entry-point from RTOS
{
    Task_setPri(Task_self(), 1);                                                            // Set main task to higher priority

    /* Declaring main variables */
    char        input[BUFFER_SIZE];                                                         // Declaring buffer for user input
    size_t      index;                                                                      // Declaring index tracker to input buffer
    UART_Handle uart;                                                                       // Declaring UART handle variable
    UART_Params uartParams;                                                                 // Declaring variable for UART configuration parameters
    bool        overflow = false;                                                           // Declaring overflow bool

    /* Call driver init functions */
    GPIO_init();                                                                            // Initializing GPIO driver
    UART_init();                                                                            // Initializing UART driver
    Timer_init();                                                                           // Initializing Timer(s) driver
    SPI_init();                                                                             // Initializing SPI driver
    init_ticker_timer();                                                                    // Initializing Ticker function
    init_registers();                                                                       // Initializing registers function
    init_script_system();                                                                   // Initializing script function

    /* Configure the LED pin */
    GPIO_setConfig(GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);                        // Declaring GPIO LED 1
    GPIO_setConfig(GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);                        // Declaring GPIO LED 2
    GPIO_setConfig(GPIO_LED_2, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);                        // Declaring GPIO LED 3
    GPIO_setConfig(GPIO_LED_3, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);                        // Declaring GPIO LED 4
    GPIO_setConfig(GPIO_PK5, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);                          // Declaring GPIO PK5
    GPIO_setConfig(GPIO_PD4, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_HIGH);                         // Declaring GPIO PD5
    GPIO_setConfig(GPIO_SWITCH_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);                // Declaring Switch 1 (Falling)
    GPIO_setConfig(GPIO_SWITCH_2, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);                // Declaring Switch 2 (Falling)

    /* Set initial GPIO states */
    GPIO_write(GPIO_PK5, 0);                                                                // Set PK5 to 0 as required
    GPIO_write(GPIO_PD4, 1);                                                                // Start with audio amp disabled

    /* Create UART0 with data processing off. */
    UART_Params_init(&uartParams);                                                          // Initializing uartParams w/ TI default settings
    uartParams.writeDataMode = UART_DATA_BINARY;                                            // Setting UART0 data writing to binary mode
    uartParams.readDataMode = UART_DATA_BINARY;                                             // Setting UART0 reading data to binary mode
    uartParams.readReturnMode = UART_RETURN_FULL;                                           // Configuring returned data by UART_read() function
    uartParams.baudRate = 115200;                                                           // Declaring COM speed for UART0 interface
    uart = UART_open(CONFIG_UART_0, &uartParams);                                           // Assigning handle to UART0 COM peripheral
    if (uart == NULL) {                                                                     // Checking if UART0 failed to open
        while (1);                                                                          // Forever loop
    } else {                                                                                // UART0 successfully launched
        const char *successMsg = "\033[32m......UART0 initialized successfully......\033[0m\r\n";
        UART_write(uart, successMsg, strlen(successMsg));                                   // Printing success message prompt
    }
    main_uart = uart;                                                                       // Storing the main UART handle

    /* Create UART7 with data processing off. */
    UART_Params uart7Params;                                                                // Declaring UART7 parameter
    UART_Params_init(&uart7Params);                                                         // Initializing UART7 parameter
    uart7Params.writeDataMode = UART_DATA_BINARY;                                           // Declaring writeDataMode
    uart7Params.readDataMode = UART_DATA_TEXT;                                              // Declaring readDataMode
    uart7Params.readReturnMode = UART_RETURN_FULL;                                          // Declaring readReturnMode
    uart7Params.baudRate = 115200;                                                          // Declaring baudRate
    uart7Params.readTimeout = UART_WAIT_FOREVER;                                            // Declaring readTimeout
    uart7Params.readEcho = UART_ECHO_OFF;                                                   // Declaring readEcho
    uart7 = UART_open(CONFIG_UART_1, &uart7Params);                                         // Declaring UART7 parameter opener
    if (uart7 == NULL) {                                                                    // Checking if UART7 failed to open
        increment_error(ERR_UART);                                                          // Incrementing UART7 error
        const char *errorMsg = "\r\n\033[31mError\033[0m: Failed to initialize UART7\r\n";  // Printing UART7 failed prompt
        UART_write(uart, errorMsg, strlen(errorMsg));                                       // Writing error message to UART
    } else {                                                                                // UART7 successfully launched
        Task_Params taskParams;                                                             // Declaring task parameters
        Task_Params_init(&taskParams);                                                      // Initializing task parameters
        taskParams.stackSize = 4096;                                                        // Adjust stack size as needed
        taskParams.priority = 2;                                                            // Higher priority than main task
        taskParams.stack = &uart7TaskStack;                                                 // Adding stack declaration to global variables
        taskParams.arg0 = (UArg)uart;                                                       // Passing UART0 handle
        uart7TaskHandle = Task_create((Task_FuncPtr)uart7Task, &taskParams, NULL);          // Declaring UART7 task handler parameter creation
        if (uart7TaskHandle == NULL) {                                                      // For NULL tasks
            increment_error(ERR_UART);                                                      // Incrementing UART error
            const char *errorMsg = "\r\n\033[31mError\033[0m: Failed to create UART7 task\r\n";
            UART_write(uart, errorMsg, strlen(errorMsg));                                   // Writing error prompt
        } else {                                                                            // For UART7 successful boot
            const char *successMsg = "\033[32m......UART7 initialized successfully......\033[0m\r\n";
            UART_write(uart, successMsg, strlen(successMsg));                               // Writing success prompt
        }
    }

    /* Configure SPI for DAC */
    SPI_Params spiParams;                                                                   // Declaring SPI parameters
    SPI_Params_init(&spiParams);                                                            // Initializing SPI parameters
    spiParams.frameFormat = SPI_POL1_PHA1;                                                  // Mode 3 for DAC8311
    spiParams.bitRate = 4000000;                                                            // 4 MHz
    spiParams.dataSize = 16;                                                                // 16-bit transfers
    spiParams.mode = SPI_MASTER;                                                            // SPI master mode
    spiHandle = SPI_open(CONFIG_SPI_0, &spiParams);                                         // Declaring SPI handler as SPI parameter opener
    if (spiHandle == NULL) {                                                                // If SPI is NULL
        const char *errorMsg = "\r\n\033[31mError\033[0m: Failed to initialize SPI for DAC\r\n";
        UART_write(uart, errorMsg, strlen(errorMsg));                                       // Writing error prompt from uart
    }
    GPIO_write(GPIO_PK5, 0);                                                                // Ensure PK5 is 0
    uint16_t init_dac = 0x2000;                                                             // Initialize DAC with mid-scale value
    SPI_Transaction transaction;                                                            // Declaring SPI transaction
    transaction.count = 1;                                                                  // Declaring transaction count
    transaction.txBuf = &init_dac;                                                          // Declaring transaction TX pin
    transaction.rxBuf = NULL;                                                               // Declaring transaction RX pin
    SPI_transfer(spiHandle, &transaction);                                                  // Declaring SPI transfer to transaction from handler
    const char *successMsg = "\033[32m...SPI for DAC initialized successfully...\033[0m\r\n";
    UART_write(uart, successMsg, strlen(successMsg));                                       // Printing success prompt from uart

    /* UART(s) succeeded processing phase */
    const char *welcomeMsg = "\033[32m.............MSP432 Accessed!.............\033[0m\r\n"
            "Embedded System by Christian J. Maldonado (\033[31mChris\033[0m)\r\n"
            "Type '-\033[35mabout\033[0m' for compiler & configuration information.\r\n"
            "Type '-\033[35mhelp\033[0m' for available terminal commands.\r\n";
    UART_write(uart, welcomeMsg, strlen(welcomeMsg));                                       // Sending welcome prompt to UART

    /* Loop forever echoing */
    while (1) {
        index = 0;                                                                          // Resetting index for each command
        memset(input, 0, BUFFER_SIZE);                                                      // Clearing the input buffer
        overflow = false;                                                                   // Declaring false overflow tick
        current_history_index = history_count;                                              // Declaring history index with count
        while (1) {
            char c;                                                                         // Declaring character for terminal
            UART_read(uart, &c, 1);                                                         // Reading terminal line to UART
            // Tabs
            if (c == '\r' || c == '\n') {                                                   // Declaring tabs
                UART_write(uart, "\r\n", 1);                                                // Declaring new line
                input[index] = '\0';                                                        // Declaring tab to index
                break;                                                                      // Breaking loop
                }
            // Backspace
            else if (c == '\b' || c == 127) {                                               // Backspace operation
                if (index > 0) {                                                            // When characters are in terminal
                    index--;                                                                // Decrement characters in terminal
                    UART_write(uart, "\b \b", 3);                                           // Declaring spaces to UART
                }
            }
            // History
            else if (c == ESC) {                                                            // Check for arrow input
                char seq[2];                                                                // Array for next two characters
                UART_read(uart, seq, 2);                                                    // Reading next two characters
                if (seq[0] == '[') {                                                        // Checking for ANSI (ESC[)
                    switch(seq[1]) {                                                        // Checking 3rd character to determine which arrow key
                        case 'A':                                                           // Up-arrow key
                            if (current_history_index > 0) {                                // If not at older index
                                current_history_index--;                                    // Moving to previous index
                                clear_input_line(uart, input, &index);                      // Clear current input line on terminal
                                strcpy(input, command_history[current_history_index]);      // Copying historical command for input buffer
                                index = strlen(input);                                      // Updating index to the end of historical command
                                UART_write(uart, input, index);                             // Writing historical command to terminal
                            }
                            break;                                                          // Breaking case operation
                        case 'B':                                                           // Down-arrow key
                            if (current_history_index < history_count - 1) {                // If not at newest command
                                current_history_index++;                                    // Move to newer command in history
                                clear_input_line(uart, input, &index);                      // Clear current input line
                                strcpy(input, command_history[current_history_index]);      // Copying historical command for input buffer
                                index = strlen(input);                                      // Updating index to the end of historical command
                                UART_write(uart, input, index);                             // Writing historical command to terminal
                            } else if (current_history_index == history_count - 1) {        // If at the newest in index
                                current_history_index++;                                    // Moving downward for empty input
                                clear_input_line(uart, input, &index);                      // Clear current input line
                            }
                            break;                                                          // Breaking case operation
                    }
                }
            }
            // Max buffer counter
            else if (index < BUFFER_SIZE - 1) {                                             // When buffer size is under max
                input[index] = c;                                                           // Input character to input index array
                UART_write(uart, &c, 1);                                                    // Writing character pointer to UART
                index++;                                                                    // Incrementing index
                }
            else {                                                                          // Buffer overflow detected
                overflow = true;                                                            // Continuing read but not stored to clear input
                break;                                                                      // Break from operation
                }
        }
        // Overflow
        if (overflow) {                                                                     // When overflow detected
            increment_error(ERR_BUFFER_OVERFLOW);                                           // Incrementing error count
            const char *overflowMsg = "\r\n\033[31mError\033[0m: Input too long. Command ignored.\r\n";
            UART_write(uart, overflowMsg, strlen(overflowMsg));                             // Writing overflow message to UART
            }
        // History index & valid commands
        else if (index > 0) {
            add_to_history(input);                                                          // Declaring add_to_history function
            valid_cmd(uart, input);                                                         // Declaring valid_cmd function
        }
    }
}
