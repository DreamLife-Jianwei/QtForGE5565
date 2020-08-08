#ifndef	_WINLOCAL_H_
#define	_WINLOCAL_H_

/*****************************************************************
*	VS 2005 has a new security feature that complains on lots
*	of stdio routines. Defining this constant will shut them up.
******************************************************************/
#if defined(_WIN32)
	#if	(_MSC_VER == 1400)
		#pragma warning(disable:4996)
		#define	_CRT_SECURE_NO_DEPRECATE
	#endif

	#ifndef	BUILD_DRIVER
		#include <windows.h>
		#include <stdio.h>
		#include <stdlib.h>
		#include <string.h>
	#endif
#endif

#endif
