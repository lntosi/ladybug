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
// ladybugOutput3DMesh.cpp
// 
// This example shows users how to produce 3D mesh from a given calibration data.
// In this example, the calibration data from the connected camera is used.
// The output of this program can be directly used for the input of the program
// ladybugStitchFrom3DMesh.
// You can save the output of this program to a file by using redirection.
// From the command prompt, go (cd) to the Ladybug SDK's "bin" directory and type
// ladybugoutput3dmesh > mymesh.txt
// and then you will have the output in the file "mymesh.txt".
//
//=============================================================================

#ifdef _WIN32
#include <windows.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ladybug.h"
#include "ladybuggeom.h"

// These can be changed. 
// Finer mesh produces more precise result.
#define _3D_GRID_COLS    128
#define _3D_GRID_ROWS    96

#define _HANDLE_ERROR \
    if( error != LADYBUG_OK ) \
   { \
   printf( \
   "Error! Ladybug library reported %s\n", \
   ladybugErrorToString( error ) ); \
   return EXIT_FAILURE; \
   }; \
   \

int
main( int /* argc */, char* /* argv[] */ ) 
{
    LadybugContext context;
    LadybugError error;
    LadybugImage image;
    const LadybugImage3d* arpImage3d[LADYBUG_NUM_CAMERAS];
    int srcCols = 0;
    int srcRows = 0;

    // initialize the camera
    error = ladybugCreateContext( &context );
    _HANDLE_ERROR;

    error = ladybugInitializeFromIndex( context, 0 );
    if ( error != LADYBUG_OK){
        printf( "Error: Ladybug is not found.\n");
        return EXIT_FAILURE;
    }

    error = ladybugLoadConfig(context, NULL);
    _HANDLE_ERROR;

    // grab an image to check the image size
    error = ladybugStart(
        context,
        LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8);
    _HANDLE_ERROR;

    error = LADYBUG_FAILED;
    int retry = 10;
    do{
        error = ladybugGrabImage(context, &image);
    } while (error != LADYBUG_OK && retry-- > 0);

    _HANDLE_ERROR;

    error = ladybugStop(context);

    srcCols = image.uiCols;
    srcRows = image.uiRows;

    // get the mapping and print it
    printf( "cols %d rows %d\n", _3D_GRID_COLS, _3D_GRID_ROWS);

    for (unsigned int camera = 0; camera < LADYBUG_NUM_CAMERAS; camera++)
    {
        error = ladybugGet3dMap(
            context, 
            camera, 
            _3D_GRID_COLS, 
            _3D_GRID_ROWS,
            srcCols, 
            srcRows, 
            false,
            &arpImage3d[camera] );
        
        _HANDLE_ERROR;

        for (int iRow = 0; iRow < _3D_GRID_ROWS ; iRow++)
        {
            for (int iCol = 0; iCol < _3D_GRID_COLS ; iCol++)
            {
                LadybugPoint3d p3d = arpImage3d[camera]->ppoints[iRow * _3D_GRID_COLS + iCol];
                printf(
                    "%4.4lf,%4.4lf,%4.4lf\n", 
                    p3d.fX, 
                    p3d.fY, 
                    p3d.fZ);
            }
        }
    }

    // clean up
    error = ladybugDestroyContext(&context);
    _HANDLE_ERROR;
    return 0;
}

