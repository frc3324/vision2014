#ifndef GPIO_H
#define GPIO_H

extern int GPIOExport(int pin);
extern int GPIOUnexport(int pin);
extern int GPIODirection(int pin, int dir);
extern int GPIORead(int pin);
extern int GPIOWrite(int pin, int value);

const int IN  = 0;
const int OUT = 1;

const int LOW  = 0;
const int HIGH = 1;

const int PIN  = 24; /* P1-18 */
const int POUT = 4;  /* P1-07 */

#endif
