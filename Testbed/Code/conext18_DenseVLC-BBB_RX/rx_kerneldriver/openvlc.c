/*
 Copyright (c) 2014, IMDEA NETWORKS Institute
 
 This file is part of the OpenVLC's source codes.
 
 OpenVLC's source codes are free: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 OpenVLC's source codes are distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with the source codes of OpenVLC.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <linux/sched.h>
#include <linux/interrupt.h> // mark_bh 
#include <linux/in.h>
#include <linux/netdevice.h>   // struct device, and other headers 
#include <linux/skbuff.h>
#include <linux/in6.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>
#include <asm/io.h>
//#include <asm/system.h> DONE This was removed in 3.4 kernel
#include <linux/iio/consumer.h>
#include <linux/iio/iio.h>
#include <asm/checksum.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/wait.h>
#include <linux/timex.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>

#include <linux/module.h>
//#include <rtdm/rtdm_driver.h>
#include <asm/io.h>		// ioremap

#include <linux/proc_fs.h>

#include <linux/skbuff.h>
#include <linux/ip.h>          
#include <linux/tcp.h>         
#include <linux/if_arp.h>
#include <linux/rslib.h>

#include "openvlc.h"
#define DEVICE_NAME "vlc"

// start for ioctl
#include <linux/fs.h> // required for various structures related to files liked fops. 
#include <linux/semaphore.h>
#include <linux/cdev.h>
// end for ioctl

// start for eth socket
static struct socket * sock_eth_out = NULL;
struct sockaddr_in remoteaddr_eth;
int ksocket_send(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);
// end for eth socket

void print_char_array(char* data, int data_len, char* message);




#include <linux/device.h>         // Header to support the kernel Driver Model
#define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver




#define MAC 0
#define APP 1
static int mac_or_app = 1;
module_param(mac_or_app, int, 1);

static int block_size = 200;
module_param(block_size, int, 0);

uint16_t par[ECC_LEN];
int ecc_symsize = 8;
int ecc_poly = 0x11d;

// the Reed Solomon control structure 
static struct rs_control *rs_decoder;


#define MAX_RETRANSMISSION  2 // At most tx 4 times
static int cnt_retransmission = 0;

static int frq = 1; // Unit: kHz
static int tx = 0;
static int rx = 0;
//static int dst_id = 8;
static int self_id = 7;

static int tx_len = 20;
static int rx_len = 1;
static int mode = 0;
static int show_msg = 0;
static int flag_exit = 0;
static int flag_lock = 0;
module_param(flag_lock, int, 0);

static int mtu = 1200;

module_param(self_id, int, 0);
//module_param(dst_id, int, 0);
module_param(tx, int, 0);
module_param(rx, int, 0);
module_param(frq, int, 0);
module_param(rx_len, int, 0);
module_param(tx_len, int, 0);
module_param(mode, int, 0);
module_param(show_msg, int, 0);
module_param(mtu, int, 0);

struct proc_dir_entry *vlc_dir, *tx_device, *rx_device;
static int rx_device_value = 0;
static int tx_device_value = 0;

static int rx_payload_len=0;

const char *name = "any";

static struct task_struct *task_mac_tx = NULL;
static struct task_struct *task_phy_decoding = NULL;

#define RX_BUFFER_SIZE 10000
static int rx_buffer[RX_BUFFER_SIZE] = {0};
static int rx_in_index = 0;
static int rx_out_index = 0;
//static char *rx_data = NULL;
#define RX_DATA_LEN 2000
static char rx_data_1[RX_DATA_LEN]; // Length > MAC_HDR_LEN+maxpayload+rs_code; MODIFIED! TODO is not todo but is to remember

//static int *rx_data_symbol = NULL;
static int tmp_symbol_index = 0;
static int rx_in_val = 0;


// For DATA frame
//static unsigned char* data_buffer_byte = NULL;
//static _Bool* data_buffer_symbol = NULL;
#define DB_BYTE_LEN 2000
#define DB_SYMBOL_LEN 2000*16
static unsigned char data_buffer_byte[DB_BYTE_LEN];
static _Bool data_buffer_symbol[DB_SYMBOL_LEN];

//static int data_buffer_byte_len = 0;
//static int data_buffer_symbol_len = 0;
static int tx_data_curr_index = 0;
//static int data_to_process = 0;

// For ACK frame
// start for miso
#define ACK_BYTE_LEN 50
static char ack_buffer_byte_eth[ACK_BYTE_LEN];
// end for miso


// Feb. 25, 2014
static int decoding_sleep_slot = 1;


MODULE_AUTHOR("Jona BEYSENS/Ander GALISTEO/Diego JUARA");
MODULE_LICENSE("Dual BSD/GPL");

volatile void* gpio1;
volatile void* gpio2;

///For character device
static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[32001] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer

// The prototype functions for the character driver -- must come before the struct definition
static int     char_dev_open(struct inode *, struct file *);
static int     char_dev_release(struct inode *, struct file *);
static ssize_t char_dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t char_dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
   .open = char_dev_open,
   .read = char_dev_read,
   .write = char_dev_write,
   .release = char_dev_release,
};

static int char_dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}
 
/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t char_dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   int i;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   for (i=0; i<tx_data_curr_index; i++) {
	   if (data_buffer_symbol[i]) message[i]='1';
	   else message[i]='0';
   }
   message[i]=0;
   error_count = copy_to_user(buffer, message, tx_data_curr_index);//error_count = copy_to_user(buffer, message, size_of_message);
   tx_data_curr_index=0;
   message[0]=0;
   
   printk("size of data=: %d\n",tx_data_curr_index);
   if (error_count==0){            // if true then have success
      printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", tx_data_curr_index);
      return (tx_data_curr_index=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}
 

 
/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int char_dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "EBBChar: Device successfully closed\n");
   return 0;
}



int ksocket_init(void) {
	if (sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock_eth_out) >= 0) {

	   /* Set the UDP socket to send packets */
		int c;

		memset(&remoteaddr_eth, 0, sizeof(struct sockaddr_in));
		remoteaddr_eth.sin_family      = AF_INET;
		remoteaddr_eth.sin_addr.s_addr = htonl(0xc0a8077f); //"192.168.7.127"
		remoteaddr_eth.sin_port        = htons(1111);
		
		if ((c =sock_eth_out->ops->connect(sock_eth_out, (struct sockaddr *)&remoteaddr_eth, sizeof(remoteaddr_eth), 0)) < 0) {
			printk("connect() failed, error code %d!\n", c);
			return -1;
		}
		else 
			printk("eth socket out connected\n");
	}
	else {
		printk("eth socket out could not be created\n");
		return -1;
	}
	return 0;
}

int ksocket_send(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len)
{
        struct msghdr msg;
        struct iovec iov;
        mm_segment_t oldfs;
        int size = 0;

        if (sock->sk==NULL)
           return 0;

        iov.iov_base = buf;
        iov.iov_len = len;

        msg.msg_flags = 0;
        msg.msg_name = addr;
        msg.msg_namelen  = sizeof(struct sockaddr_in);
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
		
		iov_iter_init(&msg.msg_iter, WRITE, &iov, 1, len);

        oldfs = get_fs();
        set_fs(KERNEL_DS);
        size = sock_sendmsg(sock,&msg);
        set_fs(oldfs);

        return size;
}



// April 04

enum  state {SENSING, RX, TX, ILLUM, CHM, NONE} ; // Short sensing is implemented in tx state
enum  state phy_state;
static _Bool print_states = true;


/////////////////// PRU variables ///////////////////////////////////

#define PRU_ADDR 0x4A300000
#define PRU0 0x00000000
#define PRU1 0x00002000
#define PRU_SHARED 0x00010000

unsigned int *rx_channel_pru;
unsigned int *rx_data_pru;
unsigned int *shared_pru;

// start for miso
#define TX_LEN 4 // number of transmitters
static _Bool activity_tx[TX_LEN] = {0};
#define CHMRQ_BYTE_LEN 50
static char chmrq_buffer_byte[CHMRQ_BYTE_LEN];
static int chmrq_buffer_byte_len = 0;
#define CHMRP_BYTE_LEN 50
static char chmrp_buffer_byte[CHMRP_BYTE_LEN];
//end for miso
struct net_device *vlc_devs;

struct vlc_packet {
	struct vlc_packet *next;
	struct net_device *dev;
	int	datalen;
	u8 data[2000+10];
};

struct vlc_packet *tx_pkt;
struct vlc_packet *rx_pkt_check_dup;
struct vlc_packet *rx_pkt;

int pool_size = 10;
module_param(pool_size, int, 0);

struct vlc_priv {
	struct net_device_stats stats;
	int status;
	struct vlc_packet *ppool;
	int rx_int_enabled;
	struct vlc_packet *tx_queue; // List of tx packets
	int tx_packetlen;
	u8 *tx_packetdata;
	struct sk_buff *skb;
	spinlock_t lock;
    struct net_device *dev;
};

static struct vlchdr *vlc_hdr(const struct sk_buff *skb)
{
    return (struct vlchdr *) skb_mac_header(skb);
}

// This function is called to fill up a VLC header 
int vlc_rebuild_header(struct sk_buff *skb)
{
	struct vlchdr *vlc = (struct vlchdr *) skb->data;
	struct net_device *dev = skb->dev;

	memcpy(vlc->h_source, dev->dev_addr, dev->addr_len);
	memcpy(vlc->h_dest, dev->dev_addr, dev->addr_len);
	vlc->h_dest[MAC_ADDR_LEN-1] ^= 0x01;   /* dest is us xor 1 */
	return 0;
}

int vlc_header(struct sk_buff *skb, struct net_device *dev, unsigned short type,
                const void *daddr, const void *saddr, unsigned len)
{
	struct vlchdr *vlc = (struct vlchdr *)skb_push(skb, VLC_HLEN);

	vlc->h_proto = htons(type);
	memcpy(vlc->h_source, saddr ? saddr : dev->dev_addr, dev->addr_len);
	memcpy(vlc->h_dest,   daddr ? daddr : dev->dev_addr, dev->addr_len);
	vlc->h_dest[MAC_ADDR_LEN-1] ^= 0x01;   /* dest is us xor 1 */
	return (dev->hard_header_len);
}

static const struct header_ops vlc_header_ops= {
    .create  = vlc_header
	//.rebuild = vlc_rebuild_header
};

// Return statistics to the caller
struct net_device_stats *vlc_stats(struct net_device *dev)
{
	struct vlc_priv *priv = netdev_priv(dev);
	return &priv->stats;
}

// Configuration changes (passed on by ifconfig)
int vlc_config(struct net_device *dev, struct ifmap *map)
{
	if (dev->flags & IFF_UP) // can't act on a running interface 
		return -EBUSY;

	// Don't allow changing the I/O address 
	if (map->base_addr != dev->base_addr) {
		printk(KERN_WARNING "snull: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}

	// Allow changing the IRQ 
	if (map->irq != dev->irq) {
		dev->irq = map->irq; // request_irq() is delayed to open-time 
	}

	// ignore other fields 
	return 0;
}

// Ioctl commands
int vlc_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	//printk("ioctl\n");
	return 0;
}


//Buffer/pool management.

struct vlc_packet *vlc_get_tx_buffer(struct net_device *dev)
{
	
	struct vlc_priv *priv = netdev_priv(dev);
	//unsigned long flags;
	struct vlc_packet *pkt;
    if (flag_lock)
        spin_lock_bh(&priv->lock);
	pkt = priv->ppool;
	priv->ppool = pkt->next;
	if (priv->ppool == NULL) {
		printk(KERN_INFO "The MAC layer queue is full!\n");
		netif_stop_queue(dev);
	}
    if (flag_lock)
        spin_unlock_bh(&priv->lock);
    
	return pkt;
}

void vlc_release_buffer(struct vlc_packet *pkt)
{
	//unsigned long flags;
	struct vlc_priv *priv = netdev_priv(pkt->dev);

	if (flag_lock)
        spin_lock_bh(&priv->lock);
	pkt->next = priv->ppool;
	priv->ppool = pkt;
	if (flag_lock)
        spin_unlock_bh(&priv->lock);
	if (netif_queue_stopped(pkt->dev) && pkt->next == NULL && flag_exit == 0)
		netif_wake_queue(pkt->dev);
}


// Departure a packet from the end of the MAC queue (FIFO)
struct vlc_packet *vlc_dequeue_pkt(struct net_device *dev)
{
    //unsigned long flags;
    struct vlc_priv *priv = netdev_priv(dev);
	struct vlc_packet *pkt;
    
    if (flag_lock)
        spin_lock_bh(&priv->lock);
	pkt = priv->tx_queue;
	if (pkt != NULL)
		priv->tx_queue = pkt->next;
	if (flag_lock)
        spin_unlock_bh(&priv->lock);
	printk("Dequeue a packet!\n");
    return pkt;
}

// Add a packet from upper layer to the beginning of the MAC queue
void vlc_enqueue_pkt(struct net_device *dev, struct vlc_packet *pkt)
{
    //unsigned long flags;
	struct vlc_priv *priv = netdev_priv(dev);
    struct vlc_packet *last_pkt; 
    
    /// Fix the misorder packets
    if (flag_lock)
        spin_lock_bh(&priv->lock);
    last_pkt = priv->tx_queue;
    if (last_pkt == NULL) {
        priv->tx_queue = pkt; // Enqueue the new packet
    } else {
        while (last_pkt != NULL && last_pkt->next != NULL) {
            last_pkt = last_pkt->next;
        }
        last_pkt->next = pkt; // Put new the pkt to the end of the queue
    }
    if (flag_lock)
        spin_unlock_bh(&priv->lock);
	printk("Enqueue a packet!\n");
    ///
}

int vlc_tx(struct sk_buff *skb, struct net_device *dev)
{
    struct vlc_packet *tmp_pkt;// TODO Commented so that it doesn't return!

    dev->trans_start = jiffies;
    tmp_pkt = vlc_get_tx_buffer(dev);
    tmp_pkt->next = NULL; 
    tmp_pkt->datalen = skb->len;
    memcpy(tmp_pkt->data, skb->data, skb->len);
    vlc_enqueue_pkt(dev, tmp_pkt);
    
    
    dev_kfree_skb(skb);
	return 0; // Our simple device can not fail 
}

int vlc_release(struct net_device *dev)
{
	netif_stop_queue(dev); // can't transmit any more 
	return 0;
}

int vlc_open(struct net_device *dev)
{
	memcpy(dev->dev_addr, "\0\2", MAC_ADDR_LEN);
    netif_start_queue(dev);
	return 0;
}

static const struct net_device_ops vlc_netdev_ops = {
	.ndo_open            = vlc_open,
	.ndo_stop            = vlc_release,
	.ndo_start_xmit      = vlc_tx,
	.ndo_do_ioctl        = vlc_ioctl,
	.ndo_set_config      = vlc_config,
	.ndo_get_stats       = vlc_stats,
};

// Setup VLC network device
void vlc_setup(struct net_device *dev)
{
    dev->hard_header_len = VLC_HLEN;
    dev->mtu = mtu;
    dev->tx_queue_len = 100;
    dev->priv_flags |= IFF_TX_SKB_SHARING;
}


//Enable and disable receive interrupts.

static void vlc_rx_ints(struct net_device *dev, int enable)
{
	struct vlc_priv *priv = netdev_priv(dev);
	priv->rx_int_enabled = enable;
}


//Set up a device's packet pool.
 
void vlc_setup_pool(struct net_device *dev)
{
	struct vlc_priv *priv = netdev_priv(dev);
	int i;
	struct vlc_packet *pkt;

	priv->ppool = NULL;
	for (i = 0; i < pool_size; i++) {
		pkt = kmalloc (sizeof (struct vlc_packet), GFP_KERNEL);
		if (pkt == NULL) {
			printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
			return;
		}
		pkt->dev = dev;
		pkt->next = priv->ppool;
		priv->ppool = pkt;
	}
}


void vlc_teardown_pool(struct net_device *dev)
{
	struct vlc_priv *priv = netdev_priv(dev);
	struct vlc_packet *pkt;
    //unsigned long flags;

    //spin_lock_bh(&priv->lock);
	while ((pkt = priv->ppool)) {
		priv->ppool = pkt->next;
		if (pkt) kfree (pkt);
		/* FIXME - in-flight packets ? */
	}
    //spin_unlock_bh(&priv->lock);
}

static unsigned char toByte(_Bool b[TX_LEN]) {
    unsigned char c = 0;
	int i;
    for (i=0; i < TX_LEN; ++i)
        if (b[i])
            c |= 1 << i;
    return c;
}

void print_char_array(char* data, int data_len, char* message) 
{
	int i;
	printk(message);
	printk(": ");
	for (i=0; i<data_len; i++) {
		printk("%x ",data[i]);
	}
	printk("\n");
}

static void construct_frame_header(char* buffer, int buffer_len, int payload_len, int mac_dest, int mac_src)
{
    int i;
    //unsigned short crc;

    for (i=0; i<PREAMBLE_LEN; i++)
        buffer[i] = 0xaa; // Preamble
    // SFD
    buffer[PREAMBLE_LEN] = 0xa3; //10100011 0110011010100101
    // Length of payload
    buffer[PREAMBLE_LEN+1] = (unsigned char) ((payload_len>>8) & 0xff);
    buffer[PREAMBLE_LEN+2] = (unsigned char) (payload_len & 0xff);
    // Destination address
    buffer[PREAMBLE_LEN+3] = (unsigned char) ((mac_dest>>8) & 0xff);
    buffer[PREAMBLE_LEN+4] = (unsigned char) (mac_dest & 0xff);
    // Source address
    buffer[PREAMBLE_LEN+5] = (unsigned char) ((mac_src>>8) & 0xff);
    buffer[PREAMBLE_LEN+6] = (unsigned char) (mac_src & 0xff);
    // CRC
    //crc = crc16(buffer+PREAMBLE_LEN+SFD_LEN, MAC_HDR_LEN+payload_len);
    //buffer[buffer_len-2] = (char) ((0xff00&crc)>>8); // CRC byte 1
	//buffer[buffer_len-1] = (char) ((0x00ff&crc)); // CRC byte 2
}

static void OOK_with_Manchester_RLL(char *buffer_before_coding,
    _Bool *buffer_after_coding, int len_before_coding)
{
    int byte_index, symbol_index = 0;
    unsigned char curr_byte, mask;

    // Convert the preamble -- OOK w/o Manchester RLL code
    for (byte_index=0; byte_index<PREAMBLE_LEN; byte_index++) {
        mask = 0x80;
        curr_byte = buffer_before_coding[byte_index] & 0xff;
        while (mask) {
            buffer_after_coding[symbol_index++] = (_Bool) (curr_byte & mask);
            mask >>= 1;
        }
    }
    // Convert the parts after the preamble -- OOK w Manchester RLL code
    for (byte_index=PREAMBLE_LEN; byte_index<len_before_coding; byte_index++) {
        mask = 0x80;
        curr_byte = buffer_before_coding[byte_index] & 0xff;
        while (mask) {
            if ((_Bool) (curr_byte & mask)) { // Bit 1 -- LOW-HIGH
                buffer_after_coding[symbol_index++] = false;
                buffer_after_coding[symbol_index++] = true;
            } else { // Bit 0 -- HIGH-LOW
                buffer_after_coding[symbol_index++] = true;
                buffer_after_coding[symbol_index++] = false;
            }
            mask >>= 1;
        }
    }
}

static int write_to_pru(_Bool *symbols_to_send, int symbol_length)
{
	unsigned int mask = 0x80000000, actual_value = 0;
	int ind_pru = 1, i = 0;
	
	while(i<symbol_length)
	{
		if(symbols_to_send[i])
			actual_value += mask;
		mask = mask >> 1;
		if((mask==0)||(i==(symbol_length-1)))
		{
			rx_channel_pru[ind_pru++]=actual_value;
			actual_value = 0;
			mask = 0x80000000;
		}
		i++;
	}
	
	return ind_pru-1;
	
}
 __be16 vlc_type_trans(struct sk_buff *skb, struct net_device *dev)
{
    struct vlchdr *vlc;
    skb->dev = dev;
    skb_reset_mac_header(skb);
    skb_pull_inline(skb, VLC_HLEN);
    vlc = vlc_hdr(skb);

    if (ntohs(vlc->h_proto) >= 1536)
        return vlc->h_proto;

    return htons(VLC_P_DEFAULT);
}


 //Receive a packet: retrieve, encapsulate and pass over to upper levels
 
void mac_rx(struct net_device *dev, struct vlc_packet *pkt)
{
	struct sk_buff *skb;
	struct vlc_priv *priv = netdev_priv(dev);

	skb = dev_alloc_skb(pkt->datalen + 2);
	if (!skb) {
		if (printk_ratelimit())
			printk(KERN_NOTICE "snull rx: low on mem - packet dropped\n");
		priv->stats.rx_dropped++;
		goto out;
	}
	skb_reserve(skb, 2); // align IP on 16B boundary 
	memcpy(skb_put(skb, pkt->datalen), pkt->data, pkt->datalen);

	// Write metadata, and then pass to the receive level 
	skb->dev = dev;
	skb->protocol = vlc_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_UNNECESSARY; // don't check it 
	priv->stats.tx_packets++;
	priv->stats.tx_bytes += pkt->datalen;
	netif_rx(skb);
	//kfree(pkt);
  out:
	return;
}

static int cmp_packets(char *pkt_1, char *pkt_2, int len)
{
    int i = 0;
    for (i=0; i<len; i++) {
        if (pkt_1[i] != pkt_2[i])
            return 1; // Packets are not equal
    }
    return 0; // Packets are equal
}

static void get_the_data_rx(char * rx_data)
{
	uint16_t par_rx[ECC_LEN];
	struct vlc_priv *priv = netdev_priv(vlc_devs);
	unsigned int payload_len_rx = ((rx_data[0] & 0xff) << 8) | (rx_data[1] & 0xff);
	unsigned int mac_dest = ((rx_data[2] & 0xff) << 8) | (rx_data[3] & 0xff);
	int encoded_len = payload_len_rx+2*MAC_ADDR_LEN+PROTOCOL_LEN;
	int num_of_blocks;
	int i,index_block, num_err=0;
	
	//printk("MAC ADDR: %x\n",mac_dest);
	if( mac_dest == self_id || mac_dest == 0xffff) // only process the packet if it is intended for this receiver
	{
		// Calculate the number of blocks
		if (encoded_len % block_size)
			num_of_blocks = encoded_len / block_size + 1;
		else
			num_of_blocks = encoded_len / block_size;
		//printk("payload_len_rx=%d\n",payload_len_rx);
		//return;
		int j = OCTET_LEN+2*MAC_ADDR_LEN+PROTOCOL_LEN+payload_len_rx;
		int total_num_err = 0;
		
		for (index_block=0; index_block < num_of_blocks; index_block++) {
			for (i = 0; i < ECC_LEN; i++) {
				par_rx[i] = rx_data[j+index_block*ECC_LEN+i];
				//printk(" %02x", par[i]);
			}
			
			if (index_block < num_of_blocks-1) {
				num_err = decode_rs8(rs_decoder, rx_data+OCTET_LEN+index_block*block_size, 
					par_rx, block_size, NULL, 0, NULL, 0, NULL);
			} else { // The last block
				num_err = decode_rs8(rs_decoder, rx_data+OCTET_LEN+index_block*block_size, 
					par_rx, encoded_len % block_size, NULL, 0, NULL, 0, NULL);
			}
			if (num_err < 0) {
				printk("*** CRC error. ***\n");
				//f_adjust_slot = 1;
				goto end;
			}
			total_num_err += num_err;
		}
		if (total_num_err > 0) {
			//printk("*** Has corrected %d error(s) *** ", total_num_err);
		}
		if (payload_len_rx>0) { // Data frame
			if (mac_or_app == APP) {// To upper layer
				//rx_pkt = kmalloc (sizeof (struct vlc_packet), GFP_KERNEL);
				rx_pkt->datalen = MAC_HDR_LEN-OCTET_LEN+payload_len_rx;
				// Copy the received data (omit the datalen octet)
				memcpy(rx_pkt->data, rx_data+OCTET_LEN, rx_pkt->datalen);
				if (cmp_packets(rx_pkt->data,rx_pkt_check_dup->data,rx_pkt->datalen)) {
					// Frist copy the packet; the rx_pkt will be released in max_rx
					memcpy(rx_pkt_check_dup->data, rx_pkt->data, rx_pkt->datalen);
					rx_pkt_check_dup->datalen = rx_pkt->datalen;
					mac_rx(vlc_devs, rx_pkt);
					printk("Sent to the Internet layer.\n");
					priv->stats.rx_packets++;
					priv->stats.rx_bytes += rx_pkt->datalen;/**/
				}
				else printk("Repeated packet\n");
			}
		}
	}
	
end:
	return;
}


/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t char_dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   copy_from_user(message, buffer, len);
   sprintf(message, "%s", message, len);   // appending received string with its length
   size_of_message = strlen(message);// store the length of the stored message
   int ift,lastindex;
   rx_payload_len=0;
   char rx_data[1000];
   for (ift=0;ift<size_of_message;ift++)
   {
	   if (message[ift]==37){
		   lastindex=ift;
		   ift++;
		   while(message[ift]!=37) {
			   rx_payload_len=rx_payload_len*10+(message[ift]-48);
			   ift++;
		   }
		   break;
	   }
   }
   message[lastindex]=0;
   size_of_message = strlen(message);
   rx_data[0]=(rx_payload_len>> 8)& 0xff;
   rx_data[1]=rx_payload_len & 0xff;
   int rx_data_index=2,cnt=0;
   char tmp_char=0;
   for (ift=0;ift<=size_of_message;ift++)
   {
	   if (message[ift]==49) tmp_char++;
	   if (cnt==7){
		   rx_data[rx_data_index]=tmp_char;
		   rx_data_index++;
		   cnt=0;
		   tmp_char=0;
	   }else{
			tmp_char=tmp_char<<1;
			cnt++;
	   }
   }
   message[0]=0;
   buffer = NULL;
   printk("DONE putting in the rx_data\n");
   //rx_data[rx_data_index]=tmp_char<<(size_of_message%8);
   //rx_data[rx_data_index++]=0;
  // for (ift=0;ift<rx_data_index;ift++)
   //{
	   //printk(" %x ",rx_data[ift]);
   //}
   
   
   
   
   
   //printk("%s\nSize=%d data1= %x data2= %x",message,size_of_message,rx_data[0],rx_data[1]);
   printk("rx_payload_len %d\n",rx_payload_len);
   get_the_data_rx(rx_data);
   
   //data_to_process=1;
   //printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);
   return len;
}

static int phy_decoding(void *data)
{
    int i, j, num_err = 0;
    char sfd_data = 0xa3;
    char sfd_ack = 0x3a;
    //unsigned char mask;
    int cnt_preamble = 0;
    int cnt_byte = 0;
    int cnt_symbol = 1;
    int curr_symbol = 0;
    _Bool curr_read = 0;
    //_Bool curr_cmp;
    char recv_sfd = 0;
    unsigned int payload_len = 0;
    unsigned int buffer_len = 0;
    int rx_ch;
    int prev_symbol = 0;
    int guarder = 2; //
    int sum_preamble=0;
    int min_preamble = 10;
    int max_preamble = 0;
    //unsigned short recv_crc = 0;
    //unsigned short cal_crc = 0;
    struct vlc_priv *priv = netdev_priv(vlc_devs);
    int index_block, encoded_len, num_of_blocks, total_num_err = 0;
    // May 31, 2015
    int max_un_reception = 10000;
    int num_un_reception = 0;
    ///******************Nov 14, 2014
    //int erasures[16];
    //int n_erasures = 0;
    ///******************************
	//ADDED FOR RX TO MX CODE
	char data_to_mx[98]; // transmitters id (1 byte) + VLC MAC ADDRESS RX (1 byte) + preamble (32 symbols * 2 bytes/symbol) + SFD (16 symbols * 2 bytes/symbol)
	int data_to_mx_len = 98;
	//END FOR RX TO MX CODE
	
	int symbol_len,byte_len,thelen1,thelen2,numofblocks, len_bit, group_32bit, rest_32bit;

start_of_process: /// Start the process of a frame
    cnt_symbol = 1;
    prev_symbol = 0;
    cnt_preamble = 0;
    sum_preamble = 0;
    tmp_symbol_index = 0;
    num_un_reception = 0;
	char rx_data[1000];
	int rx_data_pru2_LE=0;
	int rx_data_pru3_LE=0;
	int mac_src = 0;
	int tx_polling_id = 0;
	
    for (; ;) {
        //here read the memory
		//printk("%x\n",rx_data_pru[0]);
		if(rx_data_pru[0] != 0){
			/*for(i=0;i<48;i++)
			{
				printk("Value %i, %i\n",i,shared_pru[i]);
			}*/
			symbol_len = rx_data_pru[1];
			printk("Symbol length %d\n", symbol_len);
			
			//print_char_array(rx_data_pru, data_buffer_byte_len,"Data_buffer_byte");

			
			rx_data_pru2_LE = ((rx_data_pru[2]&0x000000FF)<<24)|((rx_data_pru[2]&0x0000FF00)<<8)|((rx_data_pru[2]&0x00FF0000)>>8)|((rx_data_pru[2]&0xFF000000)>>24);
			//rx_data_pru3_LE = ((rx_data_pru[3]&0x000000FF)<<24)|((rx_data_pru[3]&0x0000FF00)<<8)|((rx_data_pru[3]&0x00FF0000)>>8)|((rx_data_pru[3]&0xFF000000)>>24);
			mac_src = (rx_data_pru2_LE & 0x0000ffff);
			
			//mac_src = rx_data_pru[2] & 0xff;
			
			// Channel measurement
			if(mac_src == 127 && symbol_len == 161)
			{
				memset (data_to_mx, 0, sizeof (unsigned char) * data_to_mx_len);
				memcpy(&tx_polling_id, &rx_data_pru[3], 4);
				
				// store the transmitter id 
				//printk("pru_2_BE %x\n", rx_data_pru[2]);
				//printk("pru_2_LE %x\n", rx_data_pru2_LE);
				//printk("PRU_3_BE %d\n", rx_data_pru[3]);
				printk("tx polling id %d\n", tx_polling_id);
				//printk("LED ID %d\n", (rx_data_pru[3] >> 24) & 0xff);
				
				data_to_mx[0] =  (tx_polling_id & 0x000000ff);
				data_to_mx[1] = (self_id & 0x000000ff);
				
				for(i=0;i<48;i++)
				{	
					data_to_mx[2+2*i] = (rx_channel_pru[i] >> 8) & 0xff; //MSB
					data_to_mx[2+2*i+1] = rx_channel_pru[i]		& 0xff; // LSB
					//printk("Value %i, %i\n",i,shared_pru[i]);
				}			
				rx_data_pru[0] = 0; // data is fetched from PRU so PRU can move on
				if(ksocket_send(sock_eth_out, (struct sockaddr_in *)&remoteaddr_eth, data_to_mx, data_to_mx_len) > 0) {
					printk("CH message sent to MX\n");
				}
				else {
					printk("CH message NOT sent to MX\n");	
				}
			}
			// END ADDED FOR RX TO MX CODE
			else 
			{
				if(symbol_len > 32000)
				{
					rx_data_pru[0] = 0;
					goto error;
				} 
				
				byte_len = ((symbol_len-1-32)/16)+3;
				len_bit = ((symbol_len-81)/2); // subtract 73 = 24 (preamble) + 16 (SFD) + 32 (length) + 1 (added at TX)
				group_32bit = (len_bit+31)/32; // add 31 to round up
				rest_32bit = len_bit%(32);
				
				j=1;
				while(j<1000){//todo!
					numofblocks=j;
					thelen1=(byte_len-28)-16*(j-1);
					if (thelen1%200) numofblocks=(thelen1+6)/200 +1;
					else numofblocks=(thelen1+6)/200;
					if (numofblocks==j) break;
					j++;
				}
				printk("Payload %d\n", thelen1);
				if((thelen1>900)||(thelen1<0))
				{
					rx_data_pru[0] = 0;
					goto error;
				} 
				rx_data[0] = ((thelen1 >> 8) & 0xFF);
				rx_data[1] = (thelen1 & 0xFF);
				memcpy(&rx_data[2],&rx_data_pru[2],group_32bit*sizeof(unsigned int));
				rx_data_pru[0] = 0;
				// Show data before decoding
				/*for(i = 0;i<group_32bit*4;i++)
				{
					//rx_data[i+2] = rx_data_pru[i+2];
					int mask = 0x80;
					char last = rx_data[i+2];
					while(mask!=0)
					{
						if(mask&last)
						{
							printk("1");
						}else{
							printk("0");
						}
						mask = mask >> 1;
					}
					//printk("Data val %d\n",rx_data[i+2]);
				}*/
				
				
				get_the_data_rx(rx_data); // printk("I'm here\n");//
			}
			
		}
		error:
		msleep(decoding_sleep_slot);
		if (kthread_should_stop())
			goto ret;
	}
	ret:
    printk("\n=======EXIT phy_decoding=======\n");
    return 0;
}


void vlc_init(struct net_device *dev)
{
	struct vlc_priv *priv;

    dev->addr_len = MAC_ADDR_LEN;
    dev->type = ARPHRD_LOOPBACK ;  // ARPHRD_IEEE802154
	vlc_setup(dev); // assign some of the fields 
	dev->netdev_ops = &vlc_netdev_ops;
	dev->header_ops = &vlc_header_ops;
	// keep the default flags, just add NOARP 
	dev->flags           |= IFF_NOARP;
	dev->features        |= NETIF_F_HW_CSUM;
	
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct vlc_priv));
	
	if (mac_or_app == APP) {
        vlc_rx_ints(dev, 1);		/* enable receive interrupts */
        tx_pkt = kmalloc (sizeof (struct vlc_packet), GFP_KERNEL);
        rx_pkt = kmalloc (sizeof (struct vlc_packet), GFP_KERNEL);
        rx_pkt_check_dup = kmalloc (sizeof (struct vlc_packet), GFP_KERNEL);
        if (tx_pkt==NULL || rx_pkt_check_dup==NULL || rx_pkt==NULL) {
            printk (KERN_NOTICE "Ran out of memory allocating packet pool\n");
            return ;
        }
        rx_pkt_check_dup->datalen = 0;
        vlc_setup_pool(dev);
        priv->tx_queue = NULL;
        flag_exit = 0;
        //printk(".....8.....\n");
        //netif_start_queue(dev);
        //printk(".....9.....\n");
    }
}
static void vlc_regular_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
}

void vlc_cleanup(void)
{
	struct vlc_packet *pkt;
    struct vlc_priv *priv = netdev_priv(vlc_devs);
	int ret;
	flag_exit = 1;
    netif_stop_queue(vlc_devs);
	
	///////////// Unmap memory for communicating with PRU ///////////
	
	memunmap(rx_channel_pru);
	memunmap(rx_data_pru);
	printk("Memory unmapped correctly\n");
	
	
	// Clean the threads
    
    if (task_mac_tx) {
        ret = kthread_stop(task_mac_tx);
		if(!ret)
			printk(KERN_INFO "Thread stopped");
        task_mac_tx = NULL;
    }
	if (task_phy_decoding) {
        ret = kthread_stop(task_phy_decoding);
		if(!ret)
			printk(KERN_INFO "Thread stopped");
       task_phy_decoding = NULL;
    }
	printk("stop threads\n");
	//Clean devices
	if (vlc_devs) {
        if (mac_or_app == APP) {
            printk("clean the pool\n");
            //if (flag_lock)
                //spin_lock_bh(&priv->lock);
			while(priv->tx_queue) {
                pkt = vlc_dequeue_pkt(vlc_devs);
                vlc_release_buffer(pkt);
            }
			
            //if (flag_lock)
                //spin_unlock_bh(&priv->lock);
            vlc_teardown_pool(vlc_devs);
            kfree(rx_pkt);
            kfree(rx_pkt_check_dup);
            kfree(tx_pkt);
        }
        printk("unregister the devs\n");
        unregister_netdev(vlc_devs);
        
        //if (mac_or_app == APP) {
            //vlc_teardown_pool(vlc_devs);
        //}
        
        printk("free the devs\n");
        free_netdev(vlc_devs);
    }
	// Free the reed solomon resources
    if (rs_decoder) {
        free_rs(rs_decoder);
    }
	device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
	class_unregister(ebbcharClass);                          // unregister the device class
	class_destroy(ebbcharClass);                             // remove the device class
	unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
	printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
	
	return 0;
}


int vlc_init_module(void)
{
	int ret = -ENOMEM;
	printk("Initializing the VLC module...\n");
	phy_state = TX;
	//vlc_interrupt = vlc_regular_interrupt;
	
	printk("Opening eth socket\n");
	ret = ksocket_init();
	
	///////////// Map memory for communicating with PRU /////////////
	
	rx_data_pru = memremap(PRU_ADDR+PRU1, 0x10000 ,MEMREMAP_WT);
	shared_pru = memremap(PRU_ADDR+PRU_SHARED, 0x10000 ,MEMREMAP_WT);
	rx_channel_pru = memremap(PRU_ADDR+PRU0, 0x10000 ,MEMREMAP_WT);
	printk("Memory mapped correctly: %x\n",rx_channel_pru);
	printk("Memory mapped correctly: %x\n",rx_data_pru);
	
	/// Create the device and register it
	vlc_devs = alloc_netdev(sizeof(struct vlc_priv), "vlc0", NET_NAME_UNKNOWN, vlc_init);
	if (vlc_devs == NULL)
		goto out;
	ret = register_netdev(vlc_devs);
    if (ret)
        printk("VLC: error registering device \"%s\"\n", vlc_devs->name);
	else printk("VLC: Device registered!\n");
	
	/*if (!rx) { 
        task_mac_tx = kthread_run(mac_tx,"TX thread","VLC_TX");
        if (IS_ERR(task_mac_tx)){
            printk("Unable to start kernel threads. \n");
            ret= PTR_ERR(task_phy_decoding);
            task_mac_tx = NULL;
            task_phy_decoding = NULL;
            goto out;
        }
		
    }*/
	task_phy_decoding = kthread_run(phy_decoding,"RX thread","VLC_DECODING");
    if (IS_ERR(task_phy_decoding)){
        printk("Unable to start kernel threads. \n");
        ret= PTR_ERR(task_phy_decoding);
        task_mac_tx = NULL;
        task_phy_decoding = NULL;
        goto out;
    }else printk("Started Kernel Threads! \n");
	
	
	rs_decoder = init_rs(ecc_symsize, ecc_poly, 0, 1, ECC_LEN); // 0---FCR
    if (!rs_decoder) {
        printk(KERN_ERR "Could not create a RS decoder\n");
        ret = -ENOMEM;
        goto out;
    }
	
	
	
	///Character device
	printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");
 
   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "EBBChar failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);
 
   // Register the device class
   ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "EBBChar: device class registered correctly\n");
 
   // Register the device driver
   ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
      class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ebbcharDevice);
   }
   printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized
	
	///Character device
	
	
	
	out:
    printk("------EXIT vlc_init_module------\n");
	if (ret)
		vlc_cleanup();

	return ret;
}



module_init(vlc_init_module);
module_exit(vlc_cleanup);