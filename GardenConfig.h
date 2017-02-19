#ifndef __GARDEN_CONFIG_H__
#define __GARDEN_CONFIG_H__


class GardenConfig
{

    public:
    char gatewayID[100];
    char wifiSSID[100];
    char wifiPassword[100];
    int utcOffset;


    virtual int loadConfig(const char * filename);
    virtual int saveConfig(const char * filename);

};

#endif
