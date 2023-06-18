// srxecore
#include "screen.h"
#include "lcdtext.h"
#include "printf.h"

// function that moves the position of the cursor to the next line, and scrolls
// by modding the current counter.
static uint16_t cursor = 0;
void printLine(const char* line) {
    // get the screen height
    int height = lcdFontHeightGet();
    char buf[128] = {0};
    // put 0 padded line number thats 3 digits
    snprintf(buf, sizeof(buf), "%03d %s %20s", cursor, line, " ");
    lcdPutString(buf);
    // move the position to the next line
    lcdPositionSet(0, ((cursor % 10) + 1) * height);
    cursor++;
}