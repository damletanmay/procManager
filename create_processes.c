#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>

const char *file_path = "ids.txt"; // file path
int fd;                                                                             // file descriptor which will be shared

// signal handler for sigint which will remove the file and exit
void sig_int_handler(int signal)
{
    close(fd);
    remove(file_path);
    exit(-1);
}

// infinite loop
void loop()
{
    while (true)
    {
    }
}

// write ids to file
void write_id(int id, int fd)
{
    char *string_id = malloc(sizeof(char) * 10);
    sprintf(string_id, "%d", id);
    strcat(string_id, "\n");
    write(fd, string_id, strlen(string_id));
}

// program to create multiple processes for testing
void main()
{
    // open file
    fd = open(file_path, O_CREAT | O_RDWR, 0777);

    // comments according to the graph 4 forks.png at folder /home/damlet/Desktop/asp_assignment/assignment_2/
    int p1 = fork();
    int p2 = fork();
    int p3 = fork();
    int p4 = fork();

    // for main process M
    if (p1 > 0 && p2 > 0 && p3 > 0 && p4 > 0)
    {
        signal(SIGINT, sig_int_handler); // register signal handler for main process
        printf("Main Process ID:%d\n", getpid());
        printf("Bash Process ID:%d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p1 child
    else if (p1 == 0 && p2 > 0 && p3 > 0 && p4 > 0)
    {
        sleep(1);
        printf("Process ID of P1: %d\n", getpid());
        printf("Parent ID of P1: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p2 child
    else if (p1 > 0 && p2 == 0 && p3 > 0 && p4 > 0)
    {
        sleep(2);
        printf("Process ID of P2: %d\n", getpid());
        printf("Parent ID of P2: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p3 child
    else if (p1 > 0 && p2 > 0 && p3 == 0 && p4 > 0)
    {
        sleep(3);
        printf("Process ID of P3: %d\n", getpid());
        printf("Parent ID of P3: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p4 child
    else if (p1 > 0 && p2 > 0 && p3 > 0 && p4 == 0)
    {
        sleep(4);
        printf("Process ID of P4: %d\n", getpid());
        printf("Parent ID of P4: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p5 child
    else if (p1 == 0 && p2 == 0 && p3 > 0 && p4 > 0)
    {
        sleep(5);
        printf("Process ID of P5: %d\n", getpid());
        printf("Parent ID of P5: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p6 child
    else if (p1 == 0 && p2 > 0 && p3 == 0 && p4 > 0)
    {
        sleep(6);
        printf("Process ID of P6: %d\n", getpid());
        printf("Parent ID of P6: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p7 child
    else if (p1 == 0 && p2 > 0 && p3 > 0 && p4 == 0)
    {
        sleep(7);
        printf("Process ID of P7: %d\n", getpid());
        printf("Parent ID of P7: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p8 child
    else if (p1 > 0 && p2 == 0 && p3 == 0 && p4 > 0)
    {
        sleep(8);
        printf("Process ID of P8: %d\n", getpid());
        printf("Parent ID of P8: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p9 child
    else if (p1 > 0 && p2 == 0 && p3 > 0 && p4 == 0)
    {
        sleep(9);
        printf("Process ID of P9: %d\n", getpid());
        printf("Parent ID of P9: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p10 child
    else if (p1 > 0 && p2 > 0 && p3 == 0 && p4 == 0)
    {
        sleep(10);
        printf("Process ID of P10: %d\n", getpid());
        printf("Parent ID of P10: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p11 child
    else if (p1 == 0 && p2 == 0 && p3 == 0 && p4 > 0)
    {
        sleep(11);
        printf("Process ID of P11: %d\n", getpid());
        printf("Parent ID of P11: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p12 child
    else if (p1 == 0 && p2 == 0 && p3 > 0 && p4 == 0)
    {
        sleep(12);
        printf("Process ID of P12: %d\n", getpid());
        printf("Parent ID of P12: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p13 child
    else if (p1 == 0 && p2 > 0 && p3 == 0 && p4 == 0)
    {
        sleep(13);
        printf("Process ID of P13: %d\n", getpid());
        printf("Parent ID of P13: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p14 child
    else if (p1 > 0 && p2 == 0 && p3 == 0 && p4 == 0)
    {
        sleep(14);
        printf("Process ID of P14: %d\n", getpid());
        printf("Parent ID of P14: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
    // for p15 child
    else if (p1 == 0 && p2 == 0 && p3 == 0 && p4 == 0)
    {
        sleep(15);
        printf("Process ID of P15: %d\n", getpid());
        printf("Parent ID of P15: %d\n", getppid());
        write_id(getpid(), fd);
        loop();
    }
}