#ifndef COUNTER_H_
#define COUNTER_H_

// *************************************************************************************************
// Include section


// *************************************************************************************************
// Prototypes section

// internal functions
extern void reset_counter(void);

// menu functions
extern void mx_counter(u8 line);
extern void sx_counter(u8 line);
extern void display_counter(u8 line, u8 update);
extern u8 is_counter_measurement(void);
extern void do_counter_measurement(void);


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
struct counter
{
 // MENU_ITEM_NOT_VISIBLE, MENU_ITEM_VISIBLE
 menu_t   state;
 s16  count;
 // Sensor raw data
 u8   xyz[3];
 // Acceleration data in 10 * mgrav
 u16   data[3];
 u16         sum, low, high; // total data
 u8 rise_state; // 0: init, 1: rise, 2: fall
 u8 style; // 0: walk, 1: run
};
extern struct counter sCounter;


#endif /*COUNTER_H_*/
