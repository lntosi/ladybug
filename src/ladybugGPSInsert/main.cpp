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
// This example illustrates how to insert gps data into a pgr stream file.
//
//=============================================================================


#include <iostream>
#include <stdlib.h>
#include "GPSInsert.h"


enum {INPUT_FILE_ARG = 1, OUTPUT_FILE_ARG, NUM_OF_ARGS};

const std::string USAGE = "ladybugGPSInsert [INPUT_FILE] [OUTPUT_FILE_WITH_PATH]";


namespace
{
    void PrintUsage()
    {
        std::cout << USAGE << std::endl;
    }

    void verifyArguments(int numOfArguments)
    {
        if (numOfArguments < NUM_OF_ARGS)
        {
            PrintUsage();
            exit(EXIT_FAILURE);
        }
    }
}

int main(int  argc , char* argv[])
{
    verifyArguments(argc);

    const std::string inputFile = argv[INPUT_FILE_ARG];
    const std::string outputFile = argv[OUTPUT_FILE_ARG];

    GPSInsert gpsInsert(inputFile, outputFile);
    gpsInsert.InsertGPSData();
   
    return EXIT_SUCCESS;
}
