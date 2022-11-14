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
#include "stdafx.h"

//=============================================================================
// System Includes
//=============================================================================
#include <cassert>
#include <cstdio>
//#include <windows.h>

//=============================================================================
// ProjectIncludes
//=============================================================================
#include "PGRFrameRate.h"



PGRFrameRate::PGRFrameRate( int iBufferSize )
{
   m_dFrameRate   = 0.0;
   m_iBufferSize = iBufferSize;
   m_buffer = new __int64[ m_iBufferSize];
   m_iCurIndex = 0;

   QueryPerformanceFrequency( (LARGE_INTEGER*)&m_nFrequency );

   strcpy( m_pszTempString, "" );
}

PGRFrameRate::~PGRFrameRate()
{
   delete []m_buffer;
}


double 
PGRFrameRate::getFrameRate() const
{
   return m_dFrameRate;
}


void 
PGRFrameRate::setFrameRate( double dPGRFrameRate )
{
   m_dFrameRate = dPGRFrameRate;
}


void 
PGRFrameRate::newFrame()
{
   __int64 timeLast, timeFirst;
   int numFrames = 0;

   QueryPerformanceCounter( (LARGE_INTEGER*)&timeLast );
   
   if ( m_iCurIndex < m_iBufferSize - 1)
   {
      m_buffer[ m_iCurIndex] = timeLast;
      timeFirst = m_buffer[ 0];
      numFrames = m_iCurIndex;
      m_iCurIndex++;
   }
   else
   {
      // Buffer is full. Shift the elements.
      timeFirst = m_buffer[ 0];
      for ( int i = 1; i < m_iBufferSize; i++)
      {
         m_buffer[ i - 1] = m_buffer[ i];
      }
      // Put the latest timestamp on the last.
      m_buffer[ m_iCurIndex] = timeLast;
      numFrames = m_iBufferSize;
   }

   __int64 i64Diff = timeLast - timeFirst;
   if( i64Diff != 0 )
   {
      m_dFrameRate = (double)numFrames / ( i64Diff / (double)m_nFrequency);
   }
   else
   {
      // Set frame rate to 0 for the first call to this method
      m_dFrameRate = 0.0;
   }
}


const char* 
PGRFrameRate::toString()
{
   _snprintf( 
      m_pszTempString, sizeof( m_pszTempString ), "%.2f", m_dFrameRate );

   return m_pszTempString;
}