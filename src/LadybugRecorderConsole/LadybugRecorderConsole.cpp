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
// $Id$
//=============================================================================

#include "stdafx.h"
#include "ladybug.h"

#include "Configuration.h"
#include "ConfigurationLoader.h"
#include "ImageGrabber.h"
#include "ImageRecorder.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <iostream>

using namespace std;

namespace
{
#ifndef _WIN32

int kbhit()
{
   int count = 0;
   struct termios otty, ntty;

   tcgetattr(STDIN_FILENO, &otty);
   ntty = otty;
   ntty.c_lflag &= ~ICANON;

   if(tcsetattr(STDIN_FILENO, TCSANOW, &ntty) == 0)
   {
      ioctl(STDIN_FILENO, FIONREAD, &count);
      tcsetattr(STDIN_FILENO, TCSANOW, &otty);
   }

   return count;
}

#endif

bool WasKeyPressed()
{
#ifdef _WIN32

    return _kbhit() != 0;

#else

    return kbhit() != 0;

#endif
}

}

void GrabLoop( ImageGrabber &grabber, ImageRecorder &recorder )
{
    LadybugImage currentImage;
    while (!WasKeyPressed())
    {
        const LadybugError acquisitionError = grabber.Acquire(currentImage);
        if (acquisitionError != LADYBUG_OK)
        {
            // Error
            cerr << "Failed to acquire image. Error (" << ladybugErrorToString(acquisitionError) << ")" << endl;
            continue;
        }

        cout << "Image acquired - " << currentImage.timeStamp.ulCycleSeconds << ":" << currentImage.timeStamp.ulCycleCount << endl;

        double mbWritten = 0.0;
        unsigned long imagesWritten = 0;
        const LadybugError writeError = recorder.Write(currentImage, mbWritten, imagesWritten);
        if (writeError != LADYBUG_OK)
        {
            // Error
            cerr << "Failed to write image to stream (" << ladybugErrorToString(writeError) << ")" << endl;
            continue;
        }

        cout << imagesWritten << " images - " << mbWritten << "MB" << endl;

        grabber.Unlock(currentImage.uiBufferIndex);
    }
}

int main(int /*argc*/, char** /*argv*/)
{
    // Load configuration from XML
    ConfigurationProperties config;
	string pathToConfigFile;
    try
    {
#ifdef _WIN32
        pathToConfigFile = "LadybugRecorderConsole.xml";
#else
        pathToConfigFile = "/etc/ladybug/LadybugRecorderConsole.xml";
#endif
		cout << "Loading configuration from " << pathToConfigFile << endl;
		config  = ConfigurationLoader::Parse(pathToConfigFile);
    }
    catch (const std::runtime_error& e)
    {
        cerr << "Error: " << e.what() << endl;
        return -1;
    }

    cout << config.ToString() << endl;

    // Initialize grabber
    ImageGrabber grabber;
    const LadybugError grabberInitError = grabber.Init();
    if (grabberInitError != LADYBUG_OK)
    {
        cerr << "Error: " << "Failed to initialize camera (" << ladybugErrorToString(grabberInitError) << ")" << endl;
        return -1;
    }

    grabber.SetConfiguration(config.camera, config.gps);

    // Get the camera information
    LadybugCameraInfo camInfo;
    grabber.GetCameraInfo(camInfo);

    // Initialize recorder
    ImageRecorder recorder(config.stream);
    const LadybugError recorderInitError = recorder.Init(grabber.GetCameraContext(), camInfo.serialBase);
    if (recorderInitError != LADYBUG_OK)
    {
        std::string additionalInformation = "";

        if (recorderInitError == LADYBUG_COULD_NOT_OPEN_FILE)
        {
            additionalInformation = " This may be caused by permission issues with the destination directory. Try setting the desination directory to a location that does not require admin privilege.";
        }

        cerr << "Error: " << "Failed to initialize stream (" << ladybugErrorToString(recorderInitError) << ")." << additionalInformation << endl;
        return -1;
    }

    const LadybugError startError = grabber.Start();
    if (startError != LADYBUG_OK)
    {
        cerr << "Error: " << "Failed to start camera (" << ladybugErrorToString(startError) << ")" << endl;
        return -1;
    }

    cout << "Successfully started camera and stream" << endl;

    GrabLoop(grabber, recorder);

    cout << "Stopping..." << endl;

    // Shutdown
    grabber.Stop();
    recorder.Stop();

    cout << "Stopped" << endl;
    cout << "Goodbye" << endl;

    return 0;
}
