
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
    disp_str("-----\"kernel_main\" begins-----\n");

    struct task* p_task;
    struct proc* p_proc= proc_table;
    char* p_task_stack = task_stack + STACK_SIZE_TOTAL;
    u16   selector_ldt = SELECTOR_LDT_FIRST;
    u8    privilege;
    u8    rpl;
    int   eflags;
    int   i, j;
    int   prio;

    //似乎可以加开机动画

    //启动进程
    for (i = 0; i < NR_TASKS+NR_PROCS; i++)
    /*for (i = 0; i < NR_TASKS; i++)*/
    {
        if (i < NR_TASKS)
        {   /* 任务 */
            p_task    = task_table + i;
            privilege = PRIVILEGE_TASK;
            rpl       = RPL_TASK;
            eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1   1 0010 0000 0010(2)*/
            prio      = 15;     //设定优先级为15
        }
        else
        {   /* 用户进程 */
            p_task    = user_proc_table + (i - NR_TASKS);
            privilege = PRIVILEGE_USER;
            rpl       = RPL_USER;
            eflags    = 0x202; /* IF=1, bit 2 is always 1              0010 0000 0010(2)*/
            prio      = 5;     //设定优先级为5
        }

        strcpy(p_proc->name, p_task->name); /* 设定进程名称 */
        p_proc->pid = i;            /* 设定pid */

        p_proc->ldt_sel = selector_ldt;

        memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
               sizeof(struct descriptor));
        p_proc->ldts[0].attr1 = DA_C | privilege << 5;
        memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
               sizeof(struct descriptor));
        p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
        p_proc->regs.cs = (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.ds = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.es = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.fs = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.ss = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
        p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

        p_proc->regs.eip = (u32)p_task->initial_eip;
        p_proc->regs.esp = (u32)p_task_stack;
        p_proc->regs.eflags = eflags;

        p_proc->p_flags = 0;
        p_proc->p_msg = 0;
        p_proc->p_recvfrom = NO_TASK;
        p_proc->p_sendto = NO_TASK;
        p_proc->has_int_msg = 0;
        p_proc->q_sending = 0;
        p_proc->next_sending = 0;

        for (j = 0; j < NR_FILES; j++)
            p_proc->filp[j] = 0;

        p_proc->ticks = p_proc->priority = prio;

        p_task_stack -= p_task->stacksize;
        p_proc++;
        p_task++;
        selector_ldt += 1 << 3;
    }

    //初始化进程
    k_reenter = 0;
    ticks = 0;

    p_proc_ready = proc_table;

    init_clock();
    init_keyboard();

    restart();

    while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
    MESSAGE msg;
    reset_msg(&msg);
    msg.type = GET_TICKS;
    send_recv(BOTH, TASK_SYS, &msg);
    return msg.RETVAL;
}


/*======================================================================*
                               TestA
 *======================================================================*/

//A进程
void TestA()
{
    //0号终端
    char tty_name[] = "/dev_tty0";
    char username[128];
    char password[128];
    int fd;

    int isLogin = 0;

    char rdbuf[128];
    char cmd[128];
    char arg1[128];
    char arg2[128];
    char buf[1024];

    int fd_stdin  = open(tty_name, O_RDWR);
    assert(fd_stdin  == 0);
    int fd_stdout = open(tty_name, O_RDWR);
    assert(fd_stdout == 1);

    clear();
    printl("Sweetinux v1.0.0 tty0\n\n");

    while (1) {
        login(fd_stdin, fd_stdout, &isLogin, username, password);

        //必须要清空数组
        clearArr(rdbuf, 128);
        clearArr(cmd, 128);
        clearArr(arg1, 128);
        clearArr(arg2, 128);
        clearArr(buf, 1024);

        printf("%s@sweet:~$ ", username);

        int r = read(fd_stdin, rdbuf, 128);

        //解析命令
        int i = 0;
        int j = 0;
        while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        {
            cmd[i] = rdbuf[i];
            i++;
        }
        i++;
        while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        {
            arg1[j] = rdbuf[i];
            i++;
            j++;
        }
        i++;
        j = 0;
        while(rdbuf[i] != ' ' && rdbuf[i] != 0)
        {
            arg2[j] = rdbuf[i];
            i++;
            j++;
        }
        //清空缓冲区
        rdbuf[r] = 0;

        if (strcmp(cmd, "process") == 0)
        {
            ProcessManage();
        }
        else if (strcmp(cmd, "help") == 0)
        {
            help();
        }
        else if (strcmp(cmd, "game") == 0)
        {
            /*TTT(fd_stdin, fd_stdout);*/
            game(fd_stdin);
        }
        else if (strcmp(cmd, "clear") == 0)
        {
            printTitle();
        }
        else if (strcmp(cmd, "ls") == 0)
        {
            ls();
            /*printf("%s\n", rdbuf);*/
        }
        else if (strcmp(cmd, "touch") == 0)
        {
            fd = open(arg1, O_CREAT | O_RDWR);
            if (fd == -1)
            {
                printf("Failed to create file! Please check the filename!\n");
                continue ;
            }
            buf[0] = 0;
            write(fd, buf, 1);
            printf("File created: %s (fd %d)\n", arg1, fd);
            close(fd);
        }
        else if (strcmp(cmd, "cat") == 0)
        {
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("Failed to open file! Please check the filename!\n");
                continue ;
            }
            if (!verifyFilePass(arg1, fd_stdin))
            {
                printf("Authorization failed\n");
                continue;
            }
            read(fd, buf, 1024);
            close(fd);
            printf("%s\n", buf);
        }
        else if (strcmp(cmd, "vi") == 0)
        {
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("Failed to open file! Please check the filename!\n");
                continue ;
            }
            int tail = read(fd_stdin, rdbuf, 128);
            rdbuf[tail] = 0;

            write(fd, rdbuf, tail+1);
            close(fd);
        }
        else if (strcmp(cmd, "rm") == 0)
        {
            int result;
            result = unlink(arg1);
            if (result == 0)
            {
                printf("File deleted!\n");
                continue;
            }
            else
            {
                printf("Failed to delete file! Please check the filename!\n");
                continue;
            }
        }
        else if (strcmp(cmd, "cp") == 0)
        {
            //首先获得文件内容
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("File not exists! Please check the filename!\n");
                continue ;
            }
            int tail = read(fd, buf, 1024);
            close(fd);
            /*然后创建文件*/
            fd = open(arg2, O_CREAT | O_RDWR);
            if (fd == -1)
            {
                //文件已存在，什么都不要做
            }
            else
            {
                //文件不存在，写一个空的进去
                char temp[1024];
                temp[0] = 0;
                write(fd, temp, 1);
                close(fd);
            }
            //给文件赋值
            fd = open(arg2, O_RDWR);
            write(fd, buf, tail+1);
            close(fd);
        }
        else if (strcmp(cmd, "mv") == 0)
        {
            //首先获得文件内容
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("File not exists! Please check the filename!\n");
                continue ;
            }
            int tail = read(fd, buf, 1024);
            close(fd);
            /*然后创建文件*/
            fd = open(arg2, O_CREAT | O_RDWR);
            if (fd == -1)
            {
                //文件已存在，什么都不要做
            }
            else
            {
                //文件不存在，写一个空的进去
                char temp[1024];
                temp[0] = 0;
                write(fd, temp, 1);
                close(fd);
            }
            //给文件赋值
            fd = open(arg2, O_RDWR);
            write(fd, buf, tail+1);
            close(fd);
            //最后删除文件
            unlink(arg1);
        }
        else if (strcmp(cmd, "useradd") == 0)
        {
            doUserAdd(arg1, arg2);
        }
        else if (strcmp(cmd, "userdel") == 0)
        {
            doUserDel(arg1);
        }
        else if (strcmp(cmd, "passwd") == 0)
        {
            doPassWd(username, password, fd_stdin);
        }
        else if (strcmp(cmd, "logout") == 0)
        {
            isLogin = 0;
            clearArr(username, 128);
            clearArr(password, 128);

            clear();
            printl("Sweetinux v1.0.0 tty0\n\n");
        }
        else if (strcmp(cmd, "encrypt") == 0)
        {
            fd = open(arg1, O_RDWR);
            if (fd == -1)
            {
                printf("File not exists! Please check the filename!\n");
                continue ;
            }
            if (!verifyFilePass(arg1, fd_stdin))
            {
                printf("Authorization failed\n");
                continue;
            }
            doEncrypt(arg1, fd_stdin);
        }
        else if (strcmp(cmd, "test") == 0)
        {
            doTest(arg1);
        }
        else
            printf("Command not found, please check!\n");
    }

}

/*======================================================================*
                               TestB
 *======================================================================
*/
//B进程
void TestB()
{
    spin("TestB");
}

//C进程
void TestC()
{
    spin("TestC");
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
    int i;
    char buf[256];

    /* 4 is the size of fmt in the stack */
    va_list arg = (va_list)((char*)&fmt + 4);

    i = vsprintf(buf, fmt, arg);

    printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

    /* should never arrive here */
    __asm__ __volatile__("ud2");
}

/*****************************************************************************
 *                                Custom Command
 *****************************************************************************/
char* findpass(char *src)
{
    char pass[128];
    int flag = 0;
    char *p1, *p2;

    p1 = src;
    p2 = pass;

    while (p1 && *p1 != ' ')
    {
        if (*p1 == ':')
            flag = 1;

        if (flag && *p1 != ':')
        {
            *p2 = *p1;
            p2++;
        }
        p1++;
    }
    *p2 = '\0';

    return pass;
}

/*删除用户*/
void doUserDel(char *username)
{
    char passwd[1024];
    char passFilename[128] = "passwd";
    char *p1, *p2;

    //获取密码文件
    int fd;
    fd = open(passFilename, O_RDWR);
    read(fd, passwd, 1024);
    close(fd);

    //定位到那个位置
    char *temp = strcat(username, ":");
    temp = strstr(passwd, temp);

    if (!temp)
    {
        //用户不存在，不用删除
        printl("User not exists");
        printl("\n");
    }
    else
    {
        //处理这一堆鬼
        p1 = temp;
        p2 = temp;

        while (p1 && *p1 != ' ')
        {
            p1++;
        }
        p1++;

        while (p1 && *p1 != '\0')
        {
            *p2 = *p1;
            p1++;
            p2++;
        }

        /*做尾处理*/
        while (p2 != p1)
        {
            *p2 = '\0';
            p2++;
        }
        *p2 = '\0';

        fd = open(passFilename, O_RDWR);
        write(fd, passwd, 1024);
        close(fd);
    }

    /*printl(passwd);*/
    /*printl("\n");*/
}

void doUserAdd(char *username, char *password)
{
    char passwd[1024];
    char passFilename[128] = "passwd";
    char *p1, *p2;

    //获取密码文件
    int fd;
    fd = open(passFilename, O_RDWR);
    read(fd, passwd, 1024);
    close(fd);

    char *newUser = strcat(username, ":");
    strcat(newUser, password);
    strcat(newUser, " ");

    strcat(passwd, newUser);

    /*printl(passwd);*/
    /*printl("\n");*/

    fd = open(passFilename, O_RDWR);
    write(fd, passwd, 1024);
    close(fd);
}

void doPassWd(char *username, char *password, int fd_stdin)
{
    char currentPassword[128];
    char newPassword[128];

    int step = 0;
    while(1)
    {
        if (step == 0)
        {
            printl("Please input your current password:");
            int r = read(fd_stdin, currentPassword, 128);
            if (strcmp(currentPassword, "") == 0)
                continue;
            step = 1;
        }
        else if (step == 1)
        {
            if (strcmp(password, currentPassword) == 0)
            {
                printl("Please input your new password:");
                int r = read(fd_stdin, newPassword, 128);
                if (strcmp(newPassword, "") == 0)
                    continue;
                step = 2;
            }
            else
            {
                printl("Verify failed\n");
                return;
            }
        }
        else if (step == 2)
        {
            doUserDel(username);
            doUserAdd(username, newPassword);
            printl("Your password changed successfully\n");
            return;
        }
    }
}


void login(int fd_stdin, int fd_stdout, int *isLogin, char *user, char *pass)
{
    char username[128];
    char password[128];
    int step = 0;
    int fd;

    char passwd[1024];
    char passFilename[128] = "passwd";

    clearArr(username, 128);
    clearArr(password, 128);
    clearArr(passwd, 1024);

    /*初始化密码文件*/
    fd = open(passFilename, O_CREAT | O_RDWR);
    if (fd == -1)
    {
        //文件已存在，什么都不要做
    }
    else
    {
        //文件不存在，写一个空的进去
        char temp[1024];
        temp[0] = 0;
        write(fd, temp, 1);
        close(fd);
        //给文件赋值
        fd = open(passFilename, O_RDWR);
        write(fd, "root:admin ", 1024);
        close(fd);
    }
    //然后读密码文件
    fd = open(passFilename, O_RDWR);
    read(fd, passwd, 1024);
    close(fd);

    /*printl(passwd);*/
    /*printl("\n");*/

    while (1)
    {
        if (*isLogin)
            return;
        if (step == 0)
        {
            printl("login: ");
            int r = read(fd_stdin, username, 128);
            if (strcmp(username, "") == 0)
                continue;

            /*printl(username);*/
            /*printl("\n");*/
            step = 1;
        }
        else if (step == 1)
        {
            printl("Password: ");
            int r = read(fd_stdin, password, 128);

            /*printl(password);*/
            /*printl("\n");*/

            if (strcmp(username, "") == 0)
                continue;

            char tempArr[128];
            memcpy(tempArr, username, 128);
            strcat(tempArr, ":");
            char *temp = strstr(passwd, tempArr);

            if (!temp)
            {
                printl("Login incorrect\n\n");
            }
            else
            {
                char *myPass = findpass(temp);

                /*printl(myPass);*/
                /*printl("\n");*/

                if (strcmp(myPass, password) == 0)
                {
                    *isLogin = 1;
                    memcpy(user, username, 128);
                    memcpy(pass, password, 128);
                    printTitle();
                }
                else
                {
                    printl("Login incorrect\n\n");
                }
            }

            clearArr(username, 128);
            clearArr(password, 128);

            step = 0;
        }
    }
}


void clearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}

void printTitle()
{
        clear();
        printf("                        ==================================\n");
        printf("                                   Sweetinux v1.0.0             \n");
        printf("                                 Kernel on Orange's \n\n");
        printf("                                     Welcome !\n");
        printf("                        ==================================\n");
}

void clear()
{
    clear_screen(0,console_table[current_console].cursor);
    console_table[current_console].crtc_start = 0;
    console_table[current_console].cursor = 0;
}

void doTest(char *path)
{
    struct dir_entry *pde = find_entry(path);
    printl(pde->name);
    printl("\n");
    printl(pde->pass);
    printl("\n");
}

int verifyFilePass(char *path, int fd_stdin)
{
    char pass[128];

    struct dir_entry *pde = find_entry(path);

    printl(pde->pass);

    if (strcmp(pde->pass, "") == 0)
        return 1;

    printl("Please input the file password: ");
    read(fd_stdin, pass, 128);

    if (strcmp(pde->pass, pass) == 0)
        return 1;

    return 0;
}

void doEncrypt(char *path, int fd_stdin)
{
    //查找文件
    /*struct dir_entry *pde = find_entry(path);*/

    char pass[128];

    printl("Please input the new file password: ");
    read(fd_stdin, pass, 128);

    if (strcmp(pass, "") == 0)
        strstr(pass, "");
    //以下内容用于加密
    int i, j;

    char filename[MAX_PATH];
    memset(filename, 0, MAX_FILENAME_LEN);
    struct inode * dir_inode;

    if (strip_path(filename, path, &dir_inode) != 0)
        return 0;

    if (filename[0] == 0)   /* path: "/" */
        return dir_inode->i_num;

    /**
     * Search the dir for the file.
     */
    int dir_blk0_nr = dir_inode->i_start_sect;
    int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    int nr_dir_entries =
      dir_inode->i_size / DIR_ENTRY_SIZE; /**
                           * including unused slots
                           * (the file has been deleted
                           * but the slot is still there)
                           */
    int m = 0;
    struct dir_entry * pde;
    for (i = 0; i < nr_dir_blks; i++) {
        RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
        pde = (struct dir_entry *)fsbuf;
        for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++)
        {
            if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
            {
                //刷新文件
                strcpy(pde->pass, pass);
                WR_SECT(dir_inode->i_dev, dir_blk0_nr + i);
                return;
                /*return pde->inode_nr;*/
            }
            if (++m > nr_dir_entries)
                break;
        }
        if (m > nr_dir_entries) /* all entries have been iterated */
            break;
    }

}


void help()
{
    printf("=============================================================================\n");
    printf("Command List     :\n");
    printf("1. process       : A process manage,show you all process-info here\n");
    printf("2. filemng       : Run the file manager\n");
    printf("3. clear         : Clear the screen\n");
    printf("4. help          : Show this help message\n");
    printf("5. runttt        : Run a small game on this OS\n");
    printf("==============================================================================\n");
}

//谢志杰进行修改
void ProcessManage()
{
    int i;
    printf("=============================================================================\n");
    printf("      myID      |    name       | spriority    | running?\n");
    //进程号，进程名，优先级，是否是系统进程，是否在运行
    printf("-----------------------------------------------------------------------------\n");
    for ( i = 0 ; i < NR_TASKS + NR_PROCS ; ++i )//逐个遍历
    {
        /*if ( proc_table[i].priority == 0) continue;//系统资源跳过*/
        printf("        %d           %s            %d                yes\n", proc_table[i].pid, proc_table[i].name, proc_table[i].priority);
    }
    printf("=============================================================================\n");
}


//游戏运行库
unsigned int _seed2 = 0xDEADBEEF;

void srand(unsigned int seed){
    _seed2 = seed;
}

int rand() {
    unsigned int next = _seed2;
    unsigned int result;

    next *= 1103515245;
    next += 12345;
    result = ( unsigned int  ) ( next / 65536 ) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= ( unsigned int ) ( next / 65536 ) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= ( unsigned int ) ( next / 65536 ) % 1024;

    _seed2 = next;

    return result;
}

int mat_init(int *mat);

int mat_left(int *mat);
int mat_right(int *mat);
int mat_up(int *mat);
int mat_down(int *mat);

int mat_reach(int *mat);
int mat_insert(int *mat);
void mat_print(int *mat);

int game(int fd_stdin){
    int mat[16] = {0};
    int state = 0;
    char keys[128];
    while(1){
        printl("Init Matrix\n");
        mat_init(mat);

        while(1){
            printf("type in the direction(w a s d):");
            clearArr(keys, 128);
            int r = read(fd_stdin, keys, 128);

            if (strcmp(keys, "a") == 0)
            {
                state = mat_left(mat);
            }
            else if (strcmp(keys, "s") == 0)
            {
                state=mat_down(mat);
            }
            else if (strcmp(keys, "w") == 0)
            {
                state=mat_up(mat);
            }
            else if (strcmp(keys, "d") == 0)
            {
                state=mat_right(mat);
            }
            else if (strcmp(keys, "q") == 0)
            {
                return 0;
            }
            else
            {
                printl("Input Invalid, Please retry\n");
                continue;
            }

            if(state==0){
                printl("can't add,try again!\n");
                continue;
            }
            if(mat_reach(mat)){
                printf("You Win\n");
                break;
            }
            if(!mat_insert(mat)){
                printf("You Lose\n");
                break;
            }
            mat_print(mat);
        }

        printf("another one?(y or n):");

        clearArr(keys, 128);
        int r = read(fd_stdin, keys, 128);
        if (strcmp(keys, "n"))
        {
            break;
        }
    }
    return 0;
}

int mat_init(int *mat)
{
    int i, j;
    //给一个随机数
    /*srand(546852);*/
    mat_insert(mat);
    mat_insert(mat);
    mat_print(mat);
    return 0;
}

int mat_left(int *mat){
    printl("Left\n");

    int i,j;
    int flag=0;
    int k=0,temp[4]={0},last=0;
    for(i=0;i<4;i++){
        memset(temp,0,sizeof(int)*4);
        for(j=0,k=0,last=0;j<4;j++){
            if(mat[i*4+j]!=0){
                temp[k]=mat[i*4+j];
                mat[i*4+j]=0;
                last=j+1;
                k++;
            }
        }
        if(k<last) flag=1;
        for(j=0;j<3;j++){
            if(temp[j]>0&&temp[j]==temp[j+1]){
                temp[j]+=temp[j];
                temp[j+1]=0;
                flag=1;
            }
        }
        for(j=0,k=0;k<4;k++){
            if(temp[k]!=0){
                mat[i*4+j]=temp[k];
                j++;
            }
        }
    }
    return flag;
}

int mat_right(int *mat){
    printl("Right\n");

    int i,j;
    int flag=0;
    int k=0,temp[4]={0},last=0;
    for(i=0;i<4;i++){
        memset(temp,0,sizeof(int)*4);
        for(j=3,k=3,last=3;j>=0;j--){
            if(mat[i*4+j]!=0){
                temp[k]=mat[i*4+j];
                mat[i*4+j]=0;
                last=j-1;
                k--;
            }
        }
        if(k>last) flag=1;
        for(j=3;j>=0;j--){
            if(temp[j]>0&&temp[j]==temp[j+1]){
                temp[j]+=temp[j];
                temp[j+1]=0;
                flag=1;
            }
        }
        for(j=3,k=3;k>=0;k--){
            if(temp[k]!=0){
                mat[i*4+j]=temp[k];
                j--;
            }
        }
    }
    return flag;
}

int mat_up(int *mat){
    printl("Up\n");

    int i,j;
    int flag=0;

    int k=0,temp[4]={0},last=0;
    for(i=0;i<4;i++){
        memset(temp,0,sizeof(int)*4);
        for(j=0,k=0,last=0;j<4;j++){
            if(mat[j*4+i]!=0){
                temp[k]=mat[j*4+i];
                mat[j*4+i]=0;
                last=j+1;
                k++;
            }
        }
        if(k<last) flag=1;
        for(j=0;j<3;j++){
            if(temp[j]>0&&temp[j]==temp[j+1]){
                temp[j]+=temp[j];
                temp[j+1]=0;
                flag=1;
            }
        }
        for(j=0,k=0;k<4;k++){
            if(temp[k]!=0){
                mat[j*4+i]=temp[k];
                j++;
            }
        }
    }
    return flag;
}

int mat_down(int *mat){
    printl("Down\n");

    int i,j;
    int flag=0;
    int k=0,temp[4]={0},last=0;
    for(j=0;j<4;j++){
        memset(temp,0,sizeof(int)*4);
        for(i=3,k=3,last=3;i>=0;i--){
            if(mat[i*4+j]!=0){
                temp[k]=mat[i*4+j];
                mat[i*4+j]=0;
                last=i-1;
                k--;
            }
        }
        if(k>last) flag=1;
        for(i=3;i>0;i--){
            if(temp[i]>0&&temp[i]==temp[i-1]){
                temp[i]+=temp[i];
                temp[i-1]=0;
                flag=1;
            }
        }
        for(i=3,k=3;k>=0;k--){
            if(temp[k]!=0){
                mat[i*4+j]=temp[k];
                i--;
            }
        }
    }
    return flag;
}

int mat_reach(int *mat){
    int i, j;
    for(i = 0; i < 4; i++){
        for(j = 0; j < 4; j++){
            if(mat[i*4+j] == 2048)
                return 1;
        }
    }
    return 0;
}

int mat_insert(int *mat){
    char temp[16] = {0};
    int i, j, k = 0;
    for(i = 0; i < 4; i++){
        for(j = 0; j < 4; j++){
            if(mat[i*4+j] == 0){
                temp[k] = 4 * i + j;
                k++;
            }
        }
    }
    if(k == 0) return 0;
    k = temp[rand() % k];
    //随便给一个地方2或者4
    mat[((k-k%4)/4)*4+k%4]=2<<(rand()%2);
    return 1;
}

void mat_print(int *mat){
    int i, j;
    for(i = 0; i < 4; i++){
        for(j = 0; j < 4; j++){
            //这里需要规格化
            printf("%4d", mat[i*4+j]);
        }
        printf("\n");
    }
}
