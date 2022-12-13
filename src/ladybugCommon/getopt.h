//=============================================================================
// Copyright © 2004 Point Grey Research, Inc. All Rights Reserved.
// 
// This software is the confidential and proprietary information of Point
// Grey Research, Inc. ("Confidential Information").  You shall not
// disclose such Confidential Information and shall use it only in
// accordance with the terms of the license agreement you entered into
// with Point Grey Research, Inc. (PGR).
// 
// PGR MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
// SOFTWARE, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE, OR NON-INFRINGEMENT. PGR SHALL NOT BE LIABLE FOR ANY DAMAGES
// SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING
// THIS SOFTWARE OR ITS DERIVATIVES.
//=============================================================================
//=============================================================================
// $Id: getopt.h,v 1.4 2004-12-02 22:00:34 mwhite Exp $
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
//
//  FILE: getopt.h
//              
//      Header for the GetOption function
//
//  COMMENTS:
//	 Code taken from MFC example and slightly modified to make it Unix 
//       compatible.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _GETOPT_H
#define _GETOPT_H

#ifdef __cplusplus
extern "C"
{
#endif /* __CPLUSPLUS */

// function prototypes
///////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: GetOption()
//
//      Get next command line option and parameter
//
//  PARAMETERS:
//
//      argc - count of command line arguments
//      argv - array of command line argument strings
//      pszValidOpts - string of valid, case-sensitive option characters,
//                     a colon ':' following a given character means that
//                     option can take a parameter
//      ppszParam - pointer to a pointer to a string for output
//
//  RETURNS:
//
//      If valid option is found, the character value of that option
//          is returned, and *ppszParam points to the parameter if given,
//          or is NULL if no param
//      If standalone parameter (with no option) is found, 1 is returned,
//          and *ppszParam points to the standalone parameter
//      If option is found, but it is not in the list of valid options,
//          -1 is returned, and *ppszParam points to the invalid argument
//      When end of argument list is reached, 0 is returned, and
//          *ppszParam is NULL
//
//  USAGE:
//    Command line options are expected to be composed of a series of option flags,
//    each preceded by the special option flag delimiter '-', and each may be  
//    optionally followed by an option arguments.
//
//    eg. -A 1 -B 2 -C -D hello
//
//    In the above example, we have four option flags, A, B, C, D.  A has an argument
//    value of 1, B has an argument value of 2, C has no arugment, and D has an
//    argument string of hello.
//
//    In order to parse the above correctly, the pszValidOpts string is expected to be
//
//    A:B:CD: 
//
//    Indicating that again there are four possible option flags, A, B, and D will be
//    followed by an argument each, whereas C will have no arugment.
//
//  COMMENTS:
//    1. Negative arguments must be preceded by an additional '-' to distinguish them
//    from regular option flags.
//    2. Option flags are preceded by '-' only ( no '/'), so as to avoid confusion 
//    with path name specification under Unix.
//
///////////////////////////////////////////////////////////////////////////////
int GetOption( int argc, char** argv, char* pszValidOpts, char** ppszParam );


/**************************************************
 * simple version of unix style getopt...
 * 
 * int getopt(int argc, char * const argv[],
 *            const char *optstring);
 *
 * extern char *optarg;
 * extern int optind, opterr, optopt;
 *
 * The getopt() function parses the command line arguments. Its arguments
 * argc and argv are the argument count and array as passed to the main()
 * function on program invocation. An element of argv that starts with
 * `-' (and is not exactly "-" or "--") is an option element.  The
 * characters of this element (aside from the initial `-') are option
 * characters. If getopt() is called repeatedly, it returns successively
 * each of the option characters from each of the option elements.
 * 
 * If getopt() finds another option character, it returns that character,
 * updating the external variable optind and a static variable nextchar
 * so that the next call to getopt() can resume the scan with the
 * following option character or argv-element.
 * 
 * If there are no more option characters, getopt() returns -1. Then
 * optind is the index in argv of the first argv-element that is not an
 * option.
 * 
 * optstring is a string containing the legitimate option characters.  If
 * such a character is followed by a colon, the option requires an
 * argument, so getopt places a pointer to the following text in the same
 * argv-element, or the text of the following argv-element, in optarg.
 * 
 * Option processing stops as soon as a non-option argument is
 * encountered. The special argument `--' forces an end of
 * option-scanning.
 * 
 * If getopt() does not recognize an option character, it stores the
 * character in optopt, and returns `?'.
 * 
 * The getopt() function returns the option character if the option was
 * found successfully, `?' for an unknown option character, or -1 for the
 * end of the option list.
 * 
 **************************************************/

extern int getopt( int argc, char* const argv[], const char* optstring );

extern char* optarg;
extern int optind, optopt;


#ifdef __cplusplus
}
#endif /* __CPLUSPLUS */

#endif /* _GETOPT_H */
