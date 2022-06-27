
#define PIN_SDA 				PB0
#define PIN_SCL 				PB2

#define WAIT_LONG				0 // 5 // 4,7us (some fine tunning can be made here...)
#define WAIT_SHORT 				0 // 4 // 4,0us

// USISR mask
#define USISR_CLOCK_8_BITS		0b11110000
#define USISR_CLOCK_1_BIT  		0b11111110