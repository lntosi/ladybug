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

#ifdef _WIN32
#include "stdafx.h"
#endif

#include <stdlib.h>
#include <iostream>
#include <string>

#include "ladybug.h"

#define _HANDLE_ERROR \
    if( error != LADYBUG_OK ) \
   { \
   std::cout << \
   "Error: Ladybug library reported - " << \
   ::ladybugErrorToString( error ) << std::endl; \
   return EXIT_FAILURE; \
   } \
   \

LadybugError setGpsTimeSync(LadybugContext context, bool enable)
{

    GpsTimeSyncSettings timeSyncSettings;
    timeSyncSettings.baudRate = 9600;
    timeSyncSettings.enableGpsTimeSync = enable;
    timeSyncSettings.enablePps = enable;

    std::cout << std::endl  << "Set gps time sync" << std::endl;
    LadybugError error = ladybugSetGpsTimeSync(context, timeSyncSettings);
    if (error != LADYBUG_OK)
    {
        return error;
    }

    std::cout << "Checking gps time sync settings." << std::endl;
    GpsTimeSyncSettings retrievedSettings;
    error = ladybugGetGpsTimeSync(context, retrievedSettings);
    if (error != LADYBUG_OK)
    {
        return error;
    }

    std::cout << "Baud Rate - " << retrievedSettings.baudRate << std::endl;
    
    std::string displayEnabled = retrievedSettings.enableGpsTimeSync ? "True" : "False";
    std::cout << "Enable Time Sync - " << displayEnabled << std::endl;

    displayEnabled = retrievedSettings.enablePps ? "True" : "False";
    std::cout << "Enable PPS - " << displayEnabled << std::endl;

    return error;
}

int main()
{
    // Initialize context.
    LadybugContext context;
    LadybugError error = ::ladybugCreateContext(&context);
    _HANDLE_ERROR;

    // Initialize the first ladybug on the bus.
    std::cout << "Initializing..." << std::endl;
    error = ::ladybugInitializeFromIndex(context, 0);
    _HANDLE_ERROR;

    // Get camera info
    LadybugCameraInfo camInfo;
    error = ladybugGetCameraInfo(context, &camInfo);
    _HANDLE_ERROR;

    // Start up the camera according to device type and data format
    std::cout << "Starting " << camInfo.pszModelName << "(" << camInfo.serialHead << ")" << std::endl;

    // enable gps time sync
    error = setGpsTimeSync(context, true);
    _HANDLE_ERROR;

    error = ::ladybugStart(
        context,
        LADYBUG_DATAFORMAT_RAW8);
    _HANDLE_ERROR;

    std::cout << std::endl;
    LadybugImage image;

    // Frames captured within the first second will not contain GPS time sync info, as it will take a second to latch on to the PPS. 
    for (int i = 0; i < 500; i++)
    {
        std::cout << "Grabbing image - " << i << std::endl;
        error = ::ladybugGrabImage(context, &image);
        _HANDLE_ERROR;

        std::cout<< "GPS status: " << image.imageInfo.bGpsStatus << std::endl << "PPS status: "<< image.imageInfo.bPpsStatus << std::endl << "GPS fixing quality: " << image.imageInfo.ulGpsFixQuality << std::endl;
    }

    // disable gps time sync
    error = setGpsTimeSync(context, false);
    _HANDLE_ERROR("setGpsTimeSync(false)");

    // Frames captured here should not contain GPS time sync info, as the GPS time sync has been disabled.
    for (int i = 0; i < 500; i++)
    {
        std::cout << "Grabbing image - " << i << std::endl;
        error = ::ladybugGrabImage(context, &image);
        _HANDLE_ERROR;

        std::cout<< "GPS status: " << image.imageInfo.bGpsStatus << std::endl << "PPS status: "<< image.imageInfo.bPpsStatus << std::endl << "GPS fixing quality: " << image.imageInfo.ulGpsFixQuality << std::endl;
    }

    // Destroy the context
    std::cout << "Destroying context..." << std::endl;
    error = ::ladybugDestroyContext(&context);
    _HANDLE_ERROR("ladybugDestroyContext()");

    return EXIT_SUCCESS;
}

