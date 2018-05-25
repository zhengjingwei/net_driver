#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/mii.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/ethtool.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/of.h>
#include <mach/board.h>
#include <mach/slcr.h>
#include <linux/interrupt.h>
#include <linux/clocksource.h>
#include <linux/timecompare.h>
#include <linux/net_tstamp.h>

#include <linux/init.h> //指定初始化和清除函数
#include <linux/cdev.h> //cdev结构的头文件
#include <linux/slab.h>
 #include <linux/socket.h>  
#include <linux/net.h>  
#include <linux/in.h> 
 
#include "linux/fs.h"   
#include "linux/types.h"  
#include "linux/errno.h"  
#include "linux/uaccess.h"  
#include "linux/kdev_t.h" 

#include "aproto.h"


#define u8 unsigned char
#define u32 unsigned int

#define SEND_ADDR_NUM       32
#define REV_ADDR_NUM        32
#define SEND_BUFF_LEN       4096
#define REV_BUFF_LEN        8192

#define SEND_BUFF_ADDR      0xe000000 //data's phys addr for sending 
#define REV_BUFF_ADDR       0xf000000 //received data's phys addr
#define CTRL_BUFF_ADDR      0xe000000 //ctrl_msg's phys addr 
//文件操作函数的声明
int paddr_manager_open(struct inode *, struct file *);
int paddr_manager_release(struct inode *, struct file *);
ssize_t paddr_manager_read(struct file *, char *, size_t, loff_t *);
ssize_t paddr_manager_write(struct file *, const char *, size_t, loff_t *);
ssize_t paddr_manager_mmap(struct file *, struct vm_area_struct *);

int dev_major = 1255; //指定主设备号
int dev_minor = 0; //指定次设备号

static struct class *firstdrv_class;
static struct device *firstdrv_class_dev;

struct cdev *paddr_manager_cdev; //内核中表示字符设备的结构
u32 gp_testdata[3];//control massage

u8 *data;
u32 time_f;

//u32 send_vaddr[SEND_ADDR_NUM];
//u32 rev_vaddr[REV_ADDR_NUM];
//u32 send_paddr[SEND_ADDR_NUM];
//u32 rev_paddr[REV_ADDR_NUM];
//wait_queue_head_t my_queue;


//*****addr manage structure
struct addr_ring {
    u8* send_vaddr[SEND_ADDR_NUM];
    u8* rev_vaddr[REV_ADDR_NUM];
    u32 send_paddr[SEND_ADDR_NUM];
    u32 rev_paddr[REV_ADDR_NUM];
    int send_addr_num;
    int rev_addr_num;

    u8* ctrl_msg;
    u32 ctrl_msg_mapping;

};


struct addr_ring addr_ring1;


//for test
void set_ctrl_msg(struct addr_ring *ar)
{
    int i,size;
    u32 user_ctrl[3]={0xe000000,104857600,0xf000000};

//******alloc ctrl buffers and ioremap_nocache it with a virture address    
    size=64;
    ar->ctrl_msg_mapping=CTRL_BUFF_ADDR;
    ar->ctrl_msg=ioremap_nocache(ar->ctrl_msg_mapping,size);

    for(i=0;i<4;i++)
    {
        *(ar->ctrl_msg+3-i)=(u8)(user_ctrl[0]>>(8*i));
    }
    for(i=0;i<4;i++)
    {
        *(ar->ctrl_msg+7-i)=(u8)(user_ctrl[1]>>(8*i));
    }
    for(i=0;i<4;i++)
    {
        *(ar->ctrl_msg+11-i)=(u8)(user_ctrl[2]>>(8*i));
    }

}

int init_addr_ring(struct addr_ring *ar)
{

    int i,size=4096;

    ar->send_addr_num=SEND_ADDR_NUM;
    ar->rev_addr_num=REV_ADDR_NUM;

//*******alloc sending data and ioremap_nocache it with a virture address
    for(i=0;i<SEND_ADDR_NUM;i++)
    {
        ar->send_paddr[i]=(SEND_BUFF_ADDR+i*SEND_BUFF_LEN);
        ar->send_vaddr[i]=ioremap_nocache(ar->send_paddr[i],size);
    }
//******initialize sending data with 2
    for(i=0;i<size;i++)
    {
        *(ar->send_vaddr[0]+i)=2;
    }

//******alloc receive buffers and ioremap_nocache it with a virture address 
    size=8192;
    for(i=0;i<REV_ADDR_NUM;i++)
    {
        ar->rev_paddr[i]=(REV_BUFF_ADDR+i*REV_BUFF_LEN);
        ar->rev_vaddr[i]=ioremap_nocache(ar->rev_paddr[i],size);
    }

//******initialize receive buffers with 1 
    for(i=0;i<size;i++)
    {
        *(ar->rev_vaddr[0]+i)=1;
    }


    set_ctrl_msg(ar);

    return 0;

}



int dev_sendmsg(u32 *user_ctrl)
{
    struct socket *sock; 
    //int    sockfd;
    //u32   sendline[1]={phy};
    int i,ret,len=64;
    struct sockaddr_in    servaddr;
    struct msghdr msg ; //non used but needed as parameter 
    struct kvec vec;

    struct net_device *ndev=get_eth0();
    struct net_local *lp = netdev_priv(ndev);
    //addr_ring1.ctrl_msg=lp->nctrl_msg;
    //addr_ring1.ctrl_msg_mapping=lp->nctrl_msg_mapping;


    for(i=0;i<4;i++)
    {
        *(addr_ring1.ctrl_msg+3-i)=(u8)(user_ctrl[0]>>(8*i));
    }
    for(i=0;i<4;i++)
    {
        *(addr_ring1.ctrl_msg+7-i)=(u8)(user_ctrl[1]>>(8*i));
    }
    for(i=0;i<4;i++)
    {
        *(addr_ring1.ctrl_msg+11-i)=(u8)(user_ctrl[2]>>(8*i));
    }
    //len=user_ctrl[1];
    lp->is_first=1;
    lp->rx_package=0;
    lp->test_num=user_ctrl[1]/4096*3;
    printk(KERN_ERR"lp->test_num=%d\n",lp->test_num); 
    lp->asend.asend_DmaSetupRecvBuffers(user_ctrl[2],user_ctrl[1], lp->ndev);
    //printk(KERN_ERR "init lp->rx_package =  %d\n",lp->rx_package);
    //addr_ring1.ctrl_msg[1]=user_ctrl[1];
    /*struct iovec iov; 
    iov.iov_base = sendline;  
    iov.iov_len = len; 
    msg.msg_name = NULL;  
    msg.msg_iov = &iov;  
    msg.msg_iovlen = 1; // 只有一个数据块   
    msg.msg_control = NULL;  
    msg.msg_controllen = 0;  
    msg.msg_namelen = 0; */
    //vec.iov_base=&user_ctrl[0];
    //vec.iov_len=user_ctrl[1];
    vec.iov_base=&user_ctrl[0];
    vec.iov_len=64;

    
    sock=(struct socket *)kmalloc(sizeof(struct socket),GFP_KERNEL); 
    ret=sock_create_kern(AF_AS, SOCK_SEQPACKET,0,&sock); 
 
        if(ret<0){  
                printk(KERN_ERR"socket create error!\n");  
                return ret;  
        }  
       // printk(KERN_ERR"socket create ok!\n"); 
        
    ret=kernel_sendmsg(sock,&msg,&vec,1,len); /*send message */  
        if(ret<0){  
                printk(KERN_ERR" kernel_sendmsg error!\n");  
                return ret;  
        }//else
    //  printk(KERN_ERR"send ok!\n"); 
    return ret;

}

int find_vaddr_idx(u32 paddr)
{
    int idx=(paddr-REV_BUFF_ADDR)/REV_BUFF_LEN;
    return idx;

}

int dev_revmsg(void)
{

    //printk(KERN_ERR"got into read\n"); 
    struct net_device *ndev=get_eth0();
    struct net_local *lp = netdev_priv(ndev);
    int i,j,k;
    //u32 time;
    u32 all_pages=gp_testdata[1]/4096;
    //u32 all_package=lp->test_num;
    u32 all_package=all_pages*3;
    //u32 vaddr_idx=find_vaddr_idx(gp_testdata[2]);
    //k=jiffies;
    //printk(KERN_ERR"got into dev_revmsg\n");
    //printk(KERN_ERR"init all_package = %d   in revmsg\n",all_package);
    //printk(KERN_ERR "init lp->rx_package =  %d\n",lp->rx_package);
    //printk(KERN_ERR "init lp->rx_state =  %d\n",lp->rx_state);
    
    /*do{
        //if (filp->f_flags & O_NONBLOCK) //判断是否按照非阻塞的方式处理。  
            //return -EAGAIN; //处理方式，直接返回。  

        //wait_event_interruptible(dev->inq,have_data); 
        printk(KERN_ERR"got into dev_revmsg's while\n");
        if(lp->rx_package==all_package)
        {

                time=jiffies-time_f;
                printk(KERN_ERR"one communication time is %d\n",time);
                for(i=0;i<all_pages;i++)
                    for(j=0;j<10;j++)
                        printk(KERN_ERR"the %d rev page's 4091+%d data is %d \n",
                            i,j,*(addr_ring1.rev_vaddr[vaddr_idx+i]+4091+j));
            
            
        }
        time=jiffies-k;
        if(time>=1000){
            printk(KERN_ERR"the out-of-time is %d\n",time);
            printk(KERN_ERR "out-of-time lp->rx_package =  %d\n",lp->rx_package);
            printk(KERN_ERR "out-of-time lp->rx_state =  %d\n",lp->rx_state);
            break;
        }
    }while(lp->rx_package!=all_package);*/
    while(lp->rx_package!=all_package)
    {
    //  k=jiffies;
        //lp->rev_time=jiffies;
    //  printk(KERN_ERR"got into dev_revmsg's while\n");
        wait_event_interruptible(lp->rev_irp,(lp->rx_package==all_package));
    //  printk(KERN_ERR"woke up in recvmsg\n");
    //  time=jiffies-k;
    //  printk(KERN_ERR"sleep time is %d\n",time);
    //  time=jiffies-time_f;
    //  printk(KERN_ERR"whole communication time is %d\n",time);
    //  printk(KERN_ERR "recvd lp->rx_package =  %d\n",lp->rx_package);
    //  if(time>=300)
        //lp->rx_package=0;
        //return 0;
        break;

    }
    //for(i=0;i<3;i++)
    //  for(j=0;j<10;j++)
    //      printk(KERN_ERR"the %d rev page's 4091+%d data is %d \n",
    //          i,j,*(addr_ring1.rev_vaddr[vaddr_idx+i]+4091+j));
    //lp->rx_state==0;
    lp->rx_package=0;
    return 0;

}



/*int u32_to_u8(u32* paddr, u8* caddr )
{
    *caddr=*((u8*)paddr);
    *(caddr+1)=(u8)(*paddr>>8);
    *(caddr+2)=(u8)(*paddr>>16);
    *(caddr+3)=(u8)(*paddr>>24);

}

int u8_to_u32(u8* caddr, u32* paddr )
{
    *paddr=(u32)caddr[0];
    *paddr=(*paddr)+((u32)caddr[1]<<8);
    *paddr=(*paddr)+((u32)caddr[2]<<16);
    *paddr=(*paddr)+((u32)caddr[3]<<24);

}*/

struct file_operations paddr_manager_fops= //将文件操作与分配的设备号相连
{
    owner: THIS_MODULE, //指向拥有该模块结构的指针
    open: paddr_manager_open,
    release: paddr_manager_release,
    read: paddr_manager_read,
    write: paddr_manager_write,
    mmap: paddr_manager_mmap,
};

static void __exit paddr_manager_exit(void) //退出模块时的操作
{
    /*int size=1496;
    if (addr_ring1.ctrl_msg) {
        dma_free_coherent(firstdrv_class_dev, size,
            addr_ring1.ctrl_msg, addr_ring1.ctrl_msg_mapping);
        addr_ring1.ctrl_msg = NULL;
    }*/

    dev_t devno=MKDEV(dev_major, dev_minor); //dev_t是用来表示设备编号的结构

    cdev_del(paddr_manager_cdev); //从系统中移除一个字符设备
        kfree(paddr_manager_cdev); //释放自定义的设备结构
    //kfree(gp_testdata);
    //kfree(data);
    unregister_chrdev_region(devno, 1); //注销已注册的驱动程序

    device_unregister(firstdrv_class_dev); //删除/dev下对应的字符设备节点
    class_destroy(firstdrv_class);

    printk("paddr_manager unregister success\n");
}

static int __init paddr_manager_init(void) //初始化模块的操作
{
    int ret, err,i,size=4096;
    dev_t devno;
#if 1
    //动态分配设备号，次设备号已经指定
    ret=alloc_chrdev_region(&devno, dev_minor, 1, "paddr_manager");
    //保存动态分配的主设备号
    dev_major=MAJOR(devno);

#else
    //根据期望值分配设备号
    devno=MKDEV(dev_major, dev_minor);
    ret=register_chrdev_region(devno, 1, "paddr_manager");
#endif

    if(ret<0)
    {
    printk("paddr_manager register failure\n");
        //paddr_manager_exit(); //如果注册设备号失败就退出系统
    return ret;
    }
    else
    {
        printk("paddr_manager register success\n");
    }

    //gp_testdata = kmalloc(sizeof(int), GFP_KERNEL);
#if 0   //两种初始化字符设备信息的方法
    paddr_manager_cdev = cdev_alloc();//调试时，此中方法在rmmod后会出现异常，原因未知
    paddr_manager_cdev->ops = &my74hc595_fops;
#else
    paddr_manager_cdev = kmalloc(sizeof(struct cdev), GFP_KERNEL);
    cdev_init(paddr_manager_cdev, &paddr_manager_fops);
#endif

    paddr_manager_cdev->owner = THIS_MODULE; //初始化cdev中的所有者字段

    err=cdev_add(paddr_manager_cdev, devno, 1); //向内核添加这个cdev结构的信息
    if(err<0)
        printk("add device failure\n"); //如果添加失败打印错误消息

    firstdrv_class = class_create(THIS_MODULE, "paddr_manager");
    firstdrv_class_dev = device_create(firstdrv_class, NULL, MKDEV(dev_major, 0), NULL,"paddr_manager-%d", 0);//在/dev下创建节点

    /*data = kmalloc(4096, GFP_KERNEL);
    for(i=0;i<4096;i++)
    {
        *(data+i)=i%100;
    }*/

    //init_waitqueue_head(&my_queue);

    init_addr_ring(&addr_ring1);

    /*data = ioremap_nocache(addr_ring1.rev_paddr[0],size);

    for(i=0;i<4096;i++)
    {
        *(data+i)=i%100;
    }
    for(i=0;i<20;i++)
    {
        printk(KERN_ERR"data[%d]= %d\n",i,*(data+i));
    }*/



    printk("register paddr_manager dev OK\n");

    return 0;
}
//打开设备文件系统调用对应的操作
int paddr_manager_open(struct inode *inode, struct file *filp)
{
    //将file结构中的private_data字段指向已分配的设备结构
    filp->private_data = gp_testdata;
    printk("open paddr_manager dev OK\n");
    return 0;
}
//关闭设备文件系统调用对应的操作
int paddr_manager_release(struct inode *inode, struct file *filp)
{
    printk("close paddr_manager dev OK\n");
    return 0;
}
//读设备文件系统调用对应的操作
ssize_t paddr_manager_read(struct file *filp, char *buf, size_t len, loff_t *off)
{

    dev_revmsg();
    return 0;
    

}
//写设备文件系统调用对应的操作
ssize_t paddr_manager_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
    
    int size=3*sizeof(u32);
    //获取指向已分配数据的指针
    u32 *p_testdata = filp->private_data;
    //time_f=jiffies;
    //printk("before copy_from_user\n");
    //从用户空间复制数据到内核中的设备变量
    if(copy_from_user(p_testdata, buf,size))
    {
        return -EFAULT;
    }

    //printk("*p_testdata = %x\n",*p_testdata);
    //printk("*(p_testdata+1) =%d\n",*(p_testdata+1));
    //printk("*(p_testdata+2) =%x\n",*(p_testdata+2));
    dev_sendmsg(p_testdata);

    
    return sizeof(int); //返回写数据的大小
}

//************mmap raw data to user
ssize_t paddr_manager_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long pfn = ((gp_testdata[2]+4096) >> PAGE_SHIFT);  //mmap at second page

    if (remap_pfn_range(vma, vma->vm_start, pfn,
        vma->vm_end - vma->vm_start,
        vma->vm_page_prot))
    return -EAGAIN;

    return 0;
}


module_init(paddr_manager_init);
module_exit(paddr_manager_exit);

MODULE_AUTHOR("Arnold");
MODULE_DESCRIPTION("Module paddr_manager");
MODULE_VERSION("1.0");
MODULE_LICENSE("Dual BSD/GPL");



