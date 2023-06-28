/*************************************************************************
 *                                                                       *
 * ODER's Utilities Library. Copyright (C) 2008 Oleh Derevenko.          *
 * All rights reserved.  e-mail: odar@eleks.com (change all "a" to "e")  *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of EITHER:                                  *
 *   (1) The GNU Lesser General Public License as published by the Free  *
 *       Software Foundation; either version 3 of the License, or (at    *
 *       your option) any later version. The text of the GNU Lesser      *
 *       General Public License is included with this library in the     *
 *       file LICENSE-LESSER.TXT. Since LGPL is the extension of GPL     *
 *       the text of GNU General Public License is also provided for     *
 *       your information in file LICENSE.TXT.                           *
 *   (2) The BSD-style license that is included with this library in     *
 *       the file LICENSE-BSD.TXT.                                       *
 *   (3) The zlib/libpng license that is included with this library in   *
 *       the file LICENSE-ZLIB.TXT                                       *
 *                                                                       *
 * This library is distributed WITHOUT ANY WARRANTY, including implied   *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.      *
 * See the files LICENSE.TXT and LICENSE-LESSER.TXT or LICENSE-BSD.TXT   *
 * or LICENSE-ZLIB.TXT for more details.                                 *
 *                                                                       *
 *************************************************************************/

#ifndef __OU_ASSERT_H_INCLUDED
#define __OU_ASSERT_H_INCLUDED


/**
 *	\file
 *	\brief Definitions of assertion checking macros.
 *	
 *	This file contains definitions of assertion failure check macros. 
 *	These include \c OU_ASSERT, \c OU_VERIFY and \c OU_CHECK.
 *	Assertion failure handler is common for all three macros and is customizable.
 *	If assertion checks are not customized, system \c assert() function is used.
 *	\see CAssertionCheckCustomization
 */


#include <ou/customization.h>
#include <ou/namespace.h>


/**
 *	\def OU_ASSERT
 *	\brief Defines a regular assertion check macro.
 *
 *	\c OU_ASSERT defines a classic assertion check macro. Normally its expression 
 *	is evaluated and if it is equal to \c false, assertion failure handler is invoked
 *	with \c AFS_ASSERT parameter. If assertion failure handler is not customized,
 *	the functionality of system \c assert() call is performed.
 *	If \c NDEBUG preprocessor symbol is defined, the macro expands to empty operator
 *	and expression of its argument is \e NOT evaluated.
 *	\note The expression is evaluated only once even though either custom handler or
 *	system \c assert() might be chosen to handle failure.
 *	\par
 *	\note The macro is designed so that "Condition" text is passed to customized 
 *	handler and  "OU__ASSERT_HANDLER(Condition)" is passed to \c assert() if 
 *	handler is not customized. However new versions of GCC (starting from 4.3.0)
 *	seem to use modified macro expansion schema which formats macro \c OU__ASSERT_HANDLER
 *	into string after full expansion.
 *
 *	\see OU_VERIFY
 *	\see OU_CHECK
 *	\see EASSERTIONFAILURESEVERITY
 *	\see CAssertionFailedProcedure
 *	\see CAssertionCheckCustomization
 */

/**
 *	\def OU_VERIFY
 *	\brief Defines an assertion check macro which always evaluates its parameter.
 *
 *	\c OU_VERIFY is similar to \c OU_ASSERT with exception that if \c NDEBUG preprocessor 
 *	symbol is defined it still evaluates its parameter.
 *	The main purpose of this macro is to prevent "unused variable" compiler warning
 *	which would otherwise appear with \c OU_ASSERT macro used when \c NDEBUG is defined.
 *
 *	\code
 *	    bool bCallStatus = CallMyFunction();
 *	    // A compiler warning would be generated with OU_ASSERT if bCallStatus is 
 *	    // not used further in the code.
 *	    OU_VERIFY(bCallStatus); 
 *	\endcode
 *
 *	\note It is not recommended to use \c OU_VERIFY with function calls directly
 *	if function result is not boolean, as otherwise in case of assertion failure
 *	it will be not possible to retrieve function result.
 *	\code
 *	    // Incorrect! Call status will not be available in case of failure!
 *	    OU_VERIFY(pthread_mutex_create(&attr) == EOK); 
 *	    
 *	    // Correct. Call status can be retrieved from a variable.
 *	    int iMutexCreateStatus = pthread_mutex_create(&attr);
 *	    OU_VERIFY(iMutexCreateStatus == EOK);
 *	\endcode
 *
 *	\see OU_ASSERT
 *	\see OU_CHECK
 *	\see EASSERTIONFAILURESEVERITY
 *	\see CAssertionFailedProcedure
 *	\see CAssertionCheckCustomization
 */

/**
 *	\def OU_CHECK
 *	\brief Defines a hard assertion check macro.
 *
 *	\c OU_CHECK evaluates its parameter and if the expression equals to \c false
 *	it invokes either a custom assertion failure handler with \c AFS_CHECK parameter
 *	or failure processing of system \c assert() function. The execution is not supposed
 *	to exit from assertion failure call. If it does (either because custom assertion 
 *	failure handler returns or handler is not customized and \c assert() function has
 *	no effect because of \c NDEBUG symbol being defined), a write attempt to NULL 
 *	pointer is performed to generate Access Violation exception or SIGSEGV signal.
 *	\c OU_CHECK is similar to \c OU_VERIFY in that it evaluates its parameter whether
 *	\c NDEBUG is defined or not.
 *	\note The expression is evaluated only once even though either custom handler or
 *	system \c assert() might be chosen to handle failure.
 *	\par
 *	\note The macro is designed so that "Condition" text is passed to customized 
 *	handler and  "OU__CHECK_HANDLER(Condition)" is passed to \c assert() if 
 *	handler is not customized. However new versions of GCC (starting from 4.3.0)
 *	seem to use modified macro expansion schema which formats macro \c OU__CHECK_HANDLER
 *	into string after full expansion.
 *
 *	\see OU_ASSERT
 *	\see OU_VERIFY
 *	\see EASSERTIONFAILURESEVERITY
 *	\see CAssertionFailedProcedure
 *	\see CAssertionCheckCustomization
 */


/*
 *	Implementation Note:
 *	1) Fully qualified names must be used in macros as they might be 
 *	used externally and forwarded from outside of _OU_NAMESPACE.
 *	2) false || ... is necessary to suppress C4800 warning in MSVC.
 */

#if defined(NDEBUG)

#define OU_ASSERT(Condition) ((void)0)

#define OU_VERIFY(Condition) ((void)(Condition))

#define OU_CHECK(Condition) (void)(false || (Condition) \
	|| (!_OU_NAMESPACE::CAssertionCheckCustomization::GetAssertFailureCustomHandler() \
		|| (_OU_NAMESPACE::CAssertionCheckCustomization::GetAssertFailureCustomHandler()( \
			_OU_NAMESPACE::AFS_CHECK,  #Condition, __FILE__, __LINE__), false)) \
		|| (*(volatile int *)0 = 0, false))


#else // #if !defined(NDEBUG)

#include <assert.h>


#define OU__ASSERT_HANDLER(Condition) (false || (Condition) \
	|| (_OU_NAMESPACE::CAssertionCheckCustomization::GetAssertFailureCustomHandler() \
		&& (_OU_NAMESPACE::CAssertionCheckCustomization::GetAssertFailureCustomHandler()( \
			_OU_NAMESPACE::AFS_ASSERT, #Condition, __FILE__, __LINE__), true)))

#define OU__CHECK_HANDLER(Condition) (((bConditionValue = false || (Condition)), bConditionValue) \
	|| (_OU_NAMESPACE::CAssertionCheckCustomization::GetAssertFailureCustomHandler() \
		&& (_OU_NAMESPACE::CAssertionCheckCustomization::GetAssertFailureCustomHandler()( \
			_OU_NAMESPACE::AFS_CHECK,  #Condition, __FILE__, __LINE__), true)))


#define OU_ASSERT(Condition) assert(OU__ASSERT_HANDLER(Condition))

#define OU_VERIFY(Condition) OU_ASSERT(Condition)

#define OU_CHECK(Condition) { \
	bool bConditionValue; \
	assert(OU__CHECK_HANDLER(Condition)); \
	(void)(bConditionValue || (*(volatile int *)0 = 0, false)); \
}


#endif // #if !defined(NDEBUG)


#endif // #ifndef __OU_ASSERT_H_INCLUDED
