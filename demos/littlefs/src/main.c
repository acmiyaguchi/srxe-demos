// littlefs
#include "lfs.h"
#include "screen.h"

// srxecore
#include "clock.h"
#include "power.h"
#include "keyboard.h"
#include "lcdtext.h"
#include "lcdbase.h"
#include "flash.h"

// variables used by the filesystem
static lfs_t lfs;
static lfs_file_t file;

int srxe_read(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, void *buffer, lfs_size_t size) {
    printLine("Reading block...");
    uint32_t addr = (block * c->block_size) + off;
    int rv = flashRead(addr, (uint8_t*)buffer, size);
    return rv ? LFS_ERR_OK : LFS_ERR_IO;
}

#define PAGE_SIZE 256
int srxe_prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size) {
    printLine("Writing block...");
    uint32_t addr = (block * c->block_size) + off;

    bool rv = 0;
    lfs_size_t pages = size / PAGE_SIZE;
    for (lfs_size_t i = 0; i < pages; i++) {
        rv &= flashWritePage(addr + i * PAGE_SIZE, (uint8_t*)buffer + i * PAGE_SIZE);
    }

    return rv ? LFS_ERR_OK : LFS_ERR_IO;
}

int srxe_erase(const struct lfs_config *c, lfs_block_t block) {
    printLine("Erasing block...");
    uint32_t addr = block * c->block_size;
    int rv = flashEraseSector(addr, 1);
    return rv ? LFS_ERR_OK : LFS_ERR_IO;
}

int srxe_sync(const struct lfs_config *c) {
  printLine("Syncing...");
  // no-op
  return LFS_ERR_OK;
}


const struct lfs_config cfg = {
    srxe_read,  // read
    srxe_prog,  // prog
    srxe_erase,  // erase
    srxe_sync,  // sync
    16,  // read_size
    PAGE_SIZE,  // prog_size
    4096,  // block_size
    30,  // block_count
    PAGE_SIZE,  // cache_size
    16,  // lookahead_size
    500,  // block_cycles
};


// initialize the file system
void initFileSystem() {
    // mount the filesystem
    printLine("Mounting filesystem...");
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        // tell the host we are formatting
        printLine("Formatting filesystem...");
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }
    printLine("Done mounting filesystem.");
}


int main() {
    clockInit();
    powerInit();
    kbdInit();
    lcdInit();

    lcdClearScreen();
    lcdFontSet(FONT2);
    lcdColorSet(LCD_BLACK, LCD_WHITE);
    printLine("Initialized SRXE core libraries.");
    printLine("Press any key to continue.");
    kbdGetKeyWait();

    initFileSystem();

    // write to a file
    printLine("Opening file...");
    lfs_file_opencfg(&lfs, &file, "hello.txt", LFS_O_WRONLY | LFS_O_CREAT, &cfg);
    printLine("Writing to file...");
    lfs_file_write(&lfs, &file, "Hello World!", 12);
    printLine("Closing file...");
    lfs_file_close(&lfs, &file);

    printLine("Press any key to continue.");
    kbdGetKeyWait();

    // read back
    printLine("Opening file...");
    lfs_file_opencfg(&lfs, &file, "hello.txt", LFS_O_RDONLY, &cfg);
    printLine("Reading from file...");
    char buf[12] = {0};
    lfs_file_read(&lfs, &file, buf, 12);
    printLine(buf);
    printLine("Closing file...");
    lfs_file_close(&lfs, &file);

    // delete the file
    printLine("Deleting file...");
    lfs_remove(&lfs, "hello.txt");

    printLine("Press any key to sleep.");
    kbdGetKeyWait();
    printLine("Sleeping...");
    lcdSleep();
    powerSleep();
}
