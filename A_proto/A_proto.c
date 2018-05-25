#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/in.h>
#include <linux/poll.h>
#include <net/sock.h>
#include "aproto.h"

int as_sendmsg(struct kiocb *iocb,struct socket *sock, struct msghdr *msg, size_t payload_len)
{
    //unsigned int phy;
    u32 paddr = *(u32*)(msg->msg_iov->iov_base);
    int len = msg->msg_iov->iov_len;
    struct net_device *ndev = get_eth0();
    struct net_local *lp = netdev_priv(ndev);
    //phy = virt_to_phys(lp->data_send);
    //printk(KERN_ERR"msg->msg_iov->iov_base in as_sendmsg before lp->asend is %x \n",paddr);
    //printk(KERN_ERR"sending msg len is %d \n",len);
    lp->asend.asend_op(paddr, len, ndev);
    //printk(KERN_ERR"\n  successfully call as_sendmsg in aproto.h\n");
    //printk(KERN_ERR"msg->msg_iov->iov_base in as_sendmsg is %x \n",paddr);
    return 0;
}

int as_recvmsg(struct kiocb *iocb, struct socket *sock, struct msghdr *msg, size_t size,
        int msg_flags)
{
    //struct net_device *ndev=get_eth0();
    //struct net_local *lp = netdev_priv(ndev);
    u32 phy;
    printk(KERN_ERR"\n  successfully call as_revmsg at first line\n");
    //u32 *pphy;
    //struct iovec iov[1];
    //printk(KERN_ERR"\n  successfully call as_revmsg after define\n");
    //phy=(u32)lp->data_rev;
    phy = 0xe200000;
    //pphy=&phy;
    printk(KERN_ERR"recvmsg msg success before copy_to_user,phy= %x\n",phy);
    if(copy_to_user(msg->msg_iov->iov_base,&phy, sizeof(u32)))
    {
        return -EFAULT;
        printk(KERN_ERR"failed copy phys_addr in as_revmsg\n");
    }else{
        printk(KERN_ERR"success copy phys_addr in as_revmsg\n");
        
    }
    //memcpy(msg->msg_iov->iov_base,&phy, sizeof(u32));
    //iov[0].iov_len=sizeof(u32);
    //msg->msg_iov=iov;

    printk(KERN_ERR"\n  successfully call as_revmsg in aproto.h\n");
    return 0;

}

void net_buff_receive(struct sk_buff* skb)
{
    printk(KERN_ERR "\n  successfully call net_buff_receive in aproto.h\n");
}

/*
 * This is called as the final descriptor referencing this socket is closed.
 * We have to unbind the socket so that another socket can be bound to the
 * address it was using.
 *
 * We have to be careful about racing with the incoming path.  sock_orphan()
 * sets SOCK_DEAD and we use that as an indicator to the rx path that new
 * messages shouldn't be queued.
 */

static int as_release(struct socket *sock)
{
    struct sock *sk = sock->sk;
    sock->sk = NULL;
    sock_put(sk);
    return 0;
}

static int as_getname(struct socket *sock, struct sockaddr *uaddr,
               int *uaddr_len, int peer)
{
    return 0;
}

/*
 * RDS' poll is without a doubt the least intuitive part of the interface,
 * as POLLIN and POLLOUT do not behave entirely as you would expect from
 * a network protocol.
 *
 * POLLIN is asserted if
 *  -   there is data on the receive queue.
 *  -   to signal that a previously congested destination may have become
 *  uncongested
 *  -   A notification has been queued to the socket (this can be a congestion
 *  update, or a RDMA completion).
 *
 * POLLOUT is asserted if there is room on the send queue. This does not mean
 * however, that the next sendmsg() call will succeed. If the application tries
 * to send to a congested destination, the system call may still fail (and
 * return ENOBUFS).
 */
static unsigned int as_poll(struct file *file, struct socket *sock,
                 poll_table *wait)
{
    struct sock *sk = sock->sk;
    unsigned int mask = 0;
    unsigned long flags;

    poll_wait(file, sk_sleep(sk), wait);
    return mask;
}

static int as_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
    return -ENOIOCTLCMD;
}


static int as_setsockopt(struct socket *sock, int level, int optname,
              char __user *optval, unsigned int optlen)
{
    int ret = 0;
    return ret;
}

static int as_getsockopt(struct socket *sock, int level, int optname,
              char __user *optval, int __user *optlen)
{
    int ret =0;
    return ret;
}

static int as_connect(struct socket *sock, struct sockaddr *uaddr,
               int addr_len, int flags)
{
    int ret = 0;
    return ret;
}

static struct proto as_proto = {
    .name     = "AS",
    .owner    = THIS_MODULE,
    .obj_size = sizeof(struct sock),
};

static const struct proto_ops as_proto_ops = {
    .family =   AF_AS,
    .owner =    THIS_MODULE,
    .release =  as_release,
    .bind =     sock_no_bind,
    .connect =  as_connect,
    .socketpair =   sock_no_socketpair,
    .accept =   sock_no_accept,
    .getname =  as_getname,
    .poll =     as_poll,
    .ioctl =    as_ioctl,
    .listen =   sock_no_listen,
    .shutdown = sock_no_shutdown,
    .setsockopt =   as_setsockopt,
    .getsockopt =   as_getsockopt,
    .sendmsg =  as_sendmsg,
    .recvmsg =  as_recvmsg,
    .mmap =     sock_no_mmap,
    .sendpage = sock_no_sendpage,
};

static int __as_create(struct socket *sock, struct sock *sk, int protocol)
{
    struct as_sock *as;

    sock_init_data(sock, sk);
    sock->ops       = &as_proto_ops;
    sk->sk_protocol     = protocol;

    return 0;
}

static int as_create(struct net *net, struct socket *sock, int protocol,
              int kern)
{
    struct sock *sk;

    if (sock->type != SOCK_SEQPACKET || protocol)
        return -ESOCKTNOSUPPORT;

    sk = sk_alloc(net, AF_AS, GFP_ATOMIC, &as_proto);
    if (!sk)
        return -ENOMEM;

    return __as_create(sock, sk, protocol);
}


static const struct net_proto_family as_family_ops = {
    .family =   AF_AS,
    .create =   as_create,
    .owner  =   THIS_MODULE,
};



static void as_exit(void)
{
    sock_unregister(as_family_ops.family);
    proto_unregister(&as_proto);
}
module_exit(as_exit);

static int as_init(void)
{
    int ret;

    printk(KERN_ERR "as_init\n");
    ret = proto_register(&as_proto, 1);
    if (ret)
    {
        goto out;
        printk(KERN_ERR "proto_register wrong\n");
    }
    ret = sock_register(&as_family_ops);
    if (ret)
    {
        goto out_proto;
        printk(KERN_ERR "sock_register wrong\n");
    }

    goto out;

out_proto:
    proto_unregister(&as_proto);
out:
    return ret;
}
module_init(as_init);

#define DRV_VERSION     "1.0"
#define DRV_RELDATE     "Dec 6, 2016"

MODULE_AUTHOR("Arnold");
MODULE_DESCRIPTION("AS: Advanced Sockets"
           " v" DRV_VERSION " (" DRV_RELDATE ")");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_ALIAS_NETPROTO(PF_AS);