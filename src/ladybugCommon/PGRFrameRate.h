//=============================================================================
// Copyright © 2017 FLIR Integrated Imaging Solutions, Inc. All Rights Reserved.
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
#ifndef __PGRFRAMERATE_H__
#define __PGRFRAMERATE_H__

//=============================================================================
// System Includes
//=============================================================================


/**
 * Calculate and store camera frame rates.  Uses the high-resolution 
 * performance counter.
 */
class PGRFrameRate  
{
public:

   /** 
    * Default constructor. 
    *
    * @param iBufferSize The size of the internal buffer to hold the timestamps.
    *                    If this is 10, then the frame rate is calculated based on the
    *                    timestamps of the past 10 frames.
    */
   PGRFrameRate( int iBufferSize = 10);

   /** Default destructor */
   virtual ~PGRFrameRate();

   /** Returns the current frame rate. */
   double getFrameRate() const;

   /** Sets the current frame rate. */
   void setFrameRate( double dFrameRate );

   /** Call when there is a new frame event. */
   void newFrame();

   /** Returns a text representation of the current frame rate. */
   const char* toString();


protected:
   
   int m_iBufferSize;
   int m_iCurIndex;
   double m_dFrameRate;

   __int64  m_nFrequency;
   __int64  *m_buffer;
   
   char  m_pszTempString[ 32 ];
   
};



#endif // #ifndef __PGRFRAMERATE_H__
