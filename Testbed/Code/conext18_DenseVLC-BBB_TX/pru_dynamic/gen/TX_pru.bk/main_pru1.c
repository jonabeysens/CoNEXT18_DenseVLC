#include <stdint.h>
#include <pru_cfg.h>
#include "resource_table_pru1.h"

// The function is defined in pru1_asm_blinky.asm in same dir
// We just need to add a declaration here, the defination can be
// seperately linked
extern void START(void);

void main(void)
{
	START();
}
