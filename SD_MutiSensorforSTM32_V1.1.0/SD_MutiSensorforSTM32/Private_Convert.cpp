#include "Private_Convert.h"

Type_Convert Var_Convert;

void Type_Convert::Var_Convert_CharArray(unsigned long var, char *cvt_char, unsigned char *cvt_len)
{
  unsigned char count = 0;
  unsigned long Var_Temp = var;
  
  if (var == 0){
      count = 1;
  }else{
    while (Var_Temp != 0){
        Var_Temp /= 10;
        count++;
    }
  }
  //判断长整形变量的位数
  switch (count){
    case 1 :  cvt_char[0] = var % 10; 
              *cvt_len = 1; break; 

    case 2 :  cvt_char[0] = var / 10; cvt_char[1] = var % 10; 
              *cvt_len = 2; break; 

    case 3 :  cvt_char[0] = var / 100; cvt_char[1] = var % 100 / 10; cvt_char[2] = var % 10; 
              *cvt_len = 3; break;

    case 4 :  cvt_char[0] = var / 1000; cvt_char[1] = var % 1000 / 100; cvt_char[2] = var % 1000 % 100 / 10; cvt_char[3] = var % 10; 
              *cvt_len = 4; break;

    case 5 :  cvt_char[0] = var / 10000; cvt_char[1] = var % 10000 / 1000; cvt_char[2] = var % 10000 % 1000 / 100; 
              cvt_char[3] = var % 10000 % 1000 % 100 / 10; cvt_char[4] = var % 10;
              *cvt_len = 5; break;

    case 6 :  cvt_char[0] = var / 100000; cvt_char[1] = var % 100000 / 10000; cvt_char[2] = var % 100000 % 10000 / 1000;
              cvt_char[3] = var % 100000 % 10000 % 1000 / 100; cvt_char[4] = var % 100000 % 10000 % 1000 % 100 / 10;
              cvt_char[5] = var % 10;
              *cvt_len = 6; break;
  }
}

void Type_Convert::Var_Convert_CharArray(unsigned int var, char *cvt_char, unsigned char *cvt_len)
{
  unsigned char count = 0;
  unsigned long Var_Temp = var;

  if (var == 0){
      count = 1;
  }else{
    while (Var_Temp != 0){
        Var_Temp /= 10;
        count++;
    }
  }
  //判断长整形变量的位数
  switch (count){
    case 1 :  cvt_char[0] = var % 10; 
              *cvt_len = 1; break; 

    case 2 :  cvt_char[0] = var / 10; cvt_char[1] = var % 10; 
              *cvt_len = 2; break; 

    case 3 :  cvt_char[0] = var / 100; cvt_char[1] = var % 100 / 10; cvt_char[2] = var % 10; 
              *cvt_len = 3; break;

    case 4 :  cvt_char[0] = var / 1000; cvt_char[1] = var % 1000 / 100; cvt_char[2] = var % 1000 % 100 / 10; cvt_char[3] = var % 10; 
              *cvt_len = 4; break;

    case 5 :  cvt_char[0] = var / 10000; cvt_char[1] = var % 10000 / 1000; cvt_char[2] = var % 10000 % 1000 / 100; 
              cvt_char[3] = var % 10000 % 1000 % 100 / 10; cvt_char[4] = var % 10;
              *cvt_len = 5; break;

    case 6 :  cvt_char[0] = var / 100000; cvt_char[1] = var % 100000 / 10000; cvt_char[2] = var % 100000 % 10000 / 1000;
              cvt_char[3] = var % 100000 % 10000 % 1000 / 100; cvt_char[4] = var % 100000 % 10000 % 1000 % 100 / 10;
              cvt_char[5] = var % 10;
              *cvt_len = 6; break;
  }
}