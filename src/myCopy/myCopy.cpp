//=============================================================================
// System Includes
//=============================================================================

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

//=============================================================================
// echo program usage
//=============================================================================

int main(int argc, char* argv[])
// A funcao main precisa de duas variáveis.
// A primeira corresponde ao número de paramtros que deve pelo menos 2.
// A segunda é um vetor parametros, como arquivo de origem e destino.
{
    char* pszConfigFileName = NULL;
    char* pszSrcStreamName  = NULL;
    char* pszDestStreamName = NULL;  
    std::string configFileName;         
    bool bDeleteTempFile = false; 
	
    // At least two parameters are needed - Se menos de 3 parametros sao passados, é exibido o manual
    if (argc < 3)
    {
        printf("Número de parâmetros incorreto.\n ");
		printf("myCopy SrcFileName OutputFileName\n");
        return 0;
    }	
	
	pszSrcStreamName = argv[1]; //A origem é colocada nessa posicao no vetor
    pszDestStreamName = argv[2]; //O destino é colocada nessa posicao no vetor
	
	unsigned int startImageIndex = (argc > 4) ? atoi(argv[4]) : 0;
    unsigned int endImageIndex = (argc > 5) ? atoi(argv[5]) : startImageIndex;
	
	// Create stream context for reading - Um contexto precisa ser criado para que se tenha acesso a métodos de leitura e escrita
    LadybugStreamContext readingContext;
    LadybugError error = ladybugCreateStreamContext( &readingContext );
    _HANDLE_ERROR
	
	// Create stream context for writing - Um contexto precisa ser criado para que se tenha acesso a métodos de leitura e escrita
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
        error = ladybugGetStreamNumOfImages( readingContext, &uiNumOfImages );
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
        

        // Copy all the specified images to the destination file - AQUI A COPIA REALMENTE OCORR
        
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

    // Destroy stream context
    ladybugDestroyStreamContext ( &readingContext );
    ladybugDestroyStreamContext ( &writingContext );

    return 0;
}
	
	