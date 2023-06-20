// implement a basic vt220 emulator and see how well mcurses works
#define MCURSES_LINES 10
#define MCURSES_COLS 30

#include "mcurses.h"
#include "lcd.h"
#include <avr/pgmspace.h>
#include <string.h>

#define SEQ_CSI                                 PSTR("\033[")                   // code introducer
#define SEQ_CLEAR                               PSTR("\033[2J")                 // clear screen
#define SEQ_CLRTOBOT                            PSTR("\033[J")                  // clear to bottom
#define SEQ_CLRTOEOL                            PSTR("\033[K")                  // clear to end of line
#define SEQ_DELCH                               PSTR("\033[P")                  // delete character
#define SEQ_NEXTLINE                            PSTR("\033E")                   // goto next line (scroll up at end of scrolling region)
#define SEQ_INSERTLINE                          PSTR("\033[L")                  // insert line
#define SEQ_DELETELINE                          PSTR("\033[M")                  // delete line
#define SEQ_ATTRSET                             PSTR("\033[0")                  // set attributes, e.g. "\033[0;7;1m"
#define SEQ_ATTRSET_REVERSE                     PSTR(";7")                      // reverse
#define SEQ_ATTRSET_UNDERLINE                   PSTR(";4")                      // underline
#define SEQ_ATTRSET_BLINK                       PSTR(";5")                      // blink
#define SEQ_ATTRSET_BOLD                        PSTR(";1")                      // bold
#define SEQ_ATTRSET_DIM                         PSTR(";2")                      // dim
#define SEQ_ATTRSET_FCOLOR                      PSTR(";3")                      // forground color
#define SEQ_ATTRSET_BCOLOR                      PSTR(";4")                      // background color
#define SEQ_INSERT_MODE                         PSTR("\033[4h")                 // set insert mode
#define SEQ_REPLACE_MODE                        PSTR("\033[4l")                 // set replace mode
#define SEQ_RESET_SCRREG                        PSTR("\033[r")                  // reset scrolling region
#define SEQ_LOAD_G1                             PSTR("\033)0")                  // load G1 character set
#define SEQ_CURSOR_VIS                          PSTR("\033[?25")                // set cursor visible/not visible

// https://www.emtec.com/zoc/vt220-terminal-emulator.html
// ESC [ n @ → Insert n (Blank) Character(s)
// ESC [ n A → Cursor Up n Times
// ESC [ n B → Cursor Down n Times
// ESC [ n C → Cursor Forward n Times
// ESC [ n D → Cursor Backward n Times
// ESC [ n ; n H → Cursor Position [row;column]
// ESC [ n I → Cursor Forward Tabulation n tab ston (default = 1)
// ESC [ n J → Erase in Display ( n= 0/1/2 → below/above/all)
// ESC [ n K → Erase in Line ( n= 0/1/2 → left/right/all)
// ESC [ n K → Set text attributes ( n= 1, highlight; n= 30-37 foreground color)

char lcdBuffer[MCURSES_LINES][MCURSES_COLS];

// assuming a 2D array lcdBufferAttr to hold the attributes of the characters
char lcdBufferAttr[MCURSES_LINES][MCURSES_COLS];

char lcdBuffer[MCURSES_LINES][MCURSES_COLS];

// assuming a 2D array lcdBufferAttr to hold the attributes of the characters
char lcdBufferAttr[MCURSES_LINES][MCURSES_COLS];

// cursor position
int cursorX = 0, cursorY = 0;

// attributes
int attrReverse = 0;
int attrUnderline = 0;
int attrBlink = 0;
int attrBold = 0;
int attrDim = 0;
int attrFColor = 0;
int attrBColor = 0;

// buffer to hold incoming escape sequences
char seqBuffer[16];
int seqIndex = 0;

// cursor movement and screen control
void cursorUp(int n) {
    cursorY -= n;
    if(cursorY < 0)
        cursorY = 0;
}

void cursorDown(int n) {
    cursorY += n;
    if(cursorY >= LCD_HEIGHT)
        cursorY = LCD_HEIGHT - 1;
}

void cursorForward(int n) {
    cursorX += n;
    if(cursorX >= LCD_WIDTH)
        cursorX = LCD_WIDTH - 1;
}

void cursorBackward(int n) {
    cursorX -= n;
    if(cursorX < 0)
        cursorX = 0;
}

// Parsing and applying escape sequences
void parseAndApplySeq() {
    int param1 = 0;
    int param2 = 0;

    sscanf(seqBuffer, "\033[%d;%d", &param1, &param2);

    if(strstr(seqBuffer, "@")) {
        // insert character(s)
        memmove(&lcdBuffer[cursorY][cursorX + param1],
                &lcdBuffer[cursorY][cursorX],
                LCD_WIDTH - cursorX - param1);
        for(int i = cursorX; i < cursorX + param1; i++)
            lcdBuffer[cursorY][i] = ' ';
    } else if(strstr(seqBuffer, "A")) {
        cursorUp(param1);
    } else if(strstr(seqBuffer, "B")) {
        cursorDown(param1);
    } else if(strstr(seqBuffer, "C")) {
        cursorForward(param1);
    } else if(strstr(seqBuffer, "D")) {
        cursorBackward(param1);
    } else if(strstr(seqBuffer, "H")) {
        cursorY = param1 - 1;
        cursorX = param2 - 1;
    } else if(strstr(seqBuffer, "J")) {
        // erase in display
        //...
    } else if(strstr(seqBuffer, "K")) {
        // erase in line
        //...
    }
}


void srxe_putchar(uint8_t c) {
    if(c == '\033') {  // ESC
        seqIndex = 0;
        seqBuffer[seqIndex++] = c;
    } else if(seqIndex > 0) {
        seqBuffer[seqIndex++] = c;
        seqBuffer[seqIndex] = '\0';  // null terminate
        if(strchr("@ABCDEFGHIJK", c) != NULL) {
            parseAndApplySeq();
            seqIndex = 0;
        } else if(strstr(seqBuffer, SEQ_CLEAR)) {
            lcdClearScreen();
            cursorX = cursorY = 0;
            seqIndex = 0;
        } else if(strstr(seqBuffer, SEQ_CLRTOEOL)) {
            for(int i = cursorX; i < MCURSES_COLS; i++)
                lcdBuffer[cursorY][i] = ' ';
            seqIndex = 0;
        } else if(strstr(seqBuffer, SEQ_CLRTOBOT)) {
            for(int i = cursorY; i < MCURSES_LINES; i++)
                for(int j = 0; j < MCURSES_COLS; j++)
                    lcdBuffer[i][j] = ' ';
            seqIndex = 0;
        } else if(strstr(seqBuffer, SEQ_ATTRSET_REVERSE)) {
            attrReverse = 1;
            seqIndex = 0;
        }
        // add more cases here for other sequences
    } else if(c == '\n') {  // new line
        cursorY++;
        cursorX = 0;
        if(cursorY >= MCURSES_LINES) {
            lcdScrollLines(8);  // scroll up one line if at the bottom
            cursorY = MCURSES_LINES - 1;
        }
    } else {  // regular character
        lcdPutChar(c);
        lcdBuffer[cursorY][cursorX] = c;
        cursorX++;
        if(cursorX >= MCURSES_COLS) {  // wrap line
            cursorY++;
            cursorX = 0;
            if(cursorY >= MCURSES_LINES) {
                lcdScrollLines(8);  // scroll up one line if at the bottom
                cursorY = MCURSES_LINES - 1;
            }
        }
    }
    lcdPositionSet(cursorX*lcdFontWidthGet(), cursorY*lcdFontHeightGet());
}

int main() {
    lcdInit();
    lcdFontSet(FONT2);
    lcdColorSet(LCD_BLACK, LCD_WHITE);

    setFunction_putchar(srxe_putchar);

    initscr();
    move(5, 5);
    addstr("Hello, world!\ntesting 1 2 3\n");

    return 1;
}