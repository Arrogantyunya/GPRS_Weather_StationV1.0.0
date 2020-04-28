#ifndef _PRIVATE_CONVERT_H
#define _PRIVATE_CONVERT_H

class Type_Convert{
public:
    void Var_Convert_CharArray(unsigned long var, char *cvt_char, unsigned char *cvt_len);
    void Var_Convert_CharArray(unsigned int var, char *cvt_char, unsigned char *cvt_len);
};

extern Type_Convert Var_Convert;

#endif