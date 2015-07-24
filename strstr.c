char *mystrstr(char *s1,char *s2)  
{  
    int n;  
    if (*s2)                      //两种情况考虑  
    {  
            while(*s1)                 
            {  
                        for (n=0;*(s1+n)==*(s2+n);n++)  
                        {  
                                        if (!*(s2+n+1))            //查找的下一个字符是否为'\0'  
                                        {  
                                                            return (char*)s1;  
                                                        }  
                                    }  
                        s1++;  
                    }  
            return 0;  
        }  
    else  
    {  
            return s1;  
        }  
}
