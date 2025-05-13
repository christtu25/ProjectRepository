`timescale 1ns / 1ps

module MainProject (
    input wire clk,           // Clock signal
    input wire rst,           // Reset signal
    input wire [1:0] speed_a, // Speed control for motor A (2 bits)
    input wire [1:0] speed_b, // Speed control for motor B (2 bits)
    input current_threshold_a, // current threshold input
    input current_threshold_b, // current threshold input
    input wire ip_a, ip_b, ip_c,
    input BtnC,
    output wire a, b, c, d, e, f, g, dp, // Individual LED output
    output [3:0] an,             // 4-bit enable signal
    output reg pwm_a,         // PWM signal for motor A
    output reg pwm_b,         // PWM signal for motor B
    output reg input1, input2, input3, input4,        // Direction control for motor A & motor B (0: forward, 1: reverse)
    output reg led_a,        // LED control for motor A (connect to an LED)
    output reg led_b,         // LED control for motor B (connect to a different LED)
    output reg led_c,
    output wire trigger,
    input wire pulse,
    output wire state,
    input wire analog_pos_in, analog_neg_in,
    output reg led1, led2, led3, led4, led5, led6, led7, led8,
    output magnettoggle
);

    localparam N = 21;

    localparam ZERO = 7'b1000000;
    localparam ONE = 7'b1111001;
    localparam TWO = 7'b0100100;
    localparam THREE = 7'b0110000;
    localparam FOUR = 7'b0011001;
    localparam FIVE = 7'b0010010;
    localparam SIX = 7'b0000010;
    localparam SEVEN = 7'b1111000;
    localparam EIGHT = 7'b0000000;
    localparam NINE = 7'b0010000;
    localparam DASH = 7'b0111111;
    localparam CELSIUS = 7'b0100111;
    localparam NOTHING = 7'b1111111;
    
    localparam EN1 = 4'b0111;
    localparam EN2 = 4'b1011;
    localparam EN3 = 4'b1101;
    localparam EN4 = 4'b1110;
    
    // Internal signals
    reg [N-1:0] counter = 0;    // 21-bit counter for PWM generation
    reg [N-1:0] count = 0;    // 21-bit count for 7 seg display
    reg [N+4:0] delay = 0;    // 26-bit count for current threshold delay
    reg [N-1:0] duty_cycle_a = 0; // Duty cycle control for motor A (4 bits)
    reg [N-1:0] duty_cycle_b = 0; // Duty cycle control for motor B (4 bits)
    reg [6:0] s_temp; // 7-bit register to hold data for display
    reg [3:0] an_temp; // 4-bit enable register
    reg latch = 0;
    reg [N+6:0] turndelay = 0;
    reg delaylatch = 0;
    reg [N+4:0] controldelay = 0; 
    reg delay_done = 0;
    reg delay_trigger = 0;
    reg [32:0] tempcount = 0; 

//Ultrasonic
    reg [22:0] pcount;
    reg n_trigger;
    reg old_pulse;
    reg [32:0] count_out;
    reg [32:0] pulse_count;
    reg [32:0] status;
    
////ServoMotor
//    parameter PWM_PERIOD = 2000000; // 20ms in clock cycles (100MHz)
//    parameter MIN_WIDTH = 90000;   // 0.9ms in clock cycles
//    parameter MAX_WIDTH = 210000;  // 2.1ms in clock cycles
//    parameter TRANSITION_TIME = 100000000; // 1 seconds in clock cycles
    
//    reg [20:0] scounter = 0;
//    reg [27:0] tcounter = 0;
//    reg [20:0] temp_servo = 0;

// Delay Counts
    reg [31:0] ultracount = 0;
    reg [31:0] tempdelay = 0;

// Toggles
    reg temptoggle = 0;
    reg platform1;
    reg ultratoggle = 1;
    reg station1 = 1;
    reg station2 = 0;
    reg station3 = 0;
    reg station4 = 0;
    reg magnet = 0;

wire [15:0] do_out;  // ADC value; useful part are only [15:4] bits

wire [4 : 0] channel_out;

wire eoc_out;

xadc_wiz_1 xadc_inst(
  //.di_in(di_in),              // input wire [15 : 0] di_in
  .daddr_in(channel_out),        // input wire [6 : 0] daddr_in
  .den_in(eoc_out),            // input wire den_in
  .dwe_in(1'b0),            // input wire dwe_in
  //.drdy_out(drdy_out),        // output wire drdy_out
  .do_out(do_out),            // output wire [15 : 0] do_out
  .dclk_in(clk),          // input wire dclk_in
  .reset_in(0),        // input wire reset_in
  .vauxp6(analog_pos_in),            // note since vauxn5, channel 5, is used  .daddr_in(ADC_ADDRESS), ADC_ADRESS = 15h, i.e., 010101 
  .vauxn6(analog_neg_in),            // note since vauxn5, channel 5, is used  .daddr_in(ADC_ADDRESS), ADC_ADRESS = 15h, i.e., 010101     
  .channel_out(channel_out),  // output wire [4 : 0] channel_out
  .eoc_out(eoc_out),          // output wire eoc_out
  .alarm_out(0),      // output wire alarm_out
  .eos_out(0),         // output wire eos_out
  .busy_out(0)        // output wire busy_out
);


//assign value_in =  do_out[15:4];
    
    initial begin
        pcount = 23'b0;
        platform1 = 1'b0;
        n_trigger = 1'b0;
        old_pulse = 1'b0;
        count_out = 33'b0;
        pulse_count = 33'b0;
        status = 33'b0;
    end

// Define an integer parameter for the scaling factor

reg [15:0] value_in; // reg to hold XADC output
reg [3:0] tens;  // two 4-bit decimal outputs
reg [3:0] ones;

//Temp Sensor
always @ (posedge clk) begin
    if(temptoggle == 1) begin

        if (tempcount == 50000000) begin // checks temp. reading every .5 seconds

            value_in = do_out >> 4; // sets value_in to 12 MSBs of do_out
            
            // Calculate the ones and tens digits
            tens = (value_in - 1305) / 310; 
            ones = ((value_in - 1305) / 31) % 10;

            tempcount = 0;
        end

        tempcount = tempcount + 1;
    end
end


//Ultrasonic Sensor Code
always @(posedge clk) begin
    if (ultratoggle == 1) begin
        // Always add one to count, this is the overall counter for the script
        pcount = pcount + 1;
        // This is used to pick a very specific timeout for trigger control
        n_trigger = ~&(pcount[22:10]);
            if (n_trigger) begin
                if (pulse == 1) begin
                    pulse_count = pulse_count + 1;
                end
                if ((old_pulse == 1) && (pulse == 0)) begin
                    count_out = pulse_count;
                    pulse_count = 0;
                end
            end
        // This number is the distance control; the status is used as a buffer for filtering (OR filter below)
        if (count_out < 33'b11111100010000000) begin
            status = status << 1;
            status[0] = 1;
        end else begin
            status = status << 1;
            status[0] = 0;
        end
        old_pulse = pulse;
    end
end

assign trigger = n_trigger;
// Logic OR filter status here
assign state = |status;


    // Speed control for motor A
    always @(*) begin
    case ({speed_a[1], speed_a[0]})
        2'b00: duty_cycle_a = 21'd0;              // 0%
        2'b01: duty_cycle_a = 21'd1153433;         // 55%
        2'b10: duty_cycle_a = 21'd1468006;        // 70%
        2'b11: duty_cycle_a = 21'd2097151;        // 100%
        default: duty_cycle_a = 21'd0;            // Default to 0% if neither case matches
    endcase
    end

    // Speed control for motor B
    always @(*) begin
    case ({speed_b[1], speed_b[0]})
        2'b00: duty_cycle_b = 21'd0;              // 0%
        2'b01: duty_cycle_b = 21'd1153433;         // 55%
        2'b10: duty_cycle_b = 21'd1468006;        // 70%
        2'b11: duty_cycle_b = 21'd2097151;        // 100%
        default: duty_cycle_b = 21'd0;            // Default to 0% if neither case matches
    endcase
    end


    // Direction control for motors
    always @(posedge clk) begin
        if(delaylatch == 0) begin // this triggers if it hasn't reached the crossroads yet
            if (ip_a == 0 && ip_c == 0 && ip_b == 0) begin
                delaylatch = 1;
            end

            else if (ip_b == 0 && ip_a == 1 && ip_c == 1) begin
                if (delay_trigger == 0)
                    delay_trigger = 1;
                else if (delay_done == 1) begin
                    delay_trigger = 0;
                    input2 <= 0;
                    input1 <= 1;
                    input4 <= 0;
                    input3 <= 1;
                end
            end

            else if(ip_c == 0 && ip_a == 1 && ip_b == 1) begin
                input2 <= 0;
                input1 <= 1;
                input4 <= 1;
                input3 <= 0;
            end
            
            else if(ip_a == 0 && ip_b == 1 && ip_c == 1) begin
                input1 <= 0;
                input2 <= 1;
                input3 <= 1;
                input4 <= 0;
            
            end else begin
                input2 <= 0;
                input1 <= 1;
                input4 <= 0;
                input3 <= 1;
            end
        end else begin // this triggers when and after it reaches the crossroads the first time
            if(turndelay < 27'd90217700) begin
                turndelay = turndelay + 1;
                input2 <= 1;
                input1 <= 0;
                input4 <= 0;
                input3 <= 1;
            end else begin
                if (ip_b == 0 && ip_a == 1 && ip_c == 1) begin
                if (delay_trigger == 0)
                    delay_trigger = 1;
                else if (delay_done == 1) begin
                    delay_trigger = 0;
                    input2 <= 0;
                    input1 <= 1;
                    input4 <= 0;
                    input3 <= 1;
                end
            end
                
                else if (ip_a == 0 && ip_b == 1 && ip_c == 1) begin
                    input1 <= 0;
                    input2 <= 1;
                    input3 <= 1;
                    input4 <= 0;
                end else if (ip_a == 1 && ip_b == 1 && ip_c == 0) begin 
                    input2 <= 0;
                    input1 <= 1;
                    input4 <= 1;
                    input3 <= 0;
                end else if (ip_a == 1 && ip_b == 0 && ip_c == 0) begin
                    input2 <= 0;
                    input1 <= 1;
                    input4 <= 1;
                    input3 <= 0;
                end else begin
                    input2 <= 0;
                    input1 <= 1;
                    input4 <= 0;
                    input3 <= 1;
                end                                                          
            end
        end
    end
    always @(posedge clk) begin
        if(delay_trigger == 1) begin
            if (controldelay < 26'd50108860) begin
                controldelay <= controldelay + 1;
                delay_done <= 0;
            end else begin
                controldelay <= 0;
                delay_done <= 1;
            end
        end
    end



// PWM generation
always @(posedge clk) begin
    count <= count + 1;
    if (rst) begin
        counter <= 0;
        delay <= 0;
        pwm_a <= 0;
        pwm_b <= 0;
        
    end else begin
        // Increment counter on each clock cycle
        counter <= counter + 1;
        
        if (current_threshold_a || current_threshold_b) begin
        // Increment the delay counter and keep latch low.
        delay <= delay + 1;
            // Check if the delay counter is less than a threshold value
            if (delay >= 27'd134217720)
                // Once the delay threshold is reached, set latch high.
                latch <= 1;
            end

        if (latch==0) begin
        // Update PWM output for motor A based on duty cycle
            if (counter < duty_cycle_a)
                pwm_a <= 1;
            else
                pwm_a <= 0;

            // Update PWM output for motor B based on duty cycle
            if (counter < duty_cycle_b)
                pwm_b <= 1;
            else
                pwm_b <= 0;
        end else begin
            pwm_a <= 0;
            pwm_b <= 0;
            if(BtnC) // latch holds until button c is pushed
            latch <= 0;
            delay <= 0;
        end
        
        if(state == 1 && ultratoggle == 1) begin
            pwm_a <= 0;
            pwm_b <= 0;

        if(platform1 == 0) begin
            temptoggle = 1;
            tempdelay = tempdelay + 1;
            if(tempdelay == 200000000) begin // 2 second delay to read temperature
                if(station1 == 1 && station2 == 0 && station3 == 0 && station4 == 0) begin
                    if(value_in > 1843 && value_in <= 2375) begin
                        platform1 = 0;
                        magnet = 1;
                        ultratoggle = 0;
                        tempdelay = 0;
                        station2 = 1;
                    end else begin
                        ultratoggle = 0;
                        tempdelay = 0;
                    end
                    end if (station2 == 1 && station1 == 1 && station3 == 0 && station4 == 0) begin
                        if(value_in > 2375) begin
                            platform1 = 1;
                            magnet = 0;
                            ultratoggle = 0;
                            tempdelay = 0;
                            station3 = 1;
                        end else begin
                            ultratoggle = 0;
                            tempdelay = 0;
                        end
                    end if (station3 == 1 && station2 == 1 && station1 == 1 && station4 == 0) begin
                        if(value_in <= 1843) begin
                            platform1 = 1;
                            magnet = 0;
                            ultratoggle = 0;
                            tempdelay = 0;
                            station4 = 1;
                        end else begin
                            ultratoggle = 0;
                            tempdelay = 0;
                        end
                    end if (station4 == 1 && station3 == 1 && station2 == 1 && station1 == 1) begin
                        if(value_in > 1843 && value_in <= 2375) begin
                            platform1 = 1;
                            magnet = 0;
                            ultratoggle = 0;
                            tempdelay = 0;
                        end else begin
                            ultratoggle = 0;
                            tempdelay = 0;
                        end
                    end
                end
            end else begin
                magnet = 1;
                ultratoggle = 0;
                platform1 = 0;
            end
            
        end else begin
            temptoggle = 0;
        end
        if(ultratoggle == 0) begin
            ultracount = ultracount + 1;
            if(platform1 == 0) begin
                if(ultracount == 250000000) begin // 2.5 second delay
                    ultratoggle = 1;
                    ultracount = 0;
                end
            end else begin
                if(ultracount == 80000000) begin // .8 second delay
                    ultratoggle = 1;
                    ultracount = 0;
                    end
                end
            end 
    end
end



    // LED control signals (for testing, connect to LEDs on the board)
    always @(*) begin
        // Control LED_A based on the state of motor A
        led_a = pwm_a;
        
        // Control LED_B based on the state of motor B
        led_b = pwm_b;
        
        led_c = latch; //shows when latch is enabled
        
        led1 = magnet;
        led2 = temptoggle;
        led3 = platform1;
        led4 = ultratoggle;
        led5 = station1;
        led6 = station2;
        led7 = station3;
        led8 = station4;

        end

    always @ (*)
    begin
      case(count[N-1:N-2]) //using only the 2 MSB's of the counter 
       
       2'b00 :  //When the 2 MSB's are 00 enable the fourth display
                begin
                    if (res)
                        s_temp = DASH; // Display '-'
                    else if (oc_a || oc_b)
                        s_temp = ZERO; // Display '0'
                    else
                        s_temp = NOTHING; // Display ' '
                    an_temp = EN4;
                 end
   
            2'b01:  //When the 2 MSB's are 01 enable the third display
                begin
                    if (res)
                        s_temp = DASH; // Display '-'
                    else
                        s_temp = CELSIUS; // Display 'c'
                    an_temp = EN3;
                end
   
            2'b10:  //When the 2 MSB's are 10 enable the second display
                begin
                    if (res)
                        s_temp = DASH; // Display '-'
                    else
                        case (ones)
                            0 : s_temp = ZERO;
                            1 : s_temp = ONE;  
                            2 : s_temp = TWO;  
                            3 : s_temp = THREE;  
                            4 : s_temp = FOUR;  
                            5 : s_temp = FIVE;  
                            6 : s_temp = SIX;
                            7 : s_temp = SEVEN;  
                            8 : s_temp = EIGHT;  
                            9 : s_temp = NINE;
                            default : s_temp = DASH;      
                        endcase
                    an_temp = EN2;   
                end
    
            2'b11:  //When the 2 MSB's are 11 enable the first display
                begin
                    if (res)
                        s_temp = DASH; // Display '-'
                    else
                        case (tens)
                            0 : s_temp = ZERO;
                            1 : s_temp = ONE;  
                            2 : s_temp = TWO;  
                            3 : s_temp = THREE;  
                            4 : s_temp = FOUR;  
                            5 : s_temp = FIVE;  
                            6 : s_temp = SIX;
                            7 : s_temp = SEVEN;  
                            8 : s_temp = EIGHT;  
                            9 : s_temp = NINE;
                            default : s_temp = DASH;      
                        endcase
                    an_temp = EN1; 
                end
      endcase
     end

    assign an = an_temp; //because you can't manipulate outputs
    
    assign {g, f, e, d, c, b, a} = sseg_temp; //concatenate the outputs to the register
    
    assign dp = 1'b1; //since the decimal point is not needed, all 4 of them are turned off
    
    assign magnettoggle = magnet;
   
endmodule
    
