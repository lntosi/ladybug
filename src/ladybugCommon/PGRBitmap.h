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
#ifndef __PGRBITMAP_H__
#define __PGRBITMAP_H__

//=============================================================================
// System Includes
//=============================================================================
#include <windows.h>
#include <cstdio>
#include <cassert>

//=============================================================================
// PGR Includes
//=============================================================================
//#include "../LadybugUtils/glAPI.h"

/**
 * This class encapsulates the necessary functions to draw a bitmap to a win32
 * window, plus some extra goodies.
 *
 * @note Please use this version instead of the deprecated PGRResettableBitmap.
 * @note This function no longer has any upwards-deps.  Triclops support
 *       is now in PGRMFC/PGRBitmapTriclops.*.
 *
 *
 * @todo Further interface documentation.
 * @todo Implement an init() function that will allocate memory instead of 
 *       doing it all in the constructor.
 * @todo Support 4 bits per pixel.
 *
 * @author myk mwhite@ptgrey.com
 * $Id: PGRBitmap.h,v 1.23 2005-11-10 21:11:07 matt Exp $
 */
class PGRBitmap
{
public:

   //=============================================================================
   // Static Methods
   //=============================================================================

   /**
    * Copies the data into a pre-allocated destination bitmap.
    *
    * @bug Don't use this function.
    */
   static void copy( PGRBitmap& bitmapIn, PGRBitmap& bitmapOut );


   //==========================================================================
   // Construction/Destruction
   //==========================================================================

   /**
    * Default constructor.  Init a bitmap with an image buffer of size 
    * 1024x768x24bits
    */
   PGRBitmap();

   /**
    * Construct a bitmap of user-defined size.  Allocates an image buffer of 
    * size iWidth x iHeight x 24bits.
    */   
   PGRBitmap( int iWidth, int iHeight );

   /**
    * Construct a bitmap using external memory.  
    *
    * @bug Use (unsigned char*)NULL to not specifiy memory at this point 
    *      (using NULL (which is 0 in win32) will cause the next constructor
    *      to be used.)
    */
   PGRBitmap( int iWidth, int iHeight, unsigned char* pImageData );

   /**
    * Construct a bitmap of specified dimensions.  Allocates an appropriate
    * image buffer.
    */
   PGRBitmap( int iWidth, int iHeight, int iBitsPerPixel );

   /**
    * Construct a bitmap of specified dimensions using the passed external
    * pointer.
    */
   PGRBitmap( 
      int	     iWidth, 
      int	     iHeight, 
      int	     iBitsPerPixel, 
      unsigned char* pImageData );

   /**
    * Default destructor
    */
   virtual ~PGRBitmap();

   
   //==========================================================================
   // Public Methods
   //==========================================================================
   
   /**
    * Returns a pointer to the data, for setting the image contents directly.
    * If the image size changes, a call to setImageDimensions() must also be 
    * made.
    *
    * @see setImageDimensions()
    */
   unsigned char* getDataPointer();

   /**
    * Allows the user to directly set the data pointer.  This should only be
    * if the bitmap does not own its own data.  This has similar functionality
    * to setBitmap().
    */
   BOOL setDataPointer( unsigned char* pBuffer );

   /**
    * Set the image dimensions.  Note that this does not change the buffer
    * size, it only changes the output size.
    */
   int setImageDimensions( int iWidth, int iHeight );

   /**
    * Retrieve the image dimensions.
    */
   int getImageDimensions( int&	iWidth, int& iHeight );

   /**
    * Copy in image data.
    *
    * @bug Don't use this function.
    */
   void copyInBitmap( int iWidth, int iHeight, int iBitsPerPixel, unsigned char* pData );

   /**
    * Set the bitmap from the specified parameters.  If the current memory
    * is owned by the bitmap, it will be freed.
    */
   void	 setBitmap( int iWidth, int iHeight, int iBitsPerPixel, unsigned char* pData );

   /**
    * Paint the current bitmap to a device.
    *
    * @param hDC  The handle to the device context.  Use GetSaveHDC() (or
    *             something) under win32.
    *
    * @param iDestXOrigin  Destination X origin in the window.  Use 0.
    * @param iDestYOrigin  Destination Y origin in the window.  Use 0.
    * @param iDestWidth	   Width to use.  This determintes stretching if 
    *                      necessary.  Don't supply this param.
    * @param iDestHeight   Height to use.  THis determines stretching if 
    *                      necessary.  Don't supply this param.
    */
   int paintToDevice( 
      HDC hDC,
      int iDestXOrigin, 
      int iDestYOrigin,
      int iDestWidth = -1, 
      int iDestHeight = -1 );

   /**
    * Save image in bitmap format to the specified path and filename.
    * 
    * @note Must be in BGR (24 bit) or BGRU (32 bit) format.
    */
   BOOL saveImageToBMP( const char* pszFilename );

   /**
    * Save image in .ppm format to the specified path and filename.
    *
    * @note Assumes RGB format. (does not munge the bytes before writing to
    *       disk.)
    * @note Assumes 24 bit image.
    */
   BOOL saveImageToPPM( const char* pszFilename );

   /**
    * Save image in .ppm format to the specified path and filename.
    *
    * @note Must be in BGR (24 bit) or BGRU (32 bit) format.
    */
   BOOL saveImageBGRToPPM( const char* pszFilename );

   /**
    * Save an 8-bit image to PGM format.
    *
    * @note Assumes 8 bit image.
    */
   BOOL saveImageToPGM( const char* pszFilename );

   /**
    * Fills the current pointed-to memory with an "attractive" b/w ramp.
    * @note Assumes 8 bits!
    *
    */
   void	 fillWithBWRamp();
   

protected:
   //=============================================================================
   // Private Methods
   //=============================================================================

   /**
    * Internal helper function.
    */
   void initBitmapInfo();

   /**
    * Internal helper function.
    */
   void initBitmapInfoHeader();


protected:
   //=============================================================================
   // Private Member Variables
   //=============================================================================
   
   /**
    * Whether this object owns the data or not (ie, whether it has created it and
    * will free it upon deletion.
    */
   BOOL	 m_bOwnsData;

   /**
    * Image bpp.
    */
   int	 m_iBitsPerPixel;

   /**
    * Image cols.
    */
   int	 m_iWidth;

   /**
    * Image rows.
    */
   int	 m_iHeight;

   /**
    * The image data, in any format.  For higher colour depths than 8, this
    * is always assumed to be BGR.  But really, it doesn't matter unless you're
    * displaying or saving images.
    */
   unsigned char*  m_pData;
   
   /**
    * Bitmap info structure for painting to a windows device handle.
    */
   BITMAPINFO* m_pBitmapInfo;
};


#endif // !__PGRBITMAP_H__
