#include<stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/mman.h>
#include<sys/times.h>

#define u8 unsigned char
#define u32 unsigned int
#define page_map_file "/proc/self/pagemap"
#define PFN_MASK ((((uint64_t)1)<<55)-1)
#define PFN_PRESENT_FLAG (((uint64_t)1)<<63)

#define u8 unsigned char
#define u32 unsigned int

#define AF_AS   21
#define MAXLINE 4096
#define DCACHE  (1<<1)

int main()
{
    u32 data[3];
    u8 *rev_data;
   // u32 size=1496;
  //  u32 phy=0xe100000;
    int i,size_d=3*sizeof(u32),test_round,cnt=0;
    int fd;
    char *name="eth0";
    char dir[]="/dev/net_receive";
    char order;
    double all_time_clock,all_time_second,throughput;
    clock_t time_start;
    clock_t time_end;

    data[0]=0xe000000;
    data[1]=1024*1024*128;
    data[2]=0xf000000;


    //fd = open(dir, O_RDWR | O_NONBLOCK);
    fd=open("/dev/paddr_manager-0", O_RDWR);

   // printf("fd= %d\n",fd);
   // test_round=500;

    if (fd != -1)
    {
        time_start=times(NULL);
        while((times(NULL)-time_start)<=100)
        {
            write(fd, data, size_d);

            cnt++;
            read(fd, data, size_d);

        }
        close(fd); //关闭设备文件
        //return 0;
    }
    else
    {
        printf("Device open failed\n");
        return -1;
    }

    time_end=times(NULL);
    all_time_clock=(double)(time_end-time_start);
    all_time_second=all_time_clock/sysconf(_SC_CLK_TCK);
    throughput=cnt*data[1]*8/all_time_second/1024/1024 ;//Mbps

    printf("all_time_clock = %lf \n",all_time_clock);
    printf("all_time_second = %lf\n",all_time_second);
    data[1]=data[1]/1024;
    printf("test size = %d KB\n",data[1]);
   // printf("test_round = %d \n",test_round);
    printf("throughput = %lf Mbps\n ",throughput);
    printf("cnt = %d\n ",cnt);

}