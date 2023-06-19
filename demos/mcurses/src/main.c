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

void srxe_putchar(uint8_t c) {
    if(c == '\033') {  // ESC
        seqIndex = 0;
        seqBuffer[seqIndex++] = c;
    } else if(seqIndex > 0) {
        seqBuffer[seqIndex++] = c;
        seqBuffer[seqIndex] = '\0';  // null terminate
        if(strstr(seqBuffer, SEQ_CLEAR)) {
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
        lcdPositionSet(0, cursorY*lcdFontHeightGet());
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
}

int main() {
    lcdInit();
    lcdFontSet(FONT2);
    lcdColorSet(LCD_BLACK, LCD_WHITE);

    setFunction_putchar(srxe_putchar);

    initscr();
    move(5, 5);
    addstr("Hello, world!");

    return 1;
}