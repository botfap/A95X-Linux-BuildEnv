//-----------------------------------------------------------------------------
// add for AIFIFO channel (old $cdrom/rtl/cdr_top/getbit)
//-----------------------------------------------------------------------------
// Bit 3 	CRC pop aififo enable
// Bit 2		writing to this bit to 1 causes CRC module reset
// Bit 1		enable aififo
// Bit 0		writing to this bit to 1 causes aififo soft reset
#define AIU_AIFIFO_CTRL                            0x1580
//'h0000
// AIFIFO status register
// Bit 13		//aififo request to dcu status
// Bit 12		//dcu select status
// Bit 11:5		//aififo word counter number 
// Bit 4:0		//how many bits left in the first pop register
#define AIU_AIFIFO_STATUS                          0x1581
// Same fucntion as the AIGBIT of AIFIFO in CDROM module
// write to this register how many bits wanna pop, 
// and reading this register gets the corresponding bits data
#define AIU_AIFIFO_GBIT                            0x1582
// Same function as the AICLB of AIFIFO in CDROM module
// return the leading zeros by reading this registers
#define AIU_AIFIFO_CLB                             0x1583
// --------------------------------------------
// AIFIFO DDR Interface
// --------------------------------------------
// The AIFIFO start pointer into DDR memory is a 32-bit number
// The Start pointer will automatically be truncated to land on 
// an 8-byte boundary.  That is, bits [2:0] = 0;
#define AIU_MEM_AIFIFO_START_PTR                   0x1584
// The current pointer points so some location between the START and END 
// pointers.  The current pointer is a BYTE pointer.  That is, you can 
// point to any BYTE address within the START/END range
#define AIU_MEM_AIFIFO_CURR_PTR                    0x1585
#define AIU_MEM_AIFIFO_END_PTR                     0x1586
#define AIU_MEM_AIFIFO_BYTES_AVAIL                 0x1587
// AIFIFO FIFO Control
// bit  [15:11] unused
// bit  [10]    use_level       Set this bit to 1 to enable filling of the FIFO controlled by the buffer
//                              level control.  If this bit is 0, then use bit[1] to control the enabling of filling
// bit  [9]     Data Ready.     This bit is set when data can be popped
// bit  [8]     fill busy       This bit will be high when we're fetching data from the DDR memory
//                              To reset this module, set cntl_enable = 0, and then wait for busy = 0. 
//                              After that you can pulse cntl_init to start over
// bit  [7]     cntl_endian_jic Just in case endian.  last minute byte swap of the data out of
//                              the FIFO to getbit
// bit  [6]     unused  
// bits [5:3]   endian:         see $lib/rtl/ddr_endian.v
// bit  [2]     cntl_empty_en   Set to 1 to enable reading the DDR memory FIFO and filling the pipeline to get-bit
//                              Set cntl_empty_en = cntl_fill_en = 0 when pulsing cntl_init
// bit  [1]     cntl_fill_en    Set to 1 to enable reading data from DDR memory
// bit  [0]     cntl_init:      After setting the read pointers, sizes, channel masks
//                              and read masks, set this bit to 1 and then to 0
//                              NOTE:  You don't need to pulse cntl_init if only the start address is
//                              being changed
#define AIU_MEM_AIFIFO_CONTROL                     0x1588
// --------------------------------------------
// AIFIFO Buffer Level Manager
// --------------------------------------------
#define AIU_MEM_AIFIFO_MAN_WP                      0x1589
#define AIU_MEM_AIFIFO_MAN_RP                      0x158a
#define AIU_MEM_AIFIFO_LEVEL                       0x158b
//
// bit  [1]     manual mode     Set to 1 for manual write pointer mode
// bit  [0]     Init            Set high then low after everything has been initialized
#define AIU_MEM_AIFIFO_BUF_CNTL                    0x158c

#define AIU_MEM_AIFIFO_BUF_WRAP_COUNT              0x158d
#define AIU_MEM_AIFIFO2_BUF_WRAP_COUNT             0x158e
// bit 29:24 A_brst_num
// bit 21:16 A_id
// bit 15:0 level_hold 
#define AIU_MEM_AIFIFO_MEM_CTL                     0x158f

