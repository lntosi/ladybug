//=============================================================================
// Copyright � 2017 FLIR Integrated Imaging Solutions, Inc. All Rights Reserved.
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

/*
   This example illustrates how a point in the raw image maps to the point in the stitched
   image using the information such as focal length, image center, and camera extrinsics.
   It also shows that the same result is obtainable by using the 3D map returned by 
   ladybugGet3dMap API.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "ladybug.h"
#include "ladybuggeom.h"
#include "ladybugrenderer.h"

namespace
{
	struct Vector3D
	{
		double x;
		double y;
		double z;
	};

	struct LocationAndDirection
	{
		Vector3D location;
		Vector3D direction;
	};

	double DotProduct(const Vector3D& lhs, const Vector3D& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}

    void GetImageDimensions(LadybugDeviceType deviceType, int& cols, int& rows)
    {
        switch (deviceType)
        {
        case LADYBUG_DEVICE_COMPRESSOR:
            cols = 1024;
            rows = 768;
            break;
        case LADYBUG_DEVICE_LADYBUG3:
            cols = 1616;
            rows = 1232;
            break;
        case LADYBUG_DEVICE_LADYBUG5:
            cols = 2448;
            rows = 2048;
            break;
        case LADYBUG_DEVICE_LADYBUG5P:
            cols = 2464;
            rows = 2048;
            break;
        default:
            cols = 0;
            rows = 0;
        }
    }

	void makeTransformation( const double rotX, const double rotY, const double rotZ, const double transX, const double transY, const double transZ,
							 double matrix[4][4] /*out*/ )
	{
		double cosX, sinX, cosY, sinY, cosZ, sinZ;

		cosX = cos( rotX );		sinX = sin( rotX );
		cosY = cos( rotY );		sinY = sin( rotY );
		cosZ = cos( rotZ );		sinZ = sin( rotZ );

		// translation portion of transform
		matrix[0][3] = transX;
		matrix[1][3] = transY;
		matrix[2][3] = transZ;

		// cz*cy;
		matrix[0][0] = cosZ * cosY; 
		// cz*sy*sx - sz*cx;
		matrix[0][1] = cosZ * sinY * sinX - sinZ * cosX; 
		// cz*sy*cx + sz*sx;
		matrix[0][2] = cosZ * sinY * cosX + sinZ * sinX; 

		// sz*cy;
		matrix[1][0] = sinZ * cosY; 
		// sz*sy*sx + cz*cx;
		matrix[1][1] = sinZ * sinY * sinX + cosZ * cosX; 
		// sz*sy*cx - cz*sx;
		matrix[1][2] = sinZ * sinY * cosX - cosZ * sinX; 

		//-sy;
		matrix[2][0] = -sinY; 
		//cy*sx;
		matrix[2][1] = cosY * sinX; 
		//cy*cx;
		matrix[2][2] = cosY * cosX; 

		// bottom row, always the same
		matrix[3][0] = 0.0;
		matrix[3][1] = 0.0;
		matrix[3][2] = 0.0;
		matrix[3][3] = 1.0;
	}

	void applyTransformation(const double matrix[4][4], double & x, double & y, double & z)
	{
		double outPoint[3];
		for ( int r = 0; r < 3; r++ )
		{
			outPoint[r] = matrix[r][3] 
			            + matrix[r][0] * x
			            + matrix[r][1] * y
			            + matrix[r][2] * z ;
		}
		x = outPoint[0];
		y = outPoint[1];
		z = outPoint[2];
	}

	void manualTranslation( LadybugContext context, const int kCamera, const int kRawX, const int kRawY, const double kSphereSize,
		                    double & dLadybugX /*out*/, double & dLadybugY /*out*/, double & dLadybugZ /*out*/ )
	{
		// Offscreen image size needs to be set before using ladybugGetCameraUnitFocalLength and ladybugGetCameraUnitImageCenter
		const int kRectImageWidth = 400; // this doesn't affect the result
		const int kRectImageHeight = 300; // this doesn't affect the result
		LadybugError error = ladybugSetOffScreenImageSize(context, LADYBUG_PANORAMIC, kRectImageWidth, kRectImageHeight);
		if (error != LADYBUG_OK) { printf( "Error ladybugSetOffScreenImageSize - %s", ladybugErrorToString(error)); exit(1); }

		// Read in information about the camera.
		double dFocalLen = 0.0;
		error = ladybugGetCameraUnitFocalLength(context, kCamera, &dFocalLen);
		if (error != LADYBUG_OK) { printf( "Error ladybugGetCameraUnitFocalLength - %s", ladybugErrorToString(error)); exit(1); }

		double dCameraCenterX = 0.0;
		double dCameraCenterY = 0.0;
		error = ladybugGetCameraUnitImageCenter(context, kCamera, &dCameraCenterX, &dCameraCenterY);
		if (error != LADYBUG_OK) { printf( "Error ladybugGetCameraUnitImageCenter - %s", ladybugErrorToString(error)); exit(1); }

		double dExtrinsics[6] = {0.0};
		error = ladybugGetCameraUnitExtrinsics(context, kCamera, dExtrinsics);
		if (error != LADYBUG_OK) { printf( "Error ladybugGetCameraUnitExtrinsics - %s", ladybugErrorToString(error)); exit(1); }

	    // Map the raw coordinate to a rectified coordinate
		double dRectifiedX = 0.0;
		double dRectifiedY = 0.0;
		error = ladybugRectifyPixel( context, kCamera, kRawY, kRawX, &dRectifiedY, &dRectifiedX );
		if (error != LADYBUG_OK) { printf( "Error ladybugRectifyPixel - %s", ladybugErrorToString(error)); exit(1); }

		printf( "Raw image coordinate = (%d, %d)\n", kRawX, kRawY );
		printf( "Rectified image coordinate = (%lf, %lf)\n", dRectifiedX, dRectifiedY );

		// Map the rectified coordinate to camera-local 3D coordinate (dLocalX, dLocalY, dLocalZ)
		//
		// Here, we solve the following equations:
		//
		//  (Pin-hole camera model)
		//  kRectifiedX = ( dLocalX / dLocalZ) * dFocalLen + dCameraCenterX
		//  kRectifiedY = ( dLocalY / dLocalZ) * dFocalLen + dCameraCenterY
		//
		//  (Set the constraint that the point is on a sphere with the radius of kSphereSize
		//  dLocalX^2 + dLocalY^2 + dLocalZ^2 = kSphereSize^2
		const double kx = (dRectifiedX - dCameraCenterX) / dFocalLen;
		const double ky = (dRectifiedY - dCameraCenterY) / dFocalLen;
		const double dLocalZ = kSphereSize / sqrt(kx * kx + ky * ky + 1.0);
		const double dLocalX = kx * dLocalZ;
		const double dLocalY = ky * dLocalZ;
		printf( "Camera local coordinate = (%lf, %lf, %lf)\n", dLocalX, dLocalY, dLocalZ);

		// Map the camera-local 3D coordinate (dLocalX, dLocalY, dLocalZ) to
		// ladybug (global) coordinate (dLadybugX, dLadybugY, dLadybugZ).
		// We create an explicit representation of Craig's Matrix as described in
		// ladybuggeom.h:ladybugGetCameraUnitExtrinsics().
		const double rotX = dExtrinsics[0];
		const double rotY = dExtrinsics[1];
		const double rotZ = dExtrinsics[2];
		const double transX = dExtrinsics[3];
		const double transY = dExtrinsics[4];
		const double transZ = dExtrinsics[5];
		
		double toGlobalCoords[4][4]; // Craig's Matrix
		makeTransformation( rotX, rotY, rotZ, transX, transY, transZ, toGlobalCoords );
		dLadybugX = dLocalX;
		dLadybugY = dLocalY;
		dLadybugZ = dLocalZ;
		applyTransformation( toGlobalCoords, dLadybugX, dLadybugY, dLadybugZ );

		// scale the point so that it is on the sphere
		const double dLen = sqrt(dLadybugX * dLadybugX + dLadybugY * dLadybugY + dLadybugZ * dLadybugZ);
		dLadybugX = dLadybugX / dLen * kSphereSize;
		dLadybugY = dLadybugY / dLen * kSphereSize;
		dLadybugZ = dLadybugZ / dLen * kSphereSize;

		printf( "Ladybug global coordinates (no rot)  = (%lf, %lf, %lf)\n", dLadybugX, dLadybugY, dLadybugZ );

		// Apply rotation to camera 0 aligned global coordinates.
		double dRx, dRy, dRz;
		error = ladybugGet3dMapRotation( context, & dRx, & dRy, & dRz );
		if (error != LADYBUG_OK) { printf( "Error ladybugGet3dMapRotation - %s", ladybugErrorToString(error)); exit(1); }

		double cam0rotation[4][4];
		makeTransformation( dRx, dRy, dRz, 0.0, 0.0, 0.0, cam0rotation );
		applyTransformation( cam0rotation, dLadybugX, dLadybugY, dLadybugZ );
	}

	void imageDimensionsForContext( LadybugContext context, int& numCols /*out*/, int& numRows /*out*/)
	{
		// Get the camera information
		LadybugCameraInfo camInfo;
		LadybugError error = ladybugGetCameraInfo(context, &camInfo);
		if (error != LADYBUG_OK) { printf( "Error ladybugGetCameraInfo - %s", ladybugErrorToString(error)); exit(1); }

		// Get the image dimensions for the camera info
		GetImageDimensions(camInfo.deviceType, numCols, numRows);
	}

	LadybugPoint3d libraryTranslation( LadybugContext context, const int kCamera, const int kRawX, const int kRawY, const double kSphereSize )
	{
		int srcCols, srcRows;
		imageDimensionsForContext( context, srcCols, srcRows );

		// Set the sphere radius
		ladybugSet3dMapSphereSize( context, kSphereSize );

		// Compare the above result obtained from the manual translation with the output of ladybugGetImage3d
		//
		// There is a slight difference between them, mostly because the resolution of the 3D map created here
		// is much smaller than the resolution of raw image.
		// You can eliminate this by obtaining the 4 nearest-neighbor grid points that surround the raw pixel
		// and then applying the bi-linear interpolation on the 3D coordinates of the 4 grid points.
		const int kGridCols = srcCols;
		const int kGridRows = srcRows;
		const LadybugImage3d* pImage3d;
		LadybugError error = ladybugGet3dMap( context, kCamera, kGridCols, kGridRows, srcCols, srcRows, false, &pImage3d);
		if (error != LADYBUG_OK) { printf( "Error ladybugGet3dMap - %s", ladybugErrorToString(error)); exit(1); }

		// obtaining the grid point that is one of the closest ones.
		const int gridX = kRawX * kGridCols / srcCols;
		const int gridY = kRawY * kGridRows / srcRows;

		return pImage3d->ppoints[ gridY * kGridCols + gridX];
	}

	LocationAndDirection rayTranslation( LadybugContext context, const int kCamera, const int kRawX, const int kRawY )
	{
		LadybugError error = LADYBUG_OK;
		LocationAndDirection ret;

		double dRectifiedY, dRectifiedX;
		error = ladybugRectifyPixel( context, kCamera, kRawY, kRawX, &dRectifiedY, &dRectifiedX );
		if (error != LADYBUG_OK) { printf( "Error ladybugRectifyPixel - %s", ladybugErrorToString(error)); exit(1); }

		error = ladybugRCtoXYZ(context, dRectifiedY, dRectifiedX, kCamera,
			                   &ret.location.x, &ret.location.y, &ret.location.z,
							   &ret.direction.x, &ret.direction.y, &ret.direction.z);
		if (error != LADYBUG_OK) { printf( "Error ladybugRCtoXYZ - %s", ladybugErrorToString(error)); exit(1); }

		return ret;
	}

	bool mapOntoSphere(
		const LocationAndDirection& locAndRot,
		double radius,
		Vector3D& onSphere /*out*/)
	{
		// Check if the location lays outside the sphere
		if (sqrt(DotProduct(locAndRot.location, locAndRot.location)) > radius)
		{
			return false;
		}

		// Solve quadratic formula for:
		// ray(t) dot ray(t) = radius^2
		// where ray(t) = location + t * direction (t >= 0)
		const double a = DotProduct(locAndRot.direction, locAndRot.direction);
		const double b = 2.0 * DotProduct(locAndRot.location, locAndRot.direction);
		const double c = DotProduct(locAndRot.location, locAndRot.location) - pow(radius, 2.0);
		
		// Check the descriminant (this should never get hit).
		const double discriminant = pow(b,2.0) - 4.0 * a * c;
		if (discriminant < 0)
		{
			return false;
		}

		// Grab t
		const double t1 = (-b + sqrt(discriminant)) / (2*a);
		const double t2 = (-b - sqrt(discriminant)) / (2*a);

		double t = t1;
		if (t < 0) { t = t2; }
		if (t < 0) { return false; }

		// Get the point at which the ray intersects the sphere.
		onSphere.x = locAndRot.location.x + t * locAndRot.direction.x;
		onSphere.y = locAndRot.location.y + t * locAndRot.direction.y;
		onSphere.z = locAndRot.location.z + t * locAndRot.direction.z;

		return true;
	}

	void reverseTranslation( LadybugContext context, const int kCamera, const double x, const double y, const double z,
		                     double & unrectifiedX /*out*/, double & unrectifiedY /*out*/ ) 
	{
		unrectifiedX = -1.0;
		unrectifiedY = -1.0;

		double calculatedX = 0.0;
		double calculatedY = 0.0;
		LadybugError error = ladybugXYZtoRC(context, x, y, z, kCamera, &calculatedY, &calculatedX, NULL);
		if (error != LADYBUG_OK) { printf( "Error ladybugXYZtoRC - %s", ladybugErrorToString(error)); exit(1); }

		if (calculatedX > 0.0 && calculatedY > 0.0)
		{           
			ladybugUnrectifyPixel(context, kCamera, calculatedY, calculatedX, &unrectifiedY, &unrectifiedX);
			if (error != LADYBUG_OK) { printf( "Error ladybugUnrectifyPixel - %s", ladybugErrorToString(error)); exit(1); }
		}
	}
}

int main(int /*argc*/, char* /*argv[]*/)
{
	const int kCamera = 3; // camera number.
    const int kRawX = 100; // X position of the raw image
    const int kRawY = 200; // Y position of the raw image
    const double kSphereSize = 20.0; // The radius of the sphere
	const double kMultRay[] = {1.0, 0.2, 5.0}; // Simulates searching along a ray this many times further than the sphere.
	
    // Create a ladybug context.
    LadybugContext context;
    LadybugError error = ladybugCreateContext(&context);
    if (error != LADYBUG_OK) { printf( "Error ladybugCreateContext - %s", ladybugErrorToString(error)); return 1; }

    // Initialize the first camera found on the bus
    error = ladybugInitializeFromIndex(context, 0);
    if (error != LADYBUG_OK) { printf( "Error ladybugCreateContext - %s", ladybugErrorToString(error)); return 1; }

    // Load the calibration from the camera, a stream file is also an acceptable source for the calibration data
    error = ladybugLoadConfig(context, NULL);
    if (error != LADYBUG_OK) { printf( "Error ladybugLoadConfig - %s", ladybugErrorToString(error)); return 1; }

    error = ladybugConfigureOutputImages(context, LADYBUG_PANORAMIC);
    if (error != LADYBUG_OK) { printf( "Error ladybugConfigureOutputImages - %s", ladybugErrorToString(error)); return 1; }

	for (int iMultRay = 0; iMultRay < sizeof(kMultRay)/sizeof(double); ++iMultRay)
	{
		printf("\nRadius = %lf\nProjection multiplier = %lf\n", kSphereSize, kMultRay[iMultRay]);

		// Map coordinates (kRawX, kRawY) via manual transformation into ladybug global coordinates.
		double dLadybugX, dLadybugY, dLadybugZ;
		manualTranslation( context, kCamera, kRawX, kRawY, kSphereSize, dLadybugX, dLadybugY, dLadybugZ );
		dLadybugX *= kMultRay[iMultRay];
		dLadybugY *= kMultRay[iMultRay];
		dLadybugZ *= kMultRay[iMultRay];
		printf( "Ladybug global coordinates (manual)  = (%lf, %lf, %lf)\n", dLadybugX, dLadybugY, dLadybugZ );

		// Map  coordinates (kRawX, kRawY) via ladybug library functionality into ladybug global coorindates.
		LadybugPoint3d point3d = libraryTranslation( context, kCamera, kRawX, kRawY, kSphereSize );
		point3d.fX *= (float) kMultRay[iMultRay];
		point3d.fY *= (float) kMultRay[iMultRay];
		point3d.fZ *= (float) kMultRay[iMultRay];
		printf( "Ladybug global coordinates (library) = (%lf, %lf, %lf)\n", point3d.fX, point3d.fY, point3d.fZ );

		// Map  coordinates (kRawX, kRawY) via ladybug library functionality into ladybug global coorindates.
		LocationAndDirection locAndDir = rayTranslation( context, kCamera, kRawX, kRawY );
		Vector3D onSphere;
		if (! mapOntoSphere(locAndDir, kSphereSize*kMultRay[iMultRay], onSphere)) { printf( "Error in mapOntoSphere"); return 1; }
		printf( "Ladybug global coordinates (ray)     = (%lf, %lf, %lf)\n", onSphere.x, onSphere.y, onSphere.z );

		// Reverse back to 2D point from 3D point
		double unrectifiedX, unrectifiedY;

		reverseTranslation( context, kCamera, dLadybugX, dLadybugY, dLadybugZ, unrectifiedX, unrectifiedY );
		if (unrectifiedX != -1.0 && unrectifiedY != -1.0)
		{
			printf("2D point from Ladybug global coordinate (manual)  = (%f, %f)\n", unrectifiedX, unrectifiedY );
		}

		reverseTranslation( context, kCamera, point3d.fX, point3d.fY, point3d.fZ, unrectifiedX, unrectifiedY );
		if (unrectifiedX != -1.0 && unrectifiedY != -1.0)
		{
			printf("2D point from Ladybug global coordinate (library) = (%f, %f)\n", unrectifiedX, unrectifiedY );
		}

		reverseTranslation( context, kCamera, onSphere.x, onSphere.y, onSphere.z, unrectifiedX, unrectifiedY );
		if (unrectifiedX != -1.0 && unrectifiedY != -1.0)
		{
			printf("2D point from Ladybug global coordinate (ray)     = (%f, %f)\n", unrectifiedX, unrectifiedY );
		}
	}

	// Cleanup
    error = ladybugDestroyContext( &context );
    if (error != LADYBUG_OK) { printf( "Error ladybugDestroyContext - %s", ladybugErrorToString(error) ); return 1; }

    return 0;
}