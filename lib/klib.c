
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            klib.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "global.h"
#include "proto.h"


/*======================================================================*
                               itoa
 *======================================================================*/
//数字转字符串
PUBLIC char * itoa(char * str, int num)/* 数字前面的 0 不被显示出来, 比如 0000B800 被显示成 B800 */
{
    char *p = str;
    char ch;
    int i;
    int flag = 0;

    *p++ = '0';
    *p++ = 'x';

    if(num == 0){
        *p++ = '0';
    }
    else{
        for(i=28;i>=0;i-=4){
            ch = (num >> i) & 0xF;
            if(flag || (ch > 0)){
                flag = 1;
                ch += '0';
                if(ch > '9'){
                    ch += 7;
                }
                *p++ = ch;
            }
        }
    }

    *p = 0;

    return str;
}


//字符串转数字
PUBLIC int atoi(const char * str, int * pRet)
{
    int iRet = 0;
    int  fNeg = 0;
    if (*str == '-')
    {
        fNeg = 1;
        ++str;
    }
    else if (*str == '+')
    {
        fNeg = 0;
        ++str;
    }
    while (*str)
    {
        if (*str == ' ' || *str == '.') break;
        if (*str < '0' || *str > '9')
        {
            return 0;
        }
        iRet = iRet * 10 + (*str - '0');
        ++str;
    }
    *pRet = !fNeg ? iRet : -iRet;
    return 1;
}


/*======================================================================*
                               disp_int
 *======================================================================*/
//显示数字
PUBLIC void disp_int(int input)
{
    char output[16];
    itoa(output, input);
    disp_str(output);
}

/*======================================================================*
                               delay
 *======================================================================*/
//延迟函数
PUBLIC void delay(int time)
{
    int i, j, k;
    for(k=0;k<time;k++){
        /*for(i=0;i<10000;i++){	for Virtual PC	*/
        for(i=0;i<10;i++){/*	for Bochs	*/
            for(j=0;j<10000;j++){}
        }
    }
}
