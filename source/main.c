typedef unsigned short u16;
typedef volatile unsigned int vu32;
typedef volatile unsigned short vu16;
typedef volatile unsigned char vu8;
typedef volatile signed short vs16;

#define SCANLINECOUNTER *(vu16 *)0x4000006

#define VIDEO_BUFFER_MAIN ((u16 *)0x06000000)
#define VIDEO_BUFFER_SUB  ((u16 *)0x06200000)

#define REG_DISPCNT_MAIN  (*(vu32*)0x04000000)
#define REG_DISPCNT_SUB   (*(vu32*)0x04001000)

#define VRAM_A_CR         (*(vu8*)0x04000240)
#define VRAM_B_CR         (*(vu8*)0x04000241)
#define VRAM_C_CR         (*(vu8*)0x04000242)
#define VRAM_D_CR         (*(vu8*)0x04000243)

#define VRAM_OFFSET(n)    ((n)<<3)
#define VRAM_ENABLE       (1<<7)

//TODO why not just 3?
#define MODE3 0x10003
#define MODE5 0x10005
#define BG3_ENABLE (1<<11)

#define REG_BG3CNT       (*(vu16*)0x400000E)
#define REG_BG3CNT_SUB   (*(vu16*)0x400100E)
#define REG_BG3PA        (*(vs16*)0x4000030)
#define REG_BG3PC        (*(vs16*)0x4000034)
#define BG_256x256       (1<<14)
#define BG_15BITCOLOR    (1<<7)
#define BG_CBB1          (1<<2)

#define COLOR(r,g,b) ((r) | (g)<<5 | (b)<<10)
#define PIXEL_ENABLE (1<<15)

#define OFFSET(r,c,w) ((r)*(w)+(c))

#define SCREENWIDTH  (256)
#define SCREENHEIGHT (192)

#define REG_BG3PA        (*(vs16*)0x4000030)
#define REG_BG3PD        (*(vs16*)0x4000036)
#define REG_BG3PA_SUB    (*(vs16*)0x4001030)
#define REG_BG3PD_SUB    (*(vs16*)0x4001036)

void setPixel3_main(int row, int col, u16 color);
void setPixel3_sub(int row, int col, u16 color);
void drawRect3_main(int row, int col, int width, int height, u16 color);
void drawRect3_sub(int row, int col, int width, int height, u16 color);
void initialize_ball();
void update();
void draw();
void waitForVblank();

typedef struct ball {
    int row;
    int col;
    int size;
    int rdel;
    int cdel;
    u16 color;
} ball;

ball ball_main;
ball old_ball_main;

int main(void) {
    // Set both displays to mode3 and turn on bg3 for each
    REG_DISPCNT_MAIN = MODE3 | BG3_ENABLE;
    REG_DISPCNT_SUB = MODE3 | BG3_ENABLE;

    // Set bg3 on both displays large enough to fill the screen, use 15 bit
    // RGB color values, and look for bitmap data at character base block 1.
    REG_BG3CNT =  BG_256x256 | BG_15BITCOLOR | BG_CBB1;
    REG_BG3CNT_SUB = BG_256x256 | BG_15BITCOLOR | BG_CBB1;

    // Don't scale bg3 (set its affine transformation matrix to [[1,0],[0,1]])
    REG_BG3PA = 1 << 8;
    REG_BG3PD = 1 << 8;

    REG_BG3PA_SUB = 1 << 8;
    REG_BG3PD_SUB= 1 << 8;

    // Draw some stuff on the subdisplay
    drawRect3_sub(20, 100, 100, 42, COLOR(31, 0, 15));
    int i;
    for (i=0; i<256; i++) {
        setPixel3_sub(i,i, COLOR(0,15,31));
    }

    initialize_ball();

    // main loop
    while(1) {
        update();
        waitForVblank();
        draw();
    }

    return 0;
}

void initialize_ball() {
    ball_main.row = 10;
    ball_main.col= 10;
    ball_main.size = 5;
    ball_main.rdel= 1;
    ball_main.cdel = 2;
    ball_main.color = COLOR(15,27,7);
}

void update() {
    old_ball_main = ball_main;

    // move ball
    ball_main.row += ball_main.rdel;
    ball_main.col+= ball_main.cdel;

    // check for collisions with the sides of the screen
    if (ball_main.row + ball_main.size > SCREENHEIGHT) {
        ball_main.row = SCREENHEIGHT - ball_main.size;
        ball_main.rdel *= -1;
    }
    if (ball_main.col + ball_main.size > SCREENWIDTH) {
        ball_main.col = SCREENWIDTH- ball_main.size;
        ball_main.cdel *= -1;
    }
    if (ball_main.row < 0) {
        ball_main.row = 0;
        ball_main.rdel *= -1;
    }
    if (ball_main.col< 0) {
        ball_main.col = 0;
        ball_main.cdel *= -1;
    }
}

void draw() {
    // erase the ball
    drawRect3_main(old_ball_main.row,
                   old_ball_main.col,
                   old_ball_main.size,
                   old_ball_main.size,
                   0);

    // draw it in its new position
    drawRect3_main(ball_main.row,
                   ball_main.col,
                   ball_main.size,
                   ball_main.size,
                   ball_main.color);
}

void setPixel3_main(int row, int col, u16 color) {
    VIDEO_BUFFER_MAIN[OFFSET(row, col, SCREENWIDTH)] = color | PIXEL_ENABLE;
}


void setPixel3_sub(int row, int col, u16 color) {
    VIDEO_BUFFER_SUB[OFFSET(row, col, SCREENWIDTH)] = color | PIXEL_ENABLE;
}

void drawRect3_main(int row, int col, int width, int height, u16 color) {
    int r, c;
    for (c=col; c<col+width; c++) {
        for (r=row; r<row+height; r++) {
            setPixel3_main(r, c, color);
        }
    }
}

void drawRect3_sub(int row, int col, int width, int height, u16 color) {
    int r, c;
    for (c=col; c<col+width; c++) {
        for (r=row; r<row+height; r++) {
            setPixel3_sub(r, c, color);
        }
    }
}

void waitForVblank() {
    while (SCANLINECOUNTER > SCREENHEIGHT);
    while (SCANLINECOUNTER < SCREENHEIGHT);
}

