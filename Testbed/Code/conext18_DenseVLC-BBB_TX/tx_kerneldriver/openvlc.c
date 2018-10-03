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

#include <linux/device.h>         // Header to support the kernel Driver Model
#define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver


// start for eth socket
static struct socket * sock_eth_in = NULL;
struct sockaddr_in localaddr_eth;
int ksocket_receive(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len);
// end for eth socket

static int get_local_led_active(int led_active1, int led_active2);

static int block_size = 200;
module_param(block_size, int, 0);



#define MAX_RETRANSMISSION  2 // At most tx 4 times

static int dst_id = 8;
static int self_id = 7;

static int mode = 0;
static int threshold = 5;
static int flag_exit = 0;
static int flag_lock = 0;
module_param(flag_lock, int, 0);

static int mtu = 1200;

module_param(self_id, int, 0);
module_param(dst_id, int, 0);
module_param(mode, int, 0);
module_param(threshold, int, 0);
module_param(mtu, int, 0);

struct proc_dir_entry *vlc_dir, *tx_device, *rx_device;

const char *name = "any";

static struct task_struct *task_mac_tx = NULL;

// For ACK frame
// start for miso
#define ACK_BYTE_LEN 50
static char ack_buffer_byte_eth[ACK_BYTE_LEN];
// end for miso

static int decoding_sleep_slot = 1;


MODULE_AUTHOR("Jona BEYSENS/Ander GALISTEO/Diego JUARA");
MODULE_LICENSE("Dual BSD/GPL");

volatile void* gpio1;
volatile void* gpio2;


enum  state {TX, INTERFERING} ; // Short sensing is implemented in tx state
enum  state phy_state;

/////////////////// PRU variables ///////////////////////////////////

#define PRU_ADDR 0x4A300000
#define PRU0 0x00000000
#define PRU1_LED1 0x00002000
#define PRU1_LED2 0x00003000
#define PRU1_LED3 0x00000000
#define PRU1_LED4 0x00001000
#define PRU_SHARED 0x00010000

unsigned int *tx_pru1;
unsigned int *tx_pru2;
unsigned int *tx_pru3;
unsigned int *tx_pru4;
unsigned int *shared_pru;
unsigned int *rx_pru;

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
	printk("Enqueue a packet!\n");
    
    
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

static void construct_frame_header(char* buffer, int buffer_len, int payload_len)
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
    buffer[PREAMBLE_LEN+3] = (unsigned char) ((dst_id>>8) & 0xff);
    buffer[PREAMBLE_LEN+4] = (unsigned char) (dst_id & 0xff);
    // Source address
    buffer[PREAMBLE_LEN+5] = (unsigned char) ((self_id>>8) & 0xff);
    buffer[PREAMBLE_LEN+6] = (unsigned char) (self_id & 0xff);
    // CRC
    //crc = crc16(buffer+PREAMBLE_LEN+SFD_LEN, MAC_HDR_LEN+payload_len);
    //buffer[buffer_len-2] = (char) ((0xff00&crc)>>8); // CRC byte 1
	//buffer[buffer_len-1] = (char) ((0x00ff&crc)); // CRC byte 2
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
			//tx_pru[ind_pru++]=actual_value;
			actual_value = 0;
			mask = 0x80000000;
		}
		i++;
	}
	
	return ind_pru-1;
	
}




static int mac_tx(void *data)
{
    struct vlc_priv *priv = netdev_priv(vlc_devs);
	int backoff_timer = 0;
    int i;
	int ret;
	char rec_packet[1000];
	int packet_length;
	//char self_id=5;
	unsigned int led_active1 = 0, led_active2 = 0,led_active=0;
	int master = 0;
	
start: 
    for (; ;) 
	{
		if ((ret = ksocket_receive(sock_eth_in, (struct sockaddr_in *)&localaddr_eth, &rec_packet, sizeof(rec_packet))) > 0) {
			
			// if(shared_pru[1] != 0) { // PRU is still busy
				// printk("PRU was still busy!\n");
			// }
			led_active1 = (rec_packet[1]<<24)|(rec_packet[2]<<16)|(rec_packet[3]<<8)|rec_packet[4];
			led_active2 = (rec_packet[5]<<24)|(rec_packet[6]<<16)|(rec_packet[7]<<8)|rec_packet[8];
			packet_length = (rec_packet[9]<<8)|rec_packet[10]; //Get the length in symbols
			//printk("New eth packet: %d symbols\n",packet_length);
			//for(i=0;i<ret;i++)
			//	printk("Last reg: %x\n",rec_packet[i]);
			//printk("Bytes received: %d\n", ret);
			ret -= 11;
			
			// To get the 4 bits that say if a specific led should tx or not
					
			if (kthread_should_stop()) {
				printk("=======EXIT mac_tx=======\n");
				return 0;
			}
			
			if(phy_state == TX) {	
				memset(tx_pru1,0,4000); // memory of PRU is 1000 * 32 bit = 4000 bytes
				memset(tx_pru2,0,4000);
				memset(tx_pru3,0,4000);
				memset(tx_pru4,0,4000);
				
				led_active = get_local_led_active(led_active1,led_active2);
				
				//printk("LED active local active to this TX %x\n",led_active);
				//printk("Self id %d\n",self_id);
				//printk("Master %x\n",master);
				//
				// first check if there is a local led that should be active, only if so we copy data to pru
				if((led_active & 0x0000000F) != 0) {
					master = rec_packet[0];

					if(master == self_id)
					{
						printk("I'm master\n");
						shared_pru[2] = 1; // don't wait for prepreamble
						
						printk("Packet length to send with prepreamble %d\n", packet_length+32);
						tx_pru1[0] = 32;
						tx_pru2[0] = 32;
						tx_pru3[0] = 32;
						tx_pru4[0] = 32;
						tx_pru1[1] = 0x99999999;
						tx_pru2[1] = 0x99999999;
						tx_pru3[1] = 0x99999999;
						tx_pru4[1] = 0x99999999;

						if(led_active & 0x00000001)
						{
							//printk("LED1: I'm going to transmit\n");
							tx_pru1[0] = packet_length+32;
							//printk("Packet length %d\n",tx_pru1[0]);
							//memcpy(tx_pru1+2,rec_packet+3,ret); Problems with endiannes
							int j=0;
							for(i=0;i<=ret;i+=4)
							{	
								tx_pru1[2+j]=(rec_packet[11+i+0] << 24) |(rec_packet[11+i+1] << 16) |(rec_packet[11+i+2] << 8) | rec_packet[11+i+3];
								j+=1;
							}
							//printk("Last value: %x\n",tx_pru1[j-1+2]);
						}
						if(led_active & 0x00000002)
						{
							tx_pru2[0] = packet_length+32;
							//memcpy(tx_pru1+2,rec_packet+3,ret); Problems with endiannes
							int j=0;
							for(i=0;i<=ret;i+=4)
							{	
								tx_pru2[2+j]=(rec_packet[11+i+0] << 24) |(rec_packet[11+i+1] << 16) |(rec_packet[11+i+2] << 8) | rec_packet[11+i+3];
								j+=1;
							}
						}
						if(led_active & 0x00000004)
						{
							tx_pru3[0] = packet_length+32;
							//memcpy(tx_pru1+2,rec_packet+3,ret); Problems with endiannes
							int j=0;
							for(i=0;i<=ret;i+=4)
							{	
								tx_pru3[2+j]=(rec_packet[11+i+0] << 24) |(rec_packet[11+i+1] << 16) |(rec_packet[11+i+2] << 8) | rec_packet[11+i+3];
								j+=1;
							}
						}
						if(led_active & 0x00000008)
						{
							tx_pru4[0] = packet_length+32;
							//memcpy(tx_pru1+2,rec_packet+3,ret); Problems with endiannes
							int j=0;
							for(i=0;i<=ret;i+=4)
							{	
								tx_pru4[2+j]=(rec_packet[11+i+0] << 24) |(rec_packet[11+i+1] << 16) |(rec_packet[11+i+2] << 8) | rec_packet[11+i+3];
								j+=1;
							}
							//printk("TX4 tx_id: %x\n", tx_pru4[2+j-1]);
						}
					} else
					{
						if (packet_length == 161) 
						{
							// don't wait for prepreamble, but send right away
							shared_pru[2] = 1;
							printk("Doing channel meas with LED %x\n",led_active);
						}
						else if (master == 254) {
							shared_pru[2] = 1;
							printk("Start interfering with local led(s): %x\n", led_active);	
							phy_state = INTERFERING;
						}
						else
						{
							// we are the slave and we wait for the prepreamble
							shared_pru[2] = 0;
							printk("Packet length to send, without prepreamble %d\n", packet_length);
						}
						
						if(led_active & 0x00000001)
						{
							tx_pru1[0] = packet_length;
							int j=0;
							for(i=0;i<=ret;i+=4)
							{
								tx_pru1[1+j]=(rec_packet[11+i+0] << 24) |(rec_packet[11+i+1] << 16) |(rec_packet[11+i+2] << 8) | rec_packet[11+i+3];
								j+=1;
							}
							//printk("Last value: %x\n",tx_pru1[j]);
						}
						if(led_active & 0x00000002)
						{
							tx_pru2[0] = packet_length;
							int j=0;
							for(i=0;i<=ret;i+=4)
							{
								tx_pru2[1+j]=(rec_packet[11+i+0] << 24) |(rec_packet[11+i+1] << 16) |(rec_packet[11+i+2] << 8) | rec_packet[11+i+3];
								j+=1;
							}
						}
						if(led_active & 0x00000004)
						{
							tx_pru3[0] = packet_length;
							int j=0;
							for(i=0;i<=ret;i+=4)
							{
								tx_pru3[1+j]=(rec_packet[11+i+0] << 24) |(rec_packet[11+i+1] << 16) |(rec_packet[11+i+2] << 8) | rec_packet[11+i+3];
								j+=1;
							}
						}
						if(led_active & 0x00000008)
						{
							tx_pru4[0] = packet_length;
							int j=0;
							for(i=0;i<=ret;i+=4)
							{
								tx_pru4[1+j]=(rec_packet[11+i+0] << 24) |(rec_packet[11+i+1] << 16) |(rec_packet[11+i+2] << 8) | rec_packet[11+i+3];
								j+=1;
							}
						}
					}
					shared_pru[1] = (led_active & 0x0000000F); // Set which LED are active change depend the LED // starts the pru sending data
				}
			}
			else if(phy_state = INTERFERING) {
				master = rec_packet[0];
				if(master  == 255) { // stop interfering
					phy_state = TX;
					printk("Stop interfering\n");	
				}
			}
		}
		if(phy_state == TX) {
			if(master != 0 && master !=self_id && shared_pru[3]!=0) {
			   shared_pru[3] = 0;
			   printk("Prepreamble detected!\n");
			}
		}
		else if(phy_state == INTERFERING) {
			if(shared_pru[1] == 0) { // transmit again same data
				shared_pru[1] = (led_active & 0x0000000F); // Set which LED are active change depend the LED
				printk("resend interference data\n");
			}
				
		}
		//msleep(decoding_sleep_slot);
		//usleep_range(50,100);
		if (kthread_should_stop()) {
			printk("=======EXIT mac_tx=======\n");
			return 0;
		}
	}
}

// To get the 4 bits that say if a specific led should tx or not
static int get_local_led_active(int led_active1, int led_active2) {
	int LED1[9]={1,3,5,13,15,17,25,27,29};
	int LED2[9]={2,4,6,14,16,18,26,28,30};
	int LED3[9]={7,9,11,19,21,23,31,33,35};
	int LED4[9]={8,10,12,20,22,24,32,34,36};
	
	int led_active=0;
	char index_for_select_tx = (char)(self_id - 1);
	
	//get MSB
	int tmp_index=LED4[index_for_select_tx];
	if (tmp_index>32)
	{
		led_active=led_active | ((led_active1>>(tmp_index-32-1)) & 0x00000001);
	}
	else{
		led_active=led_active | (led_active2>>(tmp_index-1)& 0x00000001);
	}
	led_active = led_active<<1;
	//get second MSB
	tmp_index=LED3[index_for_select_tx];
	if (tmp_index>32)
	{
		led_active=led_active | ((led_active1>>(tmp_index-32-1)) & 0x00000001);
	}
	else{
		led_active=led_active | (led_active2>>(tmp_index-1)& 0x00000001);
	}
	led_active = led_active<<1;
	//get second LSB
	tmp_index=LED2[index_for_select_tx];
	if (tmp_index>32)
	{
		led_active=led_active | ((led_active1>>(tmp_index-32-1)) & 0x00000001);
	}
	else{
		led_active=led_active | (led_active2>>(tmp_index-1)& 0x00000001);
	}
	led_active = led_active<<1;
	//get LSB
	tmp_index=LED1[index_for_select_tx];
	if (tmp_index>32)
	{
		led_active=led_active | ((led_active1>>(tmp_index-32-1)) & 0x00000001);
	}
	else{
		led_active=led_active | (led_active2>>(tmp_index-1)& 0x00000001);
	}
	// Now led_active contains its Least Significant 4 bits 
	return led_active;
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


int ksocket_init(void) {
	if (sock_create(PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock_eth_in) >= 0) {
		/* set a time out value for blocking receiving mode*/
		struct timeval tv;
		tv.tv_sec = 0;  /* Secs Timeout */
		tv.tv_usec = 500;  // Not init'ing this can cause strange errors
		mm_segment_t oldfs = get_fs();
		set_fs(KERNEL_DS);
		if(sock_setsockopt(sock_eth_in, SOL_SOCKET, SO_RCVTIMEO, (char*) &tv, sizeof(struct timeval)) < 0)
			printk("Error, sock_setsockopt(SO_RCVTIMEO) failed!\n");
		set_fs(oldfs);

		int c;		
		memset(&localaddr_eth, 0, sizeof(struct sockaddr_in));
		localaddr_eth.sin_family      = AF_INET;
		localaddr_eth.sin_addr.s_addr = htonl(0xc0a807ff); //"192.168.7.255"
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
	return 0;
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
	
	memunmap(tx_pru1);
	memunmap(tx_pru2);
	memunmap(tx_pru3);
	memunmap(tx_pru4);
	memunmap(shared_pru);
	printk("Memory unmapped correctly\n");
	
	
	// Clean the threads
	if (task_mac_tx) {
        ret = kthread_stop(task_mac_tx);
		if(!ret)
			printk(KERN_INFO "Thread MAC TX stopped\n");
        task_mac_tx = NULL;
    }
	
	// start for eth socket
	if(sock_eth_in != NULL) {
		printk("release the eth socket in\n");
		sock_release(sock_eth_in);
		sock_eth_in = NULL;
	}
	// end for eth socket
	
	//Clean devices
	if (vlc_devs) {
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
		
        printk("unregister the devs\n");
        unregister_netdev(vlc_devs);
        
        
        printk("free the devs\n");
        free_netdev(vlc_devs);
    }
	
	return 0;
}


int vlc_init_module(void)
{
	int ret = -ENOMEM;
	printk("Initializing the VLC module...\n");
	phy_state = TX;
	
	// start for eth socket
	// initialize the eth socket
	printk("Opening eth socket\n");
	ret = ksocket_init();
	//vlc_interrupt = vlc_regular_interrupt;
	
	///////////// Map memory for communicating with PRU /////////////
	
	shared_pru = memremap(PRU_ADDR+PRU_SHARED, 0x10000 ,MEMREMAP_WT);
	printk("Memory mapped correctly: %x\n",shared_pru);
	tx_pru1 = memremap(PRU_ADDR+PRU1_LED1, 1000 ,MEMREMAP_WT);
	printk("Memory mapped correctly: %x\n",tx_pru1);
	tx_pru2 = memremap(PRU_ADDR+PRU1_LED2, 1000 ,MEMREMAP_WT);
	printk("Memory mapped correctly: %x\n",tx_pru2);
	tx_pru3 = memremap(PRU_ADDR+PRU1_LED3, 1000 ,MEMREMAP_WT);
	printk("Memory mapped correctly: %x\n",tx_pru3);
	tx_pru4 = memremap(PRU_ADDR+PRU1_LED4, 1000 ,MEMREMAP_WT);
	printk("Memory mapped correctly: %x\n",tx_pru4);
	
	//printk("Memory mapped correctly: %x\n",rx_pru);
	
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
		printk("Unable to start kernel thread. \n");
		task_mac_tx = NULL;
		goto out;
	}
		
	
	
	
	out:
    printk("------EXIT vlc_init_module------\n");
	if (ret)
		vlc_cleanup();

	return ret;
}



module_init(vlc_init_module);
module_exit(vlc_cleanup);