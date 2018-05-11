#include "ht_runtime.h"
#include "ht_scheduled_funs.h"


typedef struct {
    uint32_t start;
    uint32_t size;
} buffer_t;


#define MAX_BUFFERS 1
#define INVALID_BUF_ID (MAX_BUFFERS + 1)
static uint8_t cur_buf_id = INVALID_BUF_ID;
static buffer_t buffers[MAX_BUFFERS];


#ifdef USE_RAM
  // #define RAM_ORIGIN 0x20000000
  // #define RAM_SIZE (192 * 1024)
  // #define RAM_END (RAM_ORIGIN + RAM_SIZE)
  // static uint32_t free_mem_start = 0x20020000;
  // static uint32_t free_mem_end = RAM_END;
#elif defined USE_ARRAY_BUF
  #define MEM_BUF_SIZE (1024 * 2)
  static uint8_t mem_buf[MEM_BUF_SIZE]; // goes to .bss section (to RAM)
  static uint32_t free_mem_start = (uint32_t)mem_buf;
  static uint32_t free_mem_end = (uint32_t)mem_buf + MEM_BUF_SIZE;
#elif defined USE_SDRAM
  static uint32_t free_mem_end = SDRAM_DEVICE_ADDR + SDRAM_DEVICE_SIZE;
  /* LCD Framebuffer occupies SDRAM too, so, try to not touch it... */
  #ifdef LCD_FRAME_BUFFER
      #define LCD_BYTES_PER_PIXEL 2
      #define LCD_FRAME_BUFFER_SIZE (ILI9341_LCD_PIXEL_WIDTH * ILI9341_LCD_PIXEL_HEIGHT * LCD_BYTES_PER_PIXEL)
      static uint32_t free_mem_start = LCD_FRAME_BUFFER + BUFFER_OFFSET + MAX_LAYER_NUMBER * LCD_FRAME_BUFFER_SIZE;
  #else
      static uint32_t free_mem_start = SDRAM_DEVICE_ADDR;
  #endif
#endif


static char default_msg[] = "hello from there\r\n";


static bool HandleBufAccess(uint32_t addr, uint32_t size) {
    if (cur_buf_id >= MAX_BUFFERS || cur_buf_id == INVALID_BUF_ID) {
        LCD_ErrLog("invalid buf id!\n");
        return false;
    }

    buffer_t buf = buffers[cur_buf_id];
    if (addr < buf.start) {
        LCD_ErrLog("too low addr (%lx<%lx)!\n", addr, buf.start);
        return false;
    }
    if (addr + size >= buf.start + buf.size) {
        LCD_ErrLog("too high addr (%lx>=%lx)!\n", addr + size, buf.start + buf.size);
        return false;
    }

    return true;
}


#ifdef USE_HEAP
static inline size_t HTBufferAlloc(size_t size) {
    return (size_t)malloc(size);
}
#elif defined USE_SDRAM || defined USE_ARRAY_BUF
static size_t HTBufferAlloc(size_t size) {
  size_t new_buf_start = 0;
  if (size < free_mem_end - free_mem_start) {
      new_buf_start = free_mem_start;
      free_mem_start += size;
  }
  return new_buf_start;
}
#endif


void HANDLE_COMMAND(void)
{
  cmd_ret_code_t cmd_ret_code = Err;
  uint8_t* data_ptr = received_data;
  action_int_t cmd = *(action_int_t*)data_ptr;
  data_ptr += sizeof(action_int_t);
  switch(cmd)
  {
      case ActionEcho:
          {
              uint32_t msg_size = last_cmd.size - sizeof(cmd);
              LCD_DbgLog("cmd:Echo: size=%lu\n", msg_size);
              CDC_Transmit_HS(data_ptr, msg_size);
              break;
          }
      case ActionGetBuffer:
          {
              LCD_DbgLog("cmd:GetBuffer:");
              cmd_get_buffer_t cmd2 = *(cmd_get_buffer_t*)data_ptr;
              data_ptr += sizeof(cmd2);

              // reset to invalid values for case of failure
              addr_t result = 0;
              cur_buf_id = INVALID_BUF_ID;

              if (cmd2.id < MAX_BUFFERS) {

                  if (buffers[cmd2.id].start == 0) {
                      // Trying allocate new buffer
                      result = HTBufferAlloc(cmd2.size);
                      if (result) {
                          buffer_t new_buf = { result, cmd2.size };
                          buffers[cmd2.id] = new_buf;
                          cur_buf_id = cmd2.id;
                      } else {
                          LCD_ErrLog("buffer alloc mem error!\n");
                      }
                  } else {
                      // Using existing buffer
                      result = buffers[cmd2.id].start;
                      cur_buf_id = cmd2.id;
                  }

              } else {
                  LCD_ErrLog("incorrect buff id!\n");
              }

              // in case of error return zero
              LCD_DbgLog("ret addr %lx\n", result);
              CDC_Transmit_HS((uint8_t*)&result, sizeof(result));
              break;
          }
      case ActionRead:
          {
              LCD_DbgLog("cmd:Read:");
              cmd_read_t cmd2 = *(cmd_read_t*)data_ptr;
              data_ptr += sizeof(cmd2);
              LCD_DbgLog("addr=%lx;size=%lu\n", cmd2.addr, cmd2.size);

              if (true) { // we can access not only 'our' mem, but special addresses too
              // if (HandleBufAccess(cmd2.addr, cmd2.size)) {
                  CDC_Transmit_HS((uint8_t*)cmd2.addr, cmd2.size);
              } else {
                  // return empty msg (without payload)
                  uint8_t zero = 0;
                  CDC_Transmit_HS((uint8_t*)&zero, 0);
              }
              break;
          }
      case ActionWrite:
          {
              LCD_DbgLog("cmd:Write:");
              cmd_write_t cmd2 = *(cmd_write_t*)data_ptr;
              data_ptr += sizeof(cmd2);
              LCD_DbgLog("addr=%lx;size=%lu\n", cmd2.addr, cmd2.size);

              addr_t written = -1;
              if (true) { // we can access not only 'our' mem, but special addresses too
              // if (HandleBufAccess(cmd2.addr, cmd2.size)) {
                  memcpy((uint8_t*)cmd2.addr, data_ptr, cmd2.size);
                  written = cmd2.size;
              }
              CDC_Transmit_HS((uint8_t*)&written, sizeof(written));
              break;
          }
      case ActionCall:
          {
              LCD_DbgLog("cmd:Call:");
              cmd_call_t cmd2 = *(cmd_call_t*)data_ptr;
              data_ptr += sizeof(cmd2);
              LCD_DbgLog("addr=%lx\n", cmd2.addr);

              if (true) { // we can access not only 'our' mem, but special addresses too
              // if (HandleBufAccess(cmd2.addr, 0)) {
                  addr_t fnaddr = cmd2.addr | 0x1; // to correctly set ESPR.T bit on call (because ESPR.T == address<0>)
                  void(*fnptr)() = ( void(*)() )fnaddr;
                  fnptr();
                  cmd_ret_code = CallOK;
              } else {
                  cmd_ret_code = ErrCall;
              }
              CDC_Transmit_HS((uint8_t*)&cmd_ret_code, sizeof(cmd_ret_code));
              break;
          }
      case ActionSchedule:
          {
              LCD_DbgLog("cmd:Schedule:");
              cmd_schedule_t cmd2 = *(cmd_schedule_t*)data_ptr;
              data_ptr += sizeof(cmd2);
              LCD_DbgLog("addr=%lx;ms=%lu\n", cmd2.addr, cmd2.ms_delay);

              addr_t fnaddr = cmd2.addr | 0x1; // to correctly set ESPR.T bit on call (because ESPR.T == address<0>)
              void(*fnptr)() = ( void(*)() )fnaddr;
              int32_t res = HT_ScheduleFun(fnptr, cmd2.ms_delay);

              CDC_Transmit_HS((uint8_t*)&res, sizeof(res));
              break;
          }
      case ActionCall2:
          {
              LCD_DbgLog("cmd:Call2:");
              cmd_call2_t cmd2 = *(cmd_call2_t*)data_ptr;
              data_ptr += sizeof(cmd2);
              LCD_DbgLog("addr=%lx;x1=%lu;x2=%lu\n", cmd2.addr, cmd2.x1, cmd2.x2);

              int32_t res = -1;
              if (true) { // we can access not only 'our' mem, but special addresses too
              // if (HandleBufAccess(cmd2.addr, 0)) {
                  addr_t fnaddr = cmd2.addr | 0x1; // to correctly set ESPR.T bit on call (because ESPR.T == address<0>)
                  addr_t(*fnptr)(addr_t, addr_t) = ( addr_t(*)(addr_t, addr_t) )fnaddr;
                  addr_t res0 = fnptr(cmd2.x1, cmd2.x2);
                  res = (int32_t)res0;
                  LCD_DbgLog("cmd:Call2:result=%lu\n", res);
              }
              CDC_Transmit_HS((uint8_t*)&res, sizeof(res));
              break;
          }
      default:
          LCD_DbgLog("cmd:unknown!\n");
          CDC_Transmit_HS((uint8_t*)default_msg, strlen(default_msg));
          break;
  }
}


static void LCD_Config(void)
{
  /* LCD Initialization */
  BSP_LCD_Init();

  /* LCD Layers Initialization */
  BSP_LCD_LayerDefaultInit(LCD_FOREGROUND_LAYER, (LCD_FRAME_BUFFER + BUFFER_OFFSET));

  /* Configure the transparency for foreground : Increase the transparency */
  BSP_LCD_SetTransparency(LCD_BACKGROUND_LAYER, 0);
  BSP_LCD_SelectLayer(LCD_FOREGROUND_LAYER);

  /* LCD Log initialization */
  LCD_LOG_Init();

  LCD_LOG_SetHeader((uint8_t *)"LTDC Application");
  LCD_UsrLog("USB library started.\n");
  // LCD_LOG_SetFooter ((uint8_t *)"     USB Host Library V3.2.0" );
}


void HT_Runtime_Init(void)
{
  /* Initialize LCD driver */
  LCD_Config();

#ifdef USE_SDRAM
  /* Initialize SDRAM */
  BSP_SDRAM_Init();
#endif
}

