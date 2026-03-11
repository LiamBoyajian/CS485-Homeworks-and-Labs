#pragma once
#include <stdio.h>
#include <string.h>
#include <math.h>

int Mnemonic_To_Opcode(char* mnemonic, int funct);
int Assembly_Name_To_Register(char* reg);
int Char_Array_To_Register(char* reg);