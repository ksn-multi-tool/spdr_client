#include "common.h"
#include <stdarg.h>

#if !USE_LIBUSB
// Предварительное объявление ThrdFunc
DWORD WINAPI ThrdFunc(LPVOID lpParam);

DWORD curPort = 0;
DWORD *FindPort(const char *USB_DL) {
    const GUID GUID_DEVCLASS_PORTS = { 0x4d36e978, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18} };
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD dwIndex = 0;
    DWORD count = 0;
    DWORD *ports = NULL;

    DeviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);

    if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
        DBG_LOG("Failed to get device information set. Error code: %ld\n", GetLastError());
        return NULL;
    }

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    while (SetupDiEnumDeviceInfo(DeviceInfoSet, dwIndex, &DeviceInfoData)) {
        char friendlyName[MAX_PATH];
        DWORD dataType = 0;
        DWORD dataSize = sizeof(friendlyName);

        SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet, &DeviceInfoData, SPDRP_FRIENDLYNAME, &dataType, (BYTE *)friendlyName, dataSize, &dataSize);
        char *result = strstr(friendlyName, USB_DL);
        if (result != NULL) {
            char portNum_str[4];
            strncpy(portNum_str, result + strlen(USB_DL) + 5, 3);
            portNum_str[3] = 0;

            DWORD portNum = strtoul(portNum_str, NULL, 10);
            DWORD *temp = (DWORD *)realloc(ports, (count + 2) * sizeof(DWORD));
            if (temp == NULL) {
                DBG_LOG("Memory allocation failed.\n");
                SetupDiDestroyDeviceInfoList(DeviceInfoSet);
                free(ports);
                return NULL;
            }
            ports = temp;
            ports[count] = portNum;
            count++;
        }
        ++dwIndex;
    }

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    if (count > 0) ports[count] = 0;
    return ports;
}

BOOL CreateRecvThread(spdio_t *io) {
    io->m_hOprEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (io->m_hOprEvent == NULL) {
        DBG_LOG("Failed to create event. Error code: %ld\n", GetLastError());
        return FALSE;
    }

    io->m_hRecvThread = CreateThread(NULL, 0, ThrdFunc, io, 0, &io->m_dwRecvThreadID);
    if (io->m_hRecvThread == NULL) {
        DBG_LOG("Failed to create thread. Error code: %ld\n", GetLastError());
        CloseHandle(io->m_hOprEvent);
        return FALSE;
    }

    return TRUE;
}

void DestroyRecvThread(spdio_t *io) {
    if (io->m_hRecvThread) {
        TerminateThread(io->m_hRecvThread, 0);
        CloseHandle(io->m_hRecvThread);
        io->m_hRecvThread = NULL;
    }
    if (io->m_hOprEvent) {
        CloseHandle(io->m_hOprEvent);
        io->m_hOprEvent = NULL;
    }
}

DWORD WINAPI ThrdFunc(LPVOID lpParam) {
    spdio_t *io = (spdio_t *)lpParam;
    while (io->is_open) {
        DWORD dwRead = io->handle->Read(io->recv_buf, io->recv_len, io->timeout);
        if (dwRead) {
            io->recv_pos = dwRead;
            SetEvent(io->m_hOprEvent);
        }
        Sleep(1);
    }
    return 0;
}
#else
libusb_device *curPort = NULL;
libusb_device **FindPort(int pid) {
    libusb_device **devs;
    int usb_cnt, count = 0;
    libusb_device **ports = NULL;

    usb_cnt = libusb_get_device_list(NULL, &devs);
    if (usb_cnt < 0) {
        DBG_LOG("Get device list error\n");
        return NULL;
    }
    for (int i = 0; i < usb_cnt; i++) {
        libusb_device *dev = devs[i];
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            DBG_LOG("Failed to get device descriptor\n");
            continue;
        }
        if (desc.idVendor == 0x1782 && (pid == 0 || desc.idProduct == pid)) {
            libusb_device **temp = (libusb_device **)realloc(ports, (count + 2) * sizeof(libusb_device *));
            if (temp == NULL) {
                DBG_LOG("Memory allocation failed.\n");
                libusb_free_device_list(devs, 1);
                free(ports);
                return NULL;
            }
            ports = temp;
            ports[count++] = dev;
            libusb_ref_device(dev);
        }
    }
    libusb_free_device_list(devs, 1);
    if (count > 0) ports[count] = NULL;
    return ports;
}

void find_endpoints(libusb_device_handle *dev_handle, int result[2]) {
    struct libusb_config_descriptor *config;
    libusb_get_active_config_descriptor(libusb_get_device(dev_handle), &config);
    for (int i = 0; i < config->bNumInterfaces; i++) {
        const struct libusb_interface *iface = &config->interface[i];
        for (int j = 0; j < iface->num_altsetting; j++) {
            const struct libusb_interface_descriptor *alt = &iface->altsetting[j];
            for (int k = 0; k < alt->bNumEndpoints; k++) {
                const struct libusb_endpoint_descriptor *ep = &alt->endpoint[k];
                if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN)
                    result[0] = ep->bEndpointAddress;
                else
                    result[1] = ep->bEndpointAddress;
            }
        }
    }
    libusb_free_config_descriptor(config);
}

void call_Initialize_libusb(spdio_t *io) {
    int result[2] = {0, 0};
    if (libusb_init(NULL) < 0) {
        DBG_LOG("Failed to initialize libusb\n");
        return;
    }
    io->dev_handle = libusb_open_device_with_vid_pid(NULL, 0x1782, 0);
    if (io->dev_handle == NULL) {
        DBG_LOG("Failed to open device\n");
        libusb_exit(NULL);
        return;
    }
    find_endpoints(io->dev_handle, result);
    io->endp_in = result[0];
    io->endp_out = result[1];
}
#endif

#ifdef _MSC_VE
void usleep(unsigned int us) {
    Sleep(us / 1000);
}
#endif

uint16_t crc16(uint16_t crc, uint8_t c) {
    crc = crc ^ c;
    for (int i = 0; i < 8; i++) {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc >>= 1;
    }
    return crc;
}

void print_string(FILE *f, const void *src, size_t n) {
    size_t i;
    int a, b = 0;
    const uint8_t *buf = (const uint8_t *)src;
    fprintf(f, "\"");
    for (i = 0; i < n; i++) {
        a = buf[i];
        b = 0;
        switch (a) {
        case '"': case '\\': b = a; break;
        case 0: b = '0'; break;
        case '\b': b = 'b'; break;
        case '\t': b = 't'; break;
        case '\n': b = 'n'; break;
        case '\f': b = 'f'; break;
        case '\r': b = 'r'; break;
        }
        if (b) fprintf(f, "\\%c", b);
        else if (a >= 0x20 && a < 0x7f) fprintf(f, "%c", a);
        else fprintf(f, "\\x%02x", a);
    }
    fprintf(f, "\"\n");
}

spdio_t *spdio_init(int flags) {
    spdio_t *io = (spdio_t *)calloc(1, sizeof(spdio_t));
    if (!io) {
        DBG_LOG("Failed to allocate spdio_t\n");
        return NULL;
    }
    io->flags = flags;
    io->recv_len = 0x10000;
    io->recv_buf = (uint8_t *)malloc(io->recv_len);
    io->raw_buf = (uint8_t *)malloc(0x10000);
    io->enc_buf = (uint8_t *)malloc(0x20000);
    io->temp_buf = (uint8_t *)malloc(0x10000);
    io->untranscode_buf = (uint8_t *)malloc(0x10000);
    io->send_buf = (uint8_t *)malloc(0x10000);
    if (!io->recv_buf || !io->raw_buf || !io->enc_buf || !io->temp_buf || !io->untranscode_buf || !io->send_buf) {
        DBG_LOG("Failed to allocate buffers\n");
        spdio_free(io);
        return NULL;
    }
    io->timeout = 1000;
    io->verbose = 1;
    io->is_open = 0;
    return io;
}

void spdio_free(spdio_t *io) {
    if (!io) return;
#if USE_LIBUSB
    if (io->dev_handle) {
        libusb_close(io->dev_handle);
        libusb_exit(NULL);
    }
#else
    DestroyRecvThread(io);
    if (io->handle) {
        io->handle->Close();
        io->handle->FreeMem(io->handle);
        io->is_open = 0;
    }
#endif
    free(io->recv_buf);
    free(io->raw_buf);
    free(io->enc_buf);
    free(io->temp_buf);
    free(io->untranscode_buf);
    free(io->send_buf);
    free(io->ptable);
    free(io);
}

void encode_msg(spdio_t *io, int type, const void *data, size_t len) {
    uint8_t *buf = io->enc_buf;
    size_t i, j = 0;
    uint16_t crc = 0;
    buf[j++] = HDLC_HEADER;
    if (io->flags & FLAGS_TRANSCODE) {
        buf[j++] = type;
        crc = crc16(crc, type);
        for (i = 0; i < len; i++) {
            uint8_t c = ((uint8_t *)data)[i];
            crc = crc16(crc, c);
            if (c == HDLC_HEADER || c == HDLC_ESCAPE) {
                buf[j++] = HDLC_ESCAPE;
                buf[j++] = c ^ 0x20;
            } else {
                buf[j++] = c;
            }
        }
        if (io->flags & FLAGS_CRC16) {
            crc = ~crc;
            if ((crc & 0xFF) == HDLC_HEADER || (crc & 0xFF) == HDLC_ESCAPE) {
                buf[j++] = HDLC_ESCAPE;
                buf[j++] = (crc & 0xFF) ^ 0x20;
            } else {
                buf[j++] = crc & 0xFF;
            }
            if ((crc >> 8) == HDLC_HEADER || (crc >> 8) == HDLC_ESCAPE) {
                buf[j++] = HDLC_ESCAPE;
                buf[j++] = (crc >> 8) ^ 0x20;
            } else {
                buf[j++] = crc >> 8;
            }
        }
    } else {
        buf[j++] = type;
        memcpy(buf + j, data, len);
        j += len;
    }
    buf[j++] = HDLC_HEADER;
    io->enc_len = j;
}

void encode_msg_nocpy(spdio_t *io, int type, size_t len) {
    encode_msg(io, type, io->raw_buf, len);
}

int send_msg(spdio_t *io) {
    int ret;
#if USE_LIBUSB
    int actual;
    ret = libusb_bulk_transfer(io->dev_handle, io->endp_out, io->enc_buf, io->enc_len, &actual, io->timeout);
    if (ret < 0) {
        DBG_LOG("Send failed: %s\n", libusb_error_name(ret));
        return -1;
    }
    return actual;
#else
    ret = io->handle->Write(io->enc_buf, io->enc_len);
    if (ret <= 0) {
        DBG_LOG("Send failed\n");
        return -1;
    }
    return ret;
#endif
}

int recv_msg(spdio_t *io) {
    return recv_msg_timeout(io, io->timeout);
}

int recv_msg_timeout(spdio_t *io, int timeout) {
#if USE_LIBUSB
    int actual;
    int ret = libusb_bulk_transfer(io->dev_handle, io->endp_in, io->recv_buf, io->recv_len, &actual, timeout);
    if (ret < 0) {
        DBG_LOG("Receive failed: %s\n", libusb_error_name(ret));
        return -1;
    }
    io->recv_pos = actual;
    return actual;
#else
    io->recv_pos = io->handle->Read(io->recv_buf, io->recv_len, timeout);
    if (io->recv_pos <= 0) {
        DBG_LOG("Receive failed\n");
        return -1;
    }
    return io->recv_pos;
#endif
}

unsigned recv_type(spdio_t *io) {
    if (io->recv_pos < 1) return 0;
    return io->recv_buf[0];
}

int send_and_check(spdio_t *io) {
    if (send_msg(io) < 0) return -1;
    if (recv_msg(io) < 0) return -1;
    if (recv_type(io) != BSL_REP_ACK) {
        DBG_LOG("Unexpected response: 0x%02x\n", recv_type(io));
        return -1;
    }
    return 0;
}

int check_confirm(const char *name) {
    char confirm[10];
    printf("Type 'yes' to confirm %s: ", name);
    if (!fgets(confirm, sizeof(confirm), stdin)) return -1;
    confirm[strcspn(confirm, "\n")] = 0;
    return strcmp(confirm, "yes") == 0 ? 0 : -1;
}

uint8_t *loadfile(const char *fn, size_t *num, size_t extra) {
    FILE *f = my_fopen(fn, "rb");
    if (!f) {
        DBG_LOG("Failed to open file %s\n", fn);
        return NULL;
    }
    fseeko(f, 0, SEEK_END);
    *num = ftello(f);
    fseeko(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc(*num + extra);
    if (!buf) {
        DBG_LOG("Failed to allocate memory for file %s\n", fn);
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, *num, f) != *num) {
        DBG_LOG("Failed to read file %s\n", fn);
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    return buf;
}

void send_buf(spdio_t *io, uint32_t start_addr, int end_data, unsigned step, uint8_t *mem, unsigned size) {
    encode_msg(io, BSL_CMD_START_DATA, &start_addr, sizeof(start_addr));
    if (send_and_check(io) < 0) {
        DBG_LOG("Failed to send start data\n");
        return;
    }
    for (unsigned i = 0; i < size; i += step) {
        unsigned len = size - i > step ? step : size - i;
        encode_msg(io, BSL_CMD_MIDST_DATA, mem + i, len);
        if (send_and_check(io) < 0) {
            DBG_LOG("Failed to send midst data\n");
            return;
        }
    }
    if (end_data) {
        encode_msg(io, BSL_CMD_END_DATA, NULL, 0);
        if (send_and_check(io) < 0) {
            DBG_LOG("Failed to send end data\n");
            return;
        }
    }
}

size_t send_file(spdio_t *io, const char *fn, uint32_t start_addr, int end_data, unsigned step, unsigned src_offs, unsigned src_size) {
    size_t size;
    uint8_t *buf = loadfile(fn, &size, 0);
    if (!buf) return 0;
    if (src_size) size = src_size;
    send_buf(io, start_addr, end_data, step, buf + src_offs, size);
    free(buf);
    return size;
}

FILE *my_fopen(const char *fn, const char *mode) {
#ifdef _WIN32
    wchar_t wfn[256], wmode[10];
    MultiByteToWideChar(CP_UTF8, 0, fn, -1, wfn, 256);
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, 10);
    return _wfopen(wfn, wmode);
#else
    return fopen(fn, mode);
#endif
}

// Заглушки для недостающих функций
void ChangeMode(spdio_t *io, int ms, int bootmode, int at) {
    DBG_LOG("ChangeMode not implemented\n");
}

unsigned dump_flash(spdio_t *io, uint32_t addr, uint32_t start, uint32_t len, const char *fn, unsigned step) {
    DBG_LOG("dump_flash not implemented\n");
    return 0;
}

unsigned dump_mem(spdio_t *io, uint32_t start, uint32_t len, const char *fn, unsigned step) {
    DBG_LOG("dump_mem not implemented\n");
    return 0;
}

uint64_t dump_partition(spdio_t *io, const char *name, uint64_t start, uint64_t len, const char *fn, unsigned step) {
    DBG_LOG("dump_partition not implemented\n");
    return 0;
}

void dump_partitions(spdio_t *io, const char *fn, int *nand_info, unsigned step) {
    DBG_LOG("dump_partitions not implemented\n");
}

uint64_t read_pactime(spdio_t *io) {
    DBG_LOG("read_pactime not implemented\n");
    return 0;
}

partition_t *partition_list(spdio_t *io, const char *fn, int *part_count_ptr) {
    DBG_LOG("partition_list not implemented\n");
    return NULL;
}

void repartition(spdio_t *io, const char *fn) {
    DBG_LOG("repartition not implemented\n");
}

void erase_partition(spdio_t *io, const char *name) {
    DBG_LOG("erase_partition not implemented\n");
}

void load_partition(spdio_t *io, const char *name, const char *fn, unsigned step) {
    DBG_LOG("load_partition not implemented\n");
}

void load_nv_partition(spdio_t *io, const char *name, const char *fn, unsigned step) {
    DBG_LOG("load_nv_partition not implemented\n");
}

void load_partitions(spdio_t *io, const char *path, unsigned step, int force_ab) {
    DBG_LOG("load_partitions not implemented\n");
}

void load_partition_force(spdio_t *io, const int id, const char *fn, unsigned step) {
    DBG_LOG("load_partition_force not implemented\n");
}

int load_partition_unify(spdio_t *io, const char *name, const char *fn, unsigned step) {
    DBG_LOG("load_partition_unify not implemented\n");
    return -1;
}

uint64_t check_partition(spdio_t *io, const char *name, int need_size) {
    DBG_LOG("check_partition not implemented\n");
    return 0;
}

void get_partition_info(spdio_t *io, const char *name, int need_size) {
    DBG_LOG("get_partition_info not implemented\n");
}

uint64_t str_to_size(const char *str) {
    DBG_LOG("str_to_size not implemented\n");
    return 0;
}

uint64_t str_to_size_ubi(const char *str, int *nand_info) {
    DBG_LOG("str_to_size_ubi not implemented\n");
    return 0;
}

void get_Da_Info(spdio_t *io) {
    DBG_LOG("get_Da_Info not implemented\n");
}

void select_ab(spdio_t *io) {
    DBG_LOG("select_ab not implemented\n");
}

void dm_disable(spdio_t *io, unsigned step) {
    DBG_LOG("dm_disable not implemented\n");
}

void dm_enable(spdio_t *io, unsigned step) {
    DBG_LOG("dm_enable not implemented\n");
}

void w_mem_to_part_offset(spdio_t *io, const char *name, size_t offset, uint8_t *mem, size_t length, unsigned step) {
    DBG_LOG("w_mem_to_part_offset not implemented\n");
}

void set_active(spdio_t *io, char *arg) {
    DBG_LOG("set_active not implemented\n");
}