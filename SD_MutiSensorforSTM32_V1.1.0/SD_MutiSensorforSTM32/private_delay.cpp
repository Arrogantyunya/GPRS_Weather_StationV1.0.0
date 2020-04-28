#include "private_delay.h"
#include <Arduino.h>

/*
 *brief   : 将微妙延时封装成毫秒延时
 *para    : 毫秒值
 *return  : 无
 */
void Delay_ms(unsigned long nn)
{
   while(nn--)
   {
     delayMicroseconds(1000);
   }
}