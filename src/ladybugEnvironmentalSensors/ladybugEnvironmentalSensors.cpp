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
// This example illustrates the simplest procedure for querying the
// environmental sensors included on some ladybug cameras.
//
// This program does:
//  - create a context
//  - initialize a camera
//  - query the temperature sensor
//  - query the humidity sensor
//  - query the barometer sensor
//  - query the compass sensor
//  - query the accelerometer sensor
//  - query the gyroscope sensor
//  - destroy the context
//
//=============================================================================

// This is needed for math.h constants like M_PI in MSVC.
#define _USE_MATH_DEFINES
#include <math.h>

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ladybug.h"
#include "ladybugsensors.h"


void temperatureExample(LadybugContext context);
void humidityExample(LadybugContext context);
void barometerExample(LadybugContext context);
void compassExample(LadybugContext context);
void accelerometerExample(LadybugContext context);
void gyroscopeExample(LadybugContext context);
float compassHeading(float x, float y, float z);
float RadToDeg(float x);
void printSensorInfo(const LadybugSensorInfo & info);
void handleError(LadybugError error, const char* message = NULL);

int main( int /* argc */, char** /* argv[] */ )
{    
    // Initialize context.
    LadybugContext context;
    LadybugError error = ::ladybugCreateContext(&context);
    handleError( error );

    // Initialize the first ladybug on the bus.
    printf( "Initializing...\n" );
    error = ::ladybugInitializeFromIndex(context, 0);
    handleError( error );

    // Get camera info
    LadybugCameraInfo caminfo;
    error = ladybugGetCameraInfo(context, &caminfo);
    handleError( error, "ladybugGetCameraInfo()" );

    // Read the various sensors (see functions below).
	temperatureExample(context);
	humidityExample(context);
	barometerExample(context);
	compassExample(context);
	accelerometerExample(context);
	gyroscopeExample(context);

    // Destroy the context
    printf( "Destroying context...\n" );
    error = ::ladybugDestroyContext(&context);
    handleError( error );

	// Done
    printf( "Done.\n" );

    return 0;
}

void temperatureExample(LadybugContext context)
{
	// Status.
	printf("Probing temperature sensor...\n");

	// Get the sensor info.
	LadybugSensorInfo info;
	LadybugError error = ladybugGetSensorInfo( context, TEMPERATURE, & info );
	handleError( error );

	// Print sensor info
	printSensorInfo( info );

	// Print the sensor value if the sensor is supported.
	if ( info.isSupported )
	{
		float value = 0.0;
		error = ladybugGetSensor( context, TEMPERATURE, & value );
		handleError( error );
		printf("    Temperature: %f\n", value);		
	}

	// Newline at end.
	printf("\n");
}

void humidityExample(LadybugContext context)
{
	// Status.
	printf("Probing humidity sensor...\n");

	// Get the sensor info.
	LadybugSensorInfo info;
	LadybugError error = ladybugGetSensorInfo( context, HUMIDITY, & info );
	handleError( error );

	// Print sensor info
	printSensorInfo( info );

	// Print the sensor value if the sensor is supported.
	if ( info.isSupported )
	{
		float value = 0.0;
		error = ladybugGetSensor( context, HUMIDITY, & value );
		handleError( error );
		printf("    Humidity: %f\n", value);		
	}

	// Newline at end.
	printf("\n");
}

void barometerExample(LadybugContext context)
{
	// Status.
	printf("Probing barometer sensor...\n");

	// Get the sensor info.
	LadybugSensorInfo info;
	LadybugError error = ladybugGetSensorInfo( context, BAROMETER, & info );
	handleError( error );

	// Print sensor info
	printSensorInfo( info );

	// Print the sensor value if the sensor is supported.
	if ( info.isSupported )
	{
		float value = 0.0;
		error = ladybugGetSensor( context, BAROMETER, & value );
		handleError( error );
		printf("    Barometer: %f\n", value);		
	}

	// Newline at end.
	printf("\n");
}

void compassExample(LadybugContext context)
{
	// Status.
	printf("Probing compass sensor...\n");

	// Get the sensor info.
	LadybugSensorInfo info;
	LadybugError error = ladybugGetSensorInfo( context, COMPASS, & info );
	handleError( error );

	// Print sensor info
	printSensorInfo( info );

	// Print the sensor value if the sensor is supported.
	if ( info.isSupported )
	{
		LadybugTriplet value;
		error = ladybugGetSensorAxes( context, COMPASS, & value );
		handleError( error );
		printf( "    Compass: %e, %e, %e\n", value.x, value.y, value.z );		
		printf( "    Heading: %f\n", compassHeading( value.x, value.y, value.z ) );		
	}

	// Newline at end.
	printf("\n");
}

// This will compute the heading of the camera's x-axis (the focal axis of camera 0).
//  -1 => magnetic flux reading is not strong enough to determine heading.
//   0 => camera 0 is facing north.
//  90 => camera 0 is facing east.
// 180 => camera 0 is facing south.
// 270 => camera 0 is facing west.
float compassHeading(float x, float y, float z)
{
	// NOTE: If the camera is used upside down you will need to negate the y axis.

	// Compute the magnitude
	const float magnitude = static_cast<float>(sqrt(x*x + y*y + z*z));
	if ( magnitude < FLT_EPSILON )
    {
        return -1.0;
    }

	// Normalize vector.
	x /= magnitude;
	y /= magnitude;
	z /= magnitude;

	// Compute heading.
	float heading = RadToDeg(static_cast<float>(atan2(y, x)));
	if ( heading < 0.0 ) 
	{
		heading += 360;
	}

	return heading;
}

float RadToDeg(float x)
{
	return x * 180 / (float) M_PI;
}

void accelerometerExample(LadybugContext context)
{
	// Status.
	printf("Probing accelerometer sensor...\n");

	// Get the sensor info.
	LadybugSensorInfo info;
	LadybugError error = ladybugGetSensorInfo( context, ACCELEROMETER, & info );
	handleError( error );	

	// Print sensor info
	printSensorInfo( info );

	// Print the sensor value if the sensor is supported.
	if ( info.isSupported )
	{
		LadybugTriplet value;
		error = ladybugGetSensorAxes( context, ACCELEROMETER, & value );
		handleError( error );
		printf( "    Accelerometer: %e, %e, %e\n", value.x, value.y, value.z );		
	}

	// Newline at end.
	printf("\n");
}

void gyroscopeExample(LadybugContext context)
{
	// Status.
	printf("Probing gyroscope sensor...\n");

	// Get the sensor info.
	LadybugSensorInfo info;
	LadybugError error = ladybugGetSensorInfo( context, GYROSCOPE, & info );
	handleError( error );	

	// Print sensor info
	printSensorInfo( info );

	// Print the sensor value if the sensor is supported.
	if ( info.isSupported )
	{		
		LadybugTriplet value;
		error = ladybugGetSensorAxes( context, GYROSCOPE, & value );
		handleError( error );
		printf( "    Gyroscope: %e, %e, %e\n", value.x, value.y, value.z );		
	}

	// Newline at end.
	printf("\n");
}

void printSensorInfo(const LadybugSensorInfo & info)
{
	printf("    isSupported       : %s\n", info.isSupported ? "true" : "false");
	printf("    min               : %f\n", info.min);
	printf("    max               : %f\n", info.max);
	printf("    units             : %s\n", info.units);
	printf("    units abbreviated : %s\n", info.unitsAbbr);
}

void handleError(LadybugError error, const char* message)
{
	if( error != LADYBUG_OK )
	{
		if ( message == NULL )
		{
			printf("Error: Ladybug library reported - %s\n", ::ladybugErrorToString( error ) );
		}
		else
		{
			printf("Error: Ladybug library reported in %s - %s\n", message, ::ladybugErrorToString( error ) );
		}
		
		exit(1);
	}
}
