#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include<stdio.h>

int  main( )
{
    printf("%d /n",htons(16));
    return 0;
}

