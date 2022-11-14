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

#pragma once

#include <string>
#include <queue>
#include "ladybugrenderer.h"
#include "ladybugstream.h"

class GPSInsert
{
public:
    GPSInsert(std::string, std::string);
    virtual ~GPSInsert();
    
    LadybugError InsertGPSData();

private:
    
    struct ReadStream
    {
        LadybugStreamContext context;
        LadybugStreamHeadInfo headerInfo;
        std::string configFile;

        LadybugImage image;

        unsigned int numberOfFrames;
        unsigned int currentFrameNumber;

    } m_readStream;

    LadybugStreamContext m_writeContext;
    std::queue<std::string> m_nmeaSentences;

    bool GetNextImage();
    void SetNmeaSentences();
    std::string GetNmeaSentence();

};

