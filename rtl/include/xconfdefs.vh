//use xconf_mem
`define CONF_MEM_USE

// Total conf_reg bits
`define CONF_BITS (2*`nMEM*`MEMP_CONF_BITS + `nALU*`ALU_CONF_BITS + `nALULITE*`ALULITE_CONF_BITS + `nMUL*`MUL_CONF_BITS + `nMULADD*`MULADD_CONF_BITS + `nBS*`BS_CONF_BITS)

//
// CONFIGURATION REGISTER 
//
// Bit map
`define CONF_MEM0A_B (`CONF_BITS-1)
`define CONF_ALU0_B (`CONF_MEM0A_B - 2*`nMEM*`MEMP_CONF_BITS)
`define CONF_ALULITE0_B (`CONF_ALU0_B - `nALU*`ALU_CONF_BITS)
`define CONF_MUL0_B (`CONF_ALULITE0_B - `nALULITE*`ALULITE_CONF_BITS)
`define CONF_MULADD0_B (`CONF_MUL0_B - `nMUL*`MUL_CONF_BITS)
`define CONF_BS0_B (`CONF_MULADD0_B - `nMULADD*`MULADD_CONF_BITS)

//
// Memory map
//
// FU configurations 
`define CONF_MEM0A 0
`define CONF_ALU0 (`CONF_MEM0A + 2*`nMEM*`MEMP_CONF_OFFSET)
`define CONF_ALULITE0 (`CONF_ALU0 + `nALU*`ALU_CONF_OFFSET)
`define CONF_MUL0 (`CONF_ALULITE0 + `nALULITE*`ALULITE_CONF_OFFSET)
`define CONF_MULADD0 (`CONF_MUL0  + `nMUL*`MUL_CONF_OFFSET)
`define CONF_BS0 (`CONF_MULADD0  + `nMULADD*`MULADD_CONF_OFFSET)

//
// Address widths
//
//log2(max number of configuration fields)
`define CONF_REG_ADDR_W ($clog2(`CONF_BS0  + `nBS*`BS_CONF_OFFSET))
//log2(size of conf cache)
`define CONF_MEM_ADDR_W 3

// clear config register
`define CONF_CLEAR (1<<`CONF_REG_ADDR_W)
`define GLOBAL_CONF_CLEAR (`CONF_CLEAR+1)

//only used if CONF_MEM_USE is defined (ensures no conflict with FU configs)
`define CONF_MEM (`CONF_CLEAR + (1<<(`CONF_REG_ADDR_W-1)))
