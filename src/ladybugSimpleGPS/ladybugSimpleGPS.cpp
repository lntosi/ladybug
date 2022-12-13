//=============================================================================
// Copyright (c) 2001-2018 FLIR Systems, Inc. All Rights Reserved.
//
// This software is the confidential and proprietary information of FLIR
// Integrated Imaging Solutions, Inc. ("Confidential Information"). You
// shall not disclose such Confidential Information and shall use it only in
// accordance with the terms of the license agreement you entered into
// with FLIR Integrated Imaging Solutions, Inc. (FLIR).
//
// FLIR MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
// SOFTWARE, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE, OR NON-INFRINGEMENT. FLIR SHALL NOT BE LIABLE FOR ANY DAMAGES
// SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING
// THIS SOFTWARE OR ITS DERIVATIVES.
//=============================================================================

//=============================================================================
//
// ladybugSimpleGPS.cpp
// 
// This example shows users how to use a GPS device in conjunction with a
// Ladybug camera in order to combine GPS data with Ladybug images.
//
// Before running this example, you need to know what COM port the GPS device
// is mapped to. This is the case even if the GPS device uses a USB
// interface. To do this, right click on the "My Computer" icon and click the
// "Device Manager" button under the Hardware tab. Expand the "Ports (COM &
// LPT)" node and note the COM port that the GPS device is mapped to.
//
//=============================================================================

//=============================================================================
// System Includes
//=============================================================================

#ifdef _WIN32

#include <windows.h>

#else

#include <unistd.h>

#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>

//=============================================================================
// PGR Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "ladybug.h"
#include "ladybugGPS.h"
#include "ladybugstream.h"

#define _HANDLE_ERROR \
    if( error != LADYBUG_OK ) \
   { \
   printf( \
   "Error! Ladybug library reported %s\n", \
   ::ladybugErrorToString( error ) ); \
   return EXIT_FAILURE; \
   } \
   \

// Echo program usage
void usage()
{

#ifdef _WIN32
    printf(
        "Ladybug Simple GPS Example\n"
        "This example shows how to use a GPS device in conjunction with a\n"
        "Ladybug camera.\n\n"
        "Usage: ladybugSimpleGPS [numberOfImages] [comPortNumber] [baudRate] [updateInterval]\n"
        "\n"
        "where\n"
        "\t numberOfImages - Number of images to grab\n"
        "\t comPortNumber - COM port that the GPS device is connected to \n"
        "\t baudRate - Baud rate of the GPS device \n"
        "\t updateInterval - Update interval of the GPS device (in ms)\n\n"
        "Example: ladybugSimpleGPS 100 1 115200 1000\n"
        "This will start the application with a GPS device on COM port 1, a\n"
        "baud rate of 115200 and update data every second (1000ms) for 100 images.\n\n"
        "Note that all of the arguments are required.\n\n"
        );
#else
    printf(
        "Ladybug Simple GPS Example\n"
        "This example shows how to use a GPS device in conjunction with a\n"
        "Ladybug camera.\n\n"
        "Usage: LadybugSimpleGPS [numberOfImages] [devicePath] [baudRate] [updateInterval]\n"
        "\n"
        "where\n"
        "\t numberOfImages - Number of images to grab\n"
        "\t devicePath - Path and name of the GPS device you want to connect to \n"
        "\t baudRate - Baud rate of the GPS device \n"
        "\t updateInterval - Update interval of the GPS device (in ms)\n\n"
        "Example: ladybugSimpleGPS 100 /dev/ttyACM0 115200 1000\n"
        "This will start the application with the GPS device /dev/ttyACM0, a\n"
        "baud rate of 115200 and update data every second (1000ms) for 100 images.\n\n"
        "Note that all of the arguments are required.\n\n"
        );
#endif

}

std::string GeneratePositionString(double lat, double lon, double alt)
{
    std::stringstream output;
    if ( lat <= 0 && lon <= 0 )
    {
        // North of equator, East of Prime Meridian
        output << "N: " << lat << " E: " << lon << " A: " << alt << std::endl;
    }
    else if ( lat <= 0 && lon >= 0 )
    {
        // South of equator, East of Prime Meridian
        output << "S: " << lat << " E: " << lon << " A: " << alt << std::endl;
    }
    else if ( lat >= 0 && lon <= 0 )
    {
        // North of equator, West of Prime Meridian
        output << "N: " << lat << " W: " << lon << " A: " << alt << std::endl;
    }
    else if ( lat >= 0 && lon >= 0 )
    {
        // South of equator, West of Prime Meridian
        output << "S: " << lat << " W: " << lon << " A: " << alt << std::endl;
    } 

    return output.str();
}

std::string GeneratePositionString(const LadybugNMEAGPGGA& gga)
{
    std::stringstream output;
    output << (int)gga.ucGGAHour << ":" << (int)gga.ucGGAMinute << ":" << (int)gga.ucGGASecond << "." << (int)gga.wGGASubSecond;
    output << " - " << GeneratePositionString(gga.dGGALatitude, gga.dGGALongitude, gga.dGGAAltitude);

    return output.str();
}

void Method1(const LadybugImage& image, int i )
{
    // 1. Get the GPGGA NMEA sentence from the LadybugImage
    //
    // Although we can get image data from the LadybugImageInfo structure
    // contained in every LadybugImage, the advantage of this method is that 
    // it allows us to use the bValidData boolean to check if the data is 
    // valid.

    LadybugNMEAGPGGA gga;
    LadybugError gpsError = ladybugGetGPSNMEADataFromImage( &image, "GPGGA", &gga );

    std::stringstream output;
    output << i << ".1 --> ";
    if ( ( gpsError == LADYBUG_OK ) && ( gga.bValidData == true ) )     
    {
        output << GeneratePositionString(gga);              
    }
    else
    {
        if (gpsError != LADYBUG_OK)
        {
            output << "LadybugError: " << ladybugErrorToString(gpsError) << std::endl;
        }
        else
        {
            output << "GPS data is invalid" << std::endl;
        }        
    }
   
    printf("%s", output.str().c_str());
}

void Method2( LadybugGPSContext GPScontext, int i )
{
    // 2. Get a NMEA sentence from the GPS device
    //
    // This is useful if you just want to get the NMEA data from the device
    // without grabbing images.

    LadybugNMEAGPGGA gga;
    LadybugError gpsError = ladybugGetGPSNMEAData( GPScontext, "GPGGA", &gga );

    std::stringstream output;
    output << i << ".2 --> ";
    if ( ( gpsError == LADYBUG_OK ) && ( gga.bValidData == true ) )     
    {
        output << GeneratePositionString(gga);              
    }
    else
    {
        if (gpsError != LADYBUG_OK)
        {
            output << "LadybugError: " << ladybugErrorToString(gpsError) << std::endl;
        }
        else
        {
            output << "GPS data is invalid" << std::endl;
        }        
    }

    printf("%s", output.str().c_str());
}

void Method3(const LadybugImage &image, int i )
{
    // 3. Get GPS data from LadybugImageInfo structure
    //
    // When this method is used, a comparison against LADYBUG_INVALID_GPS_DATA
    // has to be made to determine if the data is valid (unlike the previous
    // methods). See below for an example.

    std::stringstream output;
    output << i << ".3 --> ";
    if (image.imageInfo.dGPSLatitude != LADYBUG_INVALID_GPS_DATA &&
        image.imageInfo.dGPSLongitude != LADYBUG_INVALID_GPS_DATA &&
        image.imageInfo.dGPSAltitude != LADYBUG_INVALID_GPS_DATA)
    {
        output << GeneratePositionString(image.imageInfo.dGPSLatitude, image.imageInfo.dGPSLongitude, image.imageInfo.dGPSAltitude);
    }
    else
    {
        output << "No GPS Position" << std::endl;
    }

    printf("%s", output.str().c_str());
}

int 
main( int argc, char* argv[] )
{
    LadybugContext context;
    LadybugGPSContext GPScontext;
    LadybugError error;
    LadybugCameraInfo caminfo;

    int nImages;
#ifdef _WIN32

        unsigned int uiGPSPortNumber;

#else 

    std::string gpsDeviceName;

#endif 

    unsigned int uiGPSBaudRate;
    unsigned int uiGPSUpdateInterval;

    if ( argc != 5 )
    {
        usage();
        printf("<PRESS ENTER TO EXIT>");
        getchar();
        return 0;
    }   

    nImages = atoi( argv[1] );

#ifdef _WIN32

    uiGPSPortNumber = atoi( argv[2] );

#else 

    gpsDeviceName = argv[2];

#endif 

    uiGPSBaudRate = atoi( argv[3] );
    uiGPSUpdateInterval = atoi( argv[4] );

    // Create Ladybug context.
    printf( "Creating Ladybug context...\n" );
    error = ladybugCreateContext( &context );
    _HANDLE_ERROR;

    // Create GPS context
    printf( "Creating GPS context...\n" );
    error = ladybugCreateGPSContext( &GPScontext );
    _HANDLE_ERROR;

    // Register the GPS context with the Ladybug context
    printf( "Registering GPS...\n" );
    error = ladybugRegisterGPS( context, &GPScontext );
    _HANDLE_ERROR;

    // Initialize the first ladybug on the bus.
    printf( "Initializing camera...\n" );
    error = ladybugInitializeFromIndex( context, 0 );
    _HANDLE_ERROR;

    // Initialize GPS context with the supplied settings
    printf( "Initializing GPS...\n" );

#ifdef _WIN32
        error = ladybugInitializeGPS(GPScontext, uiGPSPortNumber, uiGPSBaudRate, uiGPSUpdateInterval);
#else
        error = ladybugInitializeGPSEx(GPScontext, gpsDeviceName.c_str(), uiGPSBaudRate, uiGPSUpdateInterval);
#endif

    _HANDLE_ERROR;

// Echo settings
#ifdef _WIN32

    char pszGPSInitText[256] = {0};
    sprintf( pszGPSInitText,
        "GPS initialized with the following settings\n"
        "\tCOM Port: %u\n"
        "\tBaud Rate: %u\n"
        "\tUpdate Interval: %ums\n",
        uiGPSPortNumber,
        uiGPSBaudRate,
        uiGPSUpdateInterval );
    printf( pszGPSInitText );

#else

    char pszGPSInitText[256] = {0};
    sprintf( pszGPSInitText,
        "GPS initialized with the following settings\n"
        "\tDevice Path: %s\n"
        "\tBaud Rate: %u\n"
        "\tUpdate Interval: %ums\n",
        gpsDeviceName.c_str(),
        uiGPSBaudRate,
        uiGPSUpdateInterval );
    printf( pszGPSInitText );

#endif


    // Get camera info
    printf( "Getting camera info...\n" );
    error = ladybugGetCameraInfo( context, &caminfo );
    _HANDLE_ERROR;

    switch ( caminfo.deviceType)
    {
    case LADYBUG_DEVICE_COMPRESSOR:
    case LADYBUG_DEVICE_LADYBUG3:
    case LADYBUG_DEVICE_LADYBUG5:
    case LADYBUG_DEVICE_LADYBUG5P:
        //
        // Start up the camera
        //
        printf( "Starting camera...\n" );
        error = ladybugStart(
            context,
            LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8);
        _HANDLE_ERROR;
        break;

    default:
        printf( "Unknown camera detected!\n" );      
        return 0;
    }


    // Start GPS
    error = ladybugStartGPS( GPScontext );
    _HANDLE_ERROR;

    // Let the Ladybug and GPS get synchronized
    // Starting the grab immediately generally results in the initial
    // (around 30) images not having GPS data
    printf( "Waiting for 3 seconds...\n" );
    std::this_thread::sleep_for(std::chrono::seconds(3));

    for ( int i=0; i < nImages; i++ )
    {
        // Grab a single image.
        LadybugImage image; 
        error = ladybugGrabImage( context, &image );
        if ( error != LADYBUG_OK )
        {
            printf( "Error grabbing image\n" );
            continue;
        }   

        // There are several alternative ways of getting the GPS data at
        // this point:
        //
        // 1. Get a NMEA sentence from the LadybugImage.
        // 2. Get a NMEA sentence from the GPS device.
        // 3. Get the latitude, longitude and altitude from the LadybugImageInfo
        //    structure in the returned LadybugImage.
        //
        // The 3 methods are executed below.

        Method1(image, i);
        Method2(GPScontext, i);
        Method3(image, i);              
    }

    // Stop GPS
    printf( "Stopping GPS\n" );
    error = ladybugStopGPS( GPScontext );

    // Unregister GPS context
    printf( "Unregistering GPS context...\n" );
    error = ladybugUnregisterGPS( context, &GPScontext );
    _HANDLE_ERROR;

    // Destroy GPS context
    printf( "Destroying GPS context...\n" );
    error = ladybugDestroyGPSContext( &GPScontext );
    _HANDLE_ERROR;

    // Destroy Ladybug context
    printf( "Destroying Ladybug context...\n" );
    error = ladybugDestroyContext( &context );
    _HANDLE_ERROR; 

    printf( "Done.\n" );

    return 0;
}
