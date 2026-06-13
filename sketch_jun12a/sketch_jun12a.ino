// =====================================================
// ESP32-S3 + 128x160 ST7735 — 纯 GPIO 驱动
// =====================================================

#define PIN_CS   13
#define PIN_DC    9
#define PIN_RST  10
#define PIN_MOSI 11
#define PIN_SCLK 12

#define WIDTH  128
#define HEIGHT 160

#include "emoji5.h"

const uint16_t C_BLACK   = 0x0000;
const uint16_t C_WHITE   = 0xFFFF;
const uint16_t C_RED     = 0xF800;
const uint16_t C_GREEN   = 0x07E0;
const uint16_t C_BLUE    = 0x001F;
const uint16_t C_CYAN    = 0x07FF;
const uint16_t C_YELLOW  = 0xFFE0;
const uint16_t C_MAGENTA = 0xF81F;

inline void csL()  { digitalWrite(PIN_CS, LOW); }
inline void csH()  { digitalWrite(PIN_CS, HIGH); }
inline void dcL()  { digitalWrite(PIN_DC, LOW); }
inline void dcH()  { digitalWrite(PIN_DC, HIGH); }
inline void sckL() { digitalWrite(PIN_SCLK, LOW); }
inline void sckH() { digitalWrite(PIN_SCLK, HIGH); }
inline void mosiL(){ digitalWrite(PIN_MOSI, LOW); }
inline void mosiH(){ digitalWrite(PIN_MOSI, HIGH); }

void spiWrite(uint8_t b) {
    (b&0x80)?mosiH():mosiL(); delayMicroseconds(1); sckH(); delayMicroseconds(1); sckL();
    (b&0x40)?mosiH():mosiL(); delayMicroseconds(1); sckH(); delayMicroseconds(1); sckL();
    (b&0x20)?mosiH():mosiL(); delayMicroseconds(1); sckH(); delayMicroseconds(1); sckL();
    (b&0x10)?mosiH():mosiL(); delayMicroseconds(1); sckH(); delayMicroseconds(1); sckL();
    (b&0x08)?mosiH():mosiL(); delayMicroseconds(1); sckH(); delayMicroseconds(1); sckL();
    (b&0x04)?mosiH():mosiL(); delayMicroseconds(1); sckH(); delayMicroseconds(1); sckL();
    (b&0x02)?mosiH():mosiL(); delayMicroseconds(1); sckH(); delayMicroseconds(1); sckL();
    (b&0x01)?mosiH():mosiL(); delayMicroseconds(1); sckH(); delayMicroseconds(1); sckL();
}

// 快速版本：digitalWrite 无延时，比寄存器直写略慢但更可靠
void spiWriteFast(uint8_t b) {
    (b&0x80)?mosiH():mosiL(); sckH(); sckL();
    (b&0x40)?mosiH():mosiL(); sckH(); sckL();
    (b&0x20)?mosiH():mosiL(); sckH(); sckL();
    (b&0x10)?mosiH():mosiL(); sckH(); sckL();
    (b&0x08)?mosiH():mosiL(); sckH(); sckL();
    (b&0x04)?mosiH():mosiL(); sckH(); sckL();
    (b&0x02)?mosiH():mosiL(); sckH(); sckL();
    (b&0x01)?mosiH():mosiL(); sckH(); sckL();
}

// 极速版本：GPIO 寄存器直写，用于图像数据连续输出
#define MOSI_MASK (1 << PIN_MOSI)  // 0x0800
#define SCLK_MASK (1 << PIN_SCLK)  // 0x1000
#define GPIO_W1TS (*(volatile uint32_t*)0x60004008)
#define GPIO_W1TC (*(volatile uint32_t*)0x6000400C)

#define NOP asm volatile("nop")

void spiWriteUltra(uint8_t b) {
    if(b&0x80)GPIO_W1TS=MOSI_MASK;else GPIO_W1TC=MOSI_MASK;NOP;NOP;NOP;GPIO_W1TS=SCLK_MASK;NOP;NOP;GPIO_W1TC=SCLK_MASK;
    if(b&0x40)GPIO_W1TS=MOSI_MASK;else GPIO_W1TC=MOSI_MASK;NOP;NOP;NOP;GPIO_W1TS=SCLK_MASK;NOP;NOP;GPIO_W1TC=SCLK_MASK;
    if(b&0x20)GPIO_W1TS=MOSI_MASK;else GPIO_W1TC=MOSI_MASK;NOP;NOP;NOP;GPIO_W1TS=SCLK_MASK;NOP;NOP;GPIO_W1TC=SCLK_MASK;
    if(b&0x10)GPIO_W1TS=MOSI_MASK;else GPIO_W1TC=MOSI_MASK;NOP;NOP;NOP;GPIO_W1TS=SCLK_MASK;NOP;NOP;GPIO_W1TC=SCLK_MASK;
    if(b&0x08)GPIO_W1TS=MOSI_MASK;else GPIO_W1TC=MOSI_MASK;NOP;NOP;NOP;GPIO_W1TS=SCLK_MASK;NOP;NOP;GPIO_W1TC=SCLK_MASK;
    if(b&0x04)GPIO_W1TS=MOSI_MASK;else GPIO_W1TC=MOSI_MASK;NOP;NOP;NOP;GPIO_W1TS=SCLK_MASK;NOP;NOP;GPIO_W1TC=SCLK_MASK;
    if(b&0x02)GPIO_W1TS=MOSI_MASK;else GPIO_W1TC=MOSI_MASK;NOP;NOP;NOP;GPIO_W1TS=SCLK_MASK;NOP;NOP;GPIO_W1TC=SCLK_MASK;
    if(b&0x01)GPIO_W1TS=MOSI_MASK;else GPIO_W1TC=MOSI_MASK;NOP;NOP;NOP;GPIO_W1TS=SCLK_MASK;NOP;NOP;GPIO_W1TC=SCLK_MASK;
}

void cmd(uint8_t c) { dcL(); csL(); spiWrite(c); csH(); }
void dat(uint8_t d) { dcH(); csL(); spiWrite(d); csH(); }
void dat16(uint16_t d) { dcH(); csL(); spiWrite(d>>8);spiWrite(d&0xFF); csH(); }

int16_t XOFF = 2;  // GRAM 列起始偏移 (GREENTAB)
int16_t YOFF = 1;  // GRAM 行起始偏移 (GREENTAB)

void setWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    cmd(0x2A); dat16(x0+XOFF); dat16(x1+XOFF);
    cmd(0x2B); dat16(y0+YOFF); dat16(y1+YOFF);
    cmd(0x2C);
}

void fill(uint16_t c) {
    cmd(0x3A);dat(0x05);   // 强制 16-bit 模式
    setWindow(0,0,WIDTH-1,HEIGHT-1);
    dcH();csL();
    for(uint32_t i=0;i<(uint32_t)WIDTH*HEIGHT;i++){spiWrite(c>>8);spiWrite(c&0xFF);}
    csH();
}

void fillRect(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint16_t c){
    if(!w||!h)return;
    setWindow(x,y,x+w-1,y+h-1);
    dcH();csL();
    uint32_t n=(uint32_t)w*h;
    for(uint32_t i=0;i<n;i++){spiWrite(c>>8);spiWrite(c&0xFF);}
    csH();
}

void drawImage(const uint16_t* data,uint16_t count){
    cmd(0x2C);  // RAMWR only — CASET/RASET 已在 setup 中预设
    dcH();csL();
    for(uint32_t i=0;i<count;i++){
        uint16_t c=data[i];
        spiWriteUltra(c>>8);spiWriteUltra(c&0xFF);
    }
    csH();
}

void drawRect(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint16_t c){
    setWindow(x,y,x+w-1,y);dcH();csL();
    for(uint8_t i=0;i<w;i++){spiWrite(c>>8);spiWrite(c&0xFF);}csH();
    setWindow(x,y+h-1,x+w-1,y+h-1);dcH();csL();
    for(uint8_t i=0;i<w;i++){spiWrite(c>>8);spiWrite(c&0xFF);}csH();
    setWindow(x,y,x,y+h-1);dcH();csL();
    for(uint8_t i=0;i<h;i++){spiWrite(c>>8);spiWrite(c&0xFF);}csH();
    setWindow(x+w-1,y,x+w-1,y+h-1);dcH();csL();
    for(uint8_t i=0;i<h;i++){spiWrite(c>>8);spiWrite(c&0xFF);}csH();
}

void hline(uint8_t x,uint8_t y,uint8_t w,uint16_t c){
    setWindow(x,y,x+w-1,y);dcH();csL();
    for(uint8_t i=0;i<w;i++){spiWrite(c>>8);spiWrite(c&0xFF);}csH();
}

void drawPixel(uint8_t x,uint8_t y,uint16_t c){
    setWindow(x,y,x,y);dat16(c);
}

static const uint8_t FONT[64][6] PROGMEM = {
    {0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00,0x00},
    {0x00,0x07,0x00,0x07,0x00,0x00},{0x14,0x7F,0x14,0x7F,0x14,0x00},
    {0x24,0x2A,0x7F,0x2A,0x12,0x00},{0x23,0x13,0x08,0x64,0x62,0x00},
    {0x36,0x49,0x55,0x22,0x50,0x00},{0x00,0x05,0x03,0x00,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00,0x00},{0x00,0x41,0x22,0x1C,0x00,0x00},
    {0x08,0x2A,0x1C,0x2A,0x08,0x00},{0x08,0x08,0x3E,0x08,0x08,0x00},
    {0x00,0x50,0x30,0x00,0x00,0x00},{0x08,0x08,0x08,0x08,0x08,0x00},
    {0x00,0x60,0x60,0x00,0x00,0x00},{0x20,0x10,0x08,0x04,0x02,0x00},
    {0x3E,0x51,0x49,0x45,0x3E,0x00},{0x00,0x42,0x7F,0x40,0x00,0x00},
    {0x42,0x61,0x51,0x49,0x46,0x00},{0x21,0x41,0x45,0x4B,0x31,0x00},
    {0x18,0x14,0x12,0x7F,0x10,0x00},{0x27,0x45,0x45,0x45,0x39,0x00},
    {0x3C,0x4A,0x49,0x49,0x30,0x00},{0x01,0x71,0x09,0x05,0x03,0x00},
    {0x36,0x49,0x49,0x49,0x36,0x00},{0x06,0x49,0x49,0x29,0x1E,0x00},
    {0x00,0x36,0x36,0x00,0x00,0x00},{0x00,0x56,0x36,0x00,0x00,0x00},
    {0x00,0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14,0x00},
    {0x41,0x22,0x14,0x08,0x00,0x00},{0x02,0x01,0x51,0x09,0x06,0x00},
    {0x32,0x49,0x79,0x41,0x3E,0x00},{0x7E,0x11,0x11,0x11,0x7E,0x00},
    {0x7F,0x49,0x49,0x49,0x36,0x00},{0x3E,0x41,0x41,0x41,0x22,0x00},
    {0x7F,0x41,0x41,0x22,0x1C,0x00},{0x7F,0x49,0x49,0x49,0x41,0x00},
    {0x7F,0x09,0x09,0x01,0x01,0x00},{0x3E,0x41,0x41,0x51,0x32,0x00},
    {0x7F,0x08,0x08,0x08,0x7F,0x00},{0x00,0x41,0x7F,0x41,0x00,0x00},
    {0x20,0x40,0x41,0x3F,0x01,0x00},{0x7F,0x08,0x14,0x22,0x41,0x00},
    {0x7F,0x40,0x40,0x40,0x40,0x00},{0x7F,0x02,0x04,0x02,0x7F,0x00},
    {0x7F,0x04,0x08,0x10,0x7F,0x00},{0x3E,0x41,0x41,0x41,0x3E,0x00},
    {0x7F,0x09,0x09,0x09,0x06,0x00},{0x3E,0x41,0x51,0x21,0x5E,0x00},
    {0x7F,0x09,0x19,0x29,0x46,0x00},{0x46,0x49,0x49,0x49,0x31,0x00},
    {0x01,0x01,0x7F,0x01,0x01,0x00},{0x3F,0x40,0x40,0x40,0x3F,0x00},
    {0x1F,0x20,0x40,0x20,0x1F,0x00},{0x7F,0x20,0x18,0x20,0x7F,0x00},
    {0x63,0x14,0x08,0x14,0x63,0x00},{0x03,0x04,0x78,0x04,0x03,0x00},
    {0x61,0x51,0x49,0x45,0x43,0x00},
};

void drawChar(uint8_t x,uint8_t y,char ch,uint16_t fg,uint8_t sz){
    if(ch>='a'&&ch<='z')ch-=32;
    if(ch<32||ch>95)ch=32;
    uint8_t gi=ch-32;
    setWindow(x,y,x+6*sz-1,y+8*sz-1);
    dcH();csL();
    // MX=1→列地址递减，col 0→5 从左到右输出像素
    for(int8_t row=7;row>=0;row--){
        for(uint8_t rs=0;rs<sz;rs++){
            for(uint8_t col=0;col<6;col++){
                uint8_t colData=pgm_read_byte(&FONT[gi][col]);
                uint16_t c=(colData&(1<<(7-row)))?fg:C_BLACK;
                for(uint8_t ps=0;ps<sz;ps++){spiWrite(c>>8);spiWrite(c&0xFF);}
            }
        }
    }
    csH();
}

void drawText(uint8_t x,uint8_t y,const char*s,uint16_t fg,uint8_t sz){
    while(*s){drawChar(x,y,*s,fg,sz);x+=6*sz;if(x+6*sz>WIDTH){x=0;y+=8*sz;}s++;}
}

void tftInit(int mode) {
    pinMode(PIN_CS,OUTPUT);pinMode(PIN_DC,OUTPUT);pinMode(PIN_RST,OUTPUT);
    pinMode(PIN_MOSI,OUTPUT);pinMode(PIN_SCLK,OUTPUT);
    csH();sckL();

    digitalWrite(PIN_RST,HIGH);delay(10);
    digitalWrite(PIN_RST,LOW);delay(20);
    digitalWrite(PIN_RST,HIGH);delay(150);

    // === Adafruit Rcmd1 (ST7735R init part 1) ===
    cmd(0x01);delay(150);           // SWRESET
    cmd(0x11);delay(255);           // SLPOUT (500ms)
    cmd(0xB1);dat(0x01);dat(0x2C);dat(0x2D);  // FRMCTR1
    cmd(0xB2);dat(0x01);dat(0x2C);dat(0x2D);  // FRMCTR2
    cmd(0xB3);dat(0x01);dat(0x2C);dat(0x2D);dat(0x01);dat(0x2C);dat(0x2D); // FRMCTR3
    cmd(0xB4);dat(0x07);            // INVCTR
    cmd(0xC0);dat(0xA2);dat(0x02);dat(0x84);  // PWCTR1
    cmd(0xC1);dat(0xC5);            // PWCTR2
    cmd(0xC2);dat(0x0A);dat(0x00);  // PWCTR3
    cmd(0xC3);dat(0x8A);dat(0x2A);  // PWCTR4
    cmd(0xC4);dat(0x8A);dat(0xEE);  // PWCTR5
    cmd(0xC5);dat(0x0E);            // VMCTR1
    cmd(0x20);                      // INVOFF (not INVON!)

    uint8_t madctl;
    switch(mode) {
        case 0: madctl=0xC8; break;  // BLACKTAB: MY+MX+BGR
        case 1: madctl=0xC0; break;  // REDTAB:   MY+MX
        case 2: madctl=0x68; break;
        case 3: madctl=0xC0; break;  // MY+MX, RGB — MX=1 列递减修复字符镜像
        case 4: madctl=0x08; break;
        case 5: madctl=0x00; break;
        case 6: madctl=0xE0; break;
        case 7: madctl=0xE8; break;
        default: madctl=0xC8; break;
    }
    cmd(0x36);dat(madctl);          // MADCTL
    cmd(0x3A);dat(0x05);            // COLMOD = 16-bit

    // === Adafruit Rcmd2 (column/row offsets) ===
    // CASET/RASET 只接受2个16-bit参数！用独立 dat16 调用
    cmd(0x2A); dat16(2); dat16(129);   // CASET: XS=2, XE=129
    cmd(0x2B); dat16(1); dat16(160);   // RASET: YS=1, YE=160

    // === Adafruit Rcmd3 (gamma + display on) ===
    cmd(0xE0);  // GMCTRP1 (positive gamma)
    dat(0x02);dat(0x1C);dat(0x07);dat(0x12);
    dat(0x37);dat(0x32);dat(0x29);dat(0x2D);
    dat(0x29);dat(0x25);dat(0x2B);dat(0x39);
    dat(0x00);dat(0x01);dat(0x03);dat(0x10);
    cmd(0xE1);  // GMCTRN1 (negative gamma)
    dat(0x03);dat(0x1D);dat(0x07);dat(0x06);
    dat(0x2E);dat(0x2C);dat(0x29);dat(0x2D);
    dat(0x2E);dat(0x2E);dat(0x37);dat(0x3F);
    dat(0x00);dat(0x00);dat(0x02);dat(0x10);

    cmd(0x13);delay(10);            // NORON
    cmd(0x29);delay(100);           // DISPON
}

void setup() {
    Serial.begin(115200);USBSerial.begin(115200);delay(2000);
    Serial.println("\n=== GIF ANIMATION ===");
    USBSerial.println("\n=== GIF ANIMATION ===");

    tftInit(3);  // MADCTL=0xC0 (MY+MX, RGB order)
    fill(C_BLACK);

    // 预设 CASET/RASET 为 GIF 120x120 居中区域，后续只发 RAMWR 不调 setWindow
    uint8_t gx = (128 - GIF_W) / 2;
    uint8_t gy = (160 - GIF_H) / 2;
    cmd(0x2A); dat16(gx+XOFF); dat16(gx+GIF_W-1+XOFF);
    cmd(0x2B); dat16(gy+YOFF); dat16(gy+GIF_H-1+YOFF);
}

void loop(){
    for(uint8_t f=0; f<GIF_FRAMES; f++){
        unsigned long t = millis();
        drawImage(GIF_DATA[f], (uint32_t)GIF_W*GIF_H);
        while(millis() - t < GIF_DELAY[f]);
    }
}
