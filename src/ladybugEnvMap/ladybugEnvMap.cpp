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
// ladybugEnvMap.cpp
//
//=============================================================================
// This example illustrates how to apply cube mapping on Ladybug's spherical
// images to construct a skybox.
// In computer graphics, cube mapping is a type of environment mapping used to
// simulate surfaces that reflect the scene at a distant location. 
// Here, Ladybug images are used as the environment and are updated in real 
// time.
// For each scene, six surfaces of a cube are rendered.
// This is done by rendering Ladybug's spherical view 6 times, setting the 
// field of view to 90 degrees and positioning the virtual camera to specific
// surface directions. These rendering results are then used as textures for the
// cube mapping. 
// The overall scene, which is comprised of the reflective objects, 
// is then rendered. 
// All calculations required to construct the cube map are handled inside the
// OpenGL library.
//
// Note: This example must be run with freeglut.dll present.
//
//=============================================================================
//=============================================================================
// System Includes
//=============================================================================

#pragma warning(push)
#pragma warning(disable:704 4100 4201 4244 4458 4786) 

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glh/glh_glut.h>

#pragma warning(pop)

//=============================================================================
// PGR Includes
//=============================================================================
#include <ladybug.h>
#include <ladybuggeom.h>
#include <ladybugrenderer.h>

//=============================================================================
// Project Includes
//=============================================================================

using namespace glh;

#define UPDATE_SURFACE

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

#define _DISPLAY_ERROR_MSG \
    if( error != LADYBUG_OK ) \
{ \
    printf( "Ladybug library reported %s\n", \
    ladybugErrorToString( error ) ); \
} \

#define _DISPLAY_ERROR_MSG_AND_RETURN \
    if( error != LADYBUG_OK ) \
{ \
    printf( "Ladybug library reported %s\n", \
    ladybugErrorToString( error ) ); \
    return; \
} \

//
// Define various constants
//
#define PI 3.1415926535f

#define CUBE_TEX_SIZE 512
#define NUM_CUBE_SURFACES  6

const GLenum cube_map_targets[ NUM_CUBE_SURFACES] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB
};

const double sphere_view_params[ NUM_CUBE_SURFACES][ 3] = {
    { PI, 0.0, PI},
    { PI, 0.0, 0.0},
    { PI/2.0, PI/2.0, 0.0},
    { -PI/2.0, -PI/2.0, 0.0},
    { PI, 0.0, -PI/2.0},
    { PI, 0.0, PI/2.0}
};

static const GLubyte colors[ NUM_CUBE_SURFACES][ 3] = {
    { 255,   0,   0 }, // red
    {   0, 255, 255 }, // cyan
    {   0, 255,   0 }, // green
    { 255,   0, 255 }, // magenda
    {   0,   0, 255 }, // blue
    { 255, 255,   0 }  // yellow
};

enum
{
    MENU_TEAPOT,
    MENU_SPHERE,
    MENU_CUBE,
    MENU_EXIT
};

int displayMode = MENU_TEAPOT;

//
// The GLUT mouse interactor. it is used automatically to deal with
// mouse inputs.
//
glut_simple_mouse_interactor object;

//
// Various global variables used in the example
// 
LadybugContext context;
LadybugError error;
LadybugImage image;
bool b[256]; 
GLuint fbos[NUM_CUBE_SURFACES];
GLuint fboTexture;
int menu;  
GLUquadricObj* quadric;  // for sphere

void 
handleGLError( const char* pszString )
{
    const GLenum glError = glGetError();

    if (glError != GL_NO_ERROR)
    {
        char  pszErrorText[ 512 ];

        sprintf( 
            pszErrorText, 
            "OpenGL Error: %s: #(%d) %s\n", 
            pszString,
            error,
            ::gluErrorString(glError) );

        fprintf( stderr, pszErrorText );
    }
}

//=============================================================================
//
// Clean up
//
//=============================================================================
void
cleanUp()
{
    printf( "Stopping camera...\n" );
    error = ladybugStop( context );
    _HANDLE_ERROR;

    printf( "Release off-screen image resource...\n" );
    error = ladybugReleaseOffScreenImage(context, LADYBUG_SPHERICAL );
    _HANDLE_ERROR;

    printf( "Destroying context...\n" );
    error = ladybugDestroyContext( &context );
    _HANDLE_ERROR;

    gluDeleteQuadric( quadric);

    glDeleteTextures( 1, &fboTexture);

    glDeleteFramebuffersEXT( NUM_CUBE_SURFACES, &fbos[0]);

    return;
}

void 
selectFromMenu( int iCommand )
{  
    if ( iCommand == MENU_EXIT )
    {
        cleanUp();
        exit (0);
    }
    else
    {
        displayMode = iCommand;
    }
}

int 
buildPopupMenu (void)
{
    menu = glutCreateMenu( selectFromMenu );

    glutAddMenuEntry( 
        "Teapot", 
        MENU_TEAPOT );

    glutAddMenuEntry( 
        "Sphere", 
        MENU_SPHERE );

    glutAddMenuEntry( 
        "Cube", 
        MENU_CUBE);

    glutAddMenuEntry( 
        "Exit", 
        MENU_EXIT );

    return 0;
}


void init_checkers( void )
{
    GLubyte* pImage = new GLubyte[ CUBE_TEX_SIZE * CUBE_TEX_SIZE * 3];

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /* make colored checkerboard cube faces */
    for (GLint f = 0; f < NUM_CUBE_SURFACES; f++) 
    {
        for (GLint i = 0; i < CUBE_TEX_SIZE; i++)
        {
            for (GLint j = 0; j < CUBE_TEX_SIZE; j++)
            {
                if ((i/4 + j/4) & 1) 
                {
                    pImage[( i + j * CUBE_TEX_SIZE) * 3 + 0] = colors[f][0];
                    pImage[( i + j * CUBE_TEX_SIZE) * 3 + 1] = colors[f][1];
                    pImage[( i + j * CUBE_TEX_SIZE) * 3 + 2] = colors[f][2];
                }
                else 
                {
                    pImage[( i + j * CUBE_TEX_SIZE) * 3 + 0] = 255;
                    pImage[( i + j * CUBE_TEX_SIZE) * 3 + 1] = 255;
                    pImage[( i + j * CUBE_TEX_SIZE) * 3 + 2] = 255;
                }
            }
        }

        glTexImage2D( 
            cube_map_targets[f], 
            0, 
            GL_RGB, 
            CUBE_TEX_SIZE, 
            CUBE_TEX_SIZE, 
            0,
            GL_RGB, 
            GL_UNSIGNED_BYTE, 
            pImage);
        handleGLError( "glTexImage2D");
    }

    delete [] pImage;
}

void
initEnvMap( void)
{
    GLenum res;

    glGenTextures( 1, &fboTexture);

    glBindTexture( GL_TEXTURE_CUBE_MAP, fboTexture);
    handleGLError( "glBindTexture");
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 0 );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, 3 );

    init_checkers();

    glGenFramebuffersEXT( NUM_CUBE_SURFACES, &fbos[0]);
    handleGLError( "glGenFramebuffersEXT");

    for ( int i = 0; i < NUM_CUBE_SURFACES; i++){

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbos[i]);
        handleGLError( "glBindFramebufferEXT");

        //Attach 2D texture to this FBO
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, cube_map_targets[ i], fboTexture, 0);
        handleGLError( "glFramebufferTexture2DEXT");

        res = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT);
        if ( res != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            printf( "Failed - glCheckFramebufferStatusEXT = %X\n", res);
        }
    }

    // unbind
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    handleGLError( "glBindFramebufferEXT - unbind");

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // init for sphere
    quadric=gluNewQuadric();
    gluQuadricNormals( quadric, GLU_SMOOTH);
    gluQuadricTexture( quadric, GL_TRUE);
}

void drawCube()
{
    glBegin(GL_QUADS);

    // Front Face
    glNormal3f( 0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
    // Back Face
    glNormal3f( 0.0f, 0.0f,-1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
    // Top Face
    glNormal3f( 0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
    // Bottom Face
    glNormal3f( 0.0f,-1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
    // Right face
    glNormal3f( 1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
    // Left Face
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);

    glEnd();
}

void drawSkybox( void )
{
    const GLfloat br = 20.0; /* box radius */

    glBegin(GL_QUADS);

    /* +X side */
    glTexCoord3f(1.0, -1.0, -1.0);  glVertex3f(br, -br, -br);
    glTexCoord3f(1.0, -1.0,  1.0);  glVertex3f(br, -br,  br);
    glTexCoord3f(1.0,  1.0,  1.0);  glVertex3f(br,  br,  br);
    glTexCoord3f(1.0,  1.0, -1.0);  glVertex3f(br,  br, -br);

    /* -X side */
    glTexCoord3f(-1.0,  1.0, -1.0);  glVertex3f(-br,  br, -br);
    glTexCoord3f(-1.0,  1.0,  1.0);  glVertex3f(-br,  br,  br);
    glTexCoord3f(-1.0, -1.0,  1.0);  glVertex3f(-br, -br,  br);
    glTexCoord3f(-1.0, -1.0, -1.0);  glVertex3f(-br, -br, -br);

    /* +Y side */
    glTexCoord3f(-1.0, 1.0, -1.0);  glVertex3f(-br,  br, -br);
    glTexCoord3f(-1.0, 1.0,  1.0);  glVertex3f(-br,  br,  br);
    glTexCoord3f( 1.0, 1.0,  1.0);  glVertex3f( br,  br,  br);
    glTexCoord3f( 1.0, 1.0, -1.0);  glVertex3f( br,  br, -br);

    /* -Y side */
    glTexCoord3f(-1.0, -1.0, -1.0);  glVertex3f(-br, -br, -br);
    glTexCoord3f(-1.0, -1.0,  1.0);  glVertex3f(-br, -br,  br);
    glTexCoord3f( 1.0, -1.0,  1.0);  glVertex3f( br, -br,  br);
    glTexCoord3f( 1.0, -1.0, -1.0);  glVertex3f( br, -br, -br);

    /* +Z side */
    glTexCoord3f( 1.0, -1.0, 1.0);  glVertex3f( br, -br, br);
    glTexCoord3f(-1.0, -1.0, 1.0);  glVertex3f(-br, -br, br);
    glTexCoord3f(-1.0,  1.0, 1.0);  glVertex3f(-br,  br, br);
    glTexCoord3f( 1.0,  1.0, 1.0);  glVertex3f( br,  br, br);

    /* -Z side */
    glTexCoord3f( 1.0,  1.0, -1.0);  glVertex3f( br,  br, -br);
    glTexCoord3f(-1.0,  1.0, -1.0);  glVertex3f(-br,  br, -br);
    glTexCoord3f(-1.0, -1.0, -1.0);  glVertex3f(-br, -br, -br);
    glTexCoord3f( 1.0, -1.0, -1.0);  glVertex3f( br, -br, -br);

    glEnd();
}

void drawUnitSquare( void)
{
    glBegin( GL_QUADS);

    glTexCoord2d( 0.0, 1.0); glVertex3f( 0.0, 0.0, 0.0);
    glTexCoord2d( 1.0, 1.0); glVertex3f( 1.0, 0.0, 0.0);
    glTexCoord2d( 1.0, 0.0); glVertex3f( 1.0, 1.0, 0.0);
    glTexCoord2d( 0.0, 0.0); glVertex3f( 0.0, 1.0, 0.0);

    glEnd();
}

//=============================================================================
//
// Start the first Ladybug camera on the bus
//
//=============================================================================
int  
startCamera()
{    
    //
    // Initialize context.
    //
    printf( "Creating Ladybug context...\n" );
    error = ladybugCreateContext( &context );
    _HANDLE_ERROR;

    //
    // Initialize the first ladybug on the bus.
    //
    printf( "Initializing first camera on the bus...\n" );
    error = ladybugInitializeFromIndex( context, 0 );
    _HANDLE_ERROR;

    //
    // Start Ladybug with JPEG data format
    //
    printf( "Starting camera in JPEG mode...\n" );
    error = ladybugStart(
        context,
        LADYBUG_DATAFORMAT_COLOR_SEP_JPEG8);
    _HANDLE_ERROR;

    //
    // Load config file
    //
    printf( "Loading config info...\n" );
    error = ladybugLoadConfig( context, NULL );
    _HANDLE_ERROR;

    //
    // Grab one image
    //
    printf( "Grabbing an image..\n" );
    error = LADYBUG_FAILED;
    for ( int i = 0; i < 10 && error != LADYBUG_OK; i++)
    {
        error = ladybugGrabImage( context, &image);
    }
    if ( error != LADYBUG_OK)
    {
        _HANDLE_ERROR;
    }

    //
    // Set color processing method to Downsample4
    //
    error = ladybugSetColorProcessingMethod( context, LADYBUG_DOWNSAMPLE4 );
    _HANDLE_ERROR;

    return 0;
}

//=============================================================================
// Draw for environment mapping
//=============================================================================
void drawEnvironmentMapping( void)
{
    unsigned int        uiTextureID = 0;

    error = ladybugSetSphericalViewParams( context, 90.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    error = ladybugRenderOffScreenImage( context, LADYBUG_SPHERICAL, LADYBUG_BGR, NULL);
    _DISPLAY_ERROR_MSG;

    float validWidth, validHeight;
    error = ladybugGetOpenGLTextureID( 
        context, LADYBUG_SPHERICAL, &uiTextureID, &validWidth, &validHeight );
    _DISPLAY_ERROR_MSG;

    glPushAttrib (GL_ALL_ATTRIB_BITS);

#ifdef UPDATE_SURFACE
    // update the 6 cube surfaces using the spherical view's rendering output.
    for ( int f = 0; f < NUM_CUBE_SURFACES; f++)
    {
        // bind FBO
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbos[ f]);
        handleGLError( "glBindFramebufferEXT");

        GLenum res = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT);
        if ( res != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            printf( "Failed - glCheckFramebufferStatusEXT = %X\n", res);
        }

        glViewport( 0, 0, CUBE_TEX_SIZE, CUBE_TEX_SIZE);

        glClearColor( colors[f][0], colors[f][1], colors[f][2], 0.0f);
        glClearDepth( 1.0);   
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_GEN_R);

        glEnable( GL_TEXTURE_2D);
        glDisable( GL_TEXTURE_CUBE_MAP);

        glMatrixMode( GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D( 0.0, 1.0, 0.0, 1.0);

        glMatrixMode( GL_MODELVIEW);
        glPushMatrix();      
        glLoadIdentity();

        error = ladybugSetSphericalViewParams(
            context, 
            90.0f,
            static_cast<float>(sphere_view_params[f][0]),
            static_cast<float>(sphere_view_params[f][1]),
            static_cast<float>(sphere_view_params[f][2]),
            0.0f, 
            0.0f, 
            0.0f);
        error = ladybugRenderOffScreenImage( context, LADYBUG_SPHERICAL, LADYBUG_BGR, NULL);
        _DISPLAY_ERROR_MSG;

        glEnable( GL_TEXTURE_2D);

        glBindTexture( GL_TEXTURE_2D, uiTextureID);
        handleGLError( "glBindTexture");

        drawUnitSquare();

        glPopMatrix();

        glMatrixMode( GL_PROJECTION);
        glPopMatrix();

    }
#endif

    // unbind FBO
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    glPopAttrib();

    //
    // Render the scene
    //

    glEnable( GL_TEXTURE_CUBE_MAP);
    if ( GLEW_EXT_framebuffer_object )
    {
        // Generate Mipmap
        glGenerateMipmapEXT( GL_TEXTURE_CUBE_MAP);
        handleGLError( "glGenerateMipmapEXT");   
    }
    glDisable( GL_TEXTURE_2D);

    glBindTexture( GL_TEXTURE_CUBE_MAP, fboTexture);
    handleGLError( "glBindTexture");   

    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);

    glMatrixMode( GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    drawSkybox();

    glPopMatrix();

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);

    switch ( displayMode)
    {
    case MENU_TEAPOT:
        glutSolidTeapot( 1.0);  
        break;
    case MENU_SPHERE:
        gluSphere( quadric, 1.0f, 32, 32);
        break;
    case MENU_CUBE:
        drawCube();
        break;
    }

    glDisable(GL_TEXTURE_CUBE_MAP);
    glEnable( GL_TEXTURE_2D);
}

//=============================================================================
// Display Ladybug images
//=============================================================================
void
display()
{    
    glClearColor((GLclampf)0.2, (GLclampf)0.2, (GLclampf)0.2, (GLclampf)0.0);
    glClearDepth( 1.0);

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glEnable( GL_DEPTH_TEST );

    glMatrixMode( GL_MODELVIEW) ;
    glPushMatrix();
    glLoadIdentity();

    //
    // Apply transformation
    //
    object.apply_transform();

    glShadeModel( GL_FLAT );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
    glEnable( GL_TEXTURE_2D );

    drawEnvironmentMapping();

    glPopMatrix();

    //
    // Make sure changes appear on screen
    // 
    glutSwapBuffers();
}

//=============================================================================
//
// Grab image, process it and update image textures on the graphics card
//
//=============================================================================
void 
grabImage()
{
    if ( b[' '] ) 
    {
        object.trackball.increment_rotation();
    }

    //
    // Grab an image
    //
    error = ladybugGrabImage( context, &image );
    _DISPLAY_ERROR_MSG_AND_RETURN;

    //
    // Parse images to arpBuffers.
    //
    error = ladybugConvertImage( context, &image, NULL );    
    _DISPLAY_ERROR_MSG_AND_RETURN;

    //
    // Update images to the graphics card
    //
    error = ladybugUpdateTextures( 
        context, 
        LADYBUG_NUM_CAMERAS, 
        NULL );
    _DISPLAY_ERROR_MSG_AND_RETURN;

    //
    // Mark the current window for redisplay
    //
    glutPostRedisplay(); 
}

void 
key(unsigned char k, int x, int y)
{
    b[k] = !b[k];

    if( k==27 || k=='q') 
    {
        cleanUp();
        exit(0);
    }

    object.keyboard(k, x, y);

    glutPostRedisplay();
}

void 
resize( int w, int h )
{
    if (h == 0) 
    {
        h = 1;
    }

    glViewport(0, 0, w, h);

    //
    // Set OpenGL projection as perspective viewing
    //
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( 45.0, (GLfloat)w/(GLfloat)h, 0.1, 100.0 );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    object.reshape( w, h );
}

void 
mouse( int button, int state, int x, int y )
{
    object.mouse( button, state, x, y );
}

void 
motion( int x, int y )
{
    object.motion( x, y );
}

//=============================================================================
// 
// Main program
//
//=============================================================================
int 
main(int argc, char** argv)
{
    //
    // GLUT Window Initialization
    //
    glutInit( &argc, argv );
    glutInitWindowSize( 800, 600 );
    glutInitDisplayMode ( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
    glutInitWindowPosition( 120, 100 );
    glutCreateWindow ( "Environment mapping with Ladybug" );

    //
    // Initialize GLEW
    //
    GLenum err = glewInit();
    if ( err != GLEW_OK )
    {
        printf( "Failed to init GLEW.\n");
        return 0;
    }

    startCamera( );

    error = ladybugConfigureOutputImages( context, LADYBUG_SPHERICAL);
    _HANDLE_ERROR;

    error = ladybugSetDisplayWindow( context );
    _HANDLE_ERROR

        initEnvMap();

    // Track ball - left buton
    // Pan - Shift + left button
    // Dolly - Ctrl + left 
    object.configure_buttons( 1 );

    //
    // Move the view point back
    //
    object.dolly.dolly[2] = -4;

    //
    // Register callbacks:
    //
    glutDisplayFunc( display );
    glutIdleFunc( grabImage );
    glutMouseFunc( mouse );
    glutMotionFunc( motion );
    glutKeyboardFunc( key );
    glutReshapeFunc( resize );

    b[' '] = true;

    buildPopupMenu( );
    glutAttachMenu( GLUT_RIGHT_BUTTON );

    glutCloseFunc( cleanUp );

    //
    // Turn the flow of control over to GLUT
    //
    printf( "Grabbing and display...\n" );
    glutMainLoop( );

    return 0;
}
