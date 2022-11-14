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

#include "CubeMap.h"
#include "ladybugrenderer.h"
#include <iostream>

#ifdef _WIN32

#define TEMPNAM _tempnam

#else 

#define TEMPNAM tempnam

#endif 

const unsigned int NUMBER_OF_IMAGE_CHANNELS = 4;
const std::string FILE_EXTENSION = "bmp";
const std::string TEMP_CAL_FILE = "temp.cal";
const std::string ERROR_OUTPUT = "Error: Ladybug library reported - %s\n";
const std::string OUTPUT_FILE_NAME = "%s\\ladybug_cube_%06u_%d.%s";

const float FIELD_OF_VIEW = 90.0f;
const float TRANSLATION = 0.0f;

namespace
{
    void HandleError(LadybugError e)
    {
        if (e != LADYBUG_OK)
        {
            printf(ERROR_OUTPUT.c_str(), ladybugErrorToString(e));
            exit(EXIT_FAILURE);
        }
    }

    float DegreesToRadians(float d)
    {
        return d * 3.1415926535f / 180.0f;
    }

    bool IsHighBitDepth(LadybugDataFormat format)
    {
        const bool isHighBitDepth = (
            format == LADYBUG_DATAFORMAT_RAW16 || 
            format == LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW16 || 
            format == LADYBUG_DATAFORMAT_RAW12 ||
            format == LADYBUG_DATAFORMAT_HALF_HEIGHT_RAW12 ||
            format == LADYBUG_DATAFORMAT_COLOR_SEP_JPEG12 ||
            format == LADYBUG_DATAFORMAT_COLOR_SEP_HALF_HEIGHT_JPEG12);

        return isHighBitDepth;
    }

    int ToBytePerPixel(LadybugDataFormat format)
    {
        return IsHighBitDepth(format) ? 2 : 1;
    }

    std::string getTempName(std::string fallBackName)
    {
        char* tempPath = NULL;
        std::string tempPathString;
        tempPath = TEMPNAM(NULL, NULL);

        if (tempPath == NULL)
        {
            //The TMP / TMPDIR enviroment variable is invalid or missing. Default to working directory. 
            tempPathString = fallBackName;
        }
        else
        {
            tempPathString = tempPath;
            free(tempPath);
        }

        return tempPathString;
    }

}

CubeMap::CubeMap(std::string inputFile, std::string outputDir, int outputDimension)
{
    m_readData.filePath = inputFile;
    m_renderData.outputDirectory = outputDir;
    

    std::string tempPathString = getTempName(TEMP_CAL_FILE);

    LadybugError error;
    
    // Read Setup
    error = ladybugCreateStreamContext(&m_readData.context);
    HandleError(error);

    error = ladybugInitializeStreamForReading(m_readData.context, m_readData.filePath.c_str());
    HandleError(error);

    error = ladybugGetStreamNumOfImages(m_readData.context, &m_readData.numberOfFrames);
    HandleError(error);

    error = ladybugGetStreamConfigFile(m_readData.context, tempPathString.c_str());
    HandleError(error);

    LadybugImage image;
    error = ladybugReadImageFromStream(m_readData.context, &image);
    HandleError(error);

    m_readData.width = image.uiCols;
    m_readData.height = image.uiRows;	

    // Render Setup
    error = ladybugCreateContext(&m_renderData.context);
    HandleError(error);

    error = ladybugLoadConfig(m_renderData.context, tempPathString.c_str());
    HandleError(error);

    // Delete temp config file 
    if (std::remove(tempPathString.c_str()) != 0)
    {
        //Failed to remove the file. Handle error?
        std::cout << "Warning: temp file " << tempPathString << " was unable to be deleted." << std::endl;
    }

    error = ladybugSetColorProcessingMethod(m_renderData.context, LADYBUG_HQLINEAR);
    HandleError(error);

    const unsigned int bytesPerPixel = ToBytePerPixel(image.dataFormat);
    const unsigned int textureBufferSize = m_readData.width * m_readData.height * NUMBER_OF_IMAGE_CHANNELS * bytesPerPixel;
    for (int i = 0; i < LADYBUG_NUM_CAMERAS; i++)
    {
        m_renderData.textureBuffers.push_back(new unsigned char[textureBufferSize]);
    }

    error = ladybugInitializeAlphaMasks(m_renderData.context, outputDimension, outputDimension);
    HandleError(error);

    error = ladybugSetAlphaMasking(m_renderData.context, true);
    HandleError(error);

    error = ladybugConfigureOutputImages(m_renderData.context, LADYBUG_SPHERICAL);
    HandleError(error);

    error = ladybugSetOffScreenImageSize(m_renderData.context, LADYBUG_SPHERICAL, outputDimension, outputDimension);
    HandleError(error);
}


CubeMap::~CubeMap()
{
    LadybugError error;

    // Read clean up
    error = ladybugStopStream(m_readData.context);
    HandleError(error);

    error = ladybugDestroyContext(&m_readData.context);
    HandleError(error);

    // Render clean up
    error = ladybugDestroyContext(&m_renderData.context);
    HandleError(error);

    for (int i = 0; i < LADYBUG_NUM_CAMERAS; i++)
    {
        delete m_renderData.textureBuffers[i];
    }
}

LadybugError CubeMap::SaveCubeFrame(unsigned int frameIndex, LadybugDataFormat imageDataFormat)
{
    LadybugError error = LADYBUG_OK;
    LadybugSaveFileFormat fileFormat = LADYBUG_FILEFORMAT_BMP;
    
    for (int surface = FRONT; surface < NUMBER_OF_SURFACES; surface++)
    {
        float rot_x = 0.0;
        float rot_y = 0.0;
        float rot_z = 0.0;

        switch (surface)
        {
            case FRONT: rot_x = 0.0f; rot_y = 0.0f; rot_z = 0.0f; break;
            case RIGHT: rot_x = 0.0f; rot_y = 0.0f; rot_z = 90.0f; break;
            case BACK: rot_x = 0.0f; rot_y = 0.0f; rot_z = 180.0f; break;
            case LEFT: rot_x = 0.0f; rot_y = 0.0f; rot_z = 270.0f; break;
            case TOP: rot_x = 180.0f; rot_y = 90.0f; rot_z = 0.0f; break;
            case BOTTOM: rot_x = 180.0f; rot_y = 270.0f; rot_z = 0.0f; break;
        }
        
        error = ladybugSetSphericalViewParams(
            m_renderData.context,
            FIELD_OF_VIEW,
            DegreesToRadians(rot_x),
            DegreesToRadians(rot_y),
            DegreesToRadians(rot_z),
            TRANSLATION,
            TRANSLATION,
            TRANSLATION);
        HandleError(error);

        LadybugProcessedImage procImage;
        error = ladybugRenderOffScreenImage(
            m_renderData.context,
            LADYBUG_SPHERICAL,
            IsHighBitDepth(imageDataFormat) ? LADYBUG_BGR16 : LADYBUG_BGR,
            &procImage);
        HandleError(error);
        
        
        char fileName[100];
        sprintf(
            fileName,
            OUTPUT_FILE_NAME.c_str(),
            m_renderData.outputDirectory.c_str(),
            frameIndex,
            surface,
            FILE_EXTENSION.c_str());


        error = ladybugSaveImage(
            m_renderData.context, 
            &procImage, 
            fileName, 
            fileFormat, 
            false);
        HandleError(error);
    }
    
    return error;
}

LadybugError CubeMap::ProcessStream()
{
    LadybugError error = LADYBUG_OK;
    LadybugImage currentImage;

    for (unsigned int frameIndex = 0; frameIndex < m_readData.numberOfFrames; frameIndex++)
    {
        error = ladybugGoToImage(m_readData.context, frameIndex);
        HandleError(error);

        error = ladybugReadImageFromStream(
            m_readData.context,
            &currentImage);
        HandleError(error);

        const LadybugPixelFormat pixelFormatToUse = IsHighBitDepth(currentImage.dataFormat) ? LADYBUG_BGRU16 : LADYBUG_BGRU;

        error = ladybugConvertImage(
            m_renderData.context, 
            &currentImage, 
            &m_renderData.textureBuffers[0],
            pixelFormatToUse);
        HandleError(error);

        error = ladybugUpdateTextures(
            m_renderData.context, 
            LADYBUG_NUM_CAMERAS, 
            (const unsigned char**) &m_renderData.textureBuffers[0],
            pixelFormatToUse);
        HandleError(error);
        
        error = SaveCubeFrame(frameIndex, currentImage.dataFormat);
        HandleError(error);

    }

    return error;
}