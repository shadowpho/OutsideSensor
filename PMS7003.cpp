#include "PMS7003.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <cassert>
#include "i2c_helper.h"
    
#define MAGIC1 0x42
#define MAGIC2 0x4D

typedef enum
{
    BEGIN1,
    BEGIN2,
    LENGTH1,
    LENGTH2,
    DATA,
    CHECK1,
    CHECK2
} EState;

typedef struct
{
    EState state;
    uint8_t buf[32];
    const int size = 32;
    int idx, len;
    uint16_t chk, sum;
} TState;

static TState state;
static int serialport_FD;

void setup_PMS7003()
{
    state.state = BEGIN1;
    state.idx = state.len = 0;
    state.chk = state.sum = 0;
    serialport_FD = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serialport_FD == -1)
    {
        perror("Unable to open serial port!");
        return;
    }
    fcntl(serialport_FD, F_SETFL, FNDELAY); //set it to no block!
    struct termios tty
    {
    };
    tcgetattr(serialport_FD, &tty);
    cfsetospeed(&tty, 9600);
    cfsetispeed(&tty, 9600);
    tty.c_iflag &= ~IGNBRK;                     // disable break processing
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);     // shut off xon/xoff ctrl
    tty.c_lflag = 0;                            // no signaling chars, no echo,
                                                // no canonical processing
    tty.c_oflag = 0;                            // no remapping, no delays
    tty.c_cc[VMIN] = 0;                         // read doesn't block
    tty.c_cc[VTIME] = 0;                        //

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls,
                                       // enable reading
    tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
    tty.c_cflag |= 0;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    if (tcsetattr(serialport_FD, TCSANOW, &tty) != 0)
    {
        printf("error %d from tcsetattr", errno);
    }
}

static uint16_t get(uint8_t *buf, int idx)
{
    uint16_t data;
    data = buf[idx] << 8;
    data += buf[idx + 1];
    return data;
}

bool PmsProcess(uint8_t b)
{
    switch (state.state)
    {
    // wait for BEGIN1 byte
    case BEGIN1:
        if (b == MAGIC1)
        {
            state.sum = b;
            state.state = BEGIN2;
        }
        break;
    // wait for BEGIN2 byte
    case BEGIN2:
        if (b == MAGIC2)
        {
            state.sum += b;
            state.state = LENGTH1;
        }
        else
        {
            state.state = BEGIN1;
            // retry
            return PmsProcess(b);
        }
        break;
    // verify data length
    case LENGTH1:
        state.sum += b;
        state.len = b << 8;
        state.state = LENGTH2;
        break;
    case LENGTH2:
        state.sum += b;
        state.len += b;
        state.len -= 2; // exclude checksum bytes
        if (state.len <= state.size && state.len > 0)
        {
            state.idx = 0;
            state.state = DATA;
        }
        else
        {
            // bogus length
            state.state = BEGIN1;
        }
        break;
    // store data
    case DATA:
        state.sum += b;
        state.buf[state.idx++] = b;
        if (state.idx == state.len)
        {
            state.state = CHECK1;
        }
        break;
    // store checksum
    case CHECK1:
        state.chk = b << 8;
        state.state = CHECK2;
        break;
    // verify checksum
    case CHECK2:
        state.chk += b;
        state.state = BEGIN1;
        return state.chk == state.sum;
    }
    return false;
}

void read_from_PMS(uint16_t *pm10, uint16_t *pm25, uint16_t *pm1)
{
    uint8_t buf[state.size];
    while(1)
    {
    int read_chars = read(serialport_FD, buf, sizeof(buf));
    for (int i = 0; i < read_chars; i++)
        if (PmsProcess(buf[i]))
        {
            *pm10 = get(state.buf, 4);
            *pm25 = get(state.buf, 2);
            *pm1  = get(state.buf, 0);
            return;
        }
    sleep_ms(300);
    }
            
}
