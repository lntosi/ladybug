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
// LadybugCaptureHDRImage.cpp
//
// This piece of example code demonstrates how a user can use the Ladybug
// camera system to capture a series of images closely spaced in time suitable 
// for input into a high dynamic range image creation system.
//
//
// main() - This function actually initializes the camera and calls the other
//    subroutines
//
// setupHDRRegisters() - This subroutine sets all of the registers necessary to
//    put the camera into 'HDR Mode'.
//
// captureImages() - This function captures images directly from a Ladybug camera.
//
// processImages() - This function actually computes the panoramic images
//
//
// The Ladybug has a bank of 4 gain and shutter registers in addition to its 
// standard set.  When put into 'HDR Mode', the camera cycles through the 
// settings contained in these registers on an image by image basis.
// This allows users to capture a set of 4 images with widely varying exposure 
// settings. The 4 images can be captured within 4/30 of a second for Ladybug2
// if the data format is set to LADYBUG_DATAFORMAT_COLOR_SEP_SEQUENTIAL_JPEG.
//
// The shutter and gain values are read from an INI file defined by 
// INI_FILE_NAME. If you find the shutter and gain settings are not 
// appropriate, change the data in this file. 
//
// Once these images have been captured, the program process the images and
// outputs a configuration file containing exposure data suitable for input
// into a program such as 'pfstools' and 'pfscalibration'.
//
// Having captured the images, the user should then run the 'pfsinhdrgen' program in
// the image directory with a command line similar to the following:
//
//    pfsinhdrgen  HDRDescription.hdrgen | pfshdrcalibrate -v | pfsout output.hdr
//
// Where 'HDRDescription.hdrgen' is the name of the configuration file output by this
// program and 'output.hdr' is the name of the output image.
//
// Then the output file can be viewed using pfsview ( or pfsv)
//
//    pfsv output.hdr
//
// You can also make HDR image out of 4 output images using Adobe Photoshop CS3, easyHDR, etc.
// In this case, you don't need to provide additional exposure data.
//
// ----------------------------------------------------------------------------
// 'pfstools' is available at:
//
//   http://www.mpi-inf.mpg.de/resources/pfstools/
//
// 'pfscalibration' is available at:
//
//   http://www.mpi-inf.mpg.de/resources/hdr/calibration/pfs.html
//
// You will also need Cygwin to use these tools. Cygwin is available at:
//
//   http://www.cygwin.com/
//
// ----------------------------------------------------------------------------

//=============================================================================
// System Includes
//=============================================================================
#ifdef _WIN32

#include <windows.h>
#include <conio.h>

#endif 

#include <stdlib.h> 
#include <math.h>
#include <stdio.h>

//=============================================================================
// PGR Includes
//=============================================================================
#include "ladybug.h"
#include "ladybuggeom.h"
#include "ladybugrenderer.h"

//=============================================================================
// Macro Definitions
//=============================================================================

// General Ladybug Macros
#define PANORAMIC_IMAGE_ROWS	 1024
#define PANORAMIC_IMAGE_COLS	 2048

// Use falloff correction to correct lens vignetting. Initialization will take longer.
//#define	DO_FALLOFF_CORRECTION

// High Dynamic Range register macros for Ladybug2
#define LADYBUG2_HDR_REGISTER  0x13FC     /*The register to control HDR mode */
#define LADYBUG2_HDR_ON        0x82000000 /*Value needed to activate HDR mode */
#define LADYBUG2_HDR_OFF       0x80000000 /*Value needed to deactivate HDR mode*/
#define LADYBUG2_HDR_SHUTTER_TAB0  0x141C     /*First batch of HDR settings */
#define LADYBUG2_HDR_SHUTTER_TAB1  0x145C     /*Second batch of HDR settings */ 
#define LADYBUG2_HDR_SHUTTER_TAB2  0x149C     /*Third batch of HDR settings */
#define LADYBUG2_HDR_SHUTTER_TAB3  0x14DC     /*Fourth batch of HDR settings */
#define LADYBUG2_HDR_GAIN_TAB0     0x143C     /*First batch of HDR settings */
#define LADYBUG2_HDR_GAIN_TAB1     0x147C     /*Second batch of HDR settings */ 
#define LADYBUG2_HDR_GAIN_TAB2     0x14BC     /*Third batch of HDR settings */
#define LADYBUG2_HDR_GAIN_TAB3     0x14FC    /*Fourth batch of HDR settings */

// High Dynamic Range register macros for Ladybug3 and newer
#define LADYBUG3_HDR_REGISTER  0x1800     /*The register to control HDR mode */
#define LADYBUG3_HDR_ON        0x82000000 /*Value needed to activate HDR mode */
#define LADYBUG3_HDR_OFF       0x80000000 /*Value needed to deactivate HDR mode*/
#define LADYBUG3_HDR_SHUTTER_TAB0  0x1820     /*First batch of HDR settings */
#define LADYBUG3_HDR_SHUTTER_TAB1  0x1840     /*Second batch of HDR settings */ 
#define LADYBUG3_HDR_SHUTTER_TAB2  0x1860     /*Third batch of HDR settings */
#define LADYBUG3_HDR_SHUTTER_TAB3  0x1880     /*Fourth batch of HDR settings */
#define LADYBUG3_HDR_GAIN_TAB0  0x1824     /*First batch of HDR settings */
#define LADYBUG3_HDR_GAIN_TAB1  0x1844     /*Second batch of HDR settings */ 
#define LADYBUG3_HDR_GAIN_TAB2  0x1864     /*Third batch of HDR settings */
#define LADYBUG3_HDR_GAIN_TAB3  0x1884     /*Fourth batch of HDR settings */

#define BUS_INDEX	      0		  /* The index of Ladybug camera on the IEEE-1394 bus. This has to be 0 if you have only one camera */
#define IMAGES_TO_CAPTURE     4          /*Number of HDR settings available */
#define HDR_IMAGE_FILE        "HDRDescription.hdrgen" /* HDR Descriptor file */

// Define the output image type. It may be LADYBUG_PANORAMIC or LADYBUG_DOME
#define OUTPUT_IMAGE_TYPE	LADYBUG_PANORAMIC

// File name that has exposure data
// It is of the format,
//    ShutterAbsValues=value1,value2,value3,value4
//    GainAbsValues=value1,value2,value3,value4
//  where shutter values are in miliseconds and gain values are in dB.
//
// E.g. 
//    ShutterAbsValues=2.5,5,10,20
//    GainAbsValues=0,0,0,0
#define INI_FILE_NAME	"ladybugCaptureHDRImage.ini"

// Error Checking Macro
#define CHECK_ERROR(error) {if(error!=LADYBUG_OK)\
  {\
  printf("failed(LINE:%d) - %s\n",__LINE__,ladybugErrorToString( error ) );\
  return 1;\
   }\
  }\

// Ladybug context
LadybugContext context;

// Camera information
LadybugCameraInfo cameraInfo;

// Texture size
int   textureRows, textureCols;

// Absolute Shutter/Gain values
float       fShutterValue[4];
float       fGainValue[4];  

// Register Shutter/Gain values
long        lShutterValue[4];
long        lGainValue[4];  

//=============================================================================
// Functions
//=============================================================================
//
// setupHDRRegisters
//
// Description:
//   This function sets up the HDR shutter and gain registers for the camera.  
int
setupHDRRegisters( void)
{
    LadybugError error;
    bool bShutterPresent;
    unsigned int lShutterMin, lShutterMax, lShutterDefault;
    float fShutterMin, fShutterMax;
    bool bShutterAuto, bShutterManual;
    bool bGainPresent;
    unsigned int lGainMin, lGainMax, lGainDefault;
    float fGainMin, fGainMax;
    bool bGainAuto, bGainManual;
    const char *pszUnit, *pszUnitAbbr;
    int i;

    float fExposureData[2][4];

    FILE        *pfile;
    char        szReadLine[128];
    int         iLineCounter = 0;
    char        szLine[128];
    char        szTokens[5][32];
    //
    // Determine the range of the shutter register
    //
    printf("Get shutter range..\n");
    error = ladybugGetAbsPropertyRange( 
        context,
        LADYBUG_SHUTTER,
        &bShutterPresent,
        &fShutterMin, 
        &fShutterMax, 
        &pszUnit,
        &pszUnitAbbr);
    CHECK_ERROR(error);
    printf("Shutter range: %f - %f %s\n", fShutterMin, fShutterMax, pszUnitAbbr );

    error = ladybugGetPropertyRange( 
        context,
        LADYBUG_SHUTTER,
        &bShutterPresent,
        &lShutterMin, 
        &lShutterMax, 
        &lShutterDefault,
        &bShutterAuto,
        &bShutterManual);
    CHECK_ERROR(error);

    //
    // Determine the range of the gain register
    //
    printf("Get gain range...\n");
    error = ladybugGetAbsPropertyRange( 
        context,
        LADYBUG_GAIN,
        &bGainPresent,
        &fGainMin, 
        &fGainMax, 
        &pszUnit,
        &pszUnitAbbr);
    CHECK_ERROR(error);
    printf("Gain range: %f - %f %s\n", fGainMin, fGainMax, pszUnitAbbr );

    error = ladybugGetPropertyRange( 
        context,
        LADYBUG_GAIN,
        &bGainPresent,
        &lGainMin, 
        &lGainMax, 
        &lGainDefault,
        &bGainAuto,
        &bGainManual);

    printf("Read exposure data from %s.\n", INI_FILE_NAME );
    //
    // Read exposure data from the file. 
    // If you find the shutter and gain settings are not 
    // appropriate, change the data in this file. 
    //
    if((pfile = fopen( INI_FILE_NAME , "r" )) != NULL )
    {   
        int iTokenIdx = 0;
        char* szTemp = NULL;
        //
        // Parse the exposure data
        //
        while ( fgets( szReadLine, 128, pfile ) != NULL )
        {
            // Parse the line
            strcpy( szLine, szReadLine );
            iTokenIdx = 0;
            szTemp = strtok( szLine, " =,;:\t\n" );
            while ( szTemp != NULL && iTokenIdx <= 4 )
            {
                strcpy( szTokens[iTokenIdx], szTemp );
                szTemp = strtok( NULL, " =,;:\t\n"  );
                iTokenIdx++;    
            }

            // Parsed the entry, store the values
            fExposureData[iLineCounter][0] = (float)atof( szTokens[1]);
            fExposureData[iLineCounter][1] = (float)atof( szTokens[2]);
            fExposureData[iLineCounter][2] = (float)atof( szTokens[3]);
            fExposureData[iLineCounter][3] = (float)atof( szTokens[4]);

            iLineCounter++;
        }
        fclose( pfile );
    }
    else
    {
        printf("Failed to open file %s.\n", INI_FILE_NAME );
        return 1;
    }

    //
    // Determine the gain and shutter values to be used in each bank of 
    // registers.  
    //
    for ( i = 0; i < 4; i++ )
    {
        if ( (fExposureData[0][i] < fShutterMin) || 
            (fExposureData[0][i] > fShutterMax) )
        {
            printf("The shutter value is out of range: %f\n", 
                fExposureData[0][i] );
            return 1;
        }
        fShutterValue[i] = fExposureData[0][i];

        if ((fExposureData[1][i] < fGainMin) || (fExposureData[1][i] > fGainMax))
        {
            printf("The gain value is out of range: %f\n", fExposureData[1][i] );
            return 1;
        }
        fGainValue[i] = fExposureData[1][i];
    }

    //
    // Convert the absolute values to register values inside camera
    //
    printf("Converting register values...\n");

    for ( i = 0; i < 4; i++ ) 
    {
        error = ladybugSetAbsPropertyEx(
            context, LADYBUG_SHUTTER, false, true, false, fShutterValue[ i]);

        error = ladybugSetAbsPropertyEx(
            context, LADYBUG_GAIN, false,  true,  false, fGainValue[ i]);

        unsigned int lValueA, lValueB;
        bool bAuto;
        error = ladybugGetProperty( 
            context, LADYBUG_SHUTTER, &lValueA,  &lValueB, &bAuto);

        lShutterValue[ i] = lValueA;

        error = ladybugGetProperty( 
            context, LADYBUG_GAIN, &lValueA, &lValueB, &bAuto);

        lGainValue[ i] = lValueA;

        printf( "[%d] Shutter: %4.2lfms -> %ld, Gain: %3.2lfdB -> %ld\n", i, fShutterValue[ i], lShutterValue[ i], fGainValue[ i], lGainValue[ i]);
    }

    printf("Set up HDR registers...\n");
    int nError = (int)LADYBUG_OK;
    switch ( cameraInfo.deviceType)
    {
        //
        // Set the shutter and gain for each 4 settings.
        //
    case LADYBUG_DEVICE_COMPRESSOR:
        {
            int on_manual_flags = 0x82000000; // set the registers to on and to manual mode

            nError |= (int)ladybugSetRegister( 
                context, LADYBUG2_HDR_SHUTTER_TAB0, lShutterValue[0] | on_manual_flags );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG2_HDR_SHUTTER_TAB1, lShutterValue[1] | on_manual_flags );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG2_HDR_SHUTTER_TAB2, lShutterValue[2] | on_manual_flags );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG2_HDR_SHUTTER_TAB3, lShutterValue[3] | on_manual_flags );

            nError |= (int)ladybugSetRegister( 
                context, LADYBUG2_HDR_GAIN_TAB0, lGainValue[0] | on_manual_flags );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG2_HDR_GAIN_TAB1, lGainValue[1] | on_manual_flags );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG2_HDR_GAIN_TAB2, lGainValue[2] | on_manual_flags );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG2_HDR_GAIN_TAB3, lGainValue[3] | on_manual_flags );
        }
        break;

        // Ladybug 3/5 Register settings - Entire 6 cameras can be set at once.
    case LADYBUG_DEVICE_LADYBUG3:
    case LADYBUG_DEVICE_LADYBUG5:
    case LADYBUG_DEVICE_LADYBUG5P:
        {
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG3_HDR_SHUTTER_TAB0, lShutterValue[0] );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG3_HDR_SHUTTER_TAB1, lShutterValue[1] );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG3_HDR_SHUTTER_TAB2, lShutterValue[2] );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG3_HDR_SHUTTER_TAB3, lShutterValue[3] );

            nError |= (int)ladybugSetRegister( 
                context, LADYBUG3_HDR_GAIN_TAB0, lGainValue[0] );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG3_HDR_GAIN_TAB1, lGainValue[1] );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG3_HDR_GAIN_TAB2, lGainValue[2] );
            nError |= (int)ladybugSetRegister( 
                context, LADYBUG3_HDR_GAIN_TAB3, lGainValue[3] );
        }            
        break;

    default:
        printf( "Unknown device\n");
        return 1;
    }

    error = (LadybugError)nError;
    if( error != LADYBUG_OK )
    {
        printf("failed.\n");
        return 1;
    }

    return 0;
}

LadybugError
enableHDR( bool enable)
{
    LadybugError error = LADYBUG_FAILED;

    if ( enable)
    {
        printf("Enable HDR mode...\n");
    }
    else
    {
        printf("Disable HDR mode...\n");
    }

    switch ( cameraInfo.deviceType)
    {
    case LADYBUG_DEVICE_COMPRESSOR:
        error = ladybugSetRegister(context,LADYBUG2_HDR_REGISTER, enable ? LADYBUG2_HDR_ON : LADYBUG2_HDR_OFF);
        break;

    case LADYBUG_DEVICE_LADYBUG3:
    case LADYBUG_DEVICE_LADYBUG5:
    case LADYBUG_DEVICE_LADYBUG5P:
        error = ladybugSetRegister(context,LADYBUG3_HDR_REGISTER, enable ? LADYBUG3_HDR_ON : LADYBUG3_HDR_OFF);
        break;
    }

    return error;
}

//=============================================================================
//
// captureImages()
//
// This function acquires images from Ladybug camera and then converts them
// to RGBA buffers.
//
//=============================================================================
int
captureImages(
              LadybugImageInfo* parImageInfo,
              unsigned char*    arpColorImageData[IMAGES_TO_CAPTURE][LADYBUG_NUM_CAMERAS])
{
    LadybugError   error;
    LadybugImage   images[IMAGES_TO_CAPTURE];

    ladybugUnlockAll( context );
    int nImage = 0;
    for( nImage = 0; nImage < IMAGES_TO_CAPTURE; nImage++ )
    {
        //
        // Grab the image
        //
        printf("Grab image %d...\n",nImage);
        error = ladybugLockNext( context, &images[nImage]);
        printf( "Image sequence ID: %d\n", images[nImage].imageInfo.ulSequenceId);
        printf( "Shutter: %d, Gain: %d, \n", 
            images[nImage].imageInfo.ulShutter[0] & 0xfff, images[nImage].imageInfo.arulGainAdjust[0] & 0xfff);
        CHECK_ERROR(error);
    }

    //
    // Set color processing method.
    //
    printf("Debayering %d images...\n", IMAGES_TO_CAPTURE );
    error = ladybugSetColorProcessingMethod( 
        context, LADYBUG_EDGE_SENSING);

    // 
    // Convert raw images to color
    //
    for( nImage = 0; nImage < IMAGES_TO_CAPTURE; nImage++)
    {
        // 
        // Use the alpha mask 
        // Note: The images are converted to different destination buffers. 
        // ladybugSetAlphaMasking() has to be called to set alpha channel 
        // for each buffer.
        //
        ladybugSetAlphaMasking( context, true );
        error = ladybugConvertImage(context, &images[nImage], arpColorImageData[nImage]);
        parImageInfo[nImage] = images[nImage].imageInfo;
        printf("Sequence-ID:%d \n", parImageInfo[nImage].ulSequenceId );
        CHECK_ERROR(error);
    }

    ladybugUnlockAll( context );

    return 0;
}

//=============================================================================
//
// processImages()
//
// Description:
//   This function does the image stitching and blending and then outputs a 
//   series of the stitched images to disk and the necessary information to 
//   create a high dynamic range image.
//
//=============================================================================
int
processImages(
              LadybugImageInfo* parImageInfo,
              unsigned char*    arpColorImageData[IMAGES_TO_CAPTURE][LADYBUG_NUM_CAMERAS])
{
    LadybugError error;
    LadybugProcessedImage processedImage;
    char szOutputFileName[256] = {0};
    FILE* pHDRDescFile = NULL;

    double dRealGainValue = 0.0, dRealShutter = 0.0;

    // Open the hdr description file
    printf("Open HDR description file...\n");
    pHDRDescFile = fopen(HDR_IMAGE_FILE,"w");
    if( pHDRDescFile == NULL )
    {
        printf("Failed to open HDR description file.\n");
        printf("This may be caused by permission issues with the current directory. Try moving the program and configuration file to a location that does not require admin privilege.\n");
        return 1;
    }

    printf("Set off-screen panoramic image size:%dx%d image...\n", 
        PANORAMIC_IMAGE_COLS, PANORAMIC_IMAGE_ROWS );
    error = ladybugSetOffScreenImageSize(
        context, OUTPUT_IMAGE_TYPE,  
        PANORAMIC_IMAGE_COLS, PANORAMIC_IMAGE_ROWS );
    if (error != LADYBUG_OK)
    {
        fclose(pHDRDescFile);
        CHECK_ERROR(error);
    }
    

    error = ladybugSetPanoramicMappingType( context, LADYBUG_MAP_RADIAL);
    CHECK_ERROR(error);

    for (int i = 0; i < IMAGES_TO_CAPTURE; i++)
    {
        // Get the panoramic image.
        printf("Update image %d for rendering...\n", i);
        error = ladybugUpdateTextures( 
            context, 
            LADYBUG_NUM_CAMERAS,
            const_cast<const unsigned char**>(&(arpColorImageData[i][0])));
        CHECK_ERROR(error);

        printf("Render and get off-screen stitched image %d...\n", i);
        error = ladybugRenderOffScreenImage(
            context, 
            OUTPUT_IMAGE_TYPE, 
            LADYBUG_BGR,
            &processedImage);
        CHECK_ERROR(error);

        sprintf( szOutputFileName, "%u_image_hdr%d.jpg", cameraInfo.serialHead, i );
        printf("Write image %s to disk...\n",szOutputFileName);

        error = ladybugSetImageSavingJpegQuality( context, 95);
        error = ladybugSaveImage( context, &processedImage, szOutputFileName, LADYBUG_FILEFORMAT_JPG);
        CHECK_ERROR(error);

        // obtain the actual absolute value from the register value in the image
        dRealShutter = 0.0;
        dRealGainValue = 0.0;

        error = ladybugSetProperty( context, LADYBUG_SHUTTER, parImageInfo[ i].ulShutter[ 0] & 0xfff, 0, false);
        CHECK_ERROR(error);
        error = ladybugSetProperty( context, LADYBUG_GAIN, parImageInfo[ i].arulGainAdjust[ 0] & 0xfff, 0, false);
        CHECK_ERROR(error);

        float fAbsValue;
        error = ladybugGetAbsProperty( context, LADYBUG_SHUTTER, &fAbsValue);
        CHECK_ERROR(error);
        if ( error == LADYBUG_OK)
            dRealShutter = fAbsValue;

        error = ladybugGetAbsProperty( context, LADYBUG_GAIN, &fAbsValue);
        CHECK_ERROR(error);
        if ( error == LADYBUG_OK)
            dRealGainValue = fAbsValue;

        printf( "Actual shutter: %3.3lf ms, gain:%3.3lf dB\n", dRealShutter, dRealGainValue);

        // output pointers to image files in format accepted by pfstools
        // (imageFileName.ppm)(inverse of exposure time)(aperture)(gain)(neutral density)
        fprintf(pHDRDescFile,
            "%s %10.3lf 8.0 %10.3lf 0.0\n", 
            szOutputFileName, 
            1000.0 / dRealShutter, 
            dRealGainValue);
    }

    // Release the off-screen image rendering resources. 
    printf("Release off-screen image rendering resources...\n");
    error = ladybugReleaseOffScreenImage( context, OUTPUT_IMAGE_TYPE );
    CHECK_ERROR(error);

    fclose(pHDRDescFile); 
    return 0;
}

int 
main( int /* argc */, char* /* argv[] */ )
{
    LadybugError      error = LADYBUG_FAILED;
    unsigned char*    arpBGRUImageData[IMAGES_TO_CAPTURE][LADYBUG_NUM_CAMERAS];
    LadybugImageInfo  arImageInfo[IMAGES_TO_CAPTURE];
    LadybugImage      image;

    // The following variables are used to backup the current register values
    unsigned int ulCurretMasterShutter = 0;
    unsigned int ulCurretMasterGain= 0;
    unsigned int lDontCare = 0;
    bool bAutoShutterFlag;
    bool bAutoGainFlag;

    // Create the ladybug context
    printf("Create context...\n");
    error = ladybugCreateContext( &context );
    CHECK_ERROR(error);

    // Initialize the camera based on the index
    printf("Initialize camera...\n");
    error = ladybugInitializeFromIndex( context, BUS_INDEX);
    CHECK_ERROR(error);

    // Obtain camera information
    printf("Get camera information...\n");
    error = ladybugGetCameraInfo( context, &cameraInfo);
    CHECK_ERROR(error);

    // Load configuration file
    printf("Load configuration file from the camera...\n");
    error = ladybugLoadConfig( context, NULL );
    CHECK_ERROR(error);

#ifdef DO_FALLOFF_CORRECTION
    //
    // set falloff correction
    //
    printf( "Set falloff correction flag..." );
    error = ladybugSetFalloffCorrectionFlag( context, true );
    CHECK_ERROR(error);   
#endif

    // Set correction attenuation
    printf( "Set Correction Attenuation...\n" );
    error = ladybugSetFalloffCorrectionAttenuation( context, 1.0 );
    CHECK_ERROR(error);   

    // Turn off Gamma
    error = ladybugSetPropertyEx( context, LADYBUG_GAMMA, false, false, false, 0, 0);
    CHECK_ERROR(error);   

    error = enableHDR( false);
    CHECK_ERROR(error);   

    // Read and backup the current master shutter and 
    // master gain register
    int nError = (int)LADYBUG_OK;
    nError |= (int)ladybugGetProperty( context, LADYBUG_SHUTTER,
        &ulCurretMasterShutter, 
        &lDontCare,
        &bAutoShutterFlag );
    nError |= (int)ladybugGetProperty( context, LADYBUG_GAIN,
        &ulCurretMasterGain, 
        &lDontCare,
        &bAutoGainFlag );
    printf( "Backup the current master values - shutter : %u [auto:%d], gain : %u [auto:%d]\n", 
        ulCurretMasterShutter,
        bAutoShutterFlag,
        ulCurretMasterGain,
        bAutoGainFlag);

    // Start streaming

    printf("Starting camera...\n");
    error = ::ladybugStartLockNext(
        context,
        LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8);
    CHECK_ERROR(error);

    if (cameraInfo.deviceType == LADYBUG_DEVICE_LADYBUG3)
    {
        error = ::ladybugSetAbsProperty( context, LADYBUG_FRAME_RATE, 12.0);
        CHECK_ERROR(error);
    }

    // Setup the HDR registers ( this has to be done after frame rate is determined in order to get correct shutter value conversion.)
    if (setupHDRRegisters())
    {
        printf("Failed to setup HDR registers.\n");
        return 1;
    }

    error = enableHDR( true);
    CHECK_ERROR(error);   

    // Keep grabbing to get the first good image
    error = LADYBUG_FAILED;
    unsigned int uiRepeat = 10;
    printf("grab\n");
    while ( uiRepeat-- > 0 )
    {
        error = ladybugLockNext( context, &image );
        ladybugUnlock( context, image.uiBufferIndex);
    }
    CHECK_ERROR(error);

    // Determine texture size based on the captured image
    textureRows = image.uiRows;
    textureCols = image.uiCols;

    // Allocate memory storage for the color processed images
    for(int nImage = 0; nImage< IMAGES_TO_CAPTURE; nImage++)
    {
        for(int nCamera = 0; nCamera<LADYBUG_NUM_CAMERAS;nCamera++)
        {
            arpBGRUImageData[nImage][nCamera] = 
                new unsigned char[textureRows * textureCols * 4 ];
        }
    }

    // Initialize alpha mask size - this can take a long time if the
    // masks are not present in the current directory.
    printf( "Initialize alpha masks (this may take a long time)...\n" );
    error = ladybugInitializeAlphaMasks( 
        context, 
        textureCols, 
        textureRows );
    CHECK_ERROR(error);   

    // Set panoramic view angle
    error = ladybugSetPanoramicViewingAngle( context, LADYBUG_FRONT_0_POLE_5);
    CHECK_ERROR(error);   

    // Configure output images in Ladybug liabrary
    printf( "Configure output images in Ladybug library...\n" );
    error = ladybugConfigureOutputImages( context, OUTPUT_IMAGE_TYPE );
    CHECK_ERROR(error);

    // Capture images
    printf("Capture images from camera...\n");
    if ( captureImages( &arImageInfo[0], arpBGRUImageData ))
    {
        return 1;
    }

    // Stop images from flowing to the PC.
    printf("Stop camera...\n");
    error = ladybugStop( context );
    CHECK_ERROR(error);

    // Turn off HDR mode
    enableHDR( false);
    CHECK_ERROR(error); 

    // Process the images
    if ( processImages( &(arImageInfo[0]),&(arpBGRUImageData[0]) ))
    {
        return 1;
    }

    // Restore the previous values
    printf("Restoring the previous master values...\n");
    error = ladybugSetProperty( context, LADYBUG_SHUTTER,
        ulCurretMasterShutter, lDontCare, bAutoShutterFlag );
    CHECK_ERROR(error);
    error = ladybugSetProperty( context, LADYBUG_GAIN,
        ulCurretMasterGain, lDontCare, bAutoGainFlag );
    CHECK_ERROR(error);

    // Destroy the ladybug context
    printf( "Destroy context...\n" );
    error  = ladybugDestroyContext( &context );
    CHECK_ERROR( error );

    // Display the exit message
    printf("You can now use a utility like 'pfstools' to create an HDR image.\n");

    return 0;
}
