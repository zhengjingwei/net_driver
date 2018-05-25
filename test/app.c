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
    int i,size_d=3*sizeof(u32);
    int fd;
    char *name="eth0";
    char dir[]="/dev/net_receive";
    char order;

    data[0]=0xe000000;//ctrl_msg's physical address,  whole size is 64, used 3
    data[1]=104857600;//requesting data'size: 100M
    data[2]=0xf000000;//receiving data's physical address
    //fd = open(dir, O_RDWR | O_NONBLOCK);
    fd=open("/dev/paddr_manager-0", O_RDWR);

    printf("fd= %d\n",fd);

    if (fd != -1)
    {

        write(fd, data, size_d);
        read(fd, data, size_d);
        rev_data=mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        for(i=0;i<100;i++)
        {
            printf("the rev_data[%d]= %d\n",i,*(rev_data+i));
        }
        for(i=0;i<100;i++)
        {
            printf("the rev_data[%d]= %d\n",i+3996,*(rev_data+3996+i));
        }
        close(fd); //关闭设备文件
        //return 0;
    }
    else
    {
        printf("Device open failed\n");
        return -1;
    }

}
