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
//
// ladybugSimpleGrabDisplay.cpp
//
//=============================================================================
// This example shows users how to use OpenGL Utility Toolkit (GLUT) to grab
// Ladybug images and display the images in a simple window.
// This simple example starts the first Labybug camera on the bus.
// The camera is started in JPEG mode and the images are processed with 
// LADYBUG_DOWNSAMPLE4 color processign method.
//
// Right click the mouse in the client area to popup a menu and select 
// various Ladybug image types.
//
// Note: This example has to be run with freeglut.dll 
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32

#include <windows.h>

#else

#include <time.h>

#endif


#include <GL/freeglut.h>

#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>

#define _HANDLE_ERROR \
    if( error != LADYBUG_OK ) \
   { \
   printf( "Error! Ladybug library reported %s\n", \
   ::ladybugErrorToString( error ) ); \
   assert( false ); \
   exit( 1 ); \
   } \

#define _DISPLAY_ERROR_MSG_AND_RETURN \
    if( error != LADYBUG_OK ) \
   { \
   printf( "Ladybug library reported %s\n", \
   ::ladybugErrorToString( error ) ); \
   return; \
   } \


// Define display mode
enum DisplayModes
{
    MENU_PANORAMIC = 1,          //Display Panoramic 
    MENU_DOME,                  //Display dome view
    MENU_ALL_CAMERAS,          //Display all camera images in one view
    MENU_SINGLE_CAMERA_0,     //Display camera 0
    MENU_SINGLE_CAMERA_1,     //Display camera 1
    MENU_SINGLE_CAMERA_2,     //Display camera 2
    MENU_SINGLE_CAMERA_3,     //Display camera 3
    MENU_SINGLE_CAMERA_4,     //Display camera 4
    MENU_SINGLE_CAMERA_5,     //Display camera 5
    MENU_RECTIFIED_CAMERA_0,  //Rectified camera 0
    MENU_RECTIFIED_CAMERA_1,  //Rectified camera 1
    MENU_RECTIFIED_CAMERA_2,  //Rectified camera 2
    MENU_RECTIFIED_CAMERA_3,  //Rectified camera 3
    MENU_RECTIFIED_CAMERA_4,  //Rectified camera 4
    MENU_RECTIFIED_CAMERA_5,  //Rectified camera 5
    MENU_EXIT
};

static unsigned long uiDisplayMode = MENU_PANORAMIC;
static double uiLast_Idle_Time;
static unsigned long uiFrameCounter;

LadybugContext context = NULL ;  // Ladybug context
LadybugImage image;              // Ladybug image
int menu;

/*
Returns the current time in milliseconds based on a monotonically 
increasing clock with an unspecified starting time.
*/ 
double getCurrentMs()
{
#ifdef _WIN32
    return (double)GetTickCount();
#else
    timespec currentTime;
    if(clock_gettime(CLOCK_MONOTONIC, &currentTime))
    {
        return 0.0;
    }else
    {
        return (double)(currentTime.tv_sec * 1000 + (currentTime.tv_nsec /1000000.0));
    }
#endif
}

// Clean up
void cleanUp()
{
    if (context)
    { 
        printf( "Stopping camera...\n" );
        LadybugError error = ::ladybugStop(context);
        _HANDLE_ERROR;

        printf( "Destroying context...\n" );
        error = ::ladybugDestroyContext(&context);
        _HANDLE_ERROR;
    }

    glutDestroyMenu(menu);

    return;
}

// Display mode selection
void selectFromMenu(int iCommand)
{  
    if (iCommand == MENU_EXIT)
    {
        cleanUp();
        exit (0);
    }
    else
    {
        uiDisplayMode = iCommand;
    }

    // Redraw the window
    glutPostRedisplay();
}

// Create a popup menu for selecting display modes
int buildPopupMenu (void)
{
    menu = glutCreateMenu ( selectFromMenu );
    glutAddMenuEntry ( "Panoramic Image", MENU_PANORAMIC );
    glutAddMenuEntry ( "Dome Projection", MENU_DOME );
    glutAddMenuEntry ( "All Camera Images", MENU_ALL_CAMERAS );
    glutAddMenuEntry ( "Camera 0", MENU_SINGLE_CAMERA_0 );
    glutAddMenuEntry ( "Camera 1", MENU_SINGLE_CAMERA_1 );
    glutAddMenuEntry ( "Camera 2", MENU_SINGLE_CAMERA_2 );
    glutAddMenuEntry ( "Camera 3", MENU_SINGLE_CAMERA_3 );
    glutAddMenuEntry ( "Camera 4", MENU_SINGLE_CAMERA_4 );
    glutAddMenuEntry ( "Camera 5", MENU_SINGLE_CAMERA_5 );
    glutAddMenuEntry ( "Rectified Camera 0", MENU_RECTIFIED_CAMERA_0 );
    glutAddMenuEntry ( "Rectified Camera 1", MENU_RECTIFIED_CAMERA_1 );
    glutAddMenuEntry ( "Rectified Camera 2", MENU_RECTIFIED_CAMERA_2 );
    glutAddMenuEntry ( "Rectified Camera 3", MENU_RECTIFIED_CAMERA_3 );
    glutAddMenuEntry ( "Rectified Camera 4", MENU_RECTIFIED_CAMERA_4 );
    glutAddMenuEntry ( "Rectified Camera 5", MENU_RECTIFIED_CAMERA_5 );
    glutAddMenuEntry ( "Exit", MENU_EXIT);

    return 0;
}

// Start the first Ladybug camera on the bus
int startCamera()
{       
    // Initialize context.
    LadybugError error = ::ladybugCreateContext( &context );
    _HANDLE_ERROR;

    // Initialize the first ladybug on the bus.
    printf( "Initializing.\n" );
    error = ::ladybugInitializeFromIndex( context, 0 );
    _HANDLE_ERROR;

    // Load config file from the head
    printf( "Loading config info...\n" );
    error = ladybugLoadConfig( context, NULL );
    _HANDLE_ERROR;

    // Start Ladybug with the default format/resolution.
    printf( "Starting camera...\n" );
    error = ::ladybugStart(
        context,
        LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8);
    _HANDLE_ERROR;    

    // grab an image to check image size
    printf( "Grabbing an image to check the image size" );
    int iTryTimes = 0;
    do 
    {
        printf(".");
        error = ::ladybugGrabImage(context, &image);
    } while (error != LADYBUG_OK && iTryTimes++ < 10);
    _HANDLE_ERROR;
    printf( " \n" );

    // Set color processing method
    error = ::ladybugSetColorProcessingMethod(context, LADYBUG_DOWNSAMPLE4);
    _HANDLE_ERROR;

    // determine texture size (it's half because we use downsample color processing)
    const int textureWidth = image.uiCols / 2;
    const int textureHeight = image.uiRows / 2;

    // Initialize alpha mask
    printf( "Initializing Alpha mask...\n" );
    error = ladybugInitializeAlphaMasks( context, textureWidth, textureHeight );
    _HANDLE_ERROR;
    ladybugSetAlphaMasking( context, true );

    return 0;
}

// Display Ladybug images
void display()
{
    // Clear frame buffer and depth buffer
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClearDepth(1.0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glLoadIdentity();

    LadybugError error = LADYBUG_FAILED;

    // Display Ladybug images
    switch ( uiDisplayMode )
    {
    case MENU_PANORAMIC:
        error=ladybugDisplayImage(context,LADYBUG_PANORAMIC);
        break; 
    case MENU_ALL_CAMERAS:
        error=ladybugDisplayImage(context,LADYBUG_ALL_CAMERAS_VIEW);
        break;
    case MENU_DOME:
        error=ladybugDisplayImage(context,LADYBUG_DOME);
        break;
    case MENU_SINGLE_CAMERA_0:
        error=ladybugDisplayImage(context,LADYBUG_RAW_CAM0);
        break;
    case MENU_SINGLE_CAMERA_1:
        error=ladybugDisplayImage(context,LADYBUG_RAW_CAM1);
        break;
    case MENU_SINGLE_CAMERA_2:
        error=ladybugDisplayImage(context,LADYBUG_RAW_CAM2);
        break;
    case MENU_SINGLE_CAMERA_3:
        error=ladybugDisplayImage(context,LADYBUG_RAW_CAM3);
        break;
    case MENU_SINGLE_CAMERA_4:
        error=ladybugDisplayImage(context,LADYBUG_RAW_CAM4);
        break;
    case MENU_SINGLE_CAMERA_5:
        error=ladybugDisplayImage(context,LADYBUG_RAW_CAM5);
        break;
    case MENU_RECTIFIED_CAMERA_0:
        error=ladybugDisplayImage(context,LADYBUG_RECTIFIED_CAM0);
        break;
    case MENU_RECTIFIED_CAMERA_1:
        error=ladybugDisplayImage(context,LADYBUG_RECTIFIED_CAM1);
        break;
    case MENU_RECTIFIED_CAMERA_2:
        error=ladybugDisplayImage(context,LADYBUG_RECTIFIED_CAM2);
        break;
    case MENU_RECTIFIED_CAMERA_3:
        error=ladybugDisplayImage(context,LADYBUG_RECTIFIED_CAM3);
        break;
    case MENU_RECTIFIED_CAMERA_4:
        error=ladybugDisplayImage(context,LADYBUG_RECTIFIED_CAM4);
        break;
    case MENU_RECTIFIED_CAMERA_5:
        error=ladybugDisplayImage(context,LADYBUG_RECTIFIED_CAM5);
        break;
    }
    _DISPLAY_ERROR_MSG_AND_RETURN
        
    // Make sure changes appear onscreen
    glutSwapBuffers();
}

// Grab images, process images and update image textures to the graphics card
void grabImage()
{    
    LadybugError error = ::ladybugGrabImage( context, &image );
    _DISPLAY_ERROR_MSG_AND_RETURN

    // Color processing images using internal buffers.
    error = ::ladybugConvertImage( context, &image, NULL );    
    _DISPLAY_ERROR_MSG_AND_RETURN

    // Update images to the graphics card
    error = ladybugUpdateTextures( context, LADYBUG_NUM_CAMERAS, NULL );
    _DISPLAY_ERROR_MSG_AND_RETURN

    // Calculate frame rate 
    uiFrameCounter++;
    const double uiCurrentTime = getCurrentMs();

    if (uiCurrentTime - uiLast_Idle_Time > 1000 )
    {
        char pszTimeString[128] = {0};
        sprintf( 
            pszTimeString, 
            "LadybugSimpleGrabDisplay - %5.2ffps", 
            (uiFrameCounter * 1000.0f) / (uiCurrentTime - uiLast_Idle_Time));
        glutSetWindowTitle(pszTimeString);
        uiLast_Idle_Time = uiCurrentTime;
        uiFrameCounter = 0;
    } 

    // Redisplay
    glutPostRedisplay();
}

int main(int argc, char** argv)
{
    // GLUT Window Initialization
    glutInit( &argc, argv );
    glutInitWindowSize( 1000, 500 );
    glutInitDisplayMode ( GLUT_RGBA | GLUT_DOUBLE );
    glutInitWindowPosition( 100, 100 );
    glutCreateWindow ( "Ladybug Simple Grab and Display" );

    // Start camera
    startCamera();

    // Configure output images in Ladybug library
    ladybugConfigureOutputImages(context, LADYBUG_PANORAMIC | LADYBUG_DOME | LADYBUG_ALL_RECTIFIED_IMAGES);

    // Set Ladybug library for displaying Ladybug image in the window
    LadybugError error = ladybugSetDisplayWindow( context );
    _HANDLE_ERROR;

    // Register callbacks:
    glutDisplayFunc( display );

    // Set grabbing loop function
    glutIdleFunc( grabImage );

    // Create a popup menu
    buildPopupMenu();
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    // Reset frame counter
    uiLast_Idle_Time = getCurrentMs();
    uiFrameCounter = 0;

    glutCloseFunc( cleanUp );

    // Turn the flow of control over to GLUT
    printf( "Grabbing and display...\n" );
    glutMainLoop();

    return 0;
}

