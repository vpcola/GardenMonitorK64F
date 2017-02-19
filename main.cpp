#include "mbed.h"
#include "rtos.h"
#include "EthernetInterface.h"
#include "SDFileSystem.h"
#include "NTPClient.h"
#include "GardenConfig.h"
#include "XBeeLib.h"
#include "DigiLoggerMbedSerial.h"
#include <math.h>
#include "SPI_TFT_ILI9341.h"
#include "SPI_STMPE610.h"
#include "easy-connect.h"
#include "https_request.h"


#include "Arial12x12.h"
#include "Arial24x23.h"
#include "Arial28x28.h"
#include "font_big.h"

using namespace DigiLog;
using namespace XBeeLib;

// For FRDM-K64F
#define PIN_XP          PTB11
#define PIN_XM          PTB3
#define PIN_YP          PTB10
#define PIN_YM          PTB2
#define PIN_MOSI        PTD2
#define PIN_MISO        PTD3 
#define PIN_SCLK        PTD1 
#define PIN_CS_TFT      PTD0 
#define PIN_DC_TFT      PTC4 
#define PIN_BL_TFT      PTC3 
#define PIN_CS_SD       PTB23 
#define PIN_CS_TSC      PTC12
#define PIN_TSC_INTR    PTC3
#define PIN_BACKLIGHT   PTA1


// EthernetInterface eth;
Serial pc(USBTX, USBRX, 115200);
DigitalOut led1(LED1);
DigitalOut led2(LED2);
SDFileSystem sd(PTE3, PTE1, PTE2, PTE4, "sd");
XBee802 xbee(PTC15, PTC14, NC, NC, NC, 115200); // local xbee
//XBee802 xbee(D1, D0, D2, NC, NC, 115200); // local xbee
SPI_TFT_ILI9341 TFT(PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS_TFT, PIN_BL_TFT, PIN_DC_TFT) ;
SPI_STMPE610 TSC(PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_CS_TSC);
DigitalOut backlight(PIN_BACKLIGHT) ;
DigitalIn pinD7(PIN_TSC_INTR) ;

GardenConfig cfg;

Thread blinkerThd;
Thread xbeeThd(osPriorityNormal, DEFAULT_STACK_SIZE * 2);

/*-----------------------------------------------------------------------------*/
/* Callback functions                                                          */
/*-----------------------------------------------------------------------------*/

static void io_data_cb(const RemoteXBee802& remote, const IOSample802& sample_data)
{
    RadioStatus radioStatus;
    pc.printf("\r\n================ Radio Callback ==================\r\n");
    if (remote.is_valid_addr16b()) {
        pc.printf("\r\nGot a 16-bit IO Sample Data [%04X]\r\n", remote.get_addr16());
    } else {
        const uint64_t remote_addr64 = remote.get_addr64();
        pc.printf("\r\nGot a 64-bit IO Sample Data %lld[%08x:%08x]\r\n", remote.get_addr64(), UINT64_HI32(remote_addr64), UINT64_LO32(remote_addr64));
    }

    // TODO : Process received frames

}

/*------------------------------------------------------------------------------*/
/* Threads                                                                      */
/*------------------------------------------------------------------------------*/

// The xbee listener thread
void XbeeListenerThd()
{
    while(true)
    {
        xbee.process_rx_frames();
        wait_ms(100);
        led2 = !led2;
    }
}

void Blinker()
{
    while(true)
    {
        led1 = !led1;
        Thread::wait(1000);
    }
}
/*-------------------------------------------------------------------------------*/
/* Initialization Functions                                                      */
/*-------------------------------------------------------------------------------*/
void initTFT(void)
{
    //Configure the display driver
    TFT.background(Black);
    TFT.foreground(White);
    wait(0.01) ;
    TFT.cls();
		TFT.claim(stdout);
}

int InitializeRadios()
{
    /* Register callback */
    xbee.register_io_sample_cb(&io_data_cb);

    // Initialize device. Read the relevant parameters.
    pc.printf("Initializing XBee radio ...\r\n");
    RadioStatus radioStatus = xbee.init();
    if (radioStatus != Success) {
        pc.printf("Initialization failed (are the TX/RX pins and the baudrate properly configured?)\r\n");
        return -1;
    }
    pc.printf("XBee radio initialized ...\r\n");

    return 0;
}

int main()
{
    int i = 0;

    pc.printf("Starting blinker ...\r\n");
    blinkerThd.start(Blinker);

    initTFT() ;

    // Get the network interface via 
    // easy connect
    NetworkInterface* network = easy_connect(true);
    if (!network) {
        return 1;
    }

    //pc.printf("Connecting to network ...\r\n");
    //network->connect();

    pc.printf("IP Address : %s\r\n", network->get_ip_address());
    pc.printf("MAC Address : %s\r\n", network->get_mac_address());

    time_t ctTime;
    ctTime = time(NULL);

    pc.printf("Loading config from SD Card...\r\n");
    cfg.loadConfig("/sd/config.json");
    pc.printf("Gateway ID : %s\r\n", cfg.gatewayID);
    pc.printf("UTC Offset : %d\r\n", cfg.utcOffset);

    new DigiLoggerMbedSerial(&pc, LogLevelInfo);
    pc.printf("Initializing Xbee radios ...\r\n");
    InitializeRadios();
    pc.printf("Running the xbee listener thread ...\r\n");
    xbeeThd.start(XbeeListenerThd);

    NTPClient ntp(*network);
    pc.printf("Initial System Time is: %s\r\n", ctime(&ctTime));
    pc.printf("Trying to update time...\r\n");
    if (ntp.setTime("0.pool.ntp.org") == 0)
    {
       printf("Set time successfully\r\n");
       while(true)
       {
        ctTime = time(NULL) + (cfg.utcOffset * 60 * 60);
        pc.printf("Time : %s\r", ctime(&ctTime));
        Thread::wait(2000);
       }
    }

    while(true)
    {
        pc.printf("Iteration %d\r\n", i);
        i++;
        Thread::wait(2000);
    }
}

