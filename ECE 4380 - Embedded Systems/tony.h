/*
 * tony.h
 *
 * Christian J. Maldonado
 *
 */

/* Declaring main header file */
#ifndef TONY_H_
#define TONY_H_

/* Declaring driver headers */
#include <ti/drivers/UART.h>                                                                // For UART_Handle
#include <ti/drivers/Timer.h>                                                               // For Timer_Handle
#include <ti/sysbios/knl/Task.h>                                                            // For Task types and UArg
#include <ti/sysbios/knl/Queue.h>                                                           // For Queue types
#include <ti/drivers/SPI.h>                                                                 // For SPI_Handle

/* Declaring macro variables */
#define HISTORY_SIZE 10                                                                     // Defining size to history cycler
#define ESC '\x1B'                                                                          // Defining ESC key to shortcut variable
#define BUFFER_SIZE 320                                                                     // Defining size to input buffer
#define MAX_CMD_LENGTH 318                                                                  // Defining max command length
#define QUEUE_SIZE 32                                                                       // Defining timer queue size
#define MAX_OUTPUT_CHUNK 64                                                                 // Defining max chuck for callback overflow
#define USER_PROMPT_SIZE 1000                                                               // Defining terminal print length
#define USER "Christian J. Maldonado (\033[31mChris\033[0m)"                                // Defining user name
#define VERSION_SUB "v12.1"                                                                 // Defining version
#define ASSIGNMENT "Assignment X"                                                           // Defining assignment
#define ERR_UNKNOWN_CMD 0                                                                   // Defining unknown command
#define ERR_BUFFER_OVERFLOW 1                                                               // Defining overflow error
#define ERR_INVALID_ADDRESS 2                                                               // Defining address error
#define ERR_GPIO 3                                                                          // Defining GPIO error
#define ERR_TIMER 4                                                                         // Defining timer error
#define ERR_CALLBACK 5                                                                      // Defining callback error
#define ERR_RESET 6                                                                         // Defining reset error
#define ERR_TICKER 7                                                                        // Defining ticker error
#define ERR_REG 8                                                                           // Defining register error
#define ERR_SCRIPT 9                                                                        // Defining script error
#define ERR_IF 10                                                                           // Defining if conditions error
#define ERR_UART 11                                                                         // Defining UART7 error
#define ERR_SINE 12                                                                         // Defining sine errors
#define ERR_NET 13                                                                          // Defining UDP errors
#define ERROR_COUNT_SIZE 14                                                                 // Defining count size to error prompts
#define GPIO_LED_0 CONFIG_GPIO_LED_0                                                        // Defining GPIO LED IO 1
#define GPIO_LED_1 CONFIG_GPIO_LED_1                                                        // Defining GPIO LED IO 2
#define GPIO_LED_2 CONFIG_GPIO_LED_2                                                        // Defining GPIO LED IO 3
#define GPIO_LED_3 CONFIG_GPIO_LED_3                                                        // Defining GPIO LED IO 4
#define GPIO_PK5 CONFIG_GPIO_PK5                                                            // Defining GPIO PK5 IO
#define GPIO_PD4 CONFIG_GPIO_PD4                                                            // Defining GPIO PD4 IO
#define GPIO_SWITCH_1 CONFIG_GPIO_SW1                                                       // Defining GPIO Switch IO 1 (SW1)
#define GPIO_SWITCH_2 CONFIG_GPIO_SW2                                                       // Defining GPIO Switch IO 2 (SW2)
#define MAX_CALLBACKS 3                                                                     // Defining values to Callback
#define TIMER_CALLBACK 0                                                                    // Defining Timer Callback
#define SW1_CALLBACK 1                                                                      // Defining SW1 Callback
#define SW2_CALLBACK 2                                                                      // Defining SW2 Callback
#define MAX_TICKERS 16                                                                      // Defining max tickers
#define TICKER_TIMER TICKER_TIMER_0                                                         // Defining ticker timer from system configuration
#define NUM_REGISTERS 32                                                                    // Defining register count
#define MAX_REG_NAME_LENGTH 10                                                              // Defining max register length
#define SCRIPT_SIZE 64                                                                      // Defining script size length
#define MAX_SCRIPT_LINE_LENGTH 318                                                          // Defining max script input length
#define SCRIPT_QUEUE_SIZE 32                                                                // Defining script queue size length
#define SCRIPT_TASK_STACK_SIZE 2048                                                         // Defining script task stack size length
#define SINE_TABLE_SIZE 256                                                                 // Defining sine table size
#define TWO_PI 6.28318530718                                                                // Defining PI*2 as variable
#define DAC_MAX_VALUE 16383                                                                 // Defining 14-bit DAC (2^14 - 1)

/* Declaring structures */
    // Callback
typedef struct {
    int index;                                                                              // Declaring struct index
    int count;                                                                              // Declaring struct count
    int queue_front;                                                                        // Declaring front queue struct
    int queue_rear;                                                                         // Declaring queue rear struct
    int queue_size;                                                                         // Declaring struct queue size
    int output_position;                                                                    // Declaring output position struct
    int output_length;                                                                      // Declaring output length struct
    char payload[QUEUE_SIZE][MAX_CMD_LENGTH];                                               // Declaring struct command array
    char output_buffer[USER_PROMPT_SIZE];                                                   // Declaring output buffer to terminal sized array
    bool is_executing;                                                                      // Declaring active execution bool
    bool is_button_callback;                                                                // Declaring active button execution bool
    bool is_large_interval;                                                                 // Declaring large timer value bool
    UART_Handle uart;                                                                       // Processing UART to struct
} Callback;                                                                                 // Declaring struct name as Callback
    // Ticker
typedef struct {
    bool active;                                                                            // Declaring bool variable for status
    uint32_t initial_delay;                                                                 // Declaring initial time variable
    uint32_t period;                                                                        // Declaring period variable
    int32_t count;                                                                          // Declaring count variable
    char payload[MAX_CMD_LENGTH];                                                           // Declaring payload array
    uint32_t next_execution;                                                                // Declaring next execution variable
    UART_Handle uart;                                                                       // Declaring UART to struct
} Ticker;                                                                                   // Declaring struct name as Ticker
    // Script
typedef struct {
    int line_number;                                                                        // Declaring variable for script line number
    char command[MAX_CMD_LENGTH];                                                           // Declaring command array by command length
    UART_Handle uart;                                                                       // Declaring UART to struct
} ScriptCommand;                                                                            // Declaring struct name as ScriptCommand
typedef struct {
    Queue_Elem elem;                                                                        // Declaring element queue variable
    ScriptCommand cmd;                                                                      // Declaring script command variable
} ScriptQueueElement;                                                                       // Declaring struct name as ScriptQueueElement

/* Declaring function operations */
    // Input processing
void processUserInput(UART_Handle uart);                                                    // Declaring function for input processing
    // Command(s)
void valid_cmd(UART_Handle uart, char *command);                                            // Declaring function to validate/execute commands
void about_cmd(UART_Handle uart);                                                           // Declaring function for -about
void help_cmd(UART_Handle uart, char *subcmd);                                              // Declaring function for -help
void print_cmd(UART_Handle uart, char *substring);                                          // Declaring function for -print
void memr_cmd(UART_Handle uart, char *address_str);                                         // Declaring function for -memr
void error_cmd(UART_Handle uart);                                                           // Declaring function for -error
void increment_error(int error_type);                                                       // Declaring function for incrementing -error
void gpio_cmd(UART_Handle uart, char *args);                                                // Declaring function for -gpio
void timer_cmd(UART_Handle uart, char *args);                                               // Declaring function for -timer
void callback_cmd(UART_Handle uart, char *args);                                            // Declaring function for -callback
void reset_cmd(UART_Handle uart, char *args);                                               // Declaring function for -reset
void ticker_cmd(UART_Handle uart, char *args);                                              // Declaring function for -ticker
void reg_cmd(UART_Handle uart, char *args);                                                 // Declaring function for -reg
void script_cmd(UART_Handle uart, char *args);                                              // Declaring function for -script
void rem_cmd(UART_Handle uart, char *args);                                                 // Declaring function for -rem
void if_cmd(UART_Handle uart, char *args);                                                  // Declaring function for -if
void uart_cmd(UART_Handle uart, char *args);                                                // Declaring function for -uart
void sine_cmd(UART_Handle uart, char *args);                                                // Declaring function for -sine
    // Callback operations
void timer_callback(Timer_Handle handle, int_fast16_t status);                              // Declaring function for timer callback
void sw1_callback(uint_least8_t index);                                                     // Declaring function for SW1 callback
void sw2_callback(uint_least8_t index);                                                     // Declaring function for SW2 callback
    // Ticker operations
void init_ticker_timer();                                                                   // Declaring function for ticker timer
void ticker_timer_callback(Timer_Handle handle, int_fast16_t status);                       // Declaring function for ticker callback
void execute_ticker(int index);                                                             // Declaring function for ticker executions
    // Register operations
void init_registers();                                                                      // Declaring function for register initialization to UART
void set_reg_value(const char *reg_name, int32_t value);                                    // Declaring function for setting register
int32_t get_reg_value(const char *reg_name);                                                // Declaring function for getting register
int32_t parse_operand(const char *operand);                                                 // Declaring function for register parse operator
    // Script operations
void init_script_system(void);                                                              // Declaring function to initialize the script system
void script_task(UArg arg0, UArg arg1);                                                     // Declaring function to handle script tasks
bool queue_script_command(const char *command);                                             // Declaring function for queue script command processing
void execute_script(UART_Handle uart, int start_line);                                      // Declaring function to execute script
    // Conditional operations                                                               // Declaring function for conditional evaluation
bool evaluate_condition(UART_Handle uart, const char *operand1, const char *operand2, const char *condition);
void execute_conditional_dest(UART_Handle uart, const char *dest, bool is_true);            // Declaring function for conditional execution
    // UART7 operations
void uart7Task(UArg arg0, UArg arg1);                                                       // Declaring function for UART7 task handler
void processUart7Input(UART_Handle uart, char *command);                                    // Declaring function for UART7 input processor
    // Sine-DAC operations
void sine_cmd(UART_Handle uart, char *args);                                                // Declaring function for sine/DAC processor

/* External declarations */
extern Callback callbacks[MAX_CALLBACKS];                                                   // Declaring callback's external array
extern Timer_Handle timer1Handle;                                                           // Declaring callback 0 timer external variable
extern Ticker tickers[MAX_TICKERS];                                                         // Declaring ticker's external array
extern Timer_Handle ticker_timer_handle;                                                    // Declaring ticker timer external variable
extern char reg_names[NUM_REGISTERS][MAX_REG_NAME_LENGTH];                                  // Declaring external char for 2D register parameter
extern int32_t registers[NUM_REGISTERS];                                                    // Declaring external variable for register array
extern char script_space[SCRIPT_SIZE][MAX_SCRIPT_LINE_LENGTH];                              // Declaring external char for 2D script space parameter
extern int Queue_count(Queue_Handle queue);                                                 // Declaring external variable for handling queue count
extern UART_Handle uart7;                                                                   // Declaring external variable for UART7 handle
extern Timer_Handle timer0Handle;                                                           // Declaring external variable for timer0Handle
extern SPI_Handle spiHandle;                                                                // Declaring external variable for SPI handler
extern char user_prompt[USER_PROMPT_SIZE];                                                  // Declaring global char for command prompts

/* Ending header file */
#endif /* TONY_H_ */
