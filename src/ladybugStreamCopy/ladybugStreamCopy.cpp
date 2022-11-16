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
// ladybugStreamCopy.cpp
// 
// This program is used to copy images from a Ladybug source stream
// to a destination stream. If a calibration file is specified, this 
// program will write this calibration file to the destination file 
// instead of using the calibration file in the source stream. 
//
// The last two arguments are used to specify how many images to copy. 
// It they are not specified, copy all the images.
// 
//
//=============================================================================

//=============================================================================
// System Includes
//=============================================================================
//

#pragma warning(disable:4701 4703)

#include <cstdio>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <iostream>

#include <ladybugstream.h>

#define _HANDLE_ERROR \
    if( error != LADYBUG_OK ) \
{ \
    printf( "Error! Ladybug library reported %s\n", \
    ::ladybugErrorToString( error ) ); \
    goto _EXIT; \
} 

#ifdef _WIN32

#define TEMPNAM _tempnam

#else 

#define TEMPNAM tempnam

#endif 

namespace
{
    std::string getTempName(std::string fallBackName)
    {
        char* tempPath = NULL;
        std::string tempPathString;
        tempPath = TEMPNAM(NULL, NULL);

        if (tempPath == NULL)
        {
            //The TMP / TMPDIR enviroment variable is invalid or missing. Default to working directory. 
            tempPathString = fallBackName;
        }
        else
        {
            tempPathString = tempPath;
            free(tempPath);
        }

        return tempPathString;
    }
}

//
// echo program usage
//
void usage()
{
    printf (
        "Usage :\n"
        "\t ladybugStreamCopy SrcFileName OutputFileName [calFile] [From] [To]\n"
        "\n"
        "where\n"
        "\t SrcFileName - one of the source PGR stream file name \n\n"
        "\t OutputFileName - the destination PGR stream file name, including the path to the destination directory\n\n"
        "\t [calFile] - optional, the calibration file used to write to the destination file. \n"
        "\t If not specified, config file in the source stream is used.\n"
        "\t Specify \"default\" if you don't want to specify this but want to specify subsequent arguments. \n\n"
        "\t [From] - the number of the first image to copy \n\n"
        "\t [To] - the number of the last image to copy \n\n"
        "\t [From] and [To] are optional. If they are not specified, copy all the images \n"
        "\t These arguments are positional sensitive and are optional only if\n"
        "\t any subsequent arguments are left as default as well.\n"
        "\n\n"
        "\t Note: A Ladybug stream is a set of Ladybug stream files that share \n"
        "\t a common stream base name. \n"
        "\t For example: There are 11 stream files:\n"
        "\t c:\\Recorded\\LadybugStream-003000.pgr \n"
        "\t c:\\Recorded\\LadybugStream-003001.pgr \n"
        "\t c:\\Recorded\\LadybugStream-003002.pgr \n"
        "\t ... ... \n"
        "\t c:\\Recorded\\LadybugStream-003010.pgr \n\n"
        "\t ladybugStreamCopy c:\\Recorded\\LadybugStream-003000.pgr c:\\Recorded\\myStream ladybug5120003.cal \n"
        "\t will copy all the images in the 11 stream files \n"
        "\t with calibration file ladybug5120003.cal \n"
        "\t to c:\\Recorded\\myStream-000000.pgr,c:\\Recorded\\myStream-000001.pgr ...\n\n"
        "\t In this example, if c:\\Recorded\\myStream-000000.pgr alreay exists on the disk,"
        "\t the images will be copied to c:\\Recorded\\myStream-001000.pgr,"
        "\t c:\\Recorded\\myStream-001001.pgr, ...\n\n"
        );
}

// A funcao main precisa de duas variáveis.
// A primeira conta os parametros que deve ser pelo menos 2.
// A segunda é um vetor parametros, como arquivo de origem e destino.
int main(int argc, char* argv[])
{
    char* pszConfigFileName = NULL;
    char* pszSrcStreamName  = NULL;
    char* pszDestStreamName = NULL;  
    std::string configFileName;         
    bool bDeleteTempFile = false;    

    // At least two parameters are needed - Se menos de 3 parametros sao passados, é exibido o manual
    if (argc < 3)
    {
        usage();
        return 0;
    }

    pszSrcStreamName = argv[1]; //A origem é colocada nessa posicao no vetor
    pszDestStreamName = argv[2]; //O destino é colocada nessa posicao no vetor

    // The last three parameter are optional
    // [calFile] [From] [To] São opcionais, obrigatorios sao SrcFileName OutputFileName
    // É verificado se há diferenca entre os valores opcionais e os definidos como default
    if (argc > 3)
    {
        pszConfigFileName = argv[3];
        if ( strcmp ( pszConfigFileName, "default") == 0) //https://www.geeksforgeeks.org/strcmp-in-c-cpp/
        {
            pszConfigFileName = NULL;
        }
    }

    //converts the string argument str to an integer - essas variaveis sao importantes caso o valor from e to sejam definidos
    //https://www.tutorialspoint.com/c_standard_library/c_function_atoi.htm
    unsigned int startImageIndex = (argc > 4) ? atoi(argv[4]) : 0; //[From] - the number of the first image to copy
    unsigned int endImageIndex = (argc > 5) ? atoi(argv[5]) : startImageIndex; //[To] - the number of the last image to copy

    //O número inicial deve ser menor que o final... rs
    if ( startImageIndex > endImageIndex )
    {
        printf("Invalid image numbers.\n");
        return 0;
    }

    // Create stream context for reading
    LadybugStreamContext readingContext; //Um contexto precisa ser criado para que se tenha acesso a métodos de leitura e escrita
    LadybugError error = ladybugCreateStreamContext( &readingContext );
    _HANDLE_ERROR

    // Create stream context for writing
    LadybugStreamContext writingContext; 
    error = ladybugCreateStreamContext( &writingContext );
    _HANDLE_ERROR

    // Open the source stream file - O aquivo de origem é um dos parametros passados
    printf( "Opening source stream file : %s\n", pszSrcStreamName);
    error = ladybugInitializeStreamForReading( readingContext, pszSrcStreamName, true ); 
    _HANDLE_ERROR

    if ( pszConfigFileName == NULL )
    {
        // Generate a temporary file name for loading config file from the source stream
        configFileName = getTempName("config");
        // Load the configuration file from the source stream
        error = ladybugGetStreamConfigFile(readingContext, configFileName.c_str());
        _HANDLE_ERROR
        printf("Temp config file: %s\n", configFileName.c_str());
        bDeleteTempFile = true;
    }
    else
    {
        configFileName = pszConfigFileName; //o arq de config é sobrescrito com os argumentos passados pelo usuario, caso nao seja nulo, ou seja, o usuario nao quer os valores default
    }

    // Read the stream header
    LadybugStreamHeadInfo streamHeaderInfo;  
    error = ladybugGetStreamHeader( readingContext, &streamHeaderInfo );
    _HANDLE_ERROR

    {
        // Get the total number of the images in these stream files
        unsigned int uiNumOfImages = 0;
        error = ladybugGetStreamNumOfImages( readingContext, &uiNumOfImages ); //uiNumOfImages - Um método que coleta o número de imagens de um contexto
        _HANDLE_ERROR

        if ( endImageIndex > uiNumOfImages || argc < 6  ) // se a ultima img informada pelo usario for maior que : o número de imagens do contexto ou argumentos menor q 6
        {
            endImageIndex = uiNumOfImages - 1; // a variável será sobrescrita pelo número de imagens do contexto -1
        }

        printf( "The source stream file has %u images.\n", uiNumOfImages ); // exibe o número de imagens do contexto
        printf( "Copy from %u to %u to %s-000000.pgr ...\n", startImageIndex, endImageIndex, pszDestStreamName) ; //Criando uma msg pra mostrar na tela

        // Seek the position of the first image
        error = ladybugGoToImage( readingContext, startImageIndex );
        _HANDLE_ERROR

        // Open the destination file
        printf( "Opening destination stream file : %s\n", pszDestStreamName);
        error = ladybugInitializeStreamForWritingEx( 
            writingContext,
            pszDestStreamName, 
            &streamHeaderInfo, 
            configFileName.c_str(), 
            true );
        _HANDLE_ERROR

        // Copy all the specified images to the destination file
        for (unsigned int currIndex = startImageIndex; currIndex <= endImageIndex; currIndex++ ) 
        {
            printf( "Copying %u of %u\n", currIndex+1,  uiNumOfImages ) ;

            // Read a Ladybug image from stream
            LadybugImage currentImage;
            error = ladybugReadImageFromStream( readingContext, &currentImage );
            _HANDLE_ERROR

            // Write the image to the destination file
            error = ladybugWriteImageToStream( writingContext,  &currentImage );
            _HANDLE_ERROR
        };
    }

_EXIT:

    // Close the reading and writing stream 
    error = ladybugStopStream( writingContext );
    error = ladybugStopStream( readingContext );

    if ( bDeleteTempFile )
    {
        // Delete temp config file 
        if (std::remove(configFileName.c_str()) != 0)
        {
            //Failed to remove the file. Handle error?
            std::cout << "Warning: temp file " << configFileName << " was unable to be deleted." << std::endl;
        }
    }

    // Destroy stream context
    ladybugDestroyStreamContext ( &readingContext );
    ladybugDestroyStreamContext ( &writingContext );

    return 0;
}
