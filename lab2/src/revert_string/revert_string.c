#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
	int str_len =  strlen(str);
    for (int i = 0; i < str_len / 2; i++){
        int j = str_len - 1 - i;
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

