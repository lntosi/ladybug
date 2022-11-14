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
// ladybugPanoStitchExample.cpp
// 
// This example shows users how to extract an image set from a Ladybug camera,
// stitch it together and write the final stitched image to disk.
//
// Since Ladybug library version 1.3.alpha.01, this example is modified to use
// ladybugRenderOffScreenImage(), which is hardware accelerated, to render the
// the stitched images.
//
// Typing ladybugPanoStitchExample /? (or ? -?) at the command prompt will  
// print out the usage information of this application.
//
//=============================================================================

//=============================================================================
// System Includes
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>

#ifdef _WIN32

#include <windows.h>
#include <shlobj.h>
#include <conio.h>

#else

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#endif

//=============================================================================
// PGR Includes
//=============================================================================
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>
#include <ladybugstream.h>

//=============================================================================
// Macro Definitions
//=============================================================================

#define _HANDLE_ERROR \
    if( error != LADYBUG_OK ) \
    { \
    printf( "Error! Ladybug library reported %s\n", \
    ::ladybugErrorToString( error ) ); \
    assert( false ); \
    goto _EXIT; \
    } 

#define IMAGES_TO_GRAB            10

// The size of the stitched image
#define PANORAMIC_IMAGE_WIDTH    2048
#define PANORAMIC_IMAGE_HEIGHT   1024 

#define COLOR_PROCESSING_METHOD   LADYBUG_DOWNSAMPLE4     // The fastest color method suitable for real-time usages
//#define COLOR_PROCESSING_METHOD   LADYBUG_HQLINEAR          // High quality method suitable for large stitched images

namespace 
{
    std::string getWriteableDirectory()
    {
        std::string writeableDirectory;
#ifdef _WIN32
        char buf[_MAX_PATH];
        HRESULT res = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, buf);
        writeableDirectory.append(buf);
        if (res == S_OK)
        {
            writeableDirectory.append("\\");
        }
#else
        const char *homedir;

        if ((homedir = getenv("HOME")) == NULL)
        {
            uid_t uid = getuid();
            struct passwd *pw = getpwuid(uid);

            if (pw == NULL) 
            {
                homedir = NULL;
            }
            else
            {
                homedir = pw->pw_dir;
            }
        }
        if (homedir != NULL)
        {
            writeableDirectory.append(homedir).append("/");
        }
#endif
        return writeableDirectory;
    }
}


//
// This function reads an image from camera
//
LadybugError
startCamera( LadybugContext context)
{    
    // Initialize camera
    printf( "Initializing camera...\n" );
    LadybugError error = ladybugInitializeFromIndex( context, 0 );
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Get camera info
    printf( "Getting camera info...\n" );
    LadybugCameraInfo caminfo;
    error = ladybugGetCameraInfo( context, &caminfo );
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Load config file
    printf( "Load configuration file...\n" );
    error = ladybugLoadConfig( context, NULL);
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Set the panoramic view angle
    error = ladybugSetPanoramicViewingAngle( context, LADYBUG_FRONT_0_POLE_5);
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Make the rendering engine use the alpha mask
    error = ladybugSetAlphaMasking( context, true );
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Set color processing method.
    error = ladybugSetColorProcessingMethod( context, COLOR_PROCESSING_METHOD );
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Configure output images in Ladybug library
    printf( "Configure output images in Ladybug library...\n" );
    error = ladybugConfigureOutputImages( context, LADYBUG_PANORAMIC );
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    error = ladybugSetOffScreenImageSize(
        context, 
        LADYBUG_PANORAMIC,  
        PANORAMIC_IMAGE_WIDTH, 
        PANORAMIC_IMAGE_HEIGHT );  
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    switch( caminfo.deviceType )
    {
    case LADYBUG_DEVICE_COMPRESSOR:
    case LADYBUG_DEVICE_LADYBUG3:
    case LADYBUG_DEVICE_LADYBUG5:
    case LADYBUG_DEVICE_LADYBUG5P:
        printf( "Starting Ladybug camera...\n" );
        error = ladybugStart(
            context,
            LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8);
        break;

    case LADYBUG_DEVICE_LADYBUG:
    default:
        printf( "Unsupported device.\n");
        error = LADYBUG_FAILED;
    }

    return error;
}

//=============================================================================
// Main Routine
//=============================================================================
int 
main( int /*argc*/, char* /*argv[]*/ )
{
    LadybugContext context = NULL;
    LadybugError error;
    unsigned int uiRawCols = 0;
    unsigned int uiRawRows = 0;
    int retry = 10;
    LadybugImage image;

    // create ladybug context
    printf( "Creating ladybug context...\n" );
    error = ladybugCreateContext( &context );
    if ( error != LADYBUG_OK )
    {
        printf( "Failed creating ladybug context. Exit.\n" );
        return (1);
    }

    // Initialize and start the camera
    error = startCamera( context);
    _HANDLE_ERROR

    // Grab an image to inspect the image size
    printf( "Grabbing image...\n" );
    error = LADYBUG_FAILED;    
    while ( error != LADYBUG_OK && retry-- > 0)
    {
        error = ladybugGrabImage( context, &image ); 
    }
    _HANDLE_ERROR

#pragma warning(push)
#pragma warning(disable:4127)

    // Set the size of the image to be processed
    if (COLOR_PROCESSING_METHOD == LADYBUG_DOWNSAMPLE4 || 
        COLOR_PROCESSING_METHOD == LADYBUG_MONO)
    {
        uiRawCols = image.uiCols / 2;
        uiRawRows = image.uiRows / 2;
    }
    else
    {
        uiRawCols = image.uiCols;
        uiRawRows = image.uiRows;
    }

#pragma warning(pop)

    // Initialize alpha mask size - this can take a long time if the
    // masks are not present in the current directory.
    printf( "Initializing alpha masks (this may take some time)...\n" );
    error = ladybugInitializeAlphaMasks( context, uiRawCols, uiRawRows );
    _HANDLE_ERROR

    // Process loop
    printf( "Grab loop...\n" );
    for (int i = 0; i < IMAGES_TO_GRAB; i++)
    {
        printf( "Processing image %d...\n", i);

        // Grab an image from the camera
        error = ladybugGrabImage(context, &image); 
        _HANDLE_ERROR

        // Convert the image to 6 RGB buffers
        error = ladybugConvertImage(context, &image, NULL);
        _HANDLE_ERROR

        // Send the RGB buffers to the graphics card
        error = ladybugUpdateTextures(context, LADYBUG_NUM_CAMERAS, NULL);
        _HANDLE_ERROR

        // Stitch the images (inside the graphics card) and retrieve the output to the user's memory
        LadybugProcessedImage processedImage;
        error = ladybugRenderOffScreenImage(context, LADYBUG_PANORAMIC, LADYBUG_BGR, &processedImage);
        _HANDLE_ERROR

        // Save the output image to a file
        char pszOutputName[256];

        sprintf( pszOutputName, "PanoStitchOutput_%03d.jpg", i);
        const std::string outputPath = getWriteableDirectory() + std::string(pszOutputName);
        printf("Writing image %s...\n", outputPath.c_str());

        error = ladybugSaveImage( context, &processedImage, outputPath.c_str(), LADYBUG_FILEFORMAT_JPG );
        _HANDLE_ERROR
    }

    printf("Done.\n");

_EXIT:
    //
    // clean up
    //

    ladybugStop( context );
    ladybugDestroyContext( &context );

    printf("<PRESS ENTER TO EXIT>");

    getchar();

    return 0;
}

