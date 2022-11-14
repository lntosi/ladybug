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

#include "GPSInsert.h"
#include <iostream>
#include <limits>

#ifdef _WIN32

#define TEMPNAM _tempnam

#else 

#define TEMPNAM tempnam

#endif 

namespace
{
    const std::string ERROR_OUTPUT = "Error: Ladybug library reported - %s\n";
    const std::string TMP_CONFIG_FILE = "config";
    const unsigned int NO_FRAME = std::numeric_limits<unsigned int>::max();

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

    void HandleError(LadybugError e)
    {
        if (e != LADYBUG_OK)
        {
            printf(ERROR_OUTPUT.c_str(), ladybugErrorToString(e));
            exit(EXIT_FAILURE);
        }
    }

}

GPSInsert::GPSInsert(std::string inputStreamPath, std::string outputStreamPath)
{
    LadybugError error;

    // read stream 
    error = ladybugCreateStreamContext(&m_readStream.context);
    HandleError(error);

    m_readStream.configFile = getTempName(TMP_CONFIG_FILE);

    m_readStream.numberOfFrames = 0;
    m_readStream.currentFrameNumber = NO_FRAME;

    error = ladybugInitializeStreamForReading(m_readStream.context, inputStreamPath.c_str());
    HandleError(error);

    error = ladybugGetStreamNumOfImages(m_readStream.context, &m_readStream.numberOfFrames);
    HandleError(error);

    error = ladybugGetStreamHeader(m_readStream.context, &m_readStream.headerInfo);
    HandleError(error);

    error = ladybugGetStreamConfigFile(m_readStream.context, m_readStream.configFile.c_str());
    HandleError(error);

    // write stream
    error = ladybugCreateStreamContext(&m_writeContext);
    HandleError(error);

    error = ladybugInitializeStreamForWritingEx(
        m_writeContext,
        outputStreamPath.c_str(),
        &m_readStream.headerInfo,
        m_readStream.configFile.c_str(),
        true);
    HandleError(error);

    // Delete temp config file 
    if (std::remove(m_readStream.configFile.c_str()) != 0)
    {
        //Failed to remove the file. Handle error?
        std::cout << "Warning: temp file " << m_readStream.configFile << " was unable to be deleted." << std::endl;
    }

    SetNmeaSentences();
}

GPSInsert::~GPSInsert()
{
    LadybugError error;

    error = ladybugStopStream(m_readStream.context);
    HandleError(error);
    error = ladybugDestroyContext(&m_readStream.context);
    HandleError(error);

    error = ladybugStopStream(m_writeContext);
    HandleError(error);
    error = ladybugDestroyContext(&m_writeContext);
    HandleError(error);
}

bool GPSInsert::GetNextImage()
{
    LadybugError error;
    bool imageAvailable = true;

    if (m_readStream.currentFrameNumber == NO_FRAME)
    {
        m_readStream.currentFrameNumber = 0;
    }
    else
    {
        m_readStream.currentFrameNumber++;
    }

    if (m_readStream.currentFrameNumber < m_readStream.numberOfFrames)
    {
        error = ladybugGoToImage(m_readStream.context, m_readStream.currentFrameNumber);
        HandleError(error);

        error = ladybugReadImageFromStream(m_readStream.context, &m_readStream.image);
        HandleError(error);

    }
    else
    {
        imageAvailable = false;
    }

    return imageAvailable;
}

LadybugError GPSInsert::InsertGPSData()
{
    LadybugError error = LADYBUG_OK;
    
    while (GetNextImage())
    {
        std::string nmeaSentence = GetNmeaSentence();

        error = ladybugWriteGPSDataToImage(
            m_readStream.context,
            &m_readStream.image,
            nmeaSentence.c_str(),
            nmeaSentence.size());
        HandleError(error);

        error = ladybugWriteImageToStream(m_writeContext, &m_readStream.image);
        HandleError(error);

    }

    return error;
}

void GPSInsert::SetNmeaSentences()
{
    // single sentence examples
    m_nmeaSentences.push("$GPRMC,193251.167,A,5000.00001,N,04500.00001,E,10.00,0.0,240316,0.0,E*62");
    m_nmeaSentences.push("$GPGLL,5000.00001,N,04500.00001,E,193251.167,A*3D");
    m_nmeaSentences.push("$GPGGA,193251.167,5000.00001,N,04500.00001,E,1,5,0.0,0.0,M,0.0,M,,*50");

    // multi sentence examples 1024 byte max lenght
    m_nmeaSentences.push("$GPGSA,A,3,05,24,17,30,02,,,,,,,,5.6,3.3,4.5*34$GPRMC,193252.167,A,5000.00001,N,04500.00001,E,10.00,0.0,240316,0.0,E*61");
    m_nmeaSentences.push("$GPGLL,5000.00001,N,04500.00001,E,193252.167,A*3E$GPVTG,0.0,T,0.0,M,10.00,N,18.52,K*41");
    m_nmeaSentences.push("$GPGGA,193252.167,5000.00001,N,04500.00001,E,1,5,0.0,0.0,M,0.0,M,,*53$GPVTG,0.0,T,0.0,M,10.00,N,18.52,K*41");
}
std::string GPSInsert::GetNmeaSentence()
{
    // use a cyclical list to demonstrate adding GPS data to a stream

    const std::string nmeaSentence = m_nmeaSentences.front();
    m_nmeaSentences.pop();
    m_nmeaSentences.push(nmeaSentence);

    return nmeaSentence;
}

