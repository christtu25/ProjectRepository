/*
 * terminalcmd.c
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
#include "sine.h"                                                                           // Sine table header definitions

/* Global variables */
static uint32_t error_counts[ERROR_COUNT_SIZE] = {0};                                       // Global array for multiple errors (initialized to 0)
char user_prompt[USER_PROMPT_SIZE];                                                         // Global char for command prompts
Callback callbacks[MAX_CALLBACKS] = {0};                                                    // Declaring callback index to list
Timer_Handle timer1Handle = NULL;                                                           // Global handle for callback 0 timer (declared NULL)
Ticker tickers[MAX_TICKERS] = {0};                                                          // Declaring ticker index to list
Timer_Handle ticker_timer_handle = NULL;                                                    // Global handle for ticker timer (declared NULL)
static uint32_t ticker_time = 0;                                                            // Declaring static ticker timer checker variable
int32_t registers[NUM_REGISTERS] = {0};                                                     // Declaring registers 1D list values (all as 0)
char reg_names[NUM_REGISTERS][MAX_REG_NAME_LENGTH] = {0};                                   // Declaring registers 2D list values (all as 0)
char script_space[SCRIPT_SIZE][MAX_SCRIPT_LINE_LENGTH] = {0};                               // Declaring script space 2D list (all as 0)
static Task_Handle scriptTaskHandle = NULL;                                                 // Declaring script task handler
static Semaphore_Handle scriptSem = NULL;                                                   // Declaring script semaphore handler
static Queue_Handle scriptQueue = NULL;                                                     // Declaring script queue handler
static UART_Handle script_uart = NULL;                                                      // Declaring script UART handler
static uint8_t scriptTaskStack[SCRIPT_TASK_STACK_SIZE];                                     // Declaring script task stack array
static bool is_script_executing __attribute__((unused)) = false;                            // Declaring active script flag (unused attribute - declared within script_task function)
static bool script_stop __attribute__((unused)) = false;                                    // Declaring script stop flag (unused attribute - declared within script_task function)
static double phase_accumulator = 0;                                                        // Declaring double phase accumulator
static double phase_increment = 0;                                                          // Declaring double phase increment
static uint32_t sample_rate = 8000;                                                         // Declaring sample rate to match 125us period
static bool sine_active = false;                                                            // Declaring flag for active sine status
SPI_Handle spiHandle = NULL;                                                                // Declaring SPI handle as NULL
Timer_Handle timer0Handle = NULL;                                                           // Declaring timer handler 0 (callback 0) as NULL

/* Payload-Queue operations */
    // Queue increment
void enqueue_payload(Callback *cb, const char *payload) {
    if (cb->queue_size < QUEUE_SIZE) {                                                      // When queue size is greater than callback
        strcpy(cb->payload[cb->queue_rear], payload);                                       // Copying payload to rear of queue
        cb->queue_rear = (cb->queue_rear + 1) % QUEUE_SIZE;                                 // Moving rear pointer as circular queue (returning remainder)
        cb->queue_size++;                                                                   // Increment queue size
    }                                                                                       // Payload silently removed if queue is full
}
    // Queue decrement
char* dequeue_payload(Callback *cb) {
    if (cb->queue_size > 0) {                                                               // When queue size is greater than 0
        char *payload = cb->payload[cb->queue_front];                                       // Declaring payload to front of queue
        cb->queue_front = (cb->queue_front + 1) % QUEUE_SIZE;                               // Moving front pointer as circular queue (returning remainder)
        cb->queue_size--;                                                                   // Decrement queue size
        return payload;                                                                     // Return payload variable
    }
    return NULL;                                                                            // Returning NULL if queue is empty
}
    // Payload returner
char* peek_payload(Callback *cb) {
    if (cb->queue_size > 0) {                                                               // Returning front payload
        return cb->payload[cb->queue_front];                                                // without removing from queue
    }
    return NULL;                                                                            // Returning NULL if queue is empty
}
    // Output-Queue
void execute_callback(Callback *cb) {
    if (cb->count != 0 && cb->queue_size > 0 && !cb->is_executing) {                        // Checking if callback should execute
        cb->is_executing = true;                                                            // Searching when callback execution is true
        if (cb->output_length == 0) {                                                       // If no current output
            char *payload = peek_payload(cb);                                               // Preparing next payload
            if (payload != NULL) {                                                          // When payload already initialized
                user_prompt[0] = '\0';                                                      // Tab empty user prompt
                valid_cmd(cb->uart, payload);                                               // Executing valid command
                strcpy(cb->output_buffer, user_prompt);                                     // Storing result
                cb->output_length = strlen(cb->output_buffer);                              // Calculate and store length
                cb->output_position = 0;                                                    // Initializing output position back to front of buffer
            }
        }
        if (cb->is_button_callback || cb->is_large_interval) {                              // When button callbacks or large timer intervals
                                                                                            // Write the entire output at once
            UART_write(cb->uart, cb->output_buffer + cb->output_position, cb->output_length - cb->output_position);
            cb->output_position = cb->output_length;                                        // Declaring position to length
        } else {                                                                            // For small timer intervals
            int remaining = cb->output_length - cb->output_position;                        // Provide use in chunked output
            int to_write = (remaining > MAX_OUTPUT_CHUNK) ? MAX_OUTPUT_CHUNK : remaining;
            UART_write(cb->uart, cb->output_buffer + cb->output_position, to_write);
            cb->output_position += to_write;                                                // Update position for next operation to write
        }
        if (cb->output_position >= cb->output_length) {                                     // Declaring next execution to complete output
            cb->output_length = 0;                                                          // Declaring length to front
            cb->output_position = 0;                                                        // Declaring position to front
            if (cb->count > 0) {                                                            // Decrement count if greater than 0
                cb->count--;
            }
            if (cb->count == 0) {                                                           // When count reaches 0
                cb->queue_front = (cb->queue_front + 1) % QUEUE_SIZE;                       // Removing current payload from queue
                cb->queue_size--;
            }
        }
        cb->is_executing = false;                                                           // Setting callback execute flag as inactive
    }
}
    // Queue counter
int Queue_count(Queue_Handle queue) {
    int count = 0;                                                                          // Declaring count variable
    Queue_Elem *elem = Queue_head(queue);                                                   // Declaring element queue pointed from head
    while (elem != NULL) {                                                                  // While an element is NULL
        count++;                                                                            // Incrementing count
        elem = Queue_next(elem);                                                            // Declaring element variable to next in queue
    }
    return count;                                                                           // Return count from loop
}

/* Callback operations */
    // Callback 0 (timer)
void timer_callback(Timer_Handle handle, int_fast16_t status) {
    static bool first_execution = true;                                                     // Static variable for tracking first execution
    Callback *cb = &callbacks[TIMER_CALLBACK];                                              // Declaring pointer to Callback 0
    if (first_execution) {                                                                  // If first execution appears
        first_execution = false;                                                            // Handling first execution
        execute_callback(cb);                                                               // Declaring callback execution
    } else {
        while (cb->is_large_interval && cb->output_position < cb->output_length) {          // Completing the entire output for large intervals
            execute_callback(cb);                                                           // Declaring callback execution
        }
        if (!cb->is_large_interval || cb->output_position >= cb->output_length) {           // For small intervals or when large interval output completes
            execute_callback(cb);                                                           // Declaring callback execution
        }
    }
    if (cb->queue_size == 0 && cb->output_length == 0) {                                    // Checking if queue is empty (all output processed)
        Timer_stop(handle);                                                                 // Stopping timer
        first_execution = true;                                                             // Reset for next timer start
    }
}
    // SW1
void sw1_callback(uint_least8_t index) {
    if (!callbacks[SW1_CALLBACK].is_executing) {                                            // Executing Callback 1 (if not already)
        execute_callback(&callbacks[SW1_CALLBACK]);                                         // Declaring callback execution
    }
}
    // SW2
void sw2_callback(uint_least8_t index) {
    if (!callbacks[SW2_CALLBACK].is_executing) {                                            // Executing Callback 1 (if not already)
        execute_callback(&callbacks[SW2_CALLBACK]);                                         // Declaring callback execution
    }
}

/* Ticker operations */
    // Ticker timer
void init_ticker_timer() {
    Timer_Params params;                                                                    // Declaring ticker timer parameters
    Timer_Params_init(&params);                                                             // Initializing timer to parameter
    params.period = 10000;                                                                  // 10 ms
    params.periodUnits = Timer_PERIOD_US;                                                   // Declaring period library
    params.timerMode = Timer_CONTINUOUS_CALLBACK;                                           // Declaring timer mode
    params.timerCallback = ticker_timer_callback;                                           // Declaring ticker callback function to parameter operation
    ticker_timer_handle = Timer_open(TICKER_TIMER, &params);                                // Declaring open ticker timer handle
    if (ticker_timer_handle == NULL) {                                                      // If ticker timer declared as NULL
        increment_error(ERR_TICKER);                                                        // Incrementing ticker error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Failed to open the ticker timer.\r\n");
        return;                                                                             // Return from loop
    } else {                                                                                // For successful ticker input
        Timer_start(ticker_timer_handle);                                                   // Starting ticker timer
    }
}
    // Ticker callback
void ticker_timer_callback(Timer_Handle handle, int_fast16_t status) {
    int i;                                                                                  // Declare the loop variable at start of the function
    ticker_time += 1;                                                                       // Increment by 1 (10 ms) each time ticker callback is called
    for (i = 0; i < MAX_TICKERS; i++) {                                                     // For input within ticker count
        if (tickers[i].active && ticker_time >= tickers[i].next_execution) {                // If active ticker and next execution is ready for execution
            execute_ticker(i);                                                              // Declaring user input ticker to execute
        }
    }
}
    // Ticker executable
void execute_ticker(int index) {
    Ticker *ticker = &tickers[index];                                                       // Declaring ticker handle to input index
    valid_cmd(ticker->uart, ticker->payload);                                               // Executing ticker payload via valid command
    ticker->next_execution = ticker_time + ticker->period;                                  // Calculating next execution time for current ticker
    if (ticker->count > 0) {                                                                // If not -1 (infinite loop)
        ticker->count--;                                                                    // Decrementing count
        if (ticker->count == 0) {                                                           // If count reaches end
            ticker->active = false;                                                         // Deactivating finished ticker
        }
    }
}

/* Register operations */
    // Register initialization
void init_registers() {
    int i;                                                                                  // Declaring loop variable
    for (i = 0; i < NUM_REGISTERS; i++) {                                                   // For all usable registers
        snprintf(reg_names[i], MAX_REG_NAME_LENGTH, "R%d", i);                              // Printing register to assigned digit
    }
}
    // Register grab
int32_t get_reg_value(const char *reg_name) {
    int i;                                                                                  // Declaring loop variable
    for (i = 0; i < NUM_REGISTERS; i++) {                                                   // For all usable registers
        if (strcmp(reg_names[i], reg_name) == 0) {                                          // If register is assigned to name
            return registers[i];                                                            // Return assigned registers
        }
    }
    return 0;                                                                               // Return 0 if register not found
}
    // Register set
void set_reg_value(const char *reg_name, int32_t value) {
    int i;                                                                                  // Declaring loop variable
    for (i = 0; i < NUM_REGISTERS; i++) {                                                   // For all usable registers
        if (strcmp(reg_names[i], reg_name) == 0) {                                          // If register is assigned to name
            registers[i] = value;                                                           // Declaring set register to value
            return;                                                                         // Return from loop
        }
    }
}
    // Register immediate constant
int32_t parse_operand(const char *operand) {
    if (operand[0] == '#') {                                                                // If immediate '#' detected
        return atoi(operand + 1);                                                           // Parse immediate value
    } else {                                                                                // If not immediate
        return get_reg_value(operand);                                                      // Get register value
    }
}

/* Script operations */
    // Initializing script system
void init_script_system(void) {
    Semaphore_Params semParams;                                                             // Creating semaphore for script execution
    Semaphore_Params_init(&semParams);                                                      // Declaring semaphore parameter initialization
    scriptSem = Semaphore_create(0, &semParams, NULL);                                      // Declaring script semaphore
    Queue_Params queueParams;                                                               // Creating queue for script commands
    Queue_Params_init(&queueParams);                                                        // Declaring queue parameter to initialization parameter
    scriptQueue = Queue_create(NULL, NULL);                                                 // Declaring script queue as NULL
    Task_Params taskParams;                                                                 // Creating script execution task
    Task_Params_init(&taskParams);                                                          // Declaring task parameter
    taskParams.stackSize = SCRIPT_TASK_STACK_SIZE;                                          // Declaring task stack size
    taskParams.priority = 2;                                                                // Declaring task priority
    taskParams.stack = &scriptTaskStack;                                                    // Declaring task parameter to stack size
    scriptTaskHandle = Task_create(script_task, &taskParams, NULL);                         // Creating script task handler to parameter
    if (scriptTaskHandle == NULL) {                                                         // Handle error in case task handle is invalid
        increment_error(ERR_SCRIPT);                                                        // Incrementing script error
        return;                                                                             // Return from loop
    }
}
    // Script task handler
void script_task(UArg arg0, UArg arg1) {
    while (1) {                                                                             // Infinite loop
        Semaphore_pend(scriptSem, BIOS_WAIT_FOREVER);                                       // Waiting for semaphore signal
        while (!Queue_empty(scriptQueue)) {                                                 // Processing commands in queue
            Queue_Elem *elem = Queue_get(scriptQueue);                                      // Getting next command from queue
            ScriptQueueElement *queueElem = (ScriptQueueElement *)elem;                     // Declaring script element to queue pointer
            script_uart = queueElem->cmd.uart;                                              // Storing UART handle
            if (queueElem->cmd.command[0] != '\0') {                                        // Execute the command
                char cmd_copy[MAX_CMD_LENGTH];                                              // Declaring copied command as array variable
                strncpy(cmd_copy, queueElem->cmd.command, MAX_CMD_LENGTH-1);                // Check if this is a script execution command before executing
                cmd_copy[MAX_CMD_LENGTH-1] = '\0';                                          // Declaring execution tabs to copied command array
                if (strncmp(cmd_copy, "-script", 7) == 0) {                                 // If its a script command to execute
                    char *args = cmd_copy + 7;                                              // Declaring argument pointer
                    while (*args == ' ') args++;                                            // Skip whitespaces to arguments
                    int line_num;                                                           // Declaring line number variable
                    char execute_flag[2];                                                   // Declaring execution flags
                    if (sscanf(args, "%d %1s", &line_num, execute_flag) == 2 && execute_flag[0] == 'x' &&
                        line_num >= 0 && line_num < SCRIPT_SIZE) {                          // Declaring scanner to execution variables in script space
                        if (strncmp(script_space[line_num], "-script", 7) == 0) {           // For infinite loop (when script executes another script)
                            queue_script_command(script_space[line_num]);                   // Queue the command from current line
                            queue_script_command(queueElem->cmd.command);                   // Re-queue the original script command to maintain the loop
                        } else {                                                            // For sequential execution
                            int current_line = line_num;                                    // Queue all subsequent non-empty commands
                            while (current_line < SCRIPT_SIZE && script_space[current_line][0] != '\0') {
                                queue_script_command(script_space[current_line]);           // Declaring queue script in script space to each script line
                                current_line++;                                             // Incrementing from current line
                            }
                        }
                    }
                } else {                                                                    // Execute non-script commands normally
                    valid_cmd(script_uart, queueElem->cmd.command);                         // Declaring valid command processing to script space through UART to elements in queue
                }
            }
            free(queueElem);                                                                // Free the queue element
            Task_sleep(20);                                                                 // Add a small delay between commands
        }
    }
}
    // Queue script command
bool queue_script_command(const char *command) {
    if (strlen(command) >= MAX_CMD_LENGTH) {                                                // If processed command is greater than or equal to max command length
        return false;                                                                       // Return false flag from loop
    }
    ScriptQueueElement *queueElem = malloc(sizeof(ScriptQueueElement));                     // Declaring script queue pointer to elements via malloc
    if (!queueElem) {                                                                       // If queue element not detected
        return false;                                                                       // Return false flag from loop
    }
    strncpy(queueElem->cmd.command, command, MAX_CMD_LENGTH - 1);                           // Copying elements in queue command
    queueElem->cmd.command[MAX_CMD_LENGTH - 1] = '\0';                                      // Executing tabs to queue elements
    queueElem->cmd.uart = script_uart;                                                      // Executing UART to queue elements
    Queue_put(scriptQueue, &queueElem->elem);                                               // Putting queue elements to script space
    return true;                                                                            // Return true flag from loop
}
    // Execute script
void execute_script(UART_Handle uart, int start_line) {
    while (!Queue_empty(scriptQueue)) {                                                     // Clear any existing queue
        Queue_Elem *elem = Queue_get(scriptQueue);                                          // Declaring element pointer in queue
        free((ScriptQueueElement *)elem);                                                   // Freeing elements in script queue
    }
    script_uart = uart;                                                                     // Store UART handle for the script task
    int current_line = start_line;                                                          // Queue all commands from start_line until we hit an empty line
    while (current_line < SCRIPT_SIZE && script_space[current_line][0] != '\0') {           // While current line in script line isn't tabbed/executed
        if (!queue_script_command(script_space[current_line])) {                            // If queue command not in script space
            increment_error(ERR_SCRIPT);                                                    // Incrementing script error
            snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Failed to queue script command.\r\n");
            UART_write(uart, user_prompt, strlen(user_prompt));                             // Writing from UART
            return;                                                                         // Return from loop
        }
        current_line++;                                                                     // Incrementing current line
    }
    script_stop = false;                                                                    // Post to semaphore to start execution
    Semaphore_post(scriptSem);                                                              // Declaring semaphore post to script space
}
    // Stop script
void stop_script(void) {
    script_stop = true;                                                                     // Declaring script stop flag as true
}

/* Conditional operations */
    // Evaluation condition
bool evaluate_condition(UART_Handle uart, const char *operand1, const char *condition, const char *operand2) {
    int32_t value1, value2;                                                                 // Declaring condition values
    char reg_name[MAX_REG_NAME_LENGTH];                                                     // Declaring reg array
    snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nDebug: evaluating: %s %s %s\r\n",          // Debug print for input values (noting correct order)
             operand1, condition, operand2);
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing from uart
    if (operand1[0] == '#') {                                                               // Parse first operand
        value1 = atoi(operand1 + 1);                                                        // Pointing value1 variable to operand
    } else if (strncmp(operand1, "R", 1) == 0) {                                            // For register input
        value1 = get_reg_value(operand1);                                                   // Declaring value1 to register value operand
    } else {                                                                                // For single register
        snprintf(reg_name, MAX_REG_NAME_LENGTH, "R%s", operand1);                           // Printing register status
        value1 = get_reg_value(reg_name);                                                   // Pointing value1 variable to operand
    }
    if (operand2[0] == '#') {                                                               // Parse second operand (operand2, not condition)
        value2 = atoi(operand2 + 1);                                                        // Declaring value2 variable converting operand to string
    } else if (strncmp(operand2, "R", 1) == 0) {                                            // For detected register
        value2 = get_reg_value(operand2);                                                   // Declaring value2 from register value
    } else {                                                                                // For register and string detected
        snprintf(reg_name, MAX_REG_NAME_LENGTH, "R%s", operand2);                           // Printing register status
        value2 = get_reg_value(reg_name);                                                   // Declaring value2 variable to register value
    }
    snprintf(user_prompt, USER_PROMPT_SIZE, "Debug: values: %d %s %d\r\n",                  // Debug print for parsed values
             value1, condition, value2);
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing from uart
    bool result = false;                                                                    // Evaluate condition ('condition' parameter)
    if (strcmp(condition, ">") == 0) {                                                      // For >
        result = value1 > value2;                                                           // Declaring result variable
    } else if (strcmp(condition, "=") == 0) {                                               // For =
        result = value1 == value2;                                                          // Declaring result variable
    } else if (strcmp(condition, "<") == 0) {                                               // For <
        result = value1 < value2;                                                           // Declaring result variable
    }
    snprintf(user_prompt, USER_PROMPT_SIZE, "Debug: result: %d\r\n", result);               // Debug print for result
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing from uart
    return result;                                                                          // Return result variable
}
    // Execute conditional destination
void execute_conditional_dest(UART_Handle uart, const char *dest, bool is_true) {
    if (dest == NULL || strlen(dest) == 0) {                                                // If destination is null/empty
        return;                                                                             // Returning nothing
    }
    char *endptr;                                                                           // Declaring variable to check if destination is a script line number
    long line_num = strtol(dest, &endptr, 10);                                              // Convert string to long
    if (*endptr == '\0') {                                                                  // If conversion was successful
        if (line_num >= 0 && line_num < SCRIPT_SIZE) {                                      // Valid script line
            if (script_space[line_num][0] != '\0') {                                        // If line is not empty
                valid_cmd(uart, script_space[line_num]);                                    // Executing this line
            }
        }
    } else {                                                                                // Not line number (treated as command)
        valid_cmd(uart, (char *)dest);                                                      // Execute as regular command
    }
}

/* Sine operations */
    // Set frequency for sine wave
void set_sine_frequency(uint32_t freq) {
    if (freq > (sample_rate / 2)) {                                                         // Check Nyquist limit
        increment_error(ERR_SINE);                                                          // Incrementing sine error
        return;                                                                             // Return for loop
    }
    phase_increment = (freq * LUT_SIZE * 125) / 1000000.0;                                  // New formula based on the timer period (125 microseconds)
}
    // Timer callback for sine wave
void sine_timer_callback(Timer_Handle handle, int_fast16_t status) {
    if (!sine_active) return;                                                               // If sine appears inactive return from loop
    uint32_t table_index = (uint32_t)phase_accumulator % LUT_SIZE;                          // Calculate table index
    uint16_t dac_value = sineLUT[table_index];                                              // Get value directly from lookup table
    uint16_t dac_word = dac_value & 0x3FFF;                                                 // Format for DAC8311: Upper 2 bits must be 0, followed by 14-bit data
    SPI_Transaction transaction;                                                            // Perform SPI transfer
    transaction.count = 1;                                                                  // Declaring transaction count
    transaction.txBuf = &dac_word;                                                          // Declaring transaction TX buffer
    transaction.rxBuf = NULL;                                                               // Declaring transaction RX buffer
    SPI_transfer(spiHandle, &transaction);                                                  // Declaring SPI handler transfer
    phase_accumulator += phase_increment;                                                   // Phase accumulator for next sample
    if (phase_accumulator >= LUT_SIZE)                                                      // If LUT greater or equal to
        phase_accumulator -= LUT_SIZE;                                                      // Decrement from LUT size
}

/* Terminal operation functions */
    // Input processing
void processUserInput(UART_Handle uart) {
    char inputBuffer[MAX_CMD_LENGTH + 1];                                                   // +1 for null terminator
    int index = 0;                                                                          // Declaring empty index
    char c;                                                                                 // Declaring empty char
    bool overflow = false;                                                                  // Declaring false overflow
    while (1) {
        UART_read(uart, &c, 1);                                                             // Reading each character in UART
        if (c == '\r' || c == '\n') {                                                       // Including tab commands
            inputBuffer[index] = '\0';                                                      // Including tab to terminal
            break;                                                                          // Break loop
        } else if (index < MAX_CMD_LENGTH) {                                                // If under max length
            inputBuffer[index++] = c;                                                       // Reading each character in terminal
        } else {                                                                            // If overflow detected
            overflow = true;                                                                // Buffer overflow detected
            continue;                                                                       // Reading but not storing to clear the input
        }
    }
    if (overflow) {                                                                         // When overflow is detected
        increment_error(ERR_BUFFER_OVERFLOW);                                               // Incrementing error count
        const char *overflowMsg = "\033[31mError\033[0m: Input too long. Command ignored.\r\n";
        UART_write(uart, overflowMsg, strlen(overflowMsg));                                 // Writing overflow prompt to UART
    } else if (index > 0) {                                                                 // No overflow detected
        valid_cmd(uart, inputBuffer);                                                       // Pushing valid_cmd function
    }
}
    // Error incrementing
void increment_error(int error_type) {
    if (error_type >= 0 && error_type < ERROR_COUNT_SIZE) {                                 // Declaring error count boundaries
        error_counts[error_type]++;                                                         // Incrementing error count
    }
}

/* Command functions */
    // -error
void error_cmd(UART_Handle uart) {
    snprintf(user_prompt, USER_PROMPT_SIZE,                                                 // Printing error prompt/count
             "\r\n----------ERROR COUNTS----------\r\n"
             "Unknown commands:         %lu\r\n"
             "Buffer overflows:         %lu\r\n"
             "Invalid addresses:        %lu\r\n"
             "GPIO errors:              %lu\r\n"
             "Timer errors:             %lu\r\n"
             "Callback errors:          %lu\r\n"
             "Reset errors:             %lu\r\n"
             "Ticker errors:            %lu\r\n"
             "Register errors:          %lu\r\n"
             "Script errors:            %lu\r\n"
             "Conditional errors:       %lu\r\n"
             "UART7 errors:             %lu\r\n"
             "Sine errors:              %lu\r\n"
             "Network errors:           %lu\r\n",
             error_counts[ERR_UNKNOWN_CMD],                                                 // Unknown command error count
             error_counts[ERR_BUFFER_OVERFLOW],                                             // Overflow error count
             error_counts[ERR_INVALID_ADDRESS],                                             // Invalid address error count
             error_counts[ERR_GPIO],                                                        // GPIO error count
             error_counts[ERR_TIMER],                                                       // Timer (Callback 0) error count
             error_counts[ERR_CALLBACK],                                                    // Callback error count
             error_counts[ERR_RESET],                                                       // Reset error count
             error_counts[ERR_TICKER],                                                      // Ticker error count
             error_counts[ERR_REG],                                                         // Register error count
             error_counts[ERR_SCRIPT],                                                      // Script error count
             error_counts[ERR_IF],                                                          // Conditional error count
             error_counts[ERR_UART],                                                        // UART7 error count
             error_counts[ERR_SINE],                                                        // Sine-DAC error count
             error_counts[ERR_NET]);                                                        // UDP error count
       UART_write(uart, user_prompt, strlen(user_prompt));                                  // Writing UART from user_prompt
}
    // -print
void print_cmd(UART_Handle uart, char *substring) {                                         // Declaring function for -print command
    if (substring == NULL) {                                                                // User prompt for empty substring
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[33mWarning\033[0m: No text detected with -print command.\r\n");
    }
    else {
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n%s\r\n", substring);                   // Printing user input in -print substring
    }
}
    // -about
void about_cmd(UART_Handle uart) {                                                          // Declaring function for -about command
    snprintf(user_prompt, USER_PROMPT_SIZE,                                                 // Formatting user_prompt array
            "\r\n----------ABOUT----------\r\n"
            "Username       |       %s\r\n"
            "Assignment #   |       %s\r\n"
            "Version        |       %s\r\n"
            "Date           |       %s on %s\r\n"
            "Updates        |       UDP Network messaging\r\n",
            USER, ASSIGNMENT, VERSION_SUB, __TIME__, __DATE__);                             // Preprocessor macros
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Sending formatted message to UART
}
    // -help
void help_cmd(UART_Handle uart, char *subcmd) {                                             // Declaring function for -help command
    if (subcmd == NULL) {                                                                   // If subcommand is missing
    snprintf(user_prompt, USER_PROMPT_SIZE,                                                 // Formatting user_prompt array
             "\r\n----------HELP----------\r\n"
             ".....Command List.....\r\n"
             "-help:        |       Displays valid commands\r\n"
             "-about:       |       Displays system info\r\n"
             "-clear:       |       Clears the terminal\r\n"
             "-print:       |       Prints user message\r\n"
             "-memr:        |       Reads memory location\r\n"
             "-error:       |       Displays error info\r\n"
             "-gpio:        |       Displays GPIO info\r\n"
             "-timer:       |       Controls the periodic timer\r\n"
             "-callback:    |       Sets callbacks to timer and switches\r\n"
             "-reset:       |       Resets callbacks and stops timer\r\n"
             "-ticker:      |       Sets periodic-command executions\r\n"
             "-reg:         |       Perform register operations\r\n"
             "-script:      |       Manage and execute scripts\r\n"
             "-rem:         |       Add remarks in scripts\r\n"
             "-if:          |       Conditional executions\r\n"
             "-uart:        |       UART7 payload executions\r\n"
             "-sine:        |       Generate sine wave audio output\r\n"
             "-netudp:      |       Send UDP message\r\n"
             );
    }
    else if (strcmp(subcmd, "about") == 0) {                                                // Printing -about substring info
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n-about command      |       Print build info.\r\n");
    }
    else if (strcmp(subcmd, "help") == 0) {                                                 // Printing -help substring info
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n-help command       |       Print available commands.\r\n");
    }
    else if (strcmp(subcmd, "clear") == 0) {                                                // Printing -clear substring info
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n-clear command      |       Clear characters in terminal.\r\n");
    }
    else if (strcmp(subcmd, "print") == 0) {                                                // Printing -print substring info
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n-print command      |       Print user typed characters.\r\n");
    }
    else if (strcmp(subcmd, "memr") == 0) {                                                 // Printing -memr substring info
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n-memr command       |       Read memory at specified address.\r\n");
    }
    else if (strcmp(subcmd, "error") == 0) {                                                // Printing -error substring info
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n-error command      |       Display error counts.\r\n");
    }
    else if (strcmp(subcmd, "gpio") == 0) {                                                 // Printing -gpio substring info
        snprintf(user_prompt, USER_PROMPT_SIZE,
                 "\r\n-gpio command      |       Displays GPIO info.\r\n"
                 "  -gpio         (read all GPIOs)\r\n"
                 "  -gpio <number> <operation> [value]\r\n"
                 "  number: 0-3 (LEDs), 4 (PK5), 5 (PD4), 6-7 (Switches)\r\n"
                 "  operation: r (read), w (write), t (toggle)\r\n"
                 "  value: 0 or 1 (for write operation)\r\n"
                 "  Examples:\r\n"
                 "   -gpio 0 r    (read LED 0)\r\n"
                 "   -gpio 1 w 1  (turn on LED 1)\r\n"
                 "   -gpio 2 t    (toggle LED 2)\r\n"
                 "   -gpio 6 r    (read Switch 1)\r\n");
    }
    else if (strcmp(subcmd, "timer") == 0) {                                                // Printing -timer substring info
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n-timer command      |       -timer <period_ms>\r\n"
                 "  Sets up a periodic timer with the specified period in milliseconds.\r\n");
    }
    else if (strcmp(subcmd, "callback") == 0) {                                             // Printing -callback substring info
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n-callback command      |       -callback <index> <count> <payload>\r\n"
                 "  Sets up a callback with the specified index (0-2), count, and payload.\r\n"
                 "  Index 0:  Timer callback\r\n"
                 "  Index 1:  SW1 (right switch) callback\r\n"
                 "  Index 2:  SW2 (left switch) callback\r\n"
                 "  Count:    Number of times to execute (-1 for infinite)\r\n"
                 "  Payload:  Command to execute when callback is triggered\r\n");
    }
    else if (strcmp(subcmd, "reset") == 0) {                                                // Printing -reset substring info
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n-reset command      |       -reset [index]\r\n"
                 "  Resets all LEDs & the specified callback or all callbacks if no index provided.\r\n"
                 "  Also stops the timer if the timer callback is reset.\r\n"
                 "  Index: 0 (Timer), 1 (SW1), 2 (SW2)\r\n");
    }
    else if (strcmp(subcmd, "ticker") == 0) {                                               // Printing -ticker substring info
        snprintf(user_prompt, USER_PROMPT_SIZE,
                 "\r\n-ticker     |     -ticker <index> <initial_delay> <period> <count> <payload>\r\n"
                 "  index: 0-15 (ticker number)\r\n"
                 "  initial_delay: Initial delay in 10ms units (100 = 1 second)\r\n"
                 "  period: Repeat period in 10ms units (100 = 1 second)\r\n"
                 "  count: Number of repeats (-1 for infinite)\r\n"
                 "  payload: Valid command to execute\r\n"
                 "  Example: -ticker 3 100 100 5 -gpio 2 t\r\n");
    }
    else if (strcmp(subcmd, "reg") == 0) {                                                  // Printing -reg substring info
        snprintf(user_prompt, USER_PROMPT_SIZE,
                 "\r\n-reg command       |       Perform register operations\r\n"
                 "  Usage: -reg <operation> <dest> <src1> [src2]\r\n"
                 "  Operations: MOV, XCG, INC, DEC, ADD, SUB, NEG, NOT,\r\n              AND, IOR, XOR, MUL, DIV, REM, MAX, MIN\r\n"
                 "  Registers: R0 to R31\r\n"
                 "  Immediate values: Use # prefix (e.g., #10)\r\n"
                 "  Example: -reg ADD R0 R1 #5\r\n");
    }
    else if (strcmp(subcmd, "script") == 0) {                                               // Printing -script substring info
        snprintf(user_prompt, USER_PROMPT_SIZE,
                 "\r\n-script command    |       Manage and execute scripts\r\n"
                 "  Usage: -script [line_number] [payload]\r\n"
                 "  - Without arguments: Display entire script space\r\n"
                 "  - With line_number only: Execute script from that line\r\n"
                 "  - With line_number and payload: Set script line content\r\n"
                 "  Example: -script 17 -gpio 0 t\r\n");
    }
    else if (strcmp(subcmd, "rem") == 0) {                                                  // Printing -rem substring info
        snprintf(user_prompt, USER_PROMPT_SIZE,
                 "\r\n-rem command       |       Add remarks in scripts\r\n"
                 "  Usage: -rem [remark text]\r\n"
                 "  - Adds a non-executable comment in scripts\r\n");
    }
    else if (strcmp(subcmd, "if") == 0) {                                                   // Printing -if substring info
        snprintf(user_prompt, USER_PROMPT_SIZE,
                 "\r\n-if command        |       Conditional execution\r\n"
                 "  Usage: -if A COND B ? DESTT : DESTF\r\n"
                 "  A, B: Register (R0-R31) or immediate values\r\n"
                 "  COND: Condition (>, =, <)\r\n"
                 "  DESTT: Command/Script line to execute if true\r\n"
                 "  DESTF: Command/Script line to execute if false\r\n"
                 "  Example: -if R0 > #5 ? -script 10 : -print false\r\n");
    }
    else if (strcmp(subcmd, "uart") == 0) {                                                 // Printing -uart substring info
        snprintf(user_prompt, USER_PROMPT_SIZE,
                 "\r\n-uart command       |       Send payload through UART7\r\n"
                 "  Usage: -uart <payload>\r\n"
                 "  - Sends the specified payload through UART7\r\n"
                 "  - Will process any incoming messages from UART7\r\n"
                 "  - Requires proper TX/RX/GND connections for communication\r\n"
                 "  Examples:\r\n"
                 "   -uart hello        (sends 'hello' through UART7)\r\n"
                 "   -uart -gpio 0 t    (sends GPIO toggle command through UART7)\r\n");
    }
    else if (strcmp(subcmd, "sine") == 0) {                                                   // Printing -sine substring info
        snprintf(user_prompt, USER_PROMPT_SIZE,
            "\r\n-sine command       |       Generate sine wave through audio output\r\n"
            "  Usage: -sine FREQ\r\n"
            "  FREQ: Frequency in Hz (0 to stop)\r\n"
            "  - Uses Timer0 for sample rate control\r\n"
            "  - Maximum frequency limited by Nyquist rate (sample_rate/2)\r\n"
            "  - Uses SPI DAC for high-quality output\r\n"
            "  Examples:\r\n"
            "   -sine 440     (generate 440 Hz sine wave - A4 note)\r\n"
            "   -sine 1000    (generate 1 kHz test tone)\r\n"
            "   -sine 0       (stop sine wave generation)\r\n");
    }
    else if (strcmp(subcmd, "netudp") == 0) {
        snprintf(user_prompt, USER_PROMPT_SIZE,
                "\r\n-netudp command      |       Send UDP messages\r\n"
                "  Usage: -netudp IPADDR:PORT payload\r\n"
                "  - Sends the specified payload through UDP\r\n"
                "  - Will process any incoming UDP messages\r\n"
                "  Examples:\r\n"
                "   -netudp 192.168.1.100:1000 hello\r\n"
                "   -netudp 192.168.1.100:1000 -gpio 0 t\r\n");
    }
    else {                                                                                  // Printing no substring prompt
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[33mWarning\033[0m: Unknown subcommand. Type -help for available commands.\r\n");
    }
}
    // -memr
void memr_cmd(UART_Handle uart, char *address_str) {
    char *endCnt;                                                                           // Declaring end pointer
    uint32_t address = strtoul(address_str, &endCnt, 16);                                   // Changing base from 32 to 16 for hex input
    if (*endCnt != '\0') {                                                                  // If format is invalid
                                                                                            // Printing user prompt for format error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Invalid address format: %s\r\n", address_str);
        increment_error(ERR_INVALID_ADDRESS);                                               // Incrementing error prompt
        return;                                                                             // Returning operation
        }
    if ((address > 0xFFFFC && address < 0x20000000) || address > 0x2003FFFC) {              // Flash and SRAM address limit
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Address out of allowed range (0x0 to 0xFFFFC | 0x20000000 to 0x2003FFFC).\r\n");
        increment_error(ERR_INVALID_ADDRESS);                                               // Incrementing GPIO error
        return;                                                                             // Returning loop operation
        }
    address &= ~0x3;                                                                        // Reassuring format
    if ((address < 0x000FFFFF) ||                                                           // Flash address limit
       (address >= 0x20000000 && address < 0x2003FFFC)) {                                   // SRAM address limit
            volatile uint32_t *ptr = (volatile uint32_t *)address;                          // Safely cast the address to 32-bit pointer                                                                                // Reading/Formatting output string
            snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nMemory at 0x%08lX: 0x%08X\r\n", address, *ptr);
        } else {                                                                            // Else statement for out-of-range address input
            snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[33mWarning\033[0m: Address may not be in a valid memory region.\r\n");
            volatile uint32_t *ptr = (volatile uint32_t *)address;                          // Declaring volatile read to address
            snprintf(user_prompt + strlen(user_prompt), USER_PROMPT_SIZE - strlen(user_prompt),
                      "Memory at 0x%08lX: 0x%08X\r\n", address, *ptr);
        }
}
    // -gpio
void gpio_cmd(UART_Handle uart, char *args) {
    int gpio_num;                                                                           // Declaring GPIO variable
    char operation;                                                                         // Declaring operation char
    uint32_t gpio_pin;                                                                      // Declaring 32bit GPIO pin variable
    bool is_switch = false;                                                                 // Declaring bool variable for active switch
    int value;                                                                              // Declaring value variable for full GPIO read
    char buffer[256] = "";                                                                  // Declaring buffer for full GPIO print
    int offset = 0;                                                                         // Declaring offset variable
    int i;                                                                                  // Declaring i variable
    if (args == NULL || *args == '\0' || *args == '\r' || *args == '\n') {                  // Declaring empty -gpio command
        for (i = 0; i <= 7; i++) {                                                          // Declaring GPIO count
            switch(i) {                                                                     // Switch loop for GPIO print
                case 0: gpio_pin = GPIO_LED_0; break;                                       // LED 1
                case 1: gpio_pin = GPIO_LED_1; break;                                       // LED 2
                case 2: gpio_pin = GPIO_LED_2; break;                                       // LED 3
                case 3: gpio_pin = GPIO_LED_3; break;                                       // LED 4
                case 4: gpio_pin = GPIO_PK5; break;                                         // PK5
                case 5: gpio_pin = GPIO_PD4; break;                                         // PD4
                case 6: gpio_pin = GPIO_SWITCH_1; break;                                    // SW1
                case 7: gpio_pin = GPIO_SWITCH_2; break;                                    // SW2
                default:                                                                    // Declaring default function
                    continue;                                                               // Continue terminal
            }
            value = GPIO_read(gpio_pin);                                                    // Declaring value as GPIO read
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,                    // Printing to terminal with offset
                               "GPIO %d: %d\r\n", i, value);
        }
        UART_write(uart, buffer, strlen(buffer));                                           // Writing to UART
        return;                                                                             // Returning loop
    }
    if (sscanf(args, "%d %c", &gpio_num, &operation) != 2) {                                // Use prompt for input error
        increment_error(ERR_GPIO);                                                          // Incrementing GPIO error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Invalid GPIO command format. Use: -gpio <number> <r|w|t> [value]\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user_prompt to UART
        return;                                                                             // Returning loop
    }
    switch(gpio_num) {                                                                      // Declaring switch function for GPIO cases
        case 0: gpio_pin = GPIO_LED_0; break;                                               // LED 1
        case 1: gpio_pin = GPIO_LED_1; break;                                               // LED 2
        case 2: gpio_pin = GPIO_LED_2; break;                                               // LED 3
        case 3: gpio_pin = GPIO_LED_3; break;                                               // LED 4
        case 4: gpio_pin = GPIO_PK5; break;                                                 // PK5
        case 5: gpio_pin = GPIO_PD4; break;                                                 // PD4
        case 6: gpio_pin = GPIO_SWITCH_1; is_switch = true; break;                          // SW1
        case 7: gpio_pin = GPIO_SWITCH_2; is_switch = true; break;                          // SW2
        default:                                                                            // Declaring default function
            increment_error(ERR_GPIO);                                                      // Incrementing GPIO error
            snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Invalid GPIO number.\r\n");
            UART_write(uart, user_prompt, strlen(user_prompt));                             // Writing user_prompt to UART
            return;                                                                         // Returning function
    }
    if (is_switch && operation != 'r') {                                                    // Declaring statement for switches
        increment_error(ERR_GPIO);                                                          // Incrementing GPIO error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Switches can only be read. Use: -gpio %d r\r\n", gpio_num);
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user_prompt to UART
        return;                                                                             // Returning statement
    }
    switch(operation) {                                                                     // Declaring switch function for user inputs
        case 'r':                                                                           // Read operation
            {
                int value = GPIO_read(gpio_pin);                                            // Declaring user input as value read
                snprintf(user_prompt, USER_PROMPT_SIZE, "GPIO %d value: %d\r\n", gpio_num, value);
            }                                                                               // Printing user input as user_prompt
            break;                                                                          // Breaking operation
        case 'w':                                                                           // Write operation
            {
                int value;                                                                  // Declaring value variable
                if (sscanf(args, "%d %c %d", &gpio_num, &operation, &value) != 3) {         // Declaring user boundaries to cmd
                    increment_error(ERR_GPIO);                                              // Incrementing GPIO error
                    snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Invalid write command. Use: -gpio <number> w <0|1>\r\n");
            } else {
                GPIO_write(gpio_pin, value);                                                // Writing GPIO pin to user value
                snprintf(user_prompt, USER_PROMPT_SIZE, "\033[35mGPIO %d set to %d\033[0m\r\n", gpio_num, value);
                }                                                                           // Printing user input as user_prompt
            }
            break;                                                                          // Breaking operation
        case 't':                                                                           // Toggle operation
            {
                int value = GPIO_read(gpio_pin);                                            // Declaring user input as value read
                GPIO_write(gpio_pin, !value);                                               // Writing GPIO pin to user value
                snprintf(user_prompt, USER_PROMPT_SIZE, "\033[35mGPIO %d toggled to %d\033[0m\r\n", gpio_num, !value);
            }                                                                               // Printing user input
            break;                                                                          // Breaking operation
        default:                                                                            // Declaring default function
            increment_error(ERR_GPIO);                                                      // Incrementing GPIO error
            snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Invalid operation. Use r (read), w (write), or t (toggle).\r\n");
    }                                                                                       // User prompt for error
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing from uart
}
    // -callback
void callback_cmd(UART_Handle uart, char *args) {
    int index, count;                                                                       // Declaring index and count variables
    char payload[MAX_CMD_LENGTH];                                                           // Declaring payload array
    int i = 0;                                                                              // Declaring increment variable for loop
    if (args == NULL || *args == '\0' || *args == '\r' || *args == '\n') {                  // Check parameters for args
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n----- Callback Status -----\r\n");     // User prompt
        while (i < MAX_CALLBACKS) {                                                         // Continue if less than MAX_CALLBACKS
            char status[64];                                                                // Declaring status array
            if (callbacks[i].count != 0) {                                                  // Checking if count is not 0
                snprintf(status, sizeof(status), "Active (Count: %d)", callbacks[i].count); // User prompt for active callback(s)
            } else {                                                                        // Checking for inactive callback(s)
                snprintf(status, sizeof(status), "Inactive");                               // Printing inactive status to list
            }
            char *current_payload = peek_payload(&callbacks[i]);                            // Declaring payload detector
            snprintf(user_prompt, USER_PROMPT_SIZE, "Callback %d: %s\r\n"                   // User prompt with ternary operator (checks if payload isn't NULL)
                         "   Payload: %s\r\n", i, status, current_payload != NULL ? current_payload : "None");
            UART_write(uart, user_prompt, strlen(user_prompt));                             // Writing to UART
            i++;                                                                            // Increment count
            }
            return;                                                                         // Return from loop
    }
    while (args && *args == ' ') args++;                                                    // Skip leading spaces in args
    if (sscanf(args, "%d %d %[^\n]", &index, &count, payload) != 3) {                       // Checking for 3 arguments reading
        increment_error(ERR_CALLBACK);                                                      // Incrementing callback error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Invalid callback command. Use: -callback <index> <count> <payload>\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt to UART
        return;                                                                             // Return operation
    }
    if (index < 0 || index >= MAX_CALLBACKS) {                                              // Checking for index within valid range
        increment_error(ERR_CALLBACK);                                                      // Incrementing callback error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Invalid callback index. Use 0, 1, or 2.\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt to UART
        return;                                                                             // Returning operation
    }
    callbacks[index].count = 0;                                                             // Resetting count for callbacks
    callbacks[index].queue_front = 0;                                                       // Resetting front queue for callbacks
    callbacks[index].queue_rear = 0;                                                        // Resetting rear queue for callbacks
    callbacks[index].queue_size = 0;                                                        // Resetting queue size for callbacks
    callbacks[index].output_length = 0;                                                     // Resetting length for callbacks
    callbacks[index].output_position = 0;                                                   // Resetting position for callbacks
    callbacks[index].index = index;                                                         // Setting index for callbacks
    callbacks[index].count = count;                                                         // Setting new count for callbacks
    callbacks[index].uart = uart;                                                           // Setting uart for callbacks
    callbacks[index].is_executing = false;                                                  // Setting execution flag for callbacks
    callbacks[index].is_button_callback = (index == SW1_CALLBACK || index == SW2_CALLBACK); // Setting button callbacks
    enqueue_payload(&callbacks[index], payload);                                            // Declaring enqueue function to callbacks
    switch(index) {                                                                         // Setting up each callback index
        case TIMER_CALLBACK:                                                                // Timer callback set in timer_cmd function
            break;                                                                          // Breaking case
        case SW1_CALLBACK:                                                                  // SW1 index
            GPIO_setCallback(GPIO_SWITCH_1, sw1_callback);                                  // Setting callback function for SW1
            GPIO_enableInt(GPIO_SWITCH_1);                                                  // Enable interrupts for SW1
            break;                                                                          // Breaking case
        case SW2_CALLBACK:                                                                  // SW2 index
            GPIO_setCallback(GPIO_SWITCH_2, sw2_callback);                                  // Setting callback function for SW2
            GPIO_enableInt(GPIO_SWITCH_2);                                                  // Enable interrupts for SW2
            break;                                                                          // Breaking case
    }
    snprintf(user_prompt, USER_PROMPT_SIZE, "\033[35mCallback %d set with count %d and payload\033[0m: %s\r\n", index, count, payload);
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing user prompt to UART
}
    // -timer
void timer_cmd(UART_Handle uart, char *args) {
    int period;                                                                             // Declaring callback 0 period
    if (sscanf(args, "%d", &period) != 1) {                                                 // Scanning for value not set as 1
        increment_error(ERR_TIMER);                                                         // Incrementing timer error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Invalid timer command. Use: -timer <period_ms>\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt to UART
        return;                                                                             // Returning from loop
    }
    if (period < 5) {                                                                       // When callback 0 timer has value less than 5 ms
        increment_error(ERR_TIMER);                                                         // Incrementing timer error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Failed to open timer. Period must be at least 5 ms.\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt to UART
        return;                                                                             // Returning from loop
    }
    if (timer1Handle != NULL) {                                                             // When callback 0 timer not NULL
        Timer_stop(timer1Handle);                                                           // Stopping callback 0 timer
        Timer_close(timer1Handle);                                                          // Closing callback 0 timer
        timer1Handle = NULL;                                                                // Declaring callback 0 timer as NULL
    }
    Timer_Params params;                                                                    // Declare timer parameter structure
    Timer_Params_init(&params);                                                             // Initializing Timer_Params structure w/ default values
    params.period = period * 1000;                                                          // Converting mS to uS
    params.periodUnits = Timer_PERIOD_US;                                                   // Specify period to microseconds
    params.timerMode = Timer_CONTINUOUS_CALLBACK;                                           // Set timer to run continuously using callbacks
    params.timerCallback = timer_callback;                                                  // Match to Timer_CallBackFxn
    timer1Handle = Timer_open(CONFIG_TIMER_1, &params);                                     // Attempting to open timer with specified parameters
    if (timer1Handle == NULL) {                                                             // Checking for timer failure
        increment_error(ERR_TIMER);                                                         // Incrementing timer error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Failed to open timer.\r\n");
    } else {
        callbacks[TIMER_CALLBACK].is_large_interval = (period >= 100);                      // Considering intervals >= 100ms as large
        callbacks[TIMER_CALLBACK].output_length = 0;                                        // Resetting output buffer length
        callbacks[TIMER_CALLBACK].output_position = 0;                                      // Resetting output buffer position
        Timer_start(timer1Handle);                                                          // Starting callback 0 timer
        snprintf(user_prompt, USER_PROMPT_SIZE, "\033[35mTimer started with period %d ms.\033[0m\r\n", period);
    }
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing user prompt to UART
}
    // -reset
void reset_cmd(UART_Handle uart, char *args) {
    int index = -1;                                                                         // Declaring index variable as infinite value
    int i;                                                                                  // Declare i variable for loops
    GPIO_write(GPIO_LED_0, 0);                                                              // Writing LED 1 off
    GPIO_write(GPIO_LED_1, 0);                                                              // Writing LED 2 off
    GPIO_write(GPIO_LED_2, 0);                                                              // Writing LED 3 off
    GPIO_write(GPIO_LED_3, 0);                                                              // Writing LED 4 off
    if (sscanf(args, "%d", &index) == 1) {                                                  // Scanning arguments toggled as 1
        if (index >= 0 && index < MAX_CALLBACKS) {                                          // For index options
            callbacks[index].count = 0;                                                     // Resetting count for callbacks
            callbacks[index].queue_front = 0;                                               // Resetting front queue for callbacks
            callbacks[index].queue_rear = 0;                                                // Resetting rear queue for callbacks
            callbacks[index].queue_size = 0;                                                // Resetting queue size for callbacks
            if (index == TIMER_CALLBACK && timer1Handle != NULL) {                          // For inactive/expired timer
                Timer_stop(timer1Handle);                                                   // Stopping timer operations
            }
            snprintf(user_prompt, USER_PROMPT_SIZE, "\033[35mCallback %d has been reset & LEDs toggled off.\033[0m\r\n", index);
        } else {                                                                            // Error prompt for invalid index
            increment_error(ERR_RESET);                                                     // Incrementing reset error
            snprintf(user_prompt, USER_PROMPT_SIZE, "\033[31mError\033[0m: Invalid callback index.\r\n");
        }
    } else {                                                                                // Reset all callbacks & stop timer
        for (i = 0; i < MAX_CALLBACKS; i++) {                                               // 'i' declared outside loop
            callbacks[i].count = 0;                                                         // Resetting count for i callbacks
            callbacks[i].queue_front = 0;                                                   // Resetting front queue for i callbacks
            callbacks[i].queue_rear = 0;                                                    // Resetting rear queue for i callbacks
            callbacks[i].queue_size = 0;                                                    // Resetting queue size for i callbacks
        }
        if (timer1Handle != NULL) {                                                         // When timer is value greater than NULL
            Timer_stop(timer1Handle);                                                       // Stopping timer operations
        }
        for (i = 0; i < MAX_TICKERS; i++) {                                                 // For tickers detected active
            tickers[i].active = false;                                                      // Resetting active ticker flag
        }
        snprintf(user_prompt, USER_PROMPT_SIZE, "\033[35mAll callbacks, timers, & LEDs are deactivated.\033[0m\r\n");
    }
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing user prompt to UART
}
    // -ticker
void ticker_cmd(UART_Handle uart, char *args) {
    int i, index, initial_delay, period, count;                                             // Declaring ticker variables
    char payload[MAX_CMD_LENGTH];                                                           // Declaring ticker payload array
    if (args == NULL || *args == '\0' || *args == '\r' || *args == '\n') {                  // If ticker declared alone
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n----- Ticker Status -----\r\n");       // Printing ticker status prompt
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt to UART
        for (i = 0; i < MAX_TICKERS; i++) {                                                 // Checking all tickers
            Ticker *ticker = &tickers[i];                                                   // Point each ticker
            if (ticker->active) {                                                           // If ticker is active
                snprintf(user_prompt, USER_PROMPT_SIZE,                                     // Printing active ticker status
                         "Ticker %d: Active\r\n"
                         "  Initial Delay: %d\r\n"
                         "  Period:        %d\r\n"
                         "  Count:         %d\r\n"
                         "  Payload:       %s\r\n",
                         i, ticker->initial_delay, ticker->period, ticker->count, ticker->payload);
            } else {                                                                        // If ticker is inactive
                snprintf(user_prompt, USER_PROMPT_SIZE, "Ticker %d: Inactive\r\n", i);
            }
            UART_write(uart, user_prompt, strlen(user_prompt));                             // Writing to UART
        }
        return;                                                                             // Return from loop
    }
                                                                                            // For incorrect ticker command format
    if (sscanf(args, "%d %d %d %d %[^\n]", &index, &initial_delay, &period, &count, payload) != 5) {
        increment_error(ERR_TICKER);                                                        // Incrementing ticker error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Invalid ticker command format.\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt to UART
        return;                                                                             // Return from loop
    }
    if (index < 0 || index >= MAX_TICKERS) {                                                // For incorrect ticker index input
        increment_error(ERR_TICKER);                                                        // Incrementing ticker error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Invalid ticker index.\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt to UART
        return;                                                                             // Return from loop
    }
    Ticker *ticker = &tickers[index];                                                       // Getting pointer to specified ticker index
    ticker->active = true;                                                                  // Activate ticker
    ticker->initial_delay = initial_delay;                                                  // Setting initial delay
    ticker->period = period;                                                                // Period for subsequent executions
    ticker->count = count;                                                                  // Setting execution count
    strncpy(ticker->payload, payload, MAX_CMD_LENGTH - 1);                                  // Copying payload for ticker
    ticker->payload[MAX_CMD_LENGTH - 1] = '\0';                                             // Ensuring NULL of payload string
    ticker->next_execution = ticker_time + initial_delay;                                   // Calculate first execution for ticker
    ticker->uart = uart;                                                                    // Set UART handle for ticker
    snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nTicker %d set with...\r\n initial delay: %d\r\n period: %d\r\n count: %d\r\n payload: %s\r\n",
             index, initial_delay, period, count, payload);                                 // Printing active ticker prompt
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing user prompt to UART
}
    // -reg
void reg_cmd(UART_Handle uart, char *args) {                                                // Declaring operation array to dest, src1 and src2 lengths
    char op[10], dest[MAX_REG_NAME_LENGTH], src1[MAX_REG_NAME_LENGTH], src2[MAX_REG_NAME_LENGTH];
    if (args == NULL || *args == '\0' || *args == '\r' || *args == '\n') {                  // Reading all registers if no values after command
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n----- Register Status -----\r\n");     // Print status prompt
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt to UART
        int i;                                                                              // Declaring loop variable
        for (i = 0; i < NUM_REGISTERS; i++) {                                               // For all usable registers
            snprintf(user_prompt, USER_PROMPT_SIZE, "%s: %d\r\n", reg_names[i], registers[i]);
            UART_write(uart, user_prompt, strlen(user_prompt));                             // Writing user prompt to UART
        }
        return;                                                                             // Return from loop
    }
    int num_args = sscanf(args, "%s %s %s %s", op, dest, src1, src2);                       // Assigning variables to string characters
    if (num_args < 2) {                                                                     // If two or more inputs to command arguments
        increment_error(ERR_REG);                                                           // Incrementing error count
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Invalid register command format.\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt to UART
        return;                                                                             // Return from loop
    }
    int32_t value1, value2;                                                                 // Declaring values to INC/DEC operations
        if (strcmp(op, "INC") == 0 || strcmp(op, "DEC") == 0) {                             // If INC/DEC operation is detected
            value1 = get_reg_value(dest);                                                   // Treating the dest as src1 for INC/DEC
            value2 = 0;                                                                     // Declaring 0 (Since not used for INC/DEC)
        } else {                                                                            // For src2 input usage
            value1 = parse_operand(src1);                                                   // Declaring parse to src1 variable
            value2 = (num_args == 4) ? parse_operand(src2) : 0;                             // Declaring src2 operations
        }
        int32_t result = 0;                                                                 // Declaring result as 0 for other operations
        if (strcmp(op, "MOV") == 0) {                                                       // Declaring MOV operation
            result = value1;
        } else if (strcmp(op, "XCG") == 0) {                                                // Declaring XCG operation
            result = get_reg_value(dest);
            set_reg_value(src1, result);
            result = value1;
        } else if (strcmp(op, "INC") == 0) {                                                // Declaring INC operation
            result = value1 + 1;
        } else if (strcmp(op, "DEC") == 0) {                                                // Declaring DEC operation
            result = value1 - 1;
        } else if (strcmp(op, "ADD") == 0) {                                                // Declaring ADD operation
            result = value1 + value2;
        } else if (strcmp(op, "SUB") == 0) {                                                // Declaring SUB operation
            result = value1 - value2;
        } else if (strcmp(op, "NEG") == 0) {                                                // Declaring NEG operation
            result = -value1;
        } else if (strcmp(op, "NOT") == 0) {                                                // Declaring NOT operation
            result = ~value1;
        } else if (strcmp(op, "AND") == 0) {                                                // Declaring AND operation
            result = value1 & value2;
        } else if (strcmp(op, "IOR") == 0) {                                                // Declaring IOR operation
            result = value1 | value2;
        } else if (strcmp(op, "XOR") == 0) {                                                // Declaring XOR operation
            result = value1 ^ value2;
        } else if (strcmp(op, "MUL") == 0) {                                                // Declaring MUL operation
            result = value1 * value2;
        } else if (strcmp(op, "DIV") == 0) {                                                // Declaring DIV operation
            if (value2 == 0) {                                                              // Declaring division by 0 error
                increment_error(ERR_REG);                                                   // Increment error count
                snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Division by zero.\r\n");
                UART_write(uart, user_prompt, strlen(user_prompt));                         // Writing user prompt to UART
                return;                                                                     // Return from loop
            }
            result = value1 / value2;
        } else if (strcmp(op, "REM") == 0) {                                                // Declaring REM operations
            if (value2 == 0) {                                                              // Declaring division by 0 error
                increment_error(ERR_REG);                                                   // Increment error count
                snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Division by zero.\r\n");
                UART_write(uart, user_prompt, strlen(user_prompt));                         // Writing user prompt to UART
                return;                                                                     // Return from loop
            }
            result = value1 % value2;
        } else if (strcmp(op, "MAX") == 0) {                                                // Declaring MAX operations
            result = (value1 > value2) ? value1 : value2;
        } else if (strcmp(op, "MIN") == 0) {                                                // Declaring MIN operations
            result = (value1 < value2) ? value1 : value2;
        } else {                                                                            // Declaring invalid operation
            increment_error(ERR_REG);                                                       // Increment error count
            snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Unknown operation %s.\r\n", op);
            UART_write(uart, user_prompt, strlen(user_prompt));                             // Writing user prompt to UART
            return;                                                                         // Return from loop
        }
        set_reg_value(dest, result);                                                        // Using dest as register to modify (for INC/DEC)
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nOperation completed. %s = %d\r\n", dest, result);
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt to UART
}
    // -script
void script_cmd(UART_Handle uart, char *args) {
    int line_number;                                                                        // Declaring line number variable
    char payload[MAX_SCRIPT_LINE_LENGTH];                                                   // Declaring payload array to script length
    if (args != NULL && strncmp(args, " clear", 6) == 0) {                                  // Handling "clear" command
        int i;                                                                              // Declaring loop variable
        while (!Queue_empty(scriptQueue)) {                                                 // Clearing the script queue first
            Queue_Elem *elem = Queue_get(scriptQueue);                                      // Declaring queue element variable
            free((ScriptQueueElement *)elem);                                               // Freeing the script queue elements
        }
        for (i = 0; i < SCRIPT_SIZE; i++) {                                                 // Clearing all script lines
            script_space[i][0] = '\0';                                                      // Clearing each line
        }
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nEntire script space cleared and script execution halted.\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from UART
        return;                                                                             // Return from loop
    }
    if (args == NULL || *args == '\0') {                                                    // Display entire script space if no arguments
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n----- Script Space -----\r\n");        // Script space prompt
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from UART
        int i;                                                                              // Declaring loop variable
        for (i = 0; i < SCRIPT_SIZE; i++) {                                                 // Declaring script space scanner
            if (script_space[i][0] != '\0') {                                               // Omitting empty script spaces
                snprintf(user_prompt, USER_PROMPT_SIZE, "%2d: %s\r\n", i, script_space[i]); // Printing script spaces that are full
                UART_write(uart, user_prompt, strlen(user_prompt));                         // Writing from UART
            }
        }
        return;                                                                             // Return from loop
    }
    char *rest_of_line;                                                                     // Parsing arguments
    int args_read = sscanf(args, "%d %[^\n]", &line_number, payload);                       // Declaring read arguments as scan functions
    if (line_number < 0 || line_number >= SCRIPT_SIZE) {                                    // Validating line number
        increment_error(ERR_SCRIPT);                                                        // Incrementing script error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Invalid script line number.\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from UART
        return;                                                                             // Return from loop
    }
    if (args_read == 1) {                                                                   // Handle different command cases
        if (script_space[line_number][0] != '\0') {                                         // Display single line
            snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nLine %d: %s\r\n", line_number, script_space[line_number]);
        } else {                                                                            // For empty script lines
            snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nLine %d: <empty>\r\n", line_number);
        }
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from UART
    }
    else if (args_read >= 2) {                                                              // For read arguments greater than or 2
        rest_of_line = args;                                                                // Finding first non-space character after the line number
        while (*rest_of_line && (*rest_of_line < '0' || *rest_of_line > '9')) rest_of_line++;
        while (*rest_of_line && (*rest_of_line >= '0' && *rest_of_line <= '9')) rest_of_line++;
        while (*rest_of_line && *rest_of_line == ' ') rest_of_line++;
        if (strcmp(rest_of_line, "x") == 0 || strcmp(rest_of_line, "c") == 0) {             // Checking if it's an execution command
            if (rest_of_line[0] == 'c') {                                                   // For if it's a clear command
                script_space[line_number][0] = '\0';                                        // Clearing line
                snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nScript line %d cleared.\r\n", line_number);
                UART_write(uart, user_prompt, strlen(user_prompt));                         // Writing from UART
            } else {                                                                        // Executing from declaring line
                if (script_space[line_number][0] == '\0') {                                 // Execute from this line
                    increment_error(ERR_SCRIPT);                                            // Incrementing script error
                    snprintf(user_prompt, USER_PROMPT_SIZE,                                 // Printing error prompt
                            "\r\n\033[31mError\033[0m: Cannot execute empty script line.\r\n");
                    UART_write(uart, user_prompt, strlen(user_prompt));                     // Writing from UART
                } else {                                                                    // Queue script execution request
                    execute_script(uart, line_number);                                      // Declaring script execution to UART
                    snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nScript execution started.\r\n");
                    UART_write(uart, user_prompt, strlen(user_prompt));                     // Writing from UART
                }
            }
        } else {                                                                            // Add dash if not present and not a remark
            if (rest_of_line[0] != '-' && strncmp(rest_of_line, "rem", 3) != 0) {           // Store new script line - handle the case whether or not it starts with a dash
                snprintf(script_space[line_number], MAX_SCRIPT_LINE_LENGTH - 1, "-%s", rest_of_line);
            } else {                                                                        // Copying from script space for execution
                strncpy(script_space[line_number], rest_of_line, MAX_SCRIPT_LINE_LENGTH - 1);
            }
            script_space[line_number][MAX_SCRIPT_LINE_LENGTH - 1] = '\0';                   // Tabs to script space
            snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nScript line %d set to: %s\r\n",    // Printing script line prompt
                    line_number, script_space[line_number]);                                // Printing script line prompt
            UART_write(uart, user_prompt, strlen(user_prompt));                             // Writing from UART
        }
    }
}
    // -rem
void rem_cmd(UART_Handle uart, char *args) {
    snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nRemark acknowledged.\r\n");                // Printing successful remark prompt
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Do nothing for remarks, only acknowledge!
}
    // -if
void if_cmd(UART_Handle uart, char *args) {
    if (args == NULL || *args == '\0') {                                                    // For invalid format
        increment_error(ERR_IF);                                                            // Increment conditional error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Invalid if command format.\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from uart
        return;                                                                             // Return from loop
    }
    snprintf(user_prompt, USER_PROMPT_SIZE, "Debug: if command args='%s'\r\n", args);       // Debug print for input args
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing from uart
    char operand1[32], condition[8], operand2[32];                                          // Declaring operands and condition arrays
    char destt[MAX_CMD_LENGTH] = "", destf[MAX_CMD_LENGTH] = "";                            // Declaring destination variables
    char *question_mark = strchr(args, '?');                                                // Parsing conditional part '?'
    char *colon = strchr(args, ':');                                                        // Parsing conditional part ':'
    if (!question_mark || !colon) {                                                         // For missing ? or :
        increment_error(ERR_IF);                                                            // Increment conditional error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Invalid if syntax. Missing ? or :\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from uart
        return;                                                                             // Return from loop
    }
    char cond_part[MAX_CMD_LENGTH];                                                         // Creating a temporary copy of the condition part for parsing
    size_t cond_len = question_mark - args;                                                 // ^
    strncpy(cond_part, args, cond_len);                                                     // ^
    cond_part[cond_len] = '\0';                                                             // ^
    snprintf(user_prompt, USER_PROMPT_SIZE, "Debug: condition part='%s'\r\n", cond_part);   // Debugging print for condition part
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing from uart
    char *cond_symbol = NULL;                                                               // Finding condition symbol (>, =, <)
    if (strchr(cond_part, '>')) cond_symbol = strchr(cond_part, '>');                       // For '>'
    else if (strchr(cond_part, '=')) cond_symbol = strchr(cond_part, '=');                  // For '='
    else if (strchr(cond_part, '<')) cond_symbol = strchr(cond_part, '<');                  // For '<'
    if (!cond_symbol) {                                                                     // For missing conditional symbol
        increment_error(ERR_IF);                                                            // Increment conditional error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Missing condition operator (>, =, <)\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from uart
        return;                                                                             // Returning from loop
    }
    size_t op1_len = cond_symbol - cond_part;                                               // Extract operand1 (everything before the condition symbol)
    strncpy(operand1, cond_part, op1_len);                                                  // ^
    operand1[op1_len] = '\0';                                                               // ^
    condition[0] = *cond_symbol;                                                            // Extract the condition (0)
    condition[1] = '\0';                                                                    // Extract the condition (1)
    strcpy(operand2, cond_symbol + 1);                                                      // Extract operand2 (everything after the condition symbol until detected ?)
    char *start, *end;                                                                      // Trimming whitespace from operands
    start = operand1;                                                                       // Trim operand1
    while (*start == ' ') start++;                                                          // Incrementing start
    end = start + strlen(start) - 1;                                                        // Declaring end variable
    while (end > start && *end == ' ') end--;                                               // Decrementing end while greater than start variable
    *(end + 1) = '\0';                                                                      // Tabbing pointer
    strcpy(operand1, start);                                                                // Copying operand to start variable
    start = operand2;                                                                       // Trim operand2 (may cause errors but hasn't from how i've tested)
    while (*start == ' ') start++;                                                          // Incrementing start
    end = start + strlen(start) - 1;                                                        // Declaring end variable
    while (end > start && *end == ' ') end--;                                               // Decrementing end while greater than start variable
    *(end + 1) = '\0';                                                                      // Tab pointer
    strcpy(operand2, start);                                                                // Copying operand to start variable
    char *true_start = question_mark + 1;                                                   // Extract true destination
    while (*true_start == ' ') true_start++;                                                // Incrementing true start
    char *true_end = colon;                                                                 // Declaring true end variable pointer
    while (true_end > true_start && *(true_end - 1) == ' ') true_end--;                     // Decrementing true end variable
    if (true_end > true_start) {                                                            // While end is greater than start
        strncpy(destt, true_start, true_end - true_start);                                  // Copy destination from subtracted end to start
        destt[true_end - true_start] = '\0';                                                // Tab destination
    }
    char *false_start = colon + 1;                                                          // Extract true destination
    while (*false_start == ' ') false_start++;                                              // Incrementing false start
    if (strlen(false_start) > 0) {                                                          // While false start is greater than 0
        strcpy(destf, false_start);                                                         // Copying destination from false start
    }
                                                                                            // Debug prints
    snprintf(user_prompt, USER_PROMPT_SIZE, "Debug: parsed: operand1='%s', condition='%s', operand2='%s'\r\n",
             operand1, condition, operand2);
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing from uart
    snprintf(user_prompt, USER_PROMPT_SIZE, "Debug: destinations: true='%s', false='%s'\r\n",
             destt, destf);
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing from uart
    bool result = evaluate_condition(uart, operand1, condition, operand2);                  // Evaluate condition
    if (result) {                                                                           // For detected result
        execute_conditional_dest(uart, destt, true);                                        // Execute appropriate destination
    } else {                                                                                // For undetected result
        execute_conditional_dest(uart, destf, false);                                       // No result execution
    }
}
    // -uart
void uart_cmd(UART_Handle uart, char *args) {
    char *command = args;                                                                   // Declaring command pointer as parsed arguments in uart7
    while (*command == ' ') command++;                                                      // Skipping leading spaces
    if (*command != '\0') {                                                                 // Success path
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nPayload sent through UART7: %s\r\n", command);
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from uart
        strcat(command, "\n");                                                              // Adding newline and send command
        UART_write(uart7, command, strlen(command));                                        // Writing command from uart7
        return;                                                                             // Return from loop
    } else {                                                                                // Error path
        increment_error(ERR_UART);                                                          // Incrementing uart error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Must attach a payload to -uart\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from uart
        return;                                                                             // Return from loop
    }
}
    // -sine
void sine_cmd(UART_Handle uart, char *args) {
    if (args == NULL || *args == '\0') {                                                    // Nothing detected after -sine
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nUsage: -sine FREQ\r\n"                 // Printing -sine usage prompts
                "FREQ: Frequency in Hz (0 to stop)\r\n"
                "Maximum frequency: %d Hz (Nyquist limit)\r\n", sample_rate/2);
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from uart
        return;                                                                             // Return from loop
    }
    uint32_t freq;                                                                          // Declaring 32bit frequency variable
    if (sscanf(args, "%u", &freq) != 1) {                                                   // For error detected in frequency scan
        increment_error(ERR_SINE);                                                          // Incrementing sine error
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Invalid frequency format\r\n");
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing from uart
        return;                                                                             // Return from loop
    }
    if (freq == 0) {                                                                        // For inactive/halted frequency
        sine_active = false;                                                                // Declaring active sine flag as false
        if (timer0Handle != NULL) {                                                         // If timer handler is not empty
            Timer_stop(timer0Handle);                                                       // Stopping timer handler
        }
        uint16_t dac_word = 0x2000;                                                         // Setting DAC to mid-scale value
        SPI_Transaction transaction;                                                        // Declaring SPI transaction
        transaction.count = 1;                                                              // Declaring transaction count
        transaction.txBuf = &dac_word;                                                      // Declaring transaction TX pin
        transaction.rxBuf = NULL;                                                           // Declaring transaction RX pin
        SPI_transfer(spiHandle, &transaction);                                              // Declaring SPI transfer for handler to transaction
        Task_sleep(1);                                                                      // Brief delay before disabling amp
        GPIO_write(GPIO_PD4, 0);                                                            // Disable audio amp
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\nSine wave has stopped generating...\r\n");
    } else {                                                                                // For invalid frequency too large
        if (freq > sample_rate/2) {                                                         // If frequency exceeds Nyquist limit
            increment_error(ERR_SINE);                                                      // Incrementing sine error
            snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Frequency %u Hz exceeds Nyquist limit (%u Hz)\r\n", freq, sample_rate/2);
            UART_write(uart, user_prompt, strlen(user_prompt));                             // Writing from uart
            return;                                                                         // Return from loop
        }
        set_sine_frequency(freq);                                                           // Declaring frequency to sine set function
        if (!sine_active) {                                                                 // If sine is inactive
            if (timer0Handle == NULL) {                                                     // If timer handler is NULL
                Timer_Params params;                                                        // Declaring timer parameters
                Timer_Params_init(&params);                                                 // Initializing timer paramters
                params.period = 125;                                              // Declaring 8000 period
                params.periodUnits = Timer_PERIOD_US;                                       // Declaring period
                params.timerMode = Timer_CONTINUOUS_CALLBACK;                               // Declaring timer mode as forever callback
                params.timerCallback = sine_timer_callback;                                 // Declaring timer callback to callback function
                timer0Handle = Timer_open(CONFIG_TIMER_0, &params);                         // Declaring timer handler as opener to parameters
                if (timer0Handle == NULL) {                                                 // If timer is NULL
                    increment_error(ERR_SINE);                                              // Incrementing sine error
                    snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[31mError\033[0m: Failed to initialize timer\r\n");
                    UART_write(uart, user_prompt, strlen(user_prompt));                     // Writing from UART
                    return;                                                                 // Return from loop
                }
            }
            GPIO_write(GPIO_PD4, 1);                                                        // Enable audio amp first
            Task_sleep(1);                                                                  // Brief delay for amp to stabilize
            uint16_t dac_word = 0x2000;                                                     // Starting with mid-scale DAC value
            SPI_Transaction transaction;                                                    // Declaring SPI transaction
            transaction.count = 1;                                                          // Declaring transaction count
            transaction.txBuf = &dac_word;                                                  // Declaring transaction TX buffer to DAC
            transaction.rxBuf = NULL;                                                       // Declaring transaction RX buffer as NULL
            SPI_transfer(spiHandle, &transaction);                                          // Declaring SPI transfer to transaction
            Task_sleep(1);                                                                  // Another brief delay
            sine_active = true;                                                             // Declaring sine as active
            Timer_start(timer0Handle);                                                      // Starting timer handler for sine wave
        }
        snprintf(user_prompt, USER_PROMPT_SIZE, "\r\n\033[35mGenerating %u Hz sine wave! :) \033[0m\r\n", freq);
    }
    UART_write(uart, user_prompt, strlen(user_prompt));                                     // Writing from UART
}

/* Parsing command(s) */
    // Valid commands
void valid_cmd(UART_Handle uart, char *command) {                                           // Declaring function to validate/execute commands
        // -about
    if (strcmp(command, "-about") == 0) {                                                   // Check for -about command
        about_cmd(uart);                                                                    // Executing -about
    }
        // -help
    else if (strncmp(command, "-help", 5) == 0) {                                           // Check for -help command
        char *subcmd = NULL;                                                                // Declaring subcommand pointer variable
        if (strcmp(command, "-help") != 0) {                                                // Checking if help command consists of subcmd
            subcmd = command + 6;                                                           // Declaring subcommand limit
        }
        help_cmd(uart, subcmd);                                                             // Declaring -help function w/ subcommand
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing user prompt from -help into UART
    }
        // -clear
    else if (strcmp(command, "-clear") == 0) {                                              // Check for -clear command
        char clearScreen[10];                                                               // Buffer declared for ANSI esc sequence for clearing
        snprintf(clearScreen, sizeof(clearScreen), "\033[2J\033[H");                        // Formatting screen clear then storing in buffer
        UART_write(uart, clearScreen, strlen(clearScreen));                                 // Send -clear command to UART
        const char *promptMsg = "        \033[35m.....Terminal Cleared.....\033[0m\r\n";    // User prompt for successful terminal clearing
        UART_write(uart, promptMsg, strlen(promptMsg));                                     // Displaying cleared message to terminal
    }
        // -print
    else if (strncmp(command, "-print", 6) == 0) {                                          // Declaring print command to terminal
        char *substring = NULL;                                                             // Declaring substring to print command
        if (strcmp(command, "-print") != 0) {                                               // Extracting everything after print command
            substring = command + 7;                                                        // Declaring substring limit to -print command
        }
        print_cmd(uart, substring);                                                         // Declaring -print to UART substring set
        UART_write(uart, user_prompt, strlen(user_prompt));                                 // Writing UART into user prompt variable
    }
        // -memr
    else if (strncmp(command, "-memr", 5) == 0) {                                           // Handling -memr command
            char *address_str = command + 5;                                                // Extracting address string w/ moving pointer
            while (*address_str == ' ') address_str++;                                      // Skipping any additional spaces
            if (*address_str == '\0') {                                                     // Checking for address
                const char *error_msg = "\r\n\033[33mWarning\033[0m: Missing address for -memr command.\r\n";
                UART_write(uart, error_msg, strlen(error_msg));                             // Writing error prompt to UART
            } else {
                memr_cmd(uart, address_str);                                                // Address string found and process
                UART_write(uart, user_prompt, strlen(user_prompt));                         // Writing user_prompt to UART
            }
        }
        // -error
    else if (strcmp(command, "-error") == 0) {                                              // Handling -error command
        error_cmd(uart);                                                                    // Processing to UART
    }
        // -gpio
    else if (strncmp(command, "-gpio", 5) == 0) {                                           // Handling -gpio command
        gpio_cmd(uart, command + 5);                                                        // Processing to UART
    }
        // -timer
    else if (strncmp(command, "-timer", 6) == 0) {                                          // Handling -timer command
        timer_cmd(uart, command + 6);                                                       // Processing to UART
    }
        // -callback
    else if (strncmp(command, "-callback", 9) == 0) {                                       // Handling -timer command
        char *args = command + 9;                                                           // Declaring args pointer for processing
        callback_cmd(uart, args);                                                           // Processing to UART
    }
        // -reset
    else if (strncmp(command, "-reset", 6) == 0) {                                          // Handling -reset command
        reset_cmd(uart, command + 6);                                                       // Processing to UART
    }
        // -ticker
    else if (strncmp(command, "-ticker", 7) == 0) {                                         // Handling -ticker command
        ticker_cmd(uart, command + 7);                                                      // Processing to UART
    }
        // -reg
    else if (strncmp(command, "-reg", 4) == 0) {                                            // Handling -ticker command
        reg_cmd(uart, command + 4);                                                         // Processing to UART
    }
        // -script
    else if (strncmp(command, "-script", 7) == 0) {                                         // Handling -script command
        script_cmd(uart, command + 7);                                                      // Processing to UART
    }
        // -rem
    else if (strncmp(command, "-rem", 4) == 0) {                                            // Handling -rem command
        rem_cmd(uart, command + 4);                                                         // Processing to UART
    }
        // -if
    else if (strncmp(command, "-if", 3) == 0) {                                             // Handling -if command
        if_cmd(uart, command + 3);                                                          // Processing to UART
    }
        // -uart
    else if (strncmp(command, "-uart", 5) == 0) {                                           // Handling -uart command
        uart_cmd(uart, command + 5);                                                        // Processing to UART
    }
        // -sine
    else if (strncmp(command, "-sine", 5) == 0) {                                           // Handling -sine command
        sine_cmd(uart, command + 5);                                                        // Processing to UART
    }
        // N/A
    else {
        increment_error(ERR_UNKNOWN_CMD);                                                   // Increment error prompt
        char unknown_str[128];                                                              // Declaring buffer for unknown user input
        snprintf(unknown_str, sizeof(unknown_str), "\r\n\033[31mError\033[0m: Unknown command %s.\r\n", command);
        UART_write(uart, unknown_str, strlen(unknown_str));                                 // Sending unknown response to UART
    }
}
