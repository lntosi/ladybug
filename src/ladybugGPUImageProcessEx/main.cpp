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
// This example illustrates a simple use case for using the GPU to process and 
// save an image from a Ladybug camera.
//
// This program does:
//  - create a context
//  - initialize a camera
//  - start transmission of images
//  - grab an image
//  - show how to allocate Cuda device pointers
//  - use gpu to process the grabbed image
//  - save a panoramic image
//  - show how to free device pointers
//  - destroy the context 
//
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "ladybug.h"
#include "ladybugrenderer.h"

#ifdef _WIN32

#include <windows.h>
#include <shlobj.h>

#else

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#endif

// If you wish to use external buffers for access
// to device pointers which hold the processed image
// data enable USE_EXTERNAL_BUFFERS

// You will also need to have the CUDA toolkit installed and
// include the appropriate paths to include and lib folders
// in the project file. Additionally, cudart.lib will need to
// be added as a library dependency.

// #define USE_EXTERNAL_BUFFERS

#ifdef USE_EXTERNAL_BUFFERS

#include <cuda_runtime.h>

#endif

#define _HANDLE_ERROR \
    if( error != LADYBUG_OK ) \
   { \
   printf( \
   "Error: Ladybug library reported - %s\n", \
   ::ladybugErrorToString( error ) ); \
   return EXIT_FAILURE; \
   } \
   \

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


int main(int /* argc */, char* /* argv[] */)
{
    // Initialize context.
    LadybugContext context;
    LadybugError error = ::ladybugCreateContext(&context);
    _HANDLE_ERROR;

    // Initialize the first ladybug on the bus.
    printf("Initializing...\n");
    error = ::ladybugInitializeFromIndex(context, 0);
    _HANDLE_ERROR;

    // Get camera info
    LadybugCameraInfo caminfo;
    error = ladybugGetCameraInfo(context, &caminfo);
    _HANDLE_ERROR("ladybugGetCameraInfo()");

    // Load config file from the head
    printf("Loading config info...\n");
    error = ladybugLoadConfig(context, NULL);
    _HANDLE_ERROR;

    // Start up the camera according to device type and data format
    printf("Starting %s (%u)...\n", caminfo.pszModelName, caminfo.serialHead);
    error = ::ladybugStart(
        context,
        LADYBUG_DATAFORMAT_RAW8);
    _HANDLE_ERROR;

    // Grab a single image.
    printf("Grabbing image\n");
    error = LADYBUG_FAILED;
    LadybugImage image;
    for (int i = 0; i < 10 && error != LADYBUG_OK; i++)
    {
        printf(".");
        error = ::ladybugGrabImage(context, &image);
    }
    printf("\n");
    _HANDLE_ERROR;

    // Configure for rendering Image Off Screen
    error = ::ladybugConfigureOutputImages(
        context,
        LADYBUG_PANORAMIC);
    _HANDLE_ERROR;
    
    const unsigned int BYTES_PER_PIXEL = 4;
    LadybugProcessedImage processedImage;
    unsigned char* gpuBuffers[LADYBUG_NUM_CAMERAS] = { 0 };

    bool useExternalBuffers = false;

#ifdef USE_EXTERNAL_BUFFERS
     
	useExternalBuffers = true;
	for (unsigned int i = 0; i < LADYBUG_NUM_CAMERAS; i++)
	{
		cudaFree(0);
		unsigned int allocSize = image.uiRows * image.uiCols * BYTES_PER_PIXEL;
		if (cudaMalloc((void**)&gpuBuffers[i], allocSize) != cudaSuccess)
		{
			printf("CudaMalloc error\n");
		}
	}

#endif
    
    printf("Processing image\n");
    error = ::ladybugConvertImageGPU(
        context, 
        &image, 
        useExternalBuffers ? gpuBuffers : nullptr);
	_HANDLE_ERROR;

    error = ::ladybugUpdateTexturesGPU(
        context, 
        LADYBUG_NUM_CAMERAS, 
        useExternalBuffers ? (const unsigned char**)gpuBuffers : nullptr);
	_HANDLE_ERROR;
    
    error = ::ladybugRenderOffScreenImage(
        context,
        LADYBUG_PANORAMIC,
        LADYBUG_BGR,
        &processedImage);
    _HANDLE_ERROR;

    const std::string outputPath = getWriteableDirectory() + "GPUImageProcessExImage.tiff";
    printf("Saving image to %s\n", outputPath.c_str());
    error = ladybugSaveImage(
        context,
        &processedImage,
        outputPath.c_str(),
        LADYBUG_FILEFORMAT_TIFF);
    if (error != LADYBUG_OK)
    {
        fprintf(stdout, "Unable to save image to %s.\n",
            outputPath.c_str());
        fflush(stdout);
        return false;
    }

    printf("Destroying context\n");
    error = ::ladybugDestroyContext(&context);
    _HANDLE_ERROR;

    // Clean up Cuda device pointers if using external buffers

#ifdef USE_EXTERNAL_BUFFERS

    for (unsigned int i = 0; i < LADYBUG_NUM_CAMERAS; i++)
    {
        if (cudaFree(gpuBuffers[i]) != cudaSuccess)
        {
            printf("CudaFree error\n");
        }
    }

#endif

    printf("Done.\n");

    return 0;
}