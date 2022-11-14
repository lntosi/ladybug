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
//
// ladybugAdvancedRenderEx.cpp
//
//=============================================================================
// This example shows users how to draw Ladybug 3D sphere image with other 3D 
// objects. To draw any 3D objects together with Ladybug 3D sphere image,
// ladybugDisplayImage() has to be called prior to drawing any objects. 
// The size and the drawing position of the objects has to be inside of the 
// Ladybug sphere. Otherwise the objects will not be seen. The OpenGL depth 
// test has to be enabled. 
//
// Note: This example has to be run with freeglut.dll
//       This example needs to open the following PPM file as textures:
//       TextureCam0.ppm  
//       TextureCam1.ppm  
//       TextureCam2.ppm  
//       TextureCam3.ppm  
//       TextureCam4.ppm  
//       TextureCam5.ppm  
//=============================================================================
//=============================================================================
// System Includes
//=============================================================================
#pragma warning(push)
#pragma warning(disable:4100 4201 4244 4305 4458 4786)

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <assert.h>
#include <GL/freeglut.h>
#include <glh/glh_glut.h>

#pragma warning(pop)

//=============================================================================
// PGR Includes
//=============================================================================
//=============================================================================
// Project Includes
//=============================================================================
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>

using namespace glh;

//
// Define some error handlers
//
#define _HANDLE_ERROR \
   if( error != LADYBUG_OK ) \
{ \
   printf( "Error! Ladybug library reported %s\n", \
   ladybugErrorToString( error ) ); \
   assert( false ); \
   exit( 1 ); \
} \
   
#define _DISPLAY_ERROR_MSG_AND_RETURN \
   if( error != LADYBUG_OK ) \
{ \
   printf( "Ladybug library reported %s\n", \
   ladybugErrorToString( error ) ); \
   return; \
} \


//
// Define number of columns and rows in the 3D grid
//
#define	    _3D_GRID_COLS   32
#define	    _3D_GRID_ROWS   24

//
// Define display modes
//
enum 
{
   MENU_DRAW_SPHERICAL_VIEW = 1,
   MENU_DRAW_SPHERICAL_VIEW_WITH_MESH,
   MENU_DRAW_SPHERICAL_VIEW_WITH_MESH_AND_CAMERA_INDICATOR,
   MENU_EXIT
};

//
// The GLUT mouse interactor. it is used automatically to deal with
// mouse inputs.
//
glut_simple_mouse_interactor object;

//
// Various global variables used in the example
// 
static unsigned long uiDisplayMode = MENU_DRAW_SPHERICAL_VIEW;
LadybugContext context;
LadybugError error;
LadybugImage image;
bool b[256];
int textureRows, textureCols;
int menu;

//
// Buffers for holding the processed Ladybug images
//
unsigned char*		arpBuffers[ LADYBUG_NUM_CAMERAS ];

//
// 3D Maps
//
const LadybugImage3d*	arp3dImages[ LADYBUG_NUM_CAMERAS ];

//
// OpenGL texture IDs
//
unsigned int		arTextureID[ LADYBUG_NUM_CAMERAS ];


//=============================================================================
//
// Clean up
//
//=============================================================================
void
cleanUp()
{ 
   printf( "Stopping camera...\n" );
   error = ::ladybugStop( context );
   _HANDLE_ERROR;

   printf( "Destroying context...\n" );
   error = ::ladybugDestroyContext( &context );
   _HANDLE_ERROR;

   glDeleteTextures( LADYBUG_NUM_CAMERAS, arTextureID );

   for( int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
   {
      if ( arpBuffers[ uiCamera ] != NULL )
      {
	 delete  [] arpBuffers[ uiCamera ];
      }
   }

   glutDestroyMenu( menu );

   return;
}


//=============================================================================
//
// Display mode selection
//
//=============================================================================
void 
selectFromMenu( int iCommand )
{  
   if ( iCommand == MENU_EXIT)
   {
      cleanUp();
      exit (0);
   }
   else
   {
      uiDisplayMode = iCommand;
   }
   
   // Redraw the window
   glutPostRedisplay();
}


//=============================================================================
//
// Create a popup menu for selecting display modes
//
//=============================================================================
int 
buildPopupMenu()
{
   menu = glutCreateMenu ( selectFromMenu );

   glutAddMenuEntry ( 
      "Draw spherical view", 
      MENU_DRAW_SPHERICAL_VIEW );

   glutAddMenuEntry ( 
      "Draw spherical view with mesh", 
      MENU_DRAW_SPHERICAL_VIEW_WITH_MESH );

   glutAddMenuEntry ( 
      "Draw spherical view with mesh and camera indicator", 
      MENU_DRAW_SPHERICAL_VIEW_WITH_MESH_AND_CAMERA_INDICATOR );

   glutAddMenuEntry ( 
      "Exit", 
      MENU_EXIT);
   
   return 0;
}


//=============================================================================
//
// Read a binary Portable Pixel Map (PPM) file.
// Returns true if there is no error.
//
//=============================================================================
bool 
loadPPM( char		*fileName, 
	 int		*pXSize, 
	 int		*pYSize, 
	 int		*pMaxRGB,
	 unsigned char	**ppszRGBA )
{
   bool	       loadError;
   ifstream    file_In;
   int	       iStep;
   char	       pszLine[255];
   char	       pszWord[255];
   char	       *pNext;
   int	       iCount;
   int	       iWidth;
   
   //
   // Open the file
   //
   file_In.open ( fileName, ios::binary );
   if ( !file_In )
   {
      printf("\n Fatal error, Cannot open the PPM file: %s\n", fileName);
      return false;
   }

   //
   //  Read the header.
   //
   iStep = 0;
   while ( 1 )
   {
      file_In.getline( pszLine, sizeof ( pszLine ) );
      if ( file_In.eof() )
      {
	 printf("\n Fatal error, End of file. \n" );
	 return false;
      }

      pNext = pszLine;

      if ( pszLine[0] == '#' )
	 continue;

      if ( iStep == 0 )
      {
	 iCount = sscanf ( pNext, "%s%n", pszWord, &iWidth );
	 if ( iCount == EOF )
	    continue;

	 pNext = pNext + iWidth;

	 if ( strcmp ( pszWord, "P6" ) != 0 && strcmp ( pszWord, "p6" ) != 0 )
	 {
	    printf("\n Fatal error, Bad magic number. \n" );
	    return false;
	 }

	 iStep = 1;
      }

      if ( iStep == 1 )
      {
	 iCount = sscanf ( pNext, "%d%n", pXSize, &iWidth );
	 pNext = pNext + iWidth;
	 if ( iCount == EOF )
	    continue;

	 iStep = 2;
      }

      if ( iStep == 2 )
      {
	 iCount = sscanf ( pNext, "%d%n", pYSize, &iWidth );
	 pNext = pNext + iWidth;	 
	 if ( iCount == EOF )
	    continue;

	 iStep = 3;
      }

      if ( iStep == 3 )
      {
	 iCount = sscanf ( pNext, "%d%n", pMaxRGB, &iWidth );
	 pNext = pNext + iWidth;
	 if ( iCount == EOF )
	    continue;

	 break;
      }
   }

   //
   // Allocate storage for the data.
   // Add space for alpha channel too, thus we multiply by 4
   //
   *ppszRGBA = new unsigned char[(*pXSize) * (*pYSize) * 4];

   //
   //  Read in the data.
   //
   unsigned char *pIndex = *ppszRGBA;
   for ( int j = 0; j < *pYSize; j++ )
   {
      for ( int i = 0; i < *pXSize; i++ )
      {
	 file_In.read ( (char*)pIndex, 3 );
     loadError = file_In.eof();
	 if (loadError)
	 {
	    printf("\n Reading PPM - Fatal error. \n" );
	    return false;
	 }

	 //
	 // Set alpha channel. The value of the alpha channel is calculated
	 // by taking the average intensity of the RGB pixels.
	 // A value of 0 will mean that the pixel is considered to be
	 // completely transparent and vice versa.
	 // 
	 *(pIndex + 3) = ( *(pIndex+0) + *(pIndex+1) + *(pIndex+2) ) / 3;
	 pIndex = pIndex + 4;
      }
   }

   //
   //  Close the file.
   //
   file_In.close( );

   return true;
}


//=============================================================================
//
// Start the first Ladybug2/Ladybug3 camera on the bus
//
//=============================================================================
int  
startCamera()
{    
   //
   // Initialize context.
   //
   error = ladybugCreateContext( &context );
   _HANDLE_ERROR;
   
   //
   // Initialize the first ladybug on the bus.
   //
   printf( "Initializing.\n" );
   error = ladybugInitializeFromIndex( context, 0 );
   _HANDLE_ERROR;
   
   //
   // Start Ladybug2/Ladybug3 with JPEG data format
   //
   printf( "Starting camera...\n" );
   error = ladybugStart(
      context,
      LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8);
   _HANDLE_ERROR;
   
   //
   // Load config file from the head
   //
   printf( "Loading config info...\n" );
   error = ladybugLoadConfig( context, NULL );
   _HANDLE_ERROR;
   

   //
   // Grab one successful image
   //
   error = LADYBUG_FAILED;
   for ( int i = 0; i < 10 && error != LADYBUG_OK; i++)
   {
      error = ladybugGrabImage( context, &image);
   }
   _HANDLE_ERROR;
   
   //
   // Set color processing method to Downsample4
   //
   error = ladybugSetColorProcessingMethod( context, LADYBUG_DOWNSAMPLE4 );
   _HANDLE_ERROR;

   //
   // set texture size - it's half of the original image because we use LADYBUG_DOWNSAMPLE4 color processing
   //
   textureRows = image.uiRows/2;
   textureCols = image.uiCols/2;

   //
   // Initialize alpha masks
   //
   printf( "Initializing Alpha mask...\n" );
   error = ladybugInitializeAlphaMasks( context, textureCols, textureRows);
   _HANDLE_ERROR;

   //
   // Use alpha masks
   //
   printf( "Enabling alpha masks...\n" );
   ladybugSetAlphaMasking( context, true );
   _HANDLE_ERROR;
   
   
   //
   // Allocate memory for the 6 processed images
   //
   for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
   {
      arpBuffers[ uiCamera ] = new unsigned char[ textureCols * textureRows * 4 ];
   }
   
   return 0;
}


//=============================================================================
//
// Load OpenGL textures from a PPM file
//
//=============================================================================
void 
loadTextures()	
{
   GLint	     sizeX;
   GLint	     sizeY;
   GLint	     maxRGB;
   unsigned char*    pszData = NULL;
   char		     szFileName[ 16 ];
   bool		     bReadSuccess = false;
   
   //
   // Generate texture name
   //
   glGenTextures( LADYBUG_NUM_CAMERAS, arTextureID );

   for ( int i = 0; i < LADYBUG_NUM_CAMERAS; i++ )
   {
      sprintf( szFileName, "TextureCam%01d.ppm", i );
      printf( "Loading PPM file:%s as texture.\n", szFileName);

      bReadSuccess = loadPPM( szFileName, &sizeX, &sizeY, &maxRGB, &pszData );

      if( bReadSuccess )
      {
	 glBindTexture( GL_TEXTURE_2D, arTextureID[i]);
	 glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	 glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

	 glTexImage2D( 
	    GL_TEXTURE_2D, 
	    0, 
	    GL_RGBA8, 
	    sizeX, 
	    sizeY, 
	    0,
	    GL_RGBA, 
	    GL_UNSIGNED_BYTE, 
	    pszData );
      }
      else
      {
	 printf( "Failed reading %s\n", szFileName );
      }

      if( pszData != NULL )
      {
	 delete [] pszData;
	 pszData = NULL;
      }
   }
}


//=============================================================================
//
// Draw 3D objects.
// We draw the Ladybug 3D meshes here.
//
//=============================================================================
void 
drawObjectInSphere()
{
   unsigned int	     uiRows = 0;
   unsigned int	     uiCols = 0;   
   LadybugPoint3d*   ppoint3d0 = NULL;
   LadybugPoint3d*   ppoint3d1 = NULL;
   
   float fCameraColors[6][3] = {  
      { 1.0f, 1.0f, 1.0f },
      { 1.0f, 0.0f, 0.0f },
      { 0.0f, 1.0f, 0.0f },
      { 0.0f, 0.0f, 1.0f },
      { 1.0f, 1.0f, 0.0f },
      { 0.5f, 0.5f, 0.5f },
   };
      
   glPushAttrib (GL_ALL_ATTRIB_BITS);
   glDisable( GL_TEXTURE_2D );
   glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

   //
   // Draw Ladybug 3D mesh
   //
   glMatrixMode( GL_MODELVIEW );
   glPushMatrix();
   glRotatef( 90, 0, 1, 0 ); 
   glRotatef( -90, 1, 0, 0 );

   //
   // Make the mesh smaller than the 3D sphere
   //
   glScalef(0.9f, 0.9f, 0.9f);

   for( unsigned int uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
   {
      const LadybugImage3d* pimage = arp3dImages[ uiCamera ];
      if( pimage == NULL )
      {
	 continue;
      }
      
      glColor3fv( &fCameraColors[ uiCamera ][0] );
      uiRows = pimage->uiRows;
      uiCols = pimage->uiCols;

      //
      // Step through each successive pair of rows and 
      // generate triangles as a strip
      //
      for( unsigned int uiRow = 0; uiRow < uiRows - 1; uiRow++ )
      {
	 glBegin( GL_TRIANGLE_STRIP );

	 for( unsigned int uiCol = 0; uiCol < uiCols; uiCol++ )
	 {		
	    ppoint3d0 = &pimage->ppoints[ ( uiRow + 0 ) * uiCols + uiCol ];
	    ppoint3d1 = &pimage->ppoints[ ( uiRow + 1 ) * uiCols + uiCol ];
	    
	    glVertex3f( ppoint3d0->fX, ppoint3d0->fY, ppoint3d0->fZ );
	    glVertex3f( ppoint3d1->fX, ppoint3d1->fY, ppoint3d1->fZ );
	 } 
	 
	 glEnd();
      } 
   }   
   
   glPopMatrix();
   glPopAttrib();
}


//=============================================================================
//
// Draw a four-sided polygon with texture mapping.
//
//=============================================================================
void 
drawSquareAtCenter()
{
   glBegin( GL_QUADS ); 

   glTexCoord2f( 0, 0 ); 
   glVertex3f( -0.5f,  0.5f, 0.0 );
   
   glTexCoord2f( 0, 1 ); 
   glVertex3f( -0.5f, -0.5f, 0.0 );
   
   glTexCoord2f( 1, 1 ); 
   glVertex3f(  0.5f, -0.5f, 0.0 );
   
   glTexCoord2f( 1, 0 ); 
   glVertex3f(  0.5f,  0.5f, 0.0 );

   glEnd();
}


//=============================================================================
//
// Draw six squares
//
//=============================================================================
void 
drawCameraIndicators()
{
   // Set up the rasterization settings
   glEnable( GL_TEXTURE_2D );
   glPolygonMode( GL_FRONT, GL_FILL );
   glPolygonMode( GL_BACK, GL_LINE );

   //
   // Use OpengGL blending.
   // If the alpha value of the texture is 0, then it will be transparent.
   //
   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
   glEnable( GL_BLEND );

   glMatrixMode( GL_MODELVIEW );

   for ( int i = 0; i < LADYBUG_NUM_CAMERAS ; i++ )
   {
      glBindTexture( GL_TEXTURE_2D, arTextureID[ i ] );
      glPushMatrix();

      if ( i == 5 )
      {
	 //
	 // For top camera, rotate it to the top
	 //
	 glRotatef( 90, 1, 0, 0 ); 
      }
      else
      {
	 //
	 // For camera 0 - 4, rotate it to the right position.
	 //
	 glRotatef(-(i * 360.0f / (LADYBUG_NUM_CAMERAS-1)), 0, 1, 0);
      }

      //
      // Push it away from the center
      //
      glTranslatef( 0.0, 0.0, -2.0 );

      //
      // Draw a square at the center
      //
      drawSquareAtCenter();
      
      glPopMatrix();
   }

   glDisable( GL_BLEND );
}


//=============================================================================
//
// Display ladybug images
//
//=============================================================================
void
display()
{   
   glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
   glClearDepth( 1.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   //
   // Enable depth test
   //
   glEnable(GL_DEPTH_TEST);
   
   //
   // Initialize modelview projection matrix
   //
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   //
   // Apply transformation
   //
   object.apply_inverse_transform();
   
   glShadeModel( GL_FLAT );
   glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
   glEnable( GL_TEXTURE_2D );

   //
   // Draw the scene
   //
   switch ( uiDisplayMode )
   {
   case MENU_DRAW_SPHERICAL_VIEW:
      //
      // Draw spherical view using the API function
      //
      error = ladybugDisplayImage( context, LADYBUG_SPHERICAL );
      _DISPLAY_ERROR_MSG_AND_RETURN;
      break; 

   case MENU_DRAW_SPHERICAL_VIEW_WITH_MESH:
      //
      // Draw spherical view using Ladybug API function.
      // This function has to be called prior to drawing any other objects
      //
      error = ladybugDisplayImage( context, LADYBUG_SPHERICAL );
      _DISPLAY_ERROR_MSG_AND_RETURN;

      //
      // Draw Ladybug 3D mesh inside the Ladybug image sphere
      //
      drawObjectInSphere( );
      break; 

   case MENU_DRAW_SPHERICAL_VIEW_WITH_MESH_AND_CAMERA_INDICATOR:
      //
      // Draw spherical view using Ladybug API function.
      // This function has to be called prior to drawing any other objects
      //
      error = ladybugDisplayImage( context, LADYBUG_SPHERICAL );
      _DISPLAY_ERROR_MSG_AND_RETURN;

      //
      // Draw Ladybug 3D mesh inside the Ladybug image sphere
      //
      drawObjectInSphere( );

      //
      // Draw 6 3D squares with mapped images indicating the camera positions.
      // The 3D square objects are closer to the view point than all other 
      // objects (in the example). 
      //
      drawCameraIndicators();
      break;
   }
   
   glPopMatrix();
   
   //
   // Make sure changes appear onscreen
   // 
   glutSwapBuffers();
}


//=============================================================================
//
// Grab images, process images and update image textures to the graphics card
//
//=============================================================================
void 
grabImage( void )
{
   if (b[' ']) 
   {
      object.trackball.increment_rotation();
   }
   
   //
   // Grab an image from the camera
   //
   error = ladybugGrabImage( context, &image );
   _DISPLAY_ERROR_MSG_AND_RETURN;
   
   //
   // Parse images to arpBuffers.
   //
   error = ladybugConvertImage( context, &image, arpBuffers );	
   _DISPLAY_ERROR_MSG_AND_RETURN;
   
   //
   // Update images to the graphics card
   //
   error = ladybugUpdateTextures( 
      context, 
      LADYBUG_NUM_CAMERAS,
      const_cast<const unsigned char**>(arpBuffers) );
   _DISPLAY_ERROR_MSG_AND_RETURN;
   
   //
   // Mark the current window for redisplay
   //
   glutPostRedisplay();
}


void 
key(unsigned char k, int x, int y)
{
   b[k] = ! b[k];
   if( k==27 || k=='q') {
      cleanUp();
      exit(0);
   }
   
   object.keyboard(k, x, y);
   
   glutPostRedisplay();
}


void 
resize(int w, int h)
{
   if (h == 0) 
   {
      h = 1;
   }
   
   glViewport(0, 0, w, h);

   //
   // Set OpenGL projection 
   //
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(45.0, (GLfloat)w/(GLfloat)h, 0.1, 100.0);
   
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   
   object.reshape(w, h);
}


void 
mouse(int button, int state, int x, int y )
{
   object.mouse(button, state, x, y );
}


void 
motion(int x, int y)
{
   object.motion(x, y);
}


//=============================================================================
//
// Main program
//
//=============================================================================
int 
main(int argc, char** argv)
{
   unsigned int uiCamera = 0;

   //
   // Initialize the arrays   
   //
   for( uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
   {
      arpBuffers[ uiCamera ] = NULL;
      arp3dImages[ uiCamera ] = NULL;
      arTextureID[ uiCamera ] = 0;
   }
   
   //
   // GLUT Window Initialization
   //
   glutInit( &argc, argv );
   glutInitWindowSize( 800, 600);
   glutInitDisplayMode ( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
   glutInitWindowPosition( 120, 100 );
   glutCreateWindow ( "Ladybug Render images with objects (click right button for menu)" );

   //
   // Start Ladybug
   //
   startCamera();
   
   //
   // Configure Ladybug image output type
   // This example needs to generate spherical images
   //
   error = ladybugConfigureOutputImages( context, LADYBUG_SPHERICAL );
   _HANDLE_ERROR;	
      
   error = ladybugSetDisplayWindow( context );
   _HANDLE_ERROR

   // Load the textures
   loadTextures( ); 
   
   //
   // Get 3D map
   //
   for( uiCamera = 0; uiCamera < LADYBUG_NUM_CAMERAS; uiCamera++ )
   {
      error = ladybugGet3dMap( 
	 context, 
	 uiCamera, 
	 _3D_GRID_ROWS, 
	 _3D_GRID_COLS, 
	 textureRows, 
	 textureCols, 
	 false, 
	 &arp3dImages[uiCamera] );
      _HANDLE_ERROR;
   }
   
   // Track ball - left buton
   // Pan - Shift + left button
   // Dolly - Ctrl + left 
   object.configure_buttons(1);
   
   //
   // Set the viewing point at the origin of the OpengGL coordinate
   //
   object.dolly.dolly[2] = 0;
   
   //
   // Register callbacks:
   //
   glutDisplayFunc( display );
   glutIdleFunc( grabImage );
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutKeyboardFunc(key);
   glutReshapeFunc(resize);
   
   b[' '] = false;//true;
   
   //
   // Create a popup menu
   //
   buildPopupMenu();
   glutAttachMenu(GLUT_RIGHT_BUTTON);
   
   glutCloseFunc( cleanUp );

   //
   // Turn the flow of control over to GLUT
   //
   printf( "Grabbing and display...\n" );
   glutMainLoop();

   return 0;
}

