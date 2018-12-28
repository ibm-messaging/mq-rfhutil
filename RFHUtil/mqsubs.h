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

// mqsubs.h: mq related subroutines header file
//
//////////////////////////////////////////////////////////////////////
#include "cmqc.h"

int getCcsidType(const int ccsid);
void findMQInstallDirectory(char *dirName, int dirNameLen);
const char * getApplTypeDisplay(int putType);
int getEncodingValue(int encodeType, int pdEncodeType, int floatEncodeType);
int getIntEncode(int encoding);
int getPDEncode(int encoding);
int getFloatEncode(int encoding);
BOOL translateMQMD(MQMD2 * mqmd);

