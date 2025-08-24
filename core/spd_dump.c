#include "common.h"
#include "GITVER.h"

void print_help(void) {
    DBG_LOG("Usage\n"
            "\tspd_dump [OPTIONS] [COMMANDS] [EXIT COMMANDS]\n"
            "\nExamples\n"
            "\tOne-line mode\n"
            "\t\tspd_dump --wait 300 fdl /path/to/fdl1 fdl1_addr fdl /path/to/fdl2 fdl2_addr exec path savepath r all_lite reset\n"
            "\tInteractive mode\n"
            "\t\tspd_dump --wait 300 fdl /path/to/fdl1 fdl1_addr fdl /path/to/fdl2 fdl2_addr exec\n"
            "\tThen the prompt should display FDL2>.\n"
    );
    DBG_LOG("\nOptions\n"
            "\t--wait <seconds>\n"
            "\t\tSpecifies the time to wait for the device to connect.\n"
            "\t--stage <number>|-r|--reconnect\n"
            "\t\tTry to reconnect device in brom/fdl1/fdl2 stage. Any number behaves the same way.\n"
            "\t\t(unstable, a device in brom/fdl1 stage can be reconnected infinite times, but only once in fdl2 stage)\n"
            "\t--verbose <level>\n"
            "\t\tSets the verbosity level of the output (supports 0, 1, or 2).\n"
            "\t--kick\n"
            "\t\tConnects the device using the route boot_diag -> cali_diag -> dl_diag.\n"
            "\t--kickto <mode>\n"
            "\t\tConnects the device using a custom route boot_diag -> custom_diag. Supported modes are 0-127.\n"
            "\t\t(mode 0 is `kickto 2` on ums9621, mode 1 = cali_diag, mode 2 = dl_diag; not all devices support mode 2).\n"
            "\t-?|-h|--help\n"
            "\t\tShow help and usage information\n"
    );
    DBG_LOG("\nRuntime Commands\n"
            "\tverbose level\n"
            "\t\tSets the verbosity level of the output (supports 0, 1, or 2).\n"
            "\ttimeout ms\n"
            "\t\tSets the command timeout in milliseconds.\n"
            "\tbaudrate [rate]\n"
            "\t\t(Windows SPRD driver only, and brom/fdl2 stage only)\n"
            "\t\tSupported baudrates are 57600, 115200, 230400, 460800, 921600, 1000000, 2000000, 3250000, and 4000000.\n"
            "\t\tWhile in u-boot/littlekernel source code, only 115200, 230400, 460800, and 921600 are listed.\n"
            "\texec_addr [addr]\n"
            "\t\t(brom stage only)\n"
            "\t\tSends custom_exec_no_verify_addr.bin to the specified memory address to bypass the signature verification by brom for splloader/fdl1.\n"
            "\t\tUsed for CVE-2022-38694.\n"
            "\tfdl FILE addr\n"
            "\t\tSends a file (splloader, fdl1, fdl2, sml, trustos, teecfg) to the specified memory address.\n"
            "\tloadexec FILE(addr_in_name)\n"
            "\t\tSet exec_addr with the address encoded in filename and save exec_file path.\n"
            "\tloadfdl FILE(addr_in_name)\n"
            "\t\tLoad FDL file to the address encoded in filename.\n"
            "\texec\n"
            "\t\tExecutes a sent file in the fdl1 stage. Typically used with sml or fdl2 (also known as uboot/lk).\n"
            "\tpath [save_location]\n"
            "\t\tChanges the save directory for commands like r, read_part(s), read_flash, and read_mem.\n"
            "\tnand_id [id]\n"
            "\t\tSpecifies the 4th NAND ID, affecting read_part(s) size calculation, default value is 0x15.\n"
            "\trawdata {0,1,2}\n"
            "\t\t(fdl2 stage only)\n"
            "\t\tRawdata protocol helps speed up `w` and `write_part(s)` commands, when rawdata > 0, `blk_size` will not affect write speed.\n"
            "\tblk_size byte\n"
            "\t\t(fdl2 stage only)\n"
            "\t\tSets the block size, with a maximum of 65535 bytes. This option helps speed up `r`, `w`, `read_part(s)` and `write_part(s)` commands.\n"
            "\tr all|part_name|part_id\n"
            "\t\tWhen the partition table is available:\n"
            "\t\t\tr all: full backup (excluding large partitions like userdata)\n"
            "\t\t\tr all_lite: lite backup (only small partitions)\n"
            "\t\t\tr part_name: backup specific partition by name\n"
            "\t\t\tr part_id: backup specific partition by ID\n"
    );
}

#ifndef BUILD_LIB
int main(int argc, char **argv) {
    spdio_t *io = spdio_init(0);
    if (!io) {
        DBG_LOG("Failed to initialize spdio\n");
        return 1;
    }
    print_help();
    spdio_free(io);
    return 0;
}
#endif