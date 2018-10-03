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
#include <linux/unistd.h>
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

#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/netdevice.h>


#define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver

MODULE_AUTHOR("Jona BEYSENS/Ander GALISTEO/Diego JUARA");
MODULE_LICENSE("Dual BSD/GPL");

// start for eth socket
static struct socket * sock_eth_out = NULL;
static struct socket * sock_eth_in = NULL;
static struct socket * sock_pc_out = NULL;
static struct socket * sock_pc_in = NULL;

struct sockaddr_in localaddr_eth;
struct sockaddr_in remoteaddr_eth;
struct sockaddr_in localaddr_pc;
struct sockaddr_in remoteaddr_pc;

int ksocket_receive(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);
int ksocket_send(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);
static int phy_broadcast(unsigned char* data_byte, int data_byte_len);

static void assign_txs(void);
void print_char_array(char* data, int data_len, char* message);
void print_int_array_dec(int* data, int data_len, char* message);
static void convert_char_int_array(int* array_out, char* array_in, int array_len);
static int get_average(int* array_in, int array_len);
static int get_swing(int* array_in, int array_len);
static int check_empty_array(int* data, int data_len);
static int get_index_max(int* array_in, int array_len);
static void set_tx_master(int* swing, int swing_len, int mac_rx);
void perform_chm_single(int polling_id);

static void measure_SINR(char rx_id_desired);
static void start_interference(char* rx_id_interfering);
static void stop_interference(void);


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


//static int dst_id = 8;
static int self_id = 7;

static int mode = 0;
static int threshold = 5;
static int flag_exit = 0;
static int flag_lock = 0;
module_param(flag_lock, int, 0);

static int mtu = 1200;

module_param(self_id, int, 0);
//module_param(dst_id, int, 0);
module_param(threshold, int, 0);
module_param(mtu, int, 0);

struct proc_dir_entry *vlc_dir, *tx_device, *rx_device;
static int rx_device_value = 0;
static int tx_device_value = 0;

static int rx_payload_len=0;

const char *name = "any";

static struct task_struct *task_mac_tx = NULL;

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

static int data_buffer_byte_len = 0;
static int data_buffer_symbol_len = 0;
static int tx_data_curr_index = 0;
static int data_to_process = 0;

// For ACK frame
// start for miso
static int swing_rx1[36] = {0};
static int swing_rx2[36] = {0};
static int swing_rx3[36] = {0};
static int swing_rx4[36] = {0};
static int led_data_active1[4] = {0}, led_data_active2[4] = {0}; 
static int led_to_tx[36] = {1,1,2,2,3,3,1,1,2,2,3,3,4,4,5,5,6,6,4,4,5,5,6,6,7,7,8,8,9,9,7,7,8,8,9,9}; // to convert from led id to tx (BBB) id
static int tx_master[4] = {0}; // we have 4 users in the system
static bool send_swings = false;
static int no_rx = 4;
// end for miso


// Feb. 25, 2014
static int decoding_sleep_slot = 1;

int ksocket_receive(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);
int ksocket_send(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);
static int phy_broadcast(unsigned char* data_byte, int data_byte_len);
static int phy_decoding_chmrp_eth(void);
static int recv_ack_eth(void);
void print_char_array(char* data, int data_len, char* message);
// end for eth socket

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

// April 04

enum  state {SENSING, RX, TX, ILLUM, CHM, NONE} ; // Short sensing is implemented in tx state
enum  state phy_state;
static _Bool print_states = true;

///////////////////// SOCKET ////////////////////////////////////////

struct socket *socket, *socket_send;
struct sockaddr_in sin, sin_send;

/////////////////// PRU variables ///////////////////////////////////

#define PRU_ADDR 0x4A300000
#define PRU0 0x00000000
#define PRU1 0x00002000
#define PRU_SHARED 0x00010000

unsigned int *tx_pru;
unsigned int *rx_pru;

// start for miso
#define TX_LEN 4 // number of transmitters
static _Bool activity_tx[TX_LEN] = {0};
#define CHMRQ_BYTE_LEN 50
static char chmrq_buffer_byte[CHMRQ_BYTE_LEN];
static int chmrq_buffer_byte_len = 0;

//end for miso
struct net_device *vlc_devs;

struct vlc_packet {
	struct vlc_packet *next;
	struct net_device *dev;
	int	datalen;
	int mac_dest; // no src, because this is the global self_id variable
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
	
	struct iphdr *network_header = (struct iphdr *)skb_network_header(skb);
	int ip_dest  = network_header->daddr;
	tmp_pkt->mac_dest = ((ip_dest & 0xff000000) >> 24); // VLC MAC address is just last byte of IP address
	
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
	int ind_pru = 2, i = 0;
	
	while(i<symbol_length)
	{
		if(symbols_to_send[i])
			actual_value += mask;
		mask = mask >> 1;
		if((mask==0)||(i==(symbol_length-1)))
		{
			tx_pru[ind_pru++]=actual_value;
			actual_value = 0;
			mask = 0x80000000;
		}
		i++;
	}
	
	return ind_pru-1;
	
}

static void generate_DATA_frame(struct vlc_packet *pkt)
{
	
    //printk("VLC: Generating DATA frame\n");
	int i, payload_len, index_block, encoded_len, num_of_blocks = 0, symbol_last_index = 0, group_32bit = 0;
    payload_len = pkt->datalen-(MAC_HDR_LEN-OCTET_LEN);
    encoded_len = payload_len+2*MAC_ADDR_LEN+PROTOCOL_LEN;
	
    // Calculate the number of blocks
    if (encoded_len % block_size)
        num_of_blocks = encoded_len / block_size + 1;
    else
        num_of_blocks = encoded_len / block_size;
        
    data_buffer_byte_len = FRAME_LEN_WO_PAYLOAD + payload_len + ECC_LEN*(num_of_blocks-1);
	
	//printk("FRAME_LEN_WO_PAYLOAD: %d\n",FRAME_LEN_WO_PAYLOAD);
	//printk("payload_len: %d\n",payload_len);
	//printk("num_of_blocks: %d\n",num_of_blocks);
	
    memset (data_buffer_byte, 0, sizeof (unsigned char) * data_buffer_byte_len);
    data_buffer_symbol_len = (data_buffer_byte_len-PREAMBLE_LEN)*8*2
        + PREAMBLE_LEN*8 + 1; // Send a BIT more, why? -- Avoid phy error
    //printk("data_buffer_symbol_len: %d\n",data_buffer_symbol_len);
	//printk("data_buffer_byte_len: %d\n",data_buffer_byte_len);
	//printk("num_of_blocks: %d\n",num_of_blocks);
	//printk("payload_len: %d\n",payload_len);
	
	if (data_buffer_byte==NULL || data_buffer_symbol==NULL) {
        printk ("Ran out of memory generating new frames.\n");
        return;
    }
    // Construct a new data frame
    memcpy(data_buffer_byte+PREAMBLE_LEN+SFD_LEN+OCTET_LEN, pkt->data, 
        pkt->datalen); // Copy the payload
    vlc_release_buffer(pkt); // Return the buffer to the pool
    construct_frame_header(data_buffer_byte, data_buffer_byte_len, data_buffer_symbol_len, pkt->mac_dest, self_id);//construct_frame_header(data_buffer_byte, data_buffer_byte_len, payload_len);
    
    /// Encode the blocks of a frame
    for (index_block = 0; index_block < num_of_blocks; index_block++) {
        for (i = 0; i < ECC_LEN; i++)
            par[i] = 0;
        if (index_block < num_of_blocks-1) {
            encode_rs8(rs_decoder, 
                    data_buffer_byte+PREAMBLE_LEN+SFD_LEN+OCTET_LEN+index_block*block_size, 
                    block_size, par, 0);
        } else {
            encode_rs8(rs_decoder, 
                    data_buffer_byte+PREAMBLE_LEN+SFD_LEN+OCTET_LEN+index_block*block_size,
                    encoded_len%block_size, par, 0);
        }
        for (i = 0; i < ECC_LEN; i++)
            data_buffer_byte[FRAME_LEN_WO_PAYLOAD+payload_len+(index_block-1)*ECC_LEN+i] = par[i];
    }
    //Show data after coding
	/*for(i = 0;i<=data_buffer_byte_len;i++)
	{
		int mask = 0x00000080;
		unsigned int last = data_buffer_byte[i+7];
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
    // Encoding the frame
    OOK_with_Manchester_RLL(data_buffer_byte, data_buffer_symbol,
                            data_buffer_byte_len);
    tx_data_curr_index = data_buffer_symbol_len;
	printk("VLC-> Data current index: %d\n",data_buffer_symbol_len);
	
	char tx_packet[1000];
	
	unsigned int mask = 0x80, actual_value = 0, led_active1 = 0, led_active2 = 0; 
	int64_t led_active = 0;
	int ind_bytes = 11;
	i = 0;
	
	
	
	while(i<tx_data_curr_index)
	{
		if(data_buffer_symbol[i])
			actual_value += mask;
		mask = mask >> 1;
		if((mask==0)||(i==(tx_data_curr_index-1)))
		{
			tx_packet[ind_bytes++]=actual_value;
			actual_value = 0;
			mask = 0x80;
		}
		i++;
	}
	ind_bytes--;	
	
	//led_data_active1 = 0xFFFFFFFF;//0x00000000;
	//led_data_active2 = 0xFFFFF0C7;//0x000003CF;
	
	// int mac_dest = pkt->mac_dest;
	// printk("destination MAC: %d\n",mac_dest);
	
	tx_packet[0] = tx_master		[(pkt->mac_dest - 21)]; // Master ID
	tx_packet[1] = (led_data_active1[(pkt->mac_dest - 21)] >> 24) & 0xFF;
	tx_packet[2] = (led_data_active1[(pkt->mac_dest - 21)]>> 16) & 0xFF;
	tx_packet[3] = (led_data_active1[(pkt->mac_dest - 21)] >> 8) & 0xFF;
	tx_packet[4] = led_data_active1 [(pkt->mac_dest - 21)]& 0xFF;
	tx_packet[5] = (led_data_active2[(pkt->mac_dest - 21)]>> 24) & 0xFF;
	tx_packet[6] = (led_data_active2[(pkt->mac_dest - 21)] >> 16) & 0xFF;
	tx_packet[7] = (led_data_active2[(pkt->mac_dest - 21)] >> 8) & 0xFF;
	tx_packet[8] = led_data_active2 [(pkt->mac_dest - 21)] & 0xFF;
	tx_packet[9] = (data_buffer_symbol_len >> 8) & 0xFF;
	tx_packet[10] = data_buffer_symbol_len & 0xFF;
	
	
	
	phy_broadcast(tx_packet,ind_bytes);
	
	//group_32bit = write_to_pru(data_buffer_symbol, tx_data_curr_index);
	//printk("Number of address manchester data takes: %d\n", group_32bit);
	//tx_pru[0]=data_buffer_symbol_len;
	//tx_pru[1]=1;
	
	
	//msleep(1);
	//while(tx_pru[0]!=0); //if you wait, system freezes
	//memcpy(tx_pru+1, data_buffer_symbol, symbol_last_index);	
	
	
	
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

void print_int_array_dec(int* data, int data_len, char* message) 
{
	int i;
	printk(message);
	printk(": ");
	for (i=0; i<data_len; i++) {
		printk("%d ",data[i]);
	}
	printk("\n");
}

static int check_empty_array(int* data, int data_len)
{
	int i,empty;
	empty = 1;
	for (i=0; i<data_len; i++) {
		if(data[i] != 0) {
			empty = 0;
		}
	}
	return empty;
}

void perform_chm(void){
	
	char tx_packet[1000] = {0}, polling_id = 0, polling_id_recv = 0, mac_rx_recv = 0;
	unsigned int mask = 0x80, actual_value = 0, led_active1 = 0, led_active2 = 0;
	int payload_len = 1, ret=0, ind_bytes = 0, i= 0, j= 0, k=0;
	uint64_t led_active = 0;
	int swing_len = 36;
	int swing_tmp = 0;
	int swing_rx1_tmp[36] = {0};
	int swing_rx2_tmp[36] = {0};
	int swing_rx3_tmp[36] = {0};
	int swing_rx4_tmp[36] = {0};
	char chm_response[98] = {0};
	int ch_meas_int[48] = {0};
	int dest_id = 0xffff; // broadcast to all RXs
	
	//dst_id = 0xFFFF; // everyone can receive the channel measurement
	//int measure_tx[6] = {1,2,7,8,13,14};
	//int measure_tx[6] = {3,4,5,6,9,10};
	for (j=1; j<=36; j++) 
	{
		ind_bytes = 11; 

		// LED ID of the polling packet
		polling_id = j;//measure_tx[j-1];
		
		memset(data_buffer_byte, 0, sizeof(data_buffer_byte));
		//memcpy(data_buffer_byte+PREAMBLE_LEN+SFD_LEN+OCTET_LEN+MAC_ADDR_LEN*2,polling_id,1); // Copy the payload
			
		data_buffer_byte_len = (PREAMBLE_LEN+SFD_LEN+OCTET_LEN+MAC_ADDR_LEN*2)+payload_len;//FRAME_LEN_WO_PAYLOAD + payload_len;
		
		// we add + 1 to symbol_len to make sure the last symbol is transmitted by the LEDs
		data_buffer_symbol_len = PREAMBLE_LEN*8+(data_buffer_byte_len-PREAMBLE_LEN)*8*2+1; //Length in symbols with preamble 
		construct_frame_header(data_buffer_byte, data_buffer_byte_len, data_buffer_symbol_len, dest_id, self_id); // self_id is defined in the beginning
		data_buffer_byte[PREAMBLE_LEN+SFD_LEN+OCTET_LEN+MAC_ADDR_LEN*2] = polling_id;
		OOK_with_Manchester_RLL(data_buffer_byte, data_buffer_symbol, data_buffer_byte_len);
		
		//print_char_array(data_buffer_byte, data_buffer_byte_len,"Data_buffer_byte");
		
		////printk("Polling LED %d\n", polling_id);
		
		// convert 8 bit to 32 bit in order that the PRU can read it
		i = 0;
		while(i<data_buffer_symbol_len)
		{
			if(data_buffer_symbol[i])
				actual_value += mask;
			mask = mask >> 1;
			if((mask==0)||(i==(data_buffer_symbol_len-1)))
			{
				tx_packet[ind_bytes++]=actual_value;
				actual_value = 0;
				mask = 0x80;
			}
			i++;
		}
		
		// convert polling id to led_active
		if(polling_id <=32) {
			led_active1 = 0x00000000;
			led_active2 = 1 << (polling_id-1);
		} else {
			led_active1 = 1 << (polling_id-33);
			led_active2 = 0x00000000;
		}
		
		// Construct header MX to TX (will be removed at TX)
		tx_packet[0] = 0;//tx[polling_id-1]; // master TX ID should be zero for channel meas, because we don't want to send prepreamble
		tx_packet[1] = (led_active1 >> 24) & 0xFF;
		tx_packet[2] = (led_active1 >> 16) & 0xFF;
		tx_packet[3] = (led_active1 >> 8) & 0xFF;
		tx_packet[4] = led_active1 & 0xFF;
		tx_packet[5] = (led_active2 >> 24) & 0xFF;
		tx_packet[6] = (led_active2 >> 16) & 0xFF;
		tx_packet[7] = (led_active2 >> 8) & 0xFF;
		tx_packet[8] = led_active2 & 0xFF;
		tx_packet[9] = (data_buffer_symbol_len >> 8) & 0xFF;
		tx_packet[10] = data_buffer_symbol_len & 0xFF;
		
			
		phy_broadcast(tx_packet,ind_bytes);
		for (k=1; k<=no_rx;k++) {
			if ((ret = ksocket_receive(sock_eth_in, (struct sockaddr_in *)&localaddr_eth, chm_response, sizeof(chm_response))) > 0) { 
				//printk("channel message received with %d bytes\n",ret);
				polling_id_recv = chm_response[0];
				mac_rx_recv= chm_response[1];
				
				printk("message coming from RX %d for LED %d\n",mac_rx_recv,polling_id_recv);
				//print_char_array(chm_response, ret,"chm_response hex: ");
				
				convert_char_int_array(ch_meas_int, chm_response+2, 96);
				// the channel measurment contains preamble (32 symbols) + SFD (16 symbols) -> 48 symbols in total
				//print_int_array_dec(ch_meas_int, 48,"chm_response dec: ");
				
				if(polling_id_recv >= 1 && polling_id_recv <= 36) {
					swing_tmp = get_swing(ch_meas_int, 48);
					if(mac_rx_recv == 21) {
						swing_rx1_tmp[polling_id_recv-1] = swing_tmp;
					}
					else if(mac_rx_recv == 22){
						swing_rx2_tmp[polling_id_recv-1] = swing_tmp;
					}
					else if(mac_rx_recv == 23){
						swing_rx3_tmp[polling_id_recv-1] = swing_tmp;
					}
					else if(mac_rx_recv == 24){
						swing_rx4_tmp[polling_id_recv-1] = swing_tmp;
					}
					//printk("swing %d set to %d\n", swing_rx1_tmp[polling_id_recv-1], polling_id_recv);
				}
				//printk("swing: %d \n",swing_rx1[polling_id-1]);		
			}
			else { // packet did not arrive 
				////printk("channel message time-out\n");
			}
		}
	}
	if(check_empty_array(swing_rx1_tmp,swing_len) != 1) {
		memcpy(swing_rx1,swing_rx1_tmp,swing_len*4);
		printk("RX1: updated swing_array\n");	
		print_int_array_dec(swing_rx1, sizeof(swing_rx1)/sizeof(int),"swing RX1");
		set_tx_master(swing_rx1,swing_len,21);
	}
	if(check_empty_array(swing_rx2_tmp,swing_len) != 1) {
		memcpy(swing_rx2,swing_rx2_tmp,swing_len*4);
		printk("RX2: updated swing_array\n");
		print_int_array_dec(swing_rx2, sizeof(swing_rx2)/sizeof(int),"swing RX2");
		set_tx_master(swing_rx2,swing_len,22);
	}
	if(check_empty_array(swing_rx3_tmp,swing_len) != 1) {
		memcpy(swing_rx3,swing_rx3_tmp,swing_len*4);
		printk("RX3: updated swing_array\n");	
		print_int_array_dec(swing_rx3, sizeof(swing_rx3)/sizeof(int),"swing RX3");
		set_tx_master(swing_rx3,swing_len,23);
	}	
	if(check_empty_array(swing_rx4_tmp,swing_len) != 1) {
		memcpy(swing_rx4,swing_rx4_tmp,swing_len*4);
		printk("RX4: updated swing_array\n");	
		print_int_array_dec(swing_rx4, sizeof(swing_rx4)/sizeof(int),"swing RX4");
		set_tx_master(swing_rx4,swing_len,24);
	}	
	
	if(send_swings) {
		char data_out_pc[576] = {0}; // 36 TXs * 4 bytes/ TX swing * 4 RX 
		//int data_out_pc_len = 72;
		memcpy (data_out_pc, 			swing_rx1, 36*4);
		memcpy (data_out_pc+(36*4), 	swing_rx2, 36*4);
		memcpy (data_out_pc+(2*36*4), 	swing_rx3, 36*4);
		memcpy (data_out_pc+(3*36*4), 	swing_rx4, 36*4);
		if(ksocket_send(sock_pc_out, (struct sockaddr_in *)&remoteaddr_pc, data_out_pc, sizeof(data_out_pc)) > 0) {
			printk("sent swing info to pc\n");
		}
		else {
			printk("ERROR sending swing info to pc\n");
		}	
	}
}

void perform_chm_single(int polling_id){
	
	char tx_packet[1000] = {0}, polling_id_recv = 0, mac_rx_recv = 0;
	unsigned int mask = 0x80, actual_value = 0, led_active1 = 0, led_active2 = 0;
	int payload_len = 1, ret=0, ind_bytes = 0, i= 0, j= 0, k=0;
	uint64_t led_active = 0;
	char chm_response[98] = {0};
	int ch_meas_int[48] = {0};
	int dest_id = 0xffff; // broadcast to all RXs
	char data_out_pc[99] = {0};
	
	ind_bytes = 11; 
	
	memset(data_buffer_byte, 0, sizeof(data_buffer_byte));
		
	data_buffer_byte_len = (PREAMBLE_LEN+SFD_LEN+OCTET_LEN+MAC_ADDR_LEN*2)+payload_len;
	
	// we add + 1 to symbol_len to make sure the last symbol is transmitted by the LEDs
	data_buffer_symbol_len = PREAMBLE_LEN*8+(data_buffer_byte_len-PREAMBLE_LEN)*8*2+1; //Length in symbols with preamble 
	construct_frame_header(data_buffer_byte, data_buffer_byte_len, data_buffer_symbol_len, dest_id, self_id); // self_id is defined in the beginning
	data_buffer_byte[PREAMBLE_LEN+SFD_LEN+OCTET_LEN+MAC_ADDR_LEN*2] = polling_id;
	OOK_with_Manchester_RLL(data_buffer_byte, data_buffer_symbol, data_buffer_byte_len);
	
	// convert 8 bit to 32 bit in order that the PRU can read it
	i = 0;
	while(i<data_buffer_symbol_len)
	{
		if(data_buffer_symbol[i])
			actual_value += mask;
		mask = mask >> 1;
		if((mask==0)||(i==(data_buffer_symbol_len-1)))
		{
			tx_packet[ind_bytes++]=actual_value;
			actual_value = 0;
			mask = 0x80;
		}
		i++;
	}
	
	// convert polling id to led_active
	if(polling_id <=32) {
		led_active1 = 0x00000000;
		led_active2 = 1 << (polling_id-1);
	} else {
		led_active1 = 1 << (polling_id-33);
		led_active2 = 0x00000000;
	}
	
	// Construct header MX to TX (will be removed at TX)
	tx_packet[0] = 0;//tx[polling_id-1]; // master TX ID should be zero for channel meas, because we don't want to send prepreamble
	tx_packet[1] = (led_active1 >> 24) & 0xFF;
	tx_packet[2] = (led_active1 >> 16) & 0xFF;
	tx_packet[3] = (led_active1 >> 8) & 0xFF;
	tx_packet[4] = led_active1 & 0xFF;
	tx_packet[5] = (led_active2 >> 24) & 0xFF;
	tx_packet[6] = (led_active2 >> 16) & 0xFF;
	tx_packet[7] = (led_active2 >> 8) & 0xFF;
	tx_packet[8] = led_active2 & 0xFF;
	tx_packet[9] = (data_buffer_symbol_len >> 8) & 0xFF;
	tx_packet[10] = data_buffer_symbol_len & 0xFF;
	
		
	phy_broadcast(tx_packet,ind_bytes);
	for (k=1; k<=no_rx;k++) {
		memset(data_out_pc,0,98);
		if ((ret = ksocket_receive(sock_eth_in, (struct sockaddr_in *)&localaddr_eth, chm_response, sizeof(chm_response))) > 0) { 
			//printk("channel message received with %d bytes\n",ret);
			polling_id_recv = chm_response[0];
			mac_rx_recv= chm_response[1];
			
			printk("message coming from RX %d for LED %d\n",mac_rx_recv,polling_id_recv);
			
			convert_char_int_array(ch_meas_int, chm_response+2, 96);

			memcpy (data_out_pc,chm_response, 98);

			if(ksocket_send(sock_pc_out, (struct sockaddr_in *)&remoteaddr_pc, data_out_pc, sizeof(data_out_pc)) > 0) {
				printk("sent CHM info to pc\n");
			}
			else {
				printk("ERROR sending CHM info to pc\n");
			}			
		}
		else { // packet did not arrive 
			printk("channel message time-out for RX%d\n",k);
			ksocket_send(sock_pc_out, (struct sockaddr_in *)&remoteaddr_pc, data_out_pc, sizeof(data_out_pc));
		}
	}
}

static void set_tx_master(int* swing, int swing_len, int mac_rx) {
	int index_max = get_index_max(swing, swing_len); // configure who should be the master for RX1
	printk("RX %d: TX ID with best channel: %d \n",mac_rx, index_max);	
	
	int indx_array = mac_rx - 21;
	tx_master[indx_array] = led_to_tx[index_max-1];	
}

static void assign_txs(void) {
	int ret;
	char data_in_pc[36] = {0}; // 8 bytes/RX * 4 RX

	if ((ret = ksocket_receive(sock_pc_in, (struct sockaddr_in *)&localaddr_pc, data_in_pc, sizeof(data_in_pc))) > 0) { 
		int i;
		for (i=0;i<no_rx;i++) {
			led_data_active2[i] = (data_in_pc[8*i+0] << 24 & 0xff000000) | (data_in_pc[8*i+1] << 16 & 0x00ff0000) | (data_in_pc[8*i+2] << 8 & 0x0000ff00)| (data_in_pc[8*i+3] & 0x000000ff);
			led_data_active1[i] = (data_in_pc[8*i+4] << 24 & 0xff000000) | (data_in_pc[8*i+5] << 16 & 0x00ff0000) | (data_in_pc[8*i+6] << 8 & 0x0000ff00)| (data_in_pc[8*i+7] & 0x000000ff);	
			printk("RX%d: %x %x\n",(i+1),led_data_active1[i], led_data_active2[i]);				
		}
	}
	else {
		printk("ERROR getting TX allocation info from pc\n");
		int i = 0;
		for (i=0;i<no_rx;i++) {
			led_data_active1[i] = 0; 
			led_data_active2[i] = 0;				
		}
	}
}

static int get_index_max(int* array_in, int array_len) {
	int i,index_max=0,max;
	max = array_in[index_max];

	for (i = 1; i < array_len; i++) {
			if (array_in[i] > max) {
			max = array_in[i];
			index_max = i;
		}
	}
	return (index_max+1);
}	

static int get_swing(int* array_in, int array_len)
{
	int i,ch_avg = 0,ch_total_high = 0,ch_avg_high = 0,ch_total_low = 0,ch_avg_low = 0,i_high=0,i_low=0;
	int swing;
	ch_avg = get_average(array_in, 48);
	
	//printk("average channel meas: %d\n",ch_avg);
	//print_int_array_dec(array_in, 48,"chm_response dec: ");
	
	for (i=0; i<array_len; i++) {
		if(array_in[i] >= ch_avg) {
			ch_total_high += array_in[i];
			i_high ++;
			//printk("HIGH: %d \n",ch_total_high);
		} else {
			ch_total_low += array_in[i];
			i_low ++;
			//printk("LOW: %d \n",ch_total_low);
		}
	}
	printk("LOW/HIGH: %d/%d \n",i_low,i_high);
	ch_avg_high = ch_total_high / i_high;
	ch_avg_low = ch_total_low / i_low;
	swing = ch_avg_high - ch_avg_low;
	return swing;
}


static void convert_char_int_array(int* array_out, char* array_in, int array_len)
{
	int i=0,j=0;
	for (i=0; i<array_len; i=i+2) {
		array_out[j] = (array_in[i] << 8 & 0xff00) | (array_in[i+1] & 0xff);
		j++;
	}
}

static int get_average(int* array_in, int array_len)
{
	int i,total=0,average=0;
	for (i=0; i<array_len; i++) {
		total += array_in[i];
	}
	average = total/array_len;
	return average;
}

static int mac_tx(void *data)
{
    struct vlc_priv *priv = netdev_priv(vlc_devs);
	int backoff_timer = 0;
	int ret;
	char data_in_pc[5] = {0};
    
start: 
    for (; ;) {
		if ((ret = ksocket_receive(sock_pc_in, (struct sockaddr_in *)&localaddr_pc, data_in_pc, sizeof(data_in_pc))) > 0) { 
			printk("packet received from PC with data: %x \n",data_in_pc[0]);
			if(data_in_pc[0] == 1) {// channel measurement
				phy_state = CHM;
			}
			else if(data_in_pc[0] == 2){ // assign the TXs to the RXs
				msleep(10);
				assign_txs();
			}
			else if(data_in_pc[0] == 3){ // assign the TX masters
				tx_master[0] = data_in_pc[1]; 
				tx_master[1] = data_in_pc[2]; 
				tx_master[2] = data_in_pc[3]; 
				tx_master[3] = data_in_pc[4];
				printk("tx_master set: RX1:BBB%d RX2:BBB%d RX3:BBB%d RX4:BBB%d\n",data_in_pc[1],data_in_pc[2],data_in_pc[3],data_in_pc[4]);
			}
			else if(data_in_pc[0] == 4){ // measure te channels from a single TX to all RXs
				int polling_id = data_in_pc[1];
				printk("start CHM measurement for LED %d\n", polling_id);
				perform_chm_single(polling_id);  
			}			
		}
		if(phy_state == CHM) {
			perform_chm();
			phy_state = TX;
		}
		if (kthread_should_stop()) {
            printk("=======EXIT mac_tx=======\n");
            return 0;
        }
		if(phy_state == TX) 
		{
			if (mac_or_app == APP) {
				if (!priv->tx_queue) {
					msleep(decoding_sleep_slot);
					goto start;
				}
				else{
				tx_pkt = vlc_dequeue_pkt(vlc_devs);
				/// Transmit the packet
				generate_DATA_frame(tx_pkt);
				}
			
			} else if (mac_or_app == MAC) {
			}

		}
		if (kthread_should_stop()) {
            printk("=======EXIT mac_tx=======\n");
			return 0;
        }
        
    }
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
	int encoded_len = payload_len_rx+2*MAC_ADDR_LEN+PROTOCOL_LEN;
	int num_of_blocks;
	int i,index_block, num_err=0;
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
            //printk("*** CRC error. ***\n");
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
  
   printk("rx_payload_len %d\n",rx_payload_len);
   get_the_data_rx(rx_data);
   
   //data_to_process=1;
   //printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);
   return len;
}


/*
 * Initialize the ethernet socket
 */
int ksocket_init(void) {
	if (sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock_eth_out) >= 0) {
	   // enable UDP broadcast 
		int optval = 1;
		mm_segment_t oldfs = get_fs();
		set_fs(KERNEL_DS);
		if (sock_setsockopt(sock_eth_out,SOL_SOCKET,SO_BROADCAST,(char *)&optval,sizeof(int)) < 0) 
			printk("Error, sock_setsockopt(SO_BROADCAST) failed!\n");
		set_fs(oldfs);

	   /* Set the UDP socket to send packets to broadcast */
		int c;

		memset(&remoteaddr_eth, 0, sizeof(struct sockaddr_in));
		remoteaddr_eth.sin_family      = AF_INET;
		remoteaddr_eth.sin_addr.s_addr = htonl(0xc0a807ff); //"192.168.5.255"
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
	if (sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock_eth_in) >= 0) {
		/* set a time out value for blocking receiving mode*/
		struct timeval tv;
		tv.tv_sec = 0; //1  /* Secs Timeout */
		tv.tv_usec = 100000; //0 // Not init'ing this can cause strange errors
		mm_segment_t oldfs = get_fs();
		set_fs(KERNEL_DS);
		if(sock_setsockopt(sock_eth_in, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(struct timeval)) < 0)
			printk("Error, sock_setsockopt(SO_RCVTIMEO) failed!\n");
		set_fs(oldfs);

	   /* Set the UDP socket to receive packets */
		int c;
		
		memset(&localaddr_eth, 0, sizeof(struct sockaddr_in));
		localaddr_eth.sin_family      = AF_INET;
		localaddr_eth.sin_addr.s_addr = htonl(0xc0a8077f); //"192.168.7.127
		localaddr_eth.sin_port        = htons(1111);
		
		if ((c = sock_eth_in->ops->bind(sock_eth_in, (struct sockaddr *)&localaddr_eth, sizeof(localaddr_eth))) < 0) {
			printk("bind() failed, error code %d!\n", c);
			return -1;
		}
		else 
			printk("eth socket in binded\n");
	}	
	else {
		printk("eth socket in could not be created\n");
		return -1;
	}
	
	// for connection with PC
	if (sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock_pc_out) >= 0) {
		int c;

		memset(&remoteaddr_pc, 0, sizeof(struct sockaddr_in));
		remoteaddr_pc.sin_family      = AF_INET;
		remoteaddr_pc.sin_addr.s_addr = htonl(0xc0a80781); //"192.168.7.129"
		remoteaddr_pc.sin_port        = htons(1113);
		
		if ((c =sock_eth_out->ops->connect(sock_pc_out, (struct sockaddr *)&remoteaddr_pc, sizeof(remoteaddr_pc), 0)) < 0) {
			printk("connect() failed, error code %d!\n", c);
			return -1;
		}
		else 
			printk("pc socket out connected\n");
	}
	else {
		printk("pc socket out could not be created\n");
		return -1;
	}
	if (sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock_pc_in) >= 0) {
		/* set a time out value for blocking receiving mode*/
		struct timeval tv;
		tv.tv_sec = 0; //1  /* Secs Timeout */
		tv.tv_usec = 1; //0 // Not init'ing this can cause strange errors
		mm_segment_t oldfs = get_fs();
		set_fs(KERNEL_DS);
		if(sock_setsockopt(sock_pc_in, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(struct timeval)) < 0)
			printk("Error, sock_setsockopt(SO_RCVTIMEO) failed!\n");
		set_fs(oldfs);

	   /* Set the UDP socket to receive packets */
		int c;
		
		memset(&localaddr_pc, 0, sizeof(struct sockaddr_in));
		localaddr_pc.sin_family      = AF_INET;
		localaddr_pc.sin_addr.s_addr = htonl(0xc0a8077f); //"192.168.7.127
		localaddr_pc.sin_port        = htons(1112);
		
		if ((c = sock_eth_in->ops->bind(sock_pc_in, (struct sockaddr *)&localaddr_pc, sizeof(localaddr_pc))) < 0) {
			printk("bind() failed, error code %d!\n", c);
			return -1;
		}
		else 
			printk("pc socket in binded\n");
	}	
	else {
		printk("pc socket in could not be created\n");
		return -1;
	}
	return 0;
}

static int phy_broadcast(unsigned char* data_byte, int data_byte_len) {
	//printk("message length in bytes: %d\n",data_byte_len);
	//print_char_array(data_byte, data_byte_len, "eth data: ");
	if(ksocket_send(sock_eth_out, (struct sockaddr_in *)&remoteaddr_eth, data_byte, data_byte_len) > 0) {
		//printk("eth message sent to all Tx\n");
		return 0;
	}
	else {
		printk("eth message NOT sent to all Tx\n");
		return -1;		
	}
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

int ksocket_receive(struct socket* sock, struct sockaddr_in* addr, unsigned char* buf, int len)
{
        struct msghdr msg;
        struct iovec iov;
        mm_segment_t oldfs;
        int size = 0;

        if (sock->sk==NULL) return 0;

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
        size = sock_recvmsg(sock,&msg,len,msg.msg_flags);
        set_fs(oldfs);

        return size;
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
	
	memunmap(tx_pru);
	memunmap(rx_pru);
	printk("Memory unmapped correctly\n");
	
	
	// Clean the threads
    if (task_mac_tx) {
        ret = kthread_stop(task_mac_tx);
		if(!ret)
			printk(KERN_INFO "Thread MAC TX stopped");
        task_mac_tx = NULL;
    }

	if(sock_eth_in != NULL) {
		printk("release the eth socket in\n");
		sock_release(sock_eth_in);
		sock_eth_in = NULL;
	}
	if(sock_eth_out != NULL) {
		printk("release the eth socket out\n");
		sock_release(sock_eth_out);
		sock_eth_out = NULL;
	}
	if(sock_pc_in != NULL) {
		printk("release the pc socket in\n");
		sock_release(sock_pc_in);
		sock_pc_in = NULL;
	}
	if(sock_pc_out != NULL) {
		printk("release the pc socket out\n");
		sock_release(sock_pc_out);
		sock_pc_out = NULL;
	}

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
	
}


int vlc_init_module(void)
{
	int ret = -ENOMEM;
	printk("Initializing the VLC module...\n");
	phy_state = TX;
	//vlc_interrupt = vlc_regular_interrupt;
	
	// start for eth socket
	// initialize the eth socket
	printk("Opening eth socket\n");
	ret = ksocket_init();
	///////////// Map memory for communicating with PRU /////////////
	
	rx_pru = memremap(PRU_ADDR+PRU0, 0x10000 ,MEMREMAP_WT);
	tx_pru = memremap(PRU_ADDR+PRU1, 0x10000 ,MEMREMAP_WT);
	printk("Memory mapped correctly: %x\n",tx_pru);
	printk("Memory mapped correctly: %x\n",rx_pru);
	
	/// Create the device and register it
	vlc_devs = alloc_netdev(sizeof(struct vlc_priv), "vlc0", NET_NAME_UNKNOWN, vlc_init);
	if (vlc_devs == NULL)
		goto out;
	ret = register_netdev(vlc_devs);
    if (ret)
        printk("VLC: error registering device \"%s\"\n", vlc_devs->name);
	else printk("VLC: Device registered!\n");
	
	task_mac_tx = kthread_run(mac_tx,"TX thread","VLC_TX");
	if (IS_ERR(task_mac_tx)){
		printk("Unable to start kernel threads. \n");
		task_mac_tx = NULL;
		goto out;
	}	
    
	
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