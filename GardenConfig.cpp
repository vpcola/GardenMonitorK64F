#include "GardenConfig.h"
#include "Json.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

static int findVal(Json & parser, const char * key, char *value, int index)
{
    if ( !parser.isValidJson())
    {
        return -1;
    }
    if ( parser.type(0) != JSMN_OBJECT)
    {
        return -1;
    }

    // Read the station id
    int keyIndex = parser.findKeyIndexIn(key, index);
    if ( keyIndex == -1 )
    {
        return -1;
    }
    // Get the value
    int valueIndex = parser.findChildIndexOf(keyIndex, -1);
    if (valueIndex > 0)
    {
        const char * valueStart = parser.tokenAddress(valueIndex);
        uint32_t valueLength = parser.tokenLength(valueIndex);

        strncpy(value, valueStart, valueLength);
        value[valueLength] = 0; // Null terminate the string

        return 0;

    }

    return -1;

}

int GardenConfig::loadConfig(const char * filename)
{
    uint32_t sizeRead = 0;
    uint32_t fileSize = 0;

    // Read the whole json file and store it to the buffer
    FILE * fp = fopen(filename, "r");
    if (fp == NULL)
    {
        // Display error code
        return -1;
    }

    // Get the size of the file so we can estimate 
    // approximate buffer to allocate
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // allocate memory based on fileSize
    char * tmpMem = (char *) malloc(fileSize + 1);
    if (tmpMem == NULL) return -1;

    // Read the whole file ... 
    sizeRead = fread(tmpMem, 1, fileSize, fp);
    if ( sizeRead != fileSize )
    {
        // Can not read the whole file 
        free(tmpMem);

        return -1;
    }
    // Close the file
    fclose(fp);

    // Null terminate buffer
    tmpMem[sizeRead + 1] = 0;

    // memory buffer now has data
    Json parser(tmpMem, sizeRead);

    // get the gateway ID
    if (findVal(parser, "gateway_id", &gatewayID[0], 0) < 0)
    {
        printf("Can not find gateway_id settings from config file ...\r\n");
        strcpy(gatewayID, "AA00");
    }

    if (findVal(parser, "wifi_ssid", &wifiSSID[0], 0) < 0)
    {
        printf("Can not find wifi_ssid settings from the config file ...\r\n");
        return -1;
    }

    if ( findVal(parser, "wifi_password", &wifiPassword[0], 0) < 0)
    {
        printf("Can not find wifi_password settings from the config file ...\r\n");
        return -1;
    }

    char utcstr[50];
    if (findVal(parser, "utc_offset", &utcstr[0], 0) < 0)
    {
        printf("Can not find UTC offset value\r\n");
        return -1;
    }
    utcOffset = atoi((const char *) &utcstr[0]);

    // Free the temporary memory
    free(tmpMem);

    return 0;
}

int GardenConfig::saveConfig(const char * filename)
{
    return 0;
}

