#include <string.h>

#include "emuapi.h"

#include "t4_dsp.h"
#include "iopins.h" 

extern "C" {
#include "dtypes.h"
#include "options.h"
#include "autoconf.h"
#include "memory.h"
#include "custom.h"
#include "disk.h"
#include "xwin.h"
#include "gensound.h"
#include "events.h"
#include "sounddep/sound.h"
#include "keyboard.h"
void record_key (int);
void keybuf_init (void);
extern void serial_init(void);
extern void init_m68k(void);
extern void m68k_reset(void);
extern void m68k_go(int);
}

static bool hires = true;
int version = 100*UAEMAJOR + 10*UAEMINOR + UAEURSAMINOR;
struct uae_prefs changed_prefs;
struct uae_prefs currprefs = {
/* framerate */ 1,
/* illegal_mem */ 0,
/* no_xhair */ 0,
/* use_serial */ 0,
/* automount_uaedev */ 1,

/* fake_joystick */ 2 + (0<<8),
/* KbdLang keyboard_lang */ KBD_LANG_US,
/* allow_save */ 0,
/* emul_accuracy */ 2,
/* test_drawing_speed */ 0,

/* produce_sound */ 0,
/* sound_bits */ DEFAULT_SOUND_BITS,
/* sound_freq */ DEFAULT_SOUND_FREQ,
/* sound_minbsiz */ DEFAULT_SOUND_MINB,
/* sound_maxbsiz */ DEFAULT_SOUND_MAXB,

/* gfx_width */ 0,  // filled in init
/* gfx_height */ 0,
/* gfx_lores */ 0,

/* gfx_linedbl */ 0,
/* gfx_correct_aspect */ 0,

/* gfx_xcenter */ 0, // filled in init
/* gfx_ycenter */ 0,

/* color_mode */ 0,
    
/* immediate_blits */ 0,
/* blits_32bit_enabled */ 0,
    
/* floppies */ { "df0.adf", "df1.adf", "df2.adf", "df3.adf" },
/* hardfile */ "hardfile", 
};

char df0[MAX_FILENAME]="df0.adf";
char df1[MAX_FILENAME]="df1.adf", df2[MAX_FILENAME]="df2.adf", df3[MAX_FILENAME]="df3.adf";

int quit_program = 0;
int buttonstate[3];
int lastmx, lastmy;
int newmousecounters;
xcolnr xcolors[4096];
struct vidbuf_description gfxvidinfo;
char romfile[MAX_FILENAME] = "./kick13.rom";
char prtname[MAX_FILENAME] = "lpr ";
char sername[MAX_FILENAME] = "";
char warning_buffer[256];
static char slinebuf[TFT_WIDTH*2*2]; //16 bits

#define MOUSE_STEP 3
static int prev_hk = 0;
static int hk = 0;
static int prev_key = 0;
static int k = 0;
static bool isMouse = true;

#define INV_KEY -1

const int16_t keyboardAsciiConv[] = // QWERTY Keyboard
{
/* 0x00 */ INV_KEY,
/* 0x01 */ INV_KEY,
/* 0x02 */ INV_KEY,
/* 0x03 */ INV_KEY,
/* 0x04 */ INV_KEY,
/* 0x05 */ INV_KEY,
/* 0x06 */ INV_KEY,
/* 0x07 */ INV_KEY,
/* 0x08 */ INV_KEY,
/* 0x09 */ AK_TAB,
/* 0x0A */ AK_ENT,
/* 0x0B */ INV_KEY,
/* 0x0C */ INV_KEY,
/* 0x0D */ INV_KEY,
/* 0x0E */ INV_KEY,
/* 0x0F */ INV_KEY,
/* 0x10 */ INV_KEY,
/* 0x11 */ INV_KEY,
/* 0x12 */ INV_KEY,
/* 0x13 */ INV_KEY,
/* 0x14 */ INV_KEY,
/* 0x15 */ INV_KEY,
/* 0x16 */ INV_KEY,
/* 0x17 */ INV_KEY,
/* 0x18 */ INV_KEY,
/* 0x19 */ INV_KEY,
/* 0x1A */ INV_KEY,
/* 0x1B */ AK_ESC,
/* 0x1C */ INV_KEY,
/* 0x1D */ INV_KEY,
/* 0x1E */ INV_KEY,
/* 0x1F */ INV_KEY,
/* 0x20 */ AK_SPC,
/* 0x21 */ INV_KEY, // exclamation mark
/* 0x22 */ INV_KEY, // double quote
/* 0x23 */ AK_QUOTE, // diese
/* 0x24 */ INV_KEY, // dollar
/* 0x25 */ INV_KEY, // procent
/* 0x26 */ AK_NUMBERSIGN, // and
/* 0x27 */ AK_QUOTE,
/* 0x28 */ AK_NPLPAREN,
/* 0x29 */ AK_NPRPAREN,
/* 0x2A */ AK_NPMUL,
/* 0x2B */ AK_NPADD,
/* 0x2C */ AK_COMMA,
/* 0x2D */ AK_NPSUB,
/* 0x2E */ AK_PERIOD,
/* 0x2F */ AK_NPDIV,
/* 0x30 */ AK_0,
/* 0x31 */ AK_1,
/* 0x32 */ AK_2,
/* 0x33 */ AK_3,
/* 0x34 */ AK_4,
/* 0x35 */ AK_5,
/* 0x36 */ AK_6,
/* 0x37 */ AK_7,
/* 0x38 */ AK_8,
/* 0x39 */ AK_9,
/* 0x3A */ INV_KEY, // colon :
/* 0x3B */ AK_SEMICOLON, // semi colon
/* 0x3C */ AK_LTGT, // <
/* 0x3D */ AK_EQUAL, // =
/* 0x3E */ INV_KEY, // >
/* 0x3F */ INV_KEY, // ?
/* 0x40 */ INV_KEY, // Arobas
/* 0x41 */ INV_KEY, // capital A
/* 0x42 */ INV_KEY,
/* 0x43 */ INV_KEY,
/* 0x44 */ INV_KEY,
/* 0x45 */ INV_KEY,
/* 0x46 */ INV_KEY,
/* 0x47 */ INV_KEY,
/* 0x48 */ INV_KEY,
/* 0x49 */ INV_KEY,
/* 0x4A */ INV_KEY,
/* 0x4B */ INV_KEY,
/* 0x4C */ INV_KEY,
/* 0x4D */ INV_KEY,
/* 0x4E */ INV_KEY,
/* 0x4F */ INV_KEY,
/* 0x50 */ INV_KEY,
/* 0x51 */ INV_KEY,
/* 0x52 */ INV_KEY,
/* 0x53 */ INV_KEY,
/* 0x54 */ INV_KEY,
/* 0x55 */ INV_KEY,
/* 0x56 */ INV_KEY,
/* 0x57 */ INV_KEY,
/* 0x58 */ INV_KEY,
/* 0x59 */ INV_KEY,
/* 0x5A */ INV_KEY,
/* 0x5B */ AK_LBRACKET, // open crochet
/* 0x5C */ AK_BACKSLASH, // back slash
/* 0x5D */ AK_RBRACKET, // close crochet
/* 0x5E */ INV_KEY, // circonflex
/* 0x5F */ INV_KEY, // underscore
/* 0x60 */ AK_BACKQUOTE,
/* 0x61 */ AK_A,
/* 0x62 */ AK_B,
/* 0x63 */ AK_C,
/* 0x64 */ AK_D,
/* 0x65 */ AK_E,
/* 0x66 */ AK_F,
/* 0x67 */ AK_G,
/* 0x68 */ AK_H,
/* 0x69 */ AK_I,
/* 0x6A */ AK_J,
/* 0x6B */ AK_K,
/* 0x6C */ AK_L,
/* 0x6D */ AK_M,
/* 0x6E */ AK_N,
/* 0x6F */ AK_O,
/* 0x70 */ AK_P,
/* 0x71 */ AK_Q,
/* 0x72 */ AK_R,
/* 0x73 */ AK_S,
/* 0x74 */ AK_T,
/* 0x75 */ AK_U,
/* 0x76 */ AK_V,
/* 0x77 */ AK_W,
/* 0x78 */ AK_X,
/* 0x79 */ AK_Y,
/* 0x7A */ AK_Z,
/* 0x7B */ INV_KEY,
/* 0x7C */ INV_KEY,
/* 0x7D */ INV_KEY,
/* 0x7E */ INV_KEY,
/* 0x7F */ AK_BS
};


const int16_t keyboardSpecialConv[] = // Functions and other keys
{
/* 0xC0 */ INV_KEY,
/* 0xC1 */ INV_KEY,
/* 0xC2 */ AK_F1,
/* 0xC3 */ AK_F2,
/* 0xC4 */ AK_F3,
/* 0xC5 */ AK_F4,
/* 0xC6 */ AK_F5,
/* 0xC7 */ AK_F6,
/* 0xC8 */ AK_F7,
/* 0xC9 */ AK_F8,
/* 0xCA */ AK_F9,
/* 0xCB */ AK_F10,
/* 0xCC */ INV_KEY,
/* 0xCD */ INV_KEY,
/* 0xCE */ INV_KEY,
/* 0xCF */ INV_KEY,
/* 0xD0 */ INV_KEY,
/* 0xD1 */ INV_KEY,
/* 0xD2 */ INV_KEY,
/* 0xD3 */ INV_KEY,
/* 0xD4 */ AK_DEL,
/* 0xD5 */ AK_UP,
/* 0xD6 */ AK_DN,
/* 0xD7 */ AK_LF,
/* 0xD8 */ AK_RT,
/* 0xD9 */ INV_KEY,
/* 0xDA */ INV_KEY,
/* 0xDB */ INV_KEY,
/* 0xDC */ INV_KEY,
/* 0xDD */ INV_KEY,
/* 0xDE */ INV_KEY,
/* 0xDF */ INV_KEY
};



void uae_Input(int bClick) {
//  buttonstate[0] = 0;
//  buttonstate[2] = 0;
  
  hk = emu_ReadI2CKeyboard();
  k = emu_ReadKeys(); 
   
  // Toggle keymap + mouse/joystick
  if (bClick & MASK_KEY_USER2) {
    if (isMouse) isMouse = false;
    else isMouse = true;
    emu_setKeymap(0);
  }   

  // force joystick mode if mouse detected
  if (emu_MouseDetected() ) isMouse = false;
  
  int buts,dx,dy;
  int mouseEvent = emu_GetMouse(&dx,&dy,&buts);
  if (mouseEvent){
    lastmx += dx;
    if ( lastmx > 640 ) {
      lastmx = 639;
    } 
    else if ( lastmx < 0 ) {
      lastmx = 0;     
    }
    lastmy += dy;
    if ( lastmy > 480 ) {
      lastmy = 399;
    } 
    else if ( lastmy < 0 ) {
      lastmy = 0;      
    }
    if (buts & 0x1) {
      buttonstate[0] |=1; 
    }
    else {
      buttonstate[0] &=~1;       
    }
    if (buts & 0x2) {
      buttonstate[2] |=1;
    }
    else {
      buttonstate[2] &=~1;     
    }
  }

  // Diskswap in joystick mode
  if (bClick & MASK_KEY_USER1) {
    if (!isMouse) disk_swap(df0,df1); 
  }
  int iAmigaKeyCode;
  if (hk != prev_hk) {
    prev_hk = hk;
    if ( (hk != 0) && (hk != prev_key) ) {
      prev_key = hk;
      if ((hk >=0xc0) && (hk <=0xdf)) {
        iAmigaKeyCode = keyboardSpecialConv[(hk-0xc0) & 0x1f];    
      }
      else {
        iAmigaKeyCode = keyboardAsciiConv[hk & 0x7f];
      }
      record_key(iAmigaKeyCode << 1);
    }     
  }
  if ( (hk == 0) && (prev_key) ) {
      if ((prev_key >=0xc0) && (prev_key <=0xdf)) {
        iAmigaKeyCode = keyboardSpecialConv[(prev_key-0xc0) & 0x1f];    
      }
      else {
        iAmigaKeyCode = keyboardAsciiConv[prev_key & 0x7f];
      }
      record_key((iAmigaKeyCode << 1) | 1);      
      prev_key = 0;
  }      

  if (isMouse)
  {
    if (( k & MASK_JOY1_RIGHT) || ( k & MASK_JOY2_RIGHT)) {
      if ( lastmx < 640 ) {
        lastmx += MOUSE_STEP;
        newmousecounters = 1; 
      } 
    }
    else if (( k & MASK_JOY1_LEFT) || ( k & MASK_JOY2_LEFT)) {
      if ( lastmx >= MOUSE_STEP ) {
        lastmx -= MOUSE_STEP;
        newmousecounters = 1;       
      }
    }
    else if (( k & MASK_JOY1_UP) || ( k & MASK_JOY2_UP)) {
      if ( lastmy >= MOUSE_STEP ) {
        lastmy -= MOUSE_STEP;
        newmousecounters = 1;       
      }
    }
    else if (( k & MASK_JOY1_DOWN) || ( k & MASK_JOY2_DOWN)) {
      if ( lastmy < 480 ) {
        lastmy += MOUSE_STEP;
        newmousecounters = 1;        
      }
    } 
    if (( k & MASK_JOY1_BTN) || ( k & MASK_JOY2_BTN)) {
      buttonstate[0] |= 1;
    }
    else  {
      buttonstate[0] &=~1;  
    }
    if (( k & MASK_KEY_USER1)) {
      buttonstate[2] |= 1;
    }
    else  {
      buttonstate[2] &=~1;  
    }             
  }
}

void handle_events(void)
{
  uae_Input(emu_DebounceLocalKeys());
} 


void emu_KeyboardOnDown(int keymodifer, int key) {
  /*
  char buff[128] = {0};
  char poof[64] = {0};
  long lk = key;
  ltoa( lk, poof, 2);
  snprintf(buff, 127, "onPress>> sz(%u) %u 0x%x '%c' %s\n", sizeof(key), key, key, key, poof);
  Serial.print(buff);
  */ 
 //Serial.print(key);

  int iAmigaKeyCode = INV_KEY;
  if ((key >=0xc0) && (key <=0xdf)) {
    iAmigaKeyCode = keyboardSpecialConv[(key-0xc0) & 0x1f];    
  }
  else {
    iAmigaKeyCode = keyboardAsciiConv[key & 0x7f];
  }
  if (iAmigaKeyCode != INV_KEY) {
    record_key(iAmigaKeyCode << 1);
  }
}

void emu_KeyboardOnUp(int keymodifer, int key) {
  /*
  char buff[128] = {0};
  snprintf(buff, 127, "onRelease>> sz(%u) %u 0x%x %c\n", sizeof(key), key, key, key);
  Serial.print(buff); 
  */ 
  int iAmigaKeyCode = INV_KEY;
  if ((key >=0xc0) && (key <=0xdf)) {
    iAmigaKeyCode = keyboardSpecialConv[(key-0xc0) & 0x1f];    
  }
  else {
    iAmigaKeyCode = keyboardAsciiConv[key & 0x7f];
  }
  if (iAmigaKeyCode != INV_KEY) {
    record_key((iAmigaKeyCode << 1) | 1);
  }
}



extern "C" {

void read_joystick(int nr, unsigned int *dir, int *button)
{
  int left = 0, right = 0, top = 0, bot = 0;
  *dir = 0;
  *button = 0;

  if (!isMouse)
  {
    if ((k & MASK_JOY2_DOWN) || ( k & MASK_JOY1_DOWN))  bot=1;
    if ((k & MASK_JOY2_UP) || ( k & MASK_JOY1_UP))   top=1;
    if ((k & MASK_JOY2_LEFT) || ( k & MASK_JOY1_LEFT))  left=1;
    if ((k & MASK_JOY2_RIGHT) || ( k & MASK_JOY1_RIGHT)) right=1;
    if ((k & MASK_JOY2_BTN) || ( k & MASK_JOY1_BTN)) *button |=0xFF;

    if (left) top = !top;
    if (right) bot = !bot;
    *dir = bot | (right << 1) | (top << 8) | (left << 9);
  }  
}
  
void uae_reset (void)
{
  if (quit_program != 1 && quit_program != -1)
    quit_program = -2;
}

void gui_led(int led, int on){}
int needmousehack(void) { return 1;}
void LED(int on) {}
void write_log (const char *buf) { /*fprintf (stderr, buf); */ }

void flush_line(int y)
{
  if(y >= 0 && y < currprefs.gfx_height) {
    if (currprefs.gfx_correct_aspect) {
      emu_DrawLine16((unsigned short *)slinebuf, currprefs.gfx_width , currprefs.gfx_height, y);        
    }      
    else {
      if (currprefs.gfx_height > 256) {
        int nexty = ((y+1) * currprefs.gfx_height)/256;
        y = (y * currprefs.gfx_height)/256;
        if (y<gfxvidinfo.height) emu_DrawLine16((unsigned short *)slinebuf, currprefs.gfx_width , currprefs.gfx_height, y);
        if ( (nexty != y) && ((y+1)<currprefs.gfx_height))  emu_DrawLine16((unsigned short *)slinebuf, currprefs.gfx_width , currprefs.gfx_height, y+1); //emu_CopyLine(currprefs.gfx_width, gfxvidinfo.height, y, y+1);               
      } 
      else {
        emu_DrawLine16((unsigned short *)slinebuf, currprefs.gfx_width , currprefs.gfx_height, y);
      }                 
    }      
  }  
}

void flush_block(int ystart,int ystop){}


uae_u16 sndbuffer[sndbufsize];
uae_u32 sndbufpt;
static uae_u32 psndbufpt=0;

static uae_u32 *sndbuffer32=(uae_u32 *)sndbuffer;
static uae_u32 sndbufrdpt=(sndbufsize/2)<<8;
static uae_u32 psndbufrdpt=(sndbufsize/2)<<8;
static uae_u32 sndbufrdcnt=0;

static uae_u32 sndinc=0x100; // default read increment
#define sndbufrdmask ((sndbufsize/2-1)<<8)+0xff


#define AVG_COUNT 16
static int avgcounter = AVG_COUNT;
static uae_u32 avgr=0;
static uae_u32 avgw=0;
static uae_u32 inc = (0x378<<8)/(0x2E0); //(wdelta<<8)/(0x2E0);

// distance between read and write buffer
static long incdelta=0;
static long pdelta=0x1000;
static bool pdown=true; 


void flush_screen(int ystart,int ystop)
{    
  emu_DrawVsync();
//  emu_tweakVideo(1,0,0);
  
  // #sample written per frame
  int wdelta = 0;
  uae_u32  wdpt = sndbufpt;
  if (wdpt > psndbufpt) { 
    wdelta = wdpt-psndbufpt;
  }
  else if (wdpt < psndbufpt) { 
    wdelta = wdpt + sndbufsize - psndbufpt;
  }
  psndbufpt = wdpt;
 
  // #sample read per frame
  int rdelta = sndbufrdcnt;
  sndbufrdcnt = 0;
  /*
  int rdelta = 0;
  uae_u32  rdpt = sndbufrdpt>>8;
  if (rdpt > psndbufrdpt) { 
    rdelta = rdpt-psndbufrdpt;
  }
  else if (rdpt < psndbufrdpt) { 
    rdelta = rdpt + sndbufsize/2 - psndbufrdpt;
  }
  psndbufrdpt = rdpt;
  */
  
  inc = (wdelta<<8)/(0x2E0);
 
  // Compute average R/W over AVG_COUNT frame
  avgcounter--;
  avgw += wdelta;
  avgr += rdelta;
  if (avgcounter == 0) {
    wdelta = avgw/AVG_COUNT;
    rdelta = avgr/AVG_COUNT;
    avgw = 0;
    avgr = 0;
    avgcounter = AVG_COUNT;

    //emu_printi(wdelta);
    //emu_printi(rdelta);
    //inc = (wdelta<<8)/(rdelta); 
     
    uae_u32 xwrpt = sndbufpt;
    uae_u32 xrdpt = (sndbufrdpt>>8)*2;
    uae_u32 delta = 0;
    if (xrdpt > xwrpt) { 
      delta = xrdpt-xwrpt;
    }
    else if (xrdpt < xwrpt) { 
      delta = xrdpt + sndbufsize - xwrpt;
    }

    // we try to be keep read and write buffer at half distance of each other
    bool down;
    if (delta < pdelta)  {
      down = true;
      if (delta < (sndbufsize/2-0x100)) {
        if ( (down) && (pdown) )
          incdelta += 2; 
        else
          incdelta = 1;  
      }
    }
    else if (delta > pdelta) {
      down = false;
      if (delta > (sndbufsize/2+0x100)) { 
        if ( (!down) && (!pdown) )
          incdelta -= 2; 
        else
          incdelta = -1;  
      }
    }
    // Hard reset sound buffer?
    if ( (delta < sndbufsize/4) || (delta > (sndbufsize-sndbufsize/4)) ) {
      memset(sndbuffer,sizeof(sndbuffer),0);
      sndbufpt = 0;
      sndbufrdpt=(sndbufsize/2)<<8;
      psndbufrdpt=(sndbufsize/2)<<8;
      delta = sndbufsize/2;
    }
    pdelta = delta;
    pdown = down; 
    //emu_printi(delta);     
  }

  sndinc=inc+incdelta;

  yield();    
}

}





void SND_Process(void *stream, int len) {  
    short * data = (short*)stream;
    len = len >> 1;
    for (int i=0;i<len;i++)
    {      
      uae_u32 s = sndbuffer32[sndbufrdpt>>8];  
      *data++ = (s >> 16);
      *data++ = (s & 0xffff);
      sndbufrdcnt += 2;
      sndbufrdpt += sndinc;
      sndbufrdpt &= sndbufrdmask;
    }    
} 


int init_sound (void)
{
  currprefs.sound_maxbsiz = sndbufsize;
  sample_evtime = (long)maxhpos * maxvpos * 50 / currprefs.sound_freq;
  init_sound_table16();
  eventtab[ev_sample].handler = sample16_handler;
  memset(sndbuffer,sizeof(sndbuffer),0);
  sndbufpt = 0;
  sndbufrdpt=(sndbufsize/2)<<8;
  psndbufrdpt=(sndbufsize/2)<<8;
  sound_available = 1;  
  return 1;
}


static int hddfound=0;

void uae_Init(void)
{
  hires = emu_IsVgaHires()?true:false;  

  currprefs.gfx_width = hires?TFT_WIDTH*2:TFT_WIDTH;
  currprefs.gfx_height = hires?TFT_HEIGHT*2:TFT_HEIGHT;
  
  currprefs.gfx_lores = hires?0:1;
  currprefs.gfx_xcenter = 40;
  currprefs.gfx_ycenter = 40;
  
  emu_printf("Init");

  gfxvidinfo.rowbytes   = currprefs.gfx_width*2;
  gfxvidinfo.pixbytes   = 2;
  gfxvidinfo.maxlinetoscr = currprefs.gfx_width;
  gfxvidinfo.maxline = currprefs.gfx_height;
  gfxvidinfo.linemem = slinebuf;
  gfxvidinfo.maxblocklines  = 0;
  gfxvidinfo.width = currprefs.gfx_width;
  gfxvidinfo.height = currprefs.gfx_height;
    
  buttonstate[0] = buttonstate[1] = buttonstate[2] = 0;
  lastmx = lastmy = 0;
  newmousecounters = 0;
  int i;
  // Palette
  for (int i = 0; i < 4096; i++) {
    int r = i >> 8;
    int g = (i >> 4) & 0xF;
    int b = i & 0xF;
    xcolors[i] = RGBVAL16(r<<4,g<<4,b<<4);
  } 

  quit_program = 2;

#ifdef HAS_HARDFILE
  rtarea_init ();
  hardfile_install ();
#endif

#ifdef HAS_SND
  init_sound();
  currprefs.produce_sound = 2;
#else
  currprefs.produce_sound = 0;
#endif

  keybuf_init();
#ifdef HAS_EXPANMEM  
  expansion_init ();
#endif  
  memory_init();
#ifdef HAS_FILESYS
  filesys_install();
#endif  
  custom_init();
  serial_init();
  init_m68k();
  //customreset();
  // JMH to fix hdd
  if (!hddfound) {
    DISK_init();
    disk_swap(df0,df1);    
  }
  emu_printf("cpu reset");
  m68k_reset();
  emu_printf("cpu go");
  m68k_go(1);
}


void uae_Start(char * floppy1, char * floppy2)
{
  emu_printf("Start");
  emu_printf(floppy1);  
  emu_printf(floppy2);  
  int strl = strlen(floppy1);
  if ( (strl >= 4) ) {
    emu_printf(&floppy1[strl-4]);    
    if ( (!strcmp(&floppy1[strl-4], ".hdf")) || (!strcmp(&floppy1[strl-4], ".HDF")) ) {
      emu_printf("file is hdd");
      strcpy(currprefs.hf0,floppy1);
      hddfound = 1;
    }
  }
  
  if (!hddfound) {
    strcpy(df0,floppy1);
    strcpy(df1,floppy2);  
  }
  emu_printf("Start done");
}

void uae_Step(void) {
  // not reached!  
}
