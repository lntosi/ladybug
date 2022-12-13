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

#include "ladybug.h"
#include "ladybugstream.h"
#include <vector>
#include <string>

class CubeMap
{
public:
    CubeMap(std::string, std::string, int);
    virtual ~CubeMap();
    
    LadybugError ProcessStream();

private:
    
    enum Surface { FRONT, RIGHT, BACK, LEFT, TOP, BOTTOM, NUMBER_OF_SURFACES };

    struct ReadData
    {
        LadybugStreamContext context;
        std::string filePath;
        int m_currentFrame;
        unsigned int numberOfFrames;
        int width;
        int height;

    } m_readData;

    struct RenderData
    {
        LadybugContext context;
        std::vector<unsigned char*> textureBuffers;
        std::string outputDirectory;

    } m_renderData;

    LadybugError SaveCubeFrame(unsigned int, LadybugDataFormat);

};

