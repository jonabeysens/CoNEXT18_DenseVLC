//gcc read_prepre.c -o read_prepre -lpthread
/************************************************************************************
// Author: Diego Juara
// IMDEA Networks
// From the previus code of Hongjia Wu from Delft University of Technology and Derek Molloy
*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>

#define MAX_BUF 64       //File name for on board adc
#define PRU_NUM 1		//PRU 0 is used, PRU 1 is free. (Two PRUs in BBB besides main ARM processor)
#define NTHREADS 1		// An extra thread handling user input and the main thread is for initiating PRU
#define PRU_ADDR 0x4A300000
#define FREQUENCY_OFFSET 0x00010000
#define TX1_OFFSET 0x00002000
#define TX2_OFFSET 0x00003000
#define TX3_OFFSET 0x00011000
#define TX4_OFFSET 0x00012000
#define TX_PRU_NUM	   1
#define MMAP0_LOC   "/sys/class/uio/uio0/maps/map0/"
#define MMAP1_LOC   "/sys/class/uio/uio0/maps/map1/"

#define BUFFER_LENGTH 32001     

static int ret, fd_vlc, fd_mem, fd_debug;



//Shared memory pointer
ulong* config;
ulong previous=0;

void *transmit(void *arg)
{
	int i,cnt1,cnt2,cnt3,cnt4;
	unsigned int tmp1,tmp2,tmp3,tmp4,mask1,mask2,mask3,mask4;
	while(1)
	{
		if(previous!=config[4])
		{
			printf("Threshold %d\n", config[4]);
		
			previous=config[4];
		}
	}
	int unmap_error;
	printf("End of the program\n");
	unmap_error = munmap(config,0x10000);
	if(unmap_error!=0)
	{
		printf("Unmap Error\n");
		//exit(0); 
	}
   
}

/************************************************************************************
    Main function that initilizes PRU
*************************************************************************************/
int main (void)
{  
	if(getuid()!=0){
      printf("You must run this program as root. Exiting.\n");
      exit(EXIT_FAILURE);
    }
	printf("Starting device test code example...\n");
   
	
	// Map shared memory
	int fd_mem = open("/dev/mem",O_RDWR | O_SYNC);
	config = (ulong*) mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, PRU_ADDR+FREQUENCY_OFFSET);
	
	
    
	
    //UI Thread
    int tx;
	pthread_t threads[NTHREADS];
	tx = pthread_create(&threads[0], NULL, transmit, NULL);
	if(tx!=0)
	{
		printf("Error starting the thread tx\n");
	}
	
	while(1);
	
    return EXIT_SUCCESS;
}
