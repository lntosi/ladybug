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
// ladybugTriggerEx.cpp
// 
// This example shows users how to use the triggering functionality using
// Ladybug SDK.
// In this example, the camera is set to trigger mode 0 and the source of the
// trigger is set to software.
//=============================================================================

//
// Includes
//

#include "stdafx.h"

#ifdef _WIN32

#include <windows.h>

#else

#include <unistd.h>

#endif

#include <stdio.h>
#include <ladybug.h>
#include <chrono>
#include <thread>

//
// Macros for customization
//

// If you are not using software trigger, you'll need to trigger the camera separately.
// You can use the external trigger for Ladybug3, or other software to fire software trigger.
//#define USE_SOFTWARE_TRIGGER

#define IMAGES_TO_CAPTURE 10

//
// Internal macros
//
#define HANDLE_ERROR	if ( error != LADYBUG_OK) throw error
#define SOFTWARE_TRIGGER 0x62C
#define SOFTWARE_TRIGGER_FIRE_VAL 0x80000000

#define COLOR_PROCESSING_METHOD LADYBUG_DOWNSAMPLE4

//
// Global variables
//
unsigned char*    arpBuffers[ LADYBUG_NUM_CAMERAS ];

//
// Helper functions
//

void displayTriggerModeInfo( LadybugTriggerModeInfo *triggerModeInfo)
{
   printf( "--- Trigger mode info ---\n");
   printf( " bPresent %d\n", triggerModeInfo->bPresent);
   printf( " bOnOffSupported %d\n", triggerModeInfo->bOnOffSupported);
   printf( " bPolaritySupported %d\n", triggerModeInfo->bPolaritySupported);
   printf( " bReadOutSupported %d\n", triggerModeInfo->bReadOutSupported);
   printf( " bSoftwareTriggerSupported %d\n", triggerModeInfo->bSoftwareTriggerSupported);
   printf( " bValueReadable %d\n", triggerModeInfo->bValueReadable);
   printf( " uiModeMask %X\n", triggerModeInfo->uiModeMask);
   for ( int i = 0; i < 16; i++){
      if ( ( ( triggerModeInfo->uiModeMask >> i) & 0x01) == 1){
         printf( "  supports trigger mode %d\n", 15 - i);
      }
   }
   printf( " uiSourceMask %X\n", triggerModeInfo->uiSourceMask);
}

void displayTriggerMode( LadybugTriggerMode *triggerMode)
{
   printf( "--- Trigger mode ---\n");
   printf( " bOnOff %d\n", triggerMode->bOnOff);
   printf( " uiMode %d\n", triggerMode->uiMode);
   printf( " uiParameter %d\n", triggerMode->uiParameter);
   printf( " uiPolarity %d\n", triggerMode->uiPolarity);
   printf( " uiSource %d\n", triggerMode->uiSource);
}

bool fireSoftwareTrigger( LadybugContext context )
{
   LadybugError error;    

   error = ladybugSetRegister( context, SOFTWARE_TRIGGER, SOFTWARE_TRIGGER_FIRE_VAL );
   if (error != LADYBUG_OK)
   {
     return false;
   }

   return true;
}

bool saveImage( LadybugContext context, LadybugImage *image, int index)
{
   LadybugError error = ladybugConvertImage( context, image, arpBuffers );

   char pszOutputFilePath[ 256];
   for( int uiCamera = 0; uiCamera < 1 /*LADYBUG_NUM_CAMERAS*/; uiCamera++ ) // saving only the camera 0 image.
   {
      LadybugProcessedImage processedImage;
      processedImage.pData = arpBuffers[ uiCamera ];
      processedImage.pixelFormat = LADYBUG_BGRU;
      processedImage.uiCols = image->uiCols/2; // half size because LADYBUG_DOWNSAMPLE4 is used.
      processedImage.uiRows = image->uiRows/2;

      sprintf( pszOutputFilePath, "ladybug_image_camera_%02d_%02d.bmp", index, uiCamera);
      error = ladybugSaveImage( context, &processedImage, pszOutputFilePath, LADYBUG_FILEFORMAT_BMP); 
      printf( "Output to file %s.\n", pszOutputFilePath);
   }

   return true;
}

//
// The Main
//
int main(int /*argc*/, char* /*argv[]*/)
{
   LadybugContext context;
   LadybugError error;
   LadybugImage image;
   int i;

   try{
      error = ladybugCreateContext( &context);
      HANDLE_ERROR;

      error = ladybugInitializeFromIndex( context, 0);
      HANDLE_ERROR;

      LadybugTriggerModeInfo triggerModeInfo;
      LadybugTriggerMode triggerMode;

      error = ladybugGetTriggerModeInfo( context, &triggerModeInfo );
      HANDLE_ERROR;

      displayTriggerModeInfo( &triggerModeInfo);

      error = ladybugGetTriggerMode( context, &triggerMode);
      HANDLE_ERROR;

      // Set camera to trigger mode 0
      // A source of 7 means software trigger
      triggerMode.bOnOff = true;
      triggerMode.uiSource = 7;
      triggerMode.uiMode = 0;
      triggerMode.uiParameter = 0;
      triggerMode.uiPolarity = 0;

      displayTriggerMode( &triggerMode);

      error = ladybugSetColorProcessingMethod( context, COLOR_PROCESSING_METHOD );
      HANDLE_ERROR;

      error = ladybugSetGrabTimeout( context, 0);
      HANDLE_ERROR;

      // Start streaming
      error = ladybugStart( context, LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8);
      HANDLE_ERROR;

      // Grab an image to inspect the image size.
      printf( "Grabbing an image...\n");
      error = LADYBUG_FAILED;
      for ( i = 0; i < 10 && error != LADYBUG_OK; i++)
      {
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
         error = ladybugGrabImage( context, &image );
      }
      HANDLE_ERROR;
      printf( "Grabbing an image - done.\n");

      error = ladybugStop( context);
      HANDLE_ERROR;

#pragma warning(push)
#pragma warning(disable:4127)

      // Allocate memory for the 6 processed images
      unsigned int uiRawCols = 0, uiRawRows = 0;
      if( COLOR_PROCESSING_METHOD == LADYBUG_DOWNSAMPLE4 || 
          COLOR_PROCESSING_METHOD == LADYBUG_MONO )
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

      printf( "Allocate memory for the 6 processed images...\n");
      for( int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
      {
          arpBuffers[ uiCamera ] = new unsigned char[ uiRawRows * uiRawCols * 4 ];
      }

      error = ladybugSetTriggerMode( context, &triggerMode);
      HANDLE_ERROR;

      // Since Ladybug SDK version 1.6.0.1, This function needs to be called prior 
      // to the camera starting functions.
      error = ladybugSetGrabTimeout( context, LADYBUG_INFINITE);
      HANDLE_ERROR;

      // Start streaming
      error = ladybugStart( context, LADYBUG_DATAFORMAT_ANY);
      HANDLE_ERROR;

      printf ("Grab loop...\n");
      for ( i = 0; i < IMAGES_TO_CAPTURE; i++)
      {
         printf ("Waiting for a trigger...\n");

#ifdef USE_SOFTWARE_TRIGGER
         printf( "Firing software trigger.(%d)\n", i);
         if ( !fireSoftwareTrigger( context)){
            printf( "Error in firing software trigger.\n");
            break;
         }
#endif
         error = ladybugGrabImage( context, &image);
         HANDLE_ERROR;
         printf( "Image grabbed.\n");

         saveImage( context, &image, i);
      }

      error = ladybugStop( context);
      HANDLE_ERROR;

      // Turn off trigger mode
      triggerMode.bOnOff = false;

      error = ladybugSetTriggerMode( context, &triggerMode);
      HANDLE_ERROR;


   }catch (LadybugError& e){
      printf( "Ladybug SDK reported an error : %s\n", ladybugErrorToString( e));
   }

   error = ladybugDestroyContext( &context);

   //
   // Clean up the buffers
   //
   for( int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
   {
      delete [] arpBuffers[ uiCamera ];
   }

   return 0;
}

