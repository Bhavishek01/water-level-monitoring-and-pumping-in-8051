#include <reg52.h>
#include <intrins.h>  // For _nop_() used in delay_us()
#define number "YOUR PHONE NUMBER"

// ================== LCD Pin Configuration ==================
#define LCD_data P2 // LCD connected to Port 2

sbit RS = P1^0;
sbit RW = P1^1;
sbit EN = P1^2;

// ================== Ultrasonic Sensor1 Pins ==================
sbit TRIG_PIN = P1^4;
sbit ECHO_PIN = P1^5;


// ================== Ultrasonic Sensor2 Pins ==================
sbit TRIG_PIN1 = P1^6;
sbit ECHO_PIN1 = P1^7;

sbit mr = P1^3;

    unsigned long duration, height;
		unsigned long duration1, height1;


void delay(unsigned int time) {
    unsigned int i, j;
    for (i = 0; i < time; i++)
        for (j = 0; j < 1275; j++);
}

void delay_us(unsigned int us) {
    while(us--) {
        _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); // ~1us delay
    }
}

void sim_init() {
    SCON = 0x50;        // Mode 1: 8-bit UART, REN enabled
    TMOD = TMOD & 0x0F; // Clear Timer1 bits
    TMOD |= 0x20;       // Timer1 Mode 2: Auto-reload
    TH1 = 0xFD;         // 9600 baud rate (11.0592 MHz)
    TL1 = 0xFD;
    TR1 = 1;            // Start Timer1
    delay(500);        // Add 1–2 seconds delay for SIM800L to stabilize
}

void tx(unsigned char send)
{
    SBUF = send;
    while(TI==0);
    TI=0;   //reset the timer interrupt
}

void tx_string(unsigned char *s)
{
    while(*s)
        tx(*s++);
}

void sms( unsigned char *msg)
{
    tx_string("AT");
    tx(0x0d); 
		//tx(0x0a);  // Try both CR and LF
    delay(100);

    tx_string("AT+CMGF=1");
    tx(0x0d); //tx(0x0a);
    delay(100);
	
    tx_string("AT+CMGS=");
		tx('"');
    tx_string(number);
    tx('"');
    tx(0x0d); 
    delay(100); 

  tx_string(msg);
    tx(0x1A); 
    delay(100);
}


void LCD_cmd(unsigned char command) {
    LCD_data = command;
    RS = 0;
    RW = 0;
    EN = 1;
    delay(2);
    EN = 0;
}

void LCD_data_write(unsigned char dataa) {
    LCD_data = dataa;
    RS = 1;
    RW = 0;
    EN = 1;
    delay(2);
    EN = 0;
}

void LCD_string_write(unsigned char *string) {
    while (*string != '\0') {
        LCD_data_write(*string++);
    }
}

void LCD_init(void) {
    LCD_cmd(0x38); // 8-bit, 2-line, 5x7 dots
    LCD_cmd(0x0C); // Display ON, Cursor OFF
    LCD_cmd(0x06); // Entry mode
    LCD_cmd(0x01); // Clear screen
    delay(2);
}

void num_to_string(unsigned long num, char *buf) {
    unsigned char i = 0, j;
    char temp[10];
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (num > 0) {
        temp[i++] = (num % 10) + '0';
        num /= 10;
    }
    for (j = 0; j < i; j++) {
        buf[j] = temp[i - 1 - j];
    }
    buf[j] = '\0';
}

void trigger_sensor(void) {
    TRIG_PIN = 0;
    delay_us(1);
    TRIG_PIN = 1;
    delay_us(10);
    TRIG_PIN = 0;
}

void trigger_sensor1(void) {
    TRIG_PIN1 = 0;
    delay_us(1);
    TRIG_PIN1 = 1;
    delay_us(10);
    TRIG_PIN1 = 0;
}

unsigned long measure_distance(void) {
    unsigned long dura = 0;
    unsigned int timer_value;

    while (ECHO_PIN == 0); // Wait for echo to start
    TMOD = (TMOD & 0xF0) | 0x01; // Timer 0, Mode 1
    TH0 = 0;
    TL0 = 0;
    TR0 = 1;

    while (ECHO_PIN == 1); // Wait for echo to end
    TR0 = 0;

    timer_value = (TH0 << 8) | TL0;
    dura = (unsigned long)timer_value * 1085 / 1000; // Approx µs

    return dura;
}

unsigned long measure_distance1(void) {
    unsigned long dura1 = 0;
    unsigned int timer_value;

    while (ECHO_PIN1 == 0); // Wait for echo to start
    TMOD = (TMOD & 0xF0) | 0x01; // Timer 0, Mode 1
    TH0 = 0;
    TL0 = 0;
    TR0 = 1;

    while (ECHO_PIN1 == 1); // Wait for echo to end
    TR0 = 0;

    timer_value = (TH0 << 8) | TL0;
    dura1 = (unsigned long)timer_value * 1085 / 1000; // Approx µs

    return dura1;
}

void calculate_distance_cm(unsigned long duration, unsigned long *cm_out) {
	
    unsigned long distance_cm = duration / 58; // Convert µs to cm
    if (distance_cm > 100)
        distance_cm = 100;
    *cm_out = 100- distance_cm; // Convert to percentage (0 cm = 100%, 100 cm = 0%)
}

void lcd_display_height(unsigned long height) {
    char height_str[5];
    LCD_cmd(0x80);  // First line
    LCD_string_write("Tank 2: ");

    num_to_string(height, height_str);
    LCD_string_write(height_str);
    LCD_string_write(" %");
}

void lcd_display_height1(unsigned long height) {
    char height_str[5];
    LCD_cmd(0xC0);  // First line
    LCD_string_write("Tank 1: ");

    num_to_string(height, height_str);
    LCD_string_write(height_str);
    LCD_string_write(" %");
}


void ret_distance(void){
				trigger_sensor();
        duration = measure_distance();
        calculate_distance_cm(duration, &height);
        lcd_display_height(height);
}

void ret_distance1(void){
				trigger_sensor1();
        duration1 = measure_distance1();
        calculate_distance_cm(duration1, &height1);
        lcd_display_height1(height1);
}

void main(void) {

		mr = 0;
    LCD_init();
    LCD_cmd(0x80);
    LCD_string_write("CHECKING WATER");
    LCD_cmd(0xC0);
    LCD_string_write("     LEVEL");
    delay(500);
    LCD_cmd(0x01);
	


    while (1) {
				LCD_cmd(0x01);
        ret_distance();
				delay(50);
				ret_distance1();
        delay(200);
        
				if(height >= 10 && height1<= 45)
				{
								mr = 1;
								LCD_cmd(0x01);
								LCD_cmd(0x80);
								LCD_string_write("Motor is ON !!");
								LCD_cmd(0xC0);
								LCD_string_write("Tank1 Near Empty !!");
								sim_init();
								sms("Tank 1 Empty, Motor is ON");
								
								while(height1 <= 95)
								{
										mr = 1;
										LCD_cmd(0x01);
										LCD_cmd(0x80);
										LCD_string_write("Motor is ON !!");
										LCD_cmd(0xC0);
										LCD_string_write("Filing Tank");
										ret_distance1();
										delay(100);	
								}	
									
								mr = 0;
								LCD_cmd(0x01);
								LCD_cmd(0x80);
								LCD_string_write("Motor is OFF !!");
								LCD_cmd(0xC0);
								LCD_string_write("Tank 1 Full");
								ret_distance1();
								sim_init();
								sms("Tank 1 Full, Motor is OFF");
						}
						
				else if(height < 10 && height1 <= 25 )
				{
								mr = 0;
								LCD_cmd(0x01);
								LCD_cmd(0x80);
								LCD_string_write("Tank 2 Empty");
								LCD_cmd(0xC0);
								LCD_string_write("Tank 1 Empty");
								sim_init();
								sms("Both Tanks Empty");
				}
				else if(height <10 && height1 >=90 )
				{
								mr = 0;
								LCD_cmd(0x01);
								LCD_cmd(0x80);
								LCD_string_write("Tank 1 Full");
								LCD_cmd(0xC0);
								LCD_string_write("Tank 2 Empty");
								sim_init();
								sms("Tank 2 Empty");
				}
				else{}
					
				}
}