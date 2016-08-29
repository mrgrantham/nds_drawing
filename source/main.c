#include "math.h"
#include "nds.h"

typedef unsigned short u16;
//typedef volatile unsigned int vu32;
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

typedef struct POINT {
    int x;
    int y;
    int z;
    u16 color;
} POINT;

typedef struct EDGE {
    int p1;
    int p2;
} EDGE;


void setPixel3_main(int row, int col, u16 color);
void setPixel3_sub(int row, int col, u16 color);
void drawLine3_main(POINT p1, POINT p2, u16 color);
void drawLine3_sub(int x1, int y1, int x2, int y2, u16 color);
void drawRect3_main(int row, int col, int width, int height, u16 color);
void drawRect3_sub(int row, int col, int width, int height, u16 color);
void update();
void draw();
void waitForVblank();



static float current_rad = 0.0;
double inc_radians();
POINT flatten(const POINT _3Dpoint);
POINT rotate2D(POINT axis, int x, int y, double radians);
POINT rotateX3D(const POINT *axis, const POINT *point, double radians);
void drawWireQuad3_main(POINT p1, POINT p2, POINT p3, POINT p4, u16 color);

static POINT camera =  {.x=128  ,.y=96  ,.z=-400};
static POINT viewportPlane[] = { {  .x=0,   .y=0,   .z=0},
                                 {  .x=255, .y=0,   .z=0},
                                 {  .x=0,   .y=191,  .z=0}};

static EDGE cubeEdges[] = {{.p1=0 ,.p2=1 },
                        {.p1=0 ,.p2=2 },
                        {.p1=0 ,.p2=4 },
                        {.p1=1 ,.p2=3 },
                        {.p1=1 ,.p2=5 },
                        {.p1=2 ,.p2=3 },
                        {.p1=2 ,.p2=6 },
                        {.p1=3 ,.p2=7 },
                        {.p1=4 ,.p2=5 },
                        {.p1=4 ,.p2=6 },
                        {.p1=5 ,.p2=7 },
                        {.p1=6 ,.p2=7 }};

// stuctures for 3d Cube and 2D representation
static POINT _3Daxis =    {.x=128 ,.y=96  ,.z=128};

static POINT _3Dreal[] = {  {.x=98  ,.y=66  ,.z=98},
                            {.x=158 ,.y=66  ,.z=98},
                            {.x=98  ,.y=126 ,.z=98},
                            {.x=158 ,.y=126 ,.z=98},
                            {.x=98  ,.y=66  ,.z=158},
                            {.x=158 ,.y=66  ,.z=158},
                            {.x=98  ,.y=126 ,.z=158},
                            {.x=158 ,.y=126 ,.z=158}};
static POINT _3Dflattened_curr[8]= {    {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0}};

static POINT _3Dflattened_prev[8]= {    {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0},
                                        {.x=0 ,.y=0 ,.z=0}};

// structs for drawing spinning 2D box
static POINT axis =    {.x=128 ,.y=96  ,.z=0};
static POINT p1 =      {.x=98  ,.y=66  ,.z=0};
static POINT p2 =      {.x=158 ,.y=66  ,.z=0};
static POINT p3 =      {.x=158 ,.y=126 ,.z=0};
static POINT p4 =      {.x=98  ,.y=126 ,.z=0};

static POINT p1_prev;
static POINT p2_prev;
static POINT p3_prev;
static POINT p4_prev;

static POINT p1_curr;
static POINT p2_curr;
static POINT p3_curr;
static POINT p4_curr;


int main(void) {
    // Set both displays to mode3 and turn on bg3 for each
    REG_DISPCNT_MAIN = MODE3 | BG3_ENABLE;
    REG_DISPCNT_SUB = MODE3 | BG3_ENABLE;

    // Set bg3 on both displays large enough to fill the screen, use 15 bit
    // RGB color values, and look for bitmap data at character base block 1.
    REG_BG3CNT =  BG_256x256 | BG_15BITCOLOR | BG_CBB1;
    REG_BG3CNT_SUB = BG_256x256 | BG_15BITCOLOR | BG_CBB1;

    // Don't scale bg3 (set its affine transformation matrix to [[1,0],[0,1]])

    // for main screen
    REG_BG3PA = 1 << 8;
    REG_BG3PD = 1 << 8;

    // for sub screen
    REG_BG3PA_SUB = 1 << 8;
    REG_BG3PD_SUB= 1 << 8;

    // Draw some stuff on the subdisplay
    drawRect3_sub(20, 100, 100, 42, COLOR(31, 0, 15));


    drawLine3_sub(20,20,100,100,COLOR(31,31,31));
    drawLine3_sub(150,150,100,100,COLOR(31,0,0));
    drawLine3_sub(12,30,100,100,COLOR(0,31,0));

    //consoleDemoInit();
    // currently the blow causes artifacts on the screen but still seems useful for debug
    // consoleInit( NULL, 2, BgType_Text4bpp, BgSize_T_256x256, 23, 2, false, true );
    consoleInit( NULL, 2, BgType_Text4bpp, BgSize_T_256x256, 23, 2, false, true );

    // main loop
    while(1) {
        // print out point values for debugging
        for(int i=0;i < 8; i++) {
            // escape code uses i varable to determine line to print to
            iprintf("\x1b[%d;0HP%d C %-3dY%-3dZ%-3d P X%-3dY%-3dZ%-3d",i+1,i,
                                                    _3Dflattened_curr[i].x,
                                                    _3Dflattened_curr[i].y,
                                                    _3Dflattened_curr[i].z,
                                                    _3Dflattened_prev[i].x,
                                                    _3Dflattened_prev[i].y,
                                                    _3Dflattened_prev[i].z);

        }
        // iprintf didnt support float formatting so i made this hack
        iprintf("\x1b[9;0HCURRENT RADIANS: %d.%d",(int)current_rad,(int)((current_rad-(int)current_rad)*100));

        update();
        waitForVblank();
        draw();
        // add some more delay

    }

    return 0;
}



void update() {
    p1_prev = p1_curr;
    p2_prev = p2_curr;
    p3_prev = p3_curr;
    p4_prev = p4_curr;

    p1_curr = rotate2D(axis,p1.x,p1.y,current_rad);
    p2_curr = rotate2D(axis,p2.x,p2.y,current_rad);
    p3_curr = rotate2D(axis,p3.x,p3.y,current_rad);
    p4_curr = rotate2D(axis,p4.x,p4.y,current_rad);


    for (int i=0;i < 8;i++){
        _3Dflattened_prev[i].x = _3Dflattened_curr[i].x;
        _3Dflattened_prev[i].y = _3Dflattened_curr[i].y;
        _3Dflattened_prev[i].z = _3Dflattened_curr[i].z;

        static POINT temp_rotate;
        temp_rotate = rotateX3D(&_3Daxis,&_3Dreal[i],current_rad);
        iprintf("\x1b[%d;0HP%d ROT X %-3dY %-3dZ %-3d",i+10,i,
                                                temp_rotate.x,
                                                temp_rotate.y,
                                                temp_rotate.z);

        _3Dflattened_curr[i] = flatten(temp_rotate);
    }


    current_rad = inc_radians();

}

void draw() {

    // erase last frame
    drawWireQuad3_main(p1_prev,p2_prev,p3_prev,p4_prev,COLOR(0,0,0));

    // draw new frame
    drawWireQuad3_main(p1_curr,p2_curr,p3_curr,p4_curr,COLOR(0,31,0));
    for(int i=0;i<12;i++){
            drawLine3_main(_3Dflattened_prev[cubeEdges[i].p1],_3Dflattened_prev[cubeEdges[i].p2],COLOR(0,0,0));
    }
    for(int i=0;i<12;i++){
            drawLine3_main(_3Dflattened_curr[cubeEdges[i].p1],_3Dflattened_curr[cubeEdges[i].p2],COLOR(31,31,31));
    }
}


// implements a rotation matrix to move
// points in 2d around a set axis point
//          || cos() -sin() || |x|
//          || sin()  cos() || |y|
POINT rotate2D(POINT axis, int x, int y, double radians) {
    POINT new_point;

    x -= axis.x;
    y -= axis.y;


    double A = cos(radians);
    double B = -sin(radians);
    double C = -B;
    double D = A;

    new_point.x = A*x + B*y;
    new_point.y = C*x + D*y;

    new_point.x += axis.x;
    new_point.y += axis.y;

    return new_point;
}

// takes 3d point and finds where it should fall on the screen  based on veiwport
// and camera position
POINT flatten(const POINT _3Dpoint){

    static POINT _2Dpoint = {.x=0,.y=0,.z=0};
    static POINT D;

    // more complex if camera viewport is rotated
    D.x = _3Dpoint.x - camera.x;
    D.y = _3Dpoint.y - camera.y;
    D.z = _3Dpoint.z - camera.z;


    _2Dpoint.x = (float)-camera.z * (float)D.x / (float)D.z;
    _2Dpoint.y = (float)-camera.z * (float)D.y / (float)D.z;
    _2Dpoint.z = 0;

    _2Dpoint.x += camera.x;
    _2Dpoint.y += camera.y;
    _2Dpoint.z += camera.z;

    return _2Dpoint;
}

// implements an X-axis rotation matrix to move
// points in 3d around a set axis point
//          || 1    0     0     || |x|
//          || 0   cos() -sin() || |y|
//          || 0   sin()  cos() || |z|
POINT rotateX3D(const POINT *axis, const POINT *point, double radians) {

    static POINT temp;
    temp.x=point->x-axis->x;
    temp.y=point->y-axis->y;
    temp.z=point->z-axis->z;

    static double _3Dmatrix[3][3];

    _3Dmatrix[0][0] = 1;     _3Dmatrix[0][1] = 0;             _3Dmatrix[0][2] = 0;
    _3Dmatrix[1][0] = 0;     _3Dmatrix[1][1] = cos(radians);  _3Dmatrix[1][2] = sin(radians);
    _3Dmatrix[2][0] = 0;     _3Dmatrix[2][1] = -sin(radians);  _3Dmatrix[2][2] = cos(radians);

    _3Dmatrix[0][0] *= temp.x;     _3Dmatrix[0][1] *= temp.y;           _3Dmatrix[0][2] *= temp.z;
    _3Dmatrix[1][0] *= temp.x;     _3Dmatrix[1][1] *= temp.y;           _3Dmatrix[1][2] *= temp.z;
    _3Dmatrix[2][0] *= temp.x;     _3Dmatrix[2][1] *= temp.y;           _3Dmatrix[2][2] *= temp.z;

    temp.x = _3Dmatrix[0][0] + _3Dmatrix[0][1] + _3Dmatrix[0][2];
    temp.y = _3Dmatrix[1][0] + _3Dmatrix[1][1] + _3Dmatrix[1][2];
    temp.z = _3Dmatrix[2][0] + _3Dmatrix[2][1] + _3Dmatrix[2][2];

    temp.x += axis->x;
    temp.y += axis->y;
    temp.z += axis->z;

    return temp;

}

void drawWireQuad3_main(POINT p1, POINT p2, POINT p3, POINT p4, u16 color) {
    drawLine3_main(p1,p2,color);
    drawLine3_main(p2,p3,color);
    drawLine3_main(p3,p4,color);
    drawLine3_main(p4,p1,color);
}

double inc_radians() {
    static double radian_val = 0.0;

    if (radian_val < (2 * M_PI) ) {
        radian_val += 0.0087;
    } else {
        radian_val = 0.0;
    }
    return radian_val;
}

// sets the pixel for the main DS screen
void setPixel3_main(int row, int col, u16 color) {
    VIDEO_BUFFER_MAIN[OFFSET(row, col, SCREENWIDTH)] = color | PIXEL_ENABLE;
}

// sets the pixel for the Lower DS screen/touchscreen
void setPixel3_sub(int row, int col, u16 color) {
    VIDEO_BUFFER_SUB[OFFSET(row, col, SCREENWIDTH)] = color | PIXEL_ENABLE;
}


void drawLine3_main(POINT p1, POINT p2, u16 color) {

    int x0 = p1.x;
    int y0 = p1.y;
    int x1 = p2.x;
    int y1 = p2.y;

    int x = x0;
    int y = y0;

    // this indicates in absolute value of the difference
    // between starting coordinates and ending
    int dx = (x0 > x1) ? x0 - x1 : x1 - x0;
    int dy = (y0 > y1) ? y0 - y1 : y1 - y0;

    // this is the step
    // if the destnation is closer to the origin than
    // the starting point then the step is negative
    // indicating that it will go back
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = (dx>dy ? dx : -dy) >> 1;
    int e2;

    for(;;) {
        setPixel3_main(y,x,color);
        // check if x and y are equal to
        // destnation coordinates
        if (x == x1 && y == y1) break;
        e2 = err;
        // e2 must be kept instead of using err
        // since it is use twice
        if (e2 >-dx) { err -= dy; x += sx; }
        if (e2 < dy) { err += dx; y += sy; }
    }
}

void drawLine3_sub(int x0, int y0, int x1, int y1, u16 color) {

    int dx = x1 - x0;
    int dy = y1 - y0;
    int D = dy -dx;
    int y = y0;

    for(int x = x0; x < x1 ; x++) {
        setPixel3_sub(y,x,color);
        if (D >= 0) {
            y = y + 1;
            D = D - dx;
        }
        D = D + dy;
    }
}

//
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
