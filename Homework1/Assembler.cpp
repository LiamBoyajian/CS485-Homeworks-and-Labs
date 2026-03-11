#include "Header_Assembler.h"

int main(int argc, char *argv[]) {
    if (argc <= 1)
        return -1;

    char concat_argv[100];//Concat the argv into a string
    for (int i = 1; i < argc; i++) {
        strcat(concat_argv, argv[i]);
        strcat(concat_argv, " ");
    }

    int index_of_cleaned = 0;
    char cleaned_argv[4][32] = {"\0","\0","\0","\0"};
    int arg_start = -1;

    for (int i = 0; i < strlen(concat_argv); i++) {//split the concat string into clean args for the program

        if (concat_argv[i] == ' ' || concat_argv[i] == 9 || concat_argv[i] == ',' || concat_argv[i] == '(' || concat_argv[i] == ')' || concat_argv[i] == '\0' || concat_argv[i] == '#') {
            if (arg_start != -1) {
                char current_arg[i - arg_start + 100];
                for (int j = 0; j < (i - arg_start); j++) {
                    current_arg[j] = concat_argv[arg_start+j];
                }
                current_arg[i - arg_start] = '\0';

                strcpy(cleaned_argv[index_of_cleaned], current_arg); //Depot in the cleaned array
                ++index_of_cleaned;
                arg_start = -1;
            }
            if (concat_argv[i] == '#' || index_of_cleaned == 4)
                break;

            continue;
        }
        if (arg_start == -1)
            arg_start = i;

    }
    int new_argc = index_of_cleaned;

    //#####################################################################################
    //                             Begin converting to int
    //#####################################################################################
    if (new_argc == 1) { //ex: syscall
        printf("0x%08x", Mnemonic_To_Opcode(cleaned_argv[0], 0));
        return 0;
    }

    unsigned result = 0;
    //Mnemonic
    int temp = Mnemonic_To_Opcode(cleaned_argv[0], 0);
    if (temp < 0) return -1;
    temp <<= (32-6);
    result |= temp;


    if (new_argc == 2) { //EX: j 0x324234
        result |= Char_Array_To_Register(cleaned_argv[1]);
        printf("0x%08x", result);
        return 0;
    }

    if (new_argc == 3) {
        if (result >> (32-6) == 35 || result >> (32-6) == 43) {
            //lw and sw with no offset
            temp = Assembly_Name_To_Register(cleaned_argv[2]);
            if (temp < 0) return -7;
            temp <<= (32-6-5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -8;
            temp <<= (32-6-5-5);
            result |= temp;

        }else{
            //lui
            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -2;
            temp <<= (32-6-5-5);
            result |= temp;
            result |= Char_Array_To_Register(cleaned_argv[2]);
        }

        printf("0x%08x", result);
        return 0;
    }

    if (!strstr(cleaned_argv[3],"$")) { //I-type
        if (result >> (32-6) == 4 || result >> (32-6) == 5) {
            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -3;
            temp <<= (32-6-5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[2]);
            if (temp < 0) return -4;
            temp <<= (32-6-5-5);
            result |= temp;
        }else {
            temp = Assembly_Name_To_Register(cleaned_argv[2]);
            if (temp < 0) return -5;
            temp <<= (32-6-5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -6;
            temp <<= (32-6-5-5);
            result |= temp;
        }
        temp = Char_Array_To_Register(cleaned_argv[3]);
        temp <<= (32-6-5-5-16);
        result |= temp;

    }else {//R-type with exception of lw+sw
        if (result >> (32-6) == 35 || result >> (32-6) == 43) {
            //lw and sw with offset
            temp = Assembly_Name_To_Register(cleaned_argv[3]);
            if (temp < 0) return -7;
            temp <<= (32-6-5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -8;
            temp <<= (32-6-5-5);
            result |= temp;

            temp = Char_Array_To_Register(cleaned_argv[2]);
            if (temp < 0) return -9;
            temp <<= (32-6-5-5-16);
            result |= temp;
        }else
        {//R-type
            temp = Assembly_Name_To_Register(cleaned_argv[2]);
            if (temp < 0) return -10;
            temp <<= (32-6-5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[3]);
            if (temp < 0) return -11;
            temp <<= (32-6-5-5);
            result |= temp;

            temp = Assembly_Name_To_Register(cleaned_argv[1]);
            if (temp < 0) return -12;
            temp <<= (32-6-5-5-5);
            result |= temp;

            temp = Mnemonic_To_Opcode(cleaned_argv[0], 1);
            if (temp < 0) return -13;
            temp <<= (32-6-5-5-5-5-6);
            result |= temp;
        }
    }

    printf("0x%08x\n", result);

    return 0;
}