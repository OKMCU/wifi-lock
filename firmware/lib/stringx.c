#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "stringx.h"

#define SWAP(a, b)  a ^= b ^= a ^= b

/*
extern uint8_t stringx_test(void)
{
    uint8_t u8tmp1;
    uint16_t u16tmp1;
    uint32_t u32tmp1;
    int32_t s32tmp1;
    uint8_t len1;

    uint8_t u8tmp2;
    uint16_t u16tmp2;
    uint32_t u32tmp2;
    int32_t s32tmp2;
    uint8_t len2;

    uint8_t i;
    char str[12];
    

    u32tmp1 = 0;
    for(;;)
    {
        for(i = 0; i < sizeof(str); i++)
        {
            str[i] = 0;
        }
        
        len1 = hex32str(u32tmp1, str);
        len2 = hexstr2uint(str, &u32tmp2);
        if(len1 != len2 || u32tmp1 != u32tmp2)
        {
            return 1;
        }

        if(u32tmp1 < 1000)
        {
            u32tmp1++;
        }
        else if(u32tmp1 < UINT32_MAX - 5000)
        {
            u32tmp1 += 5000;
        }
        else
        {
            u32tmp1++;
        }
        
        if(u32tmp1 == 0)
        {
            break;
        }
    }

    u16tmp1 = 0;
    for(;;)
    {
        for(i = 0; i < sizeof(str); i++)
        {
            str[i] = 0;
        }
        
        len1 = hex16str(u16tmp1, str);
        len2 = hexstr2uint(str, &u32tmp2);
        u16tmp2 = (uint16_t)u32tmp2;
        if(len1 != len2 || u16tmp1 != u16tmp2)
        {
            return 2;
        }

        u16tmp1++;
        if(u16tmp1 == 0)
        {
            break;
        }
    }

    u8tmp1 = 0;
    for(;;)
    {
        for(i = 0; i < sizeof(str); i++)
        {
            str[i] = 0;
        }
        
        len1 = hex8str(u8tmp1, str);
        len2 = hexstr2uint(str, &u32tmp2);
        u8tmp2 = (uint8_t)u32tmp2;
        if(len1 != len2 || u8tmp1 != u8tmp2)
        {
            return 3;
        }

        u8tmp1++;
        if(u8tmp1 == 0)
        {
            break;
        }
    }


    u32tmp1 = 0;
    for(;;)
    {
        for(i = 0; i < sizeof(str); i++)
        {
            str[i] = 0;
        }
        
        len1 = uintstr(u32tmp1, str);
        len2 = decstr2uint(str, &u32tmp2);
        if(len1 != len2 || u32tmp1 != u32tmp2)
        {
            return 4;
        }

        if(u32tmp1 < 1000)
        {
            u32tmp1++;
        }
        else if(u32tmp1 < UINT32_MAX - 5000)
        {
            u32tmp1 += 5000;
        }
        else
        {
            u32tmp1++;
        }
        
        if(u32tmp1 == 0)
        {
            break;
        }
    }

    s32tmp1 = INT32_MIN;//INT32_MIN;
    for(;;)
    {
        for(i = 0; i < sizeof(str); i++)
        {
            str[i] = 0;
        }

        len1 = sintstr(s32tmp1, str);
        len2 = decstr2sint(str, &s32tmp2);
        if(len1 != len2 || s32tmp1 != s32tmp2)
        {
            return 5;
        }

        if(s32tmp1 < INT32_MIN + 1000)
        {
            s32tmp1++;
        }
        else if(s32tmp1 < 0 - 5000)
        {
            s32tmp1 += 5000;
        }
        else if(s32tmp1 < 0 + 5000)
        {
            s32tmp1++;
        }
        else if(s32tmp1 < INT32_MAX - 5000)
        {
            s32tmp1 += 5000;
        }
        else
        {
            s32tmp1++;
        }
        
        if(s32tmp1 == INT32_MIN)
        {
            break;
        }
    }
    
    return 0;
}
*/

extern uint8_t hexstr2uint(const char *str, uint32_t *num)
{
    char ch;
    uint8_t cnt = 0;
    uint8_t lenPrefix = 0;
    uint32_t val = 0;
    int32_t test_09;
    int32_t test_AF;
    int32_t test_af;
    
    if(str[0] == '0')
    {
        if(str[1] == 'x' || str[1] == 'X')
        {
            cnt = 2;
            lenPrefix = 2;
        }
    }
    //////////////////////
    
    for(;;)
    {
        ch = str[cnt];
        
        if(ch == '\0')
            break;

        cnt++;
        
        if(cnt-lenPrefix > HEX32_STR_LEN_MAX)
        {
            return 0;
        }
        
        test_09 = (ch - '0') * (ch - '9');
        test_AF = (ch - 'A') * (ch - 'F');
        test_af = (ch - 'a') * (ch - 'f');
        if(test_09 * test_AF * test_af <= 0)
        {
            val <<= 4;
            if(test_09 <= 0)
            {
                val += ch - '0';
            }
            else if(test_AF <= 0)
            {
                val += ch - 'A' + 10;
            }
            else if(test_af <= 0)
            {
                val += ch - 'a' + 10;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        } 
    }

    if(cnt > lenPrefix)
    {
        if(num)
        {
            *num = val;
        }
    }
    
    return cnt;
}

extern uint8_t decstr2uint(const char *str, uint32_t *num)
{
    char ch;
    uint8_t cnt = 0;
    uint32_t val = 0;

    

    for(;;)
    {
        ch = str[cnt];
        
        if(ch == '\0')
            break;
        
        cnt++;
        
        if(cnt > UINT_STR_LEN_MAX)
            return 0;

        
        if(ch >= '0' && ch <= '9')
        {
            val *= 10;
            val += ch - '0';
        }
        else
        {
            return 0;
        }
        
    }

    if(cnt > 0)
    {
        if(num)
        {
            *num = val;
        }
    }
    
    return cnt;
}

extern uint8_t decstr2sint(const char *str, int32_t *num)
{
    int32_t val = 0;
    uint32_t u32tmp;
    uint8_t cnt;
    
    if(str[0] == '-')
    {
        cnt = decstr2uint(str+1, &u32tmp);
    }
    else
    {
        cnt = decstr2uint(str, &u32tmp);
    }

    if(cnt > 0)
    {
        if(str[0] == '-')
        {
            if(u32tmp > (uint32_t)INT32_MAX + 1)
            {
                return 0;
            }
            val = (int32_t)u32tmp;
            val =-val;
            cnt++;
        }
        else
        {
            if(u32tmp > (uint32_t)INT32_MAX)
            {
                return 0;
            }
            val = (int32_t)u32tmp;
        }
        
        if(num)
        {
            *num = val;
        }
    }
    
    
    return  cnt;
}

extern uint8_t sintstr(int32_t num, char *str)
{
    uint8_t numLen = 0;
    uint8_t i;
    uint8_t j;
    int8_t flag;
    uint32_t absNum;
    
    if(num < 0)
    {
        flag = -1;
        absNum = (uint32_t)(-num);
    }
    else
    {
        flag = 1;
        absNum = (uint32_t)num;
    }
    
    do{
        str[numLen++] = '0' + (absNum%10);
        absNum /= 10;
    }
    while(absNum != 0);

    if(flag == -1)
    {
        str[numLen++] = '-';
    }

    for(i = 0; i < numLen; i++)
    {
        if(numLen >= i + 1)
        {
            j = numLen - 1 - i;
            if(i < j)
            {
                SWAP(str[i], str[j]);
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    
    return numLen;
}

extern uint8_t uintstr(uint32_t num, char *str)
{
    uint8_t numLen = 0;
    uint8_t i;
    uint8_t j;
    
    do{
        str[numLen++] = '0' + (num%10);
        num /= 10;
    }
    while(num != 0);

    for(i = 0; i < numLen; i++)
    {
        if(numLen >= i + 1)
        {
            j = numLen - 1 - i;
            if(i < j)
            {
                SWAP(str[i], str[j]);
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    return numLen;
}

extern uint8_t hex8str(uint8_t num, char *str)
{
    str[0] = num >> 4;
    str[1] = num & 0x0F;
    
    str[0] = (str[0] <= 9) ? (str[0] + '0') : (str[0] + 'A' - 10);
    str[1] = (str[1] <= 9) ? (str[1] + '0') : (str[1] + 'A' - 10);
    
    return HEX8_STR_LEN_MAX;
}

extern uint8_t hex16str(uint16_t num, char *str)
{
    int8_t i;
    
    for(i = HEX16_STR_LEN_MAX-1; i >=0; i--)
    {
        str[i] = num & 0xF;
        str[i] = (str[i] <= 9) ? (str[i] + '0') : (str[i] + 'A' - 10);
        num >>= 4;
    }

    return HEX16_STR_LEN_MAX;
}

extern uint8_t hex32str(uint32_t num, char *str)
{
    int8_t i;
    
    for(i = HEX32_STR_LEN_MAX-1; i >=0; i--)
    {
        str[i] = num & 0xF;
        str[i] = (str[i] <= 9) ? (str[i] + '0') : (str[i] + 'A' - 10);
        num >>= 4;
    }
    
    return HEX32_STR_LEN_MAX;
}

extern char *strStartsWith(char *str1, const char *str2)
{
    if(str1 != NULL && str2 != NULL)
    {
        while(*str1)
        {
            if(*str1 == *str2)
            {
                str1++;
                str2++;
                if(*str2 == '\0')
                {
                    return str1;
                }
            }
            else
            {
                break;
            }
        }
    }
        
    return NULL;
}

extern char *strEndsWith(char *str1, const char *str2)
{
    uint16_t len1;
    uint16_t len2;

    if(str1 != NULL && str2 != NULL)
    {
        len1 = strlen(str1);
        len2 = strlen(str2);

        if(len1 >= len2)
        {
            if(strcmp(str1+len1-len2, str2) == 0)
            {
                return (str1+len1-len2);
            }
        }
    }
    return NULL;
    
}


