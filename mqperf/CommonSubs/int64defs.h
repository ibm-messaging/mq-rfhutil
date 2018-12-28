/*
Copyright (c) IBM Corporation 2000, 2018
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Contributors:
Jim MacNair - Initial Contribution
*/

#ifndef _CommonSubs_int64defs_h
#define _CommonSubs_int64defs_h

/*************************************************************/
/*                                                           */
/* int64_t and uint64_t                                      */
/* Define 64-bit integers on Windows, since Windows uses a   */
/* non-standard definition.                                  */
/*                                                           */
/*************************************************************/

#ifdef WIN32
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

/**********************************************************/
/*                                                        */
/* FMT_I64                                                */
/* Definition of format characters for 64-bit integer.    */
/*                                                        */
/**********************************************************/
#ifdef WIN32
#define FMTI64		"%I64d"
#define FMTI64U		"%I64u"
#define FMTI64_66	"%6.6I64d"
#else
#define FMTI64		"%lld"
#define FMTI64U		"%llu"
#define FMTI64_66	"%6.6lld"
#endif

#endif
