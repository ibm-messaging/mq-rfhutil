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

/********************************************************************/
/*                                                                  */
/*   MQTIMES has 3 parameters                                       */
/*                                                                  */
/*      -q the name of the input queue (required)                   */
/*      -m queue manager name (optional)                            */
/*      -b number of messages in a unit of work (optional)          */
/*                                                                  */
/*    if no queue manager is specified, the default queue manager   */
/*    is used.                                                      */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifdef SOLARIS
#include <sys/types.h>
#endif

/* includes for MQI */
#include <cmqc.h>

#define MAX_MESSAGE_LENGTH 64 * 65536 + 1

#define MAX_SYNC			25
#define MAX_SYNC_ALLOW		5000

/* global error switch */
	int		err=0;

/* Globally declared variables (from arguments) */
	int		batchSize=MAX_SYNC;
	char	qname[MQ_Q_NAME_LENGTH + 1];
	char	qmname[MQ_Q_MGR_NAME_LENGTH + 1];

static char copyright[] = "(C) Copyright IBM Corp, 2001";
static char Version[]=\
"@(#)MQTimes V1.11 - MQSI V2 Performance results tool  - Jim MacNair ";

#ifdef _DEBUG
static char Level[]="mqtimes.c V1.11 Debug version ("__DATE__" "__TIME__")";
#else
static char Level[]="mqtimes.c V1.11 Release version ("__DATE__" "__TIME__")";
#endif

int getSecs(MQLONG time)

{
	int		hh;
	int		mm;
	int		ss;

	ss = time % 100;
	mm = ((time - ss) % 10000) / 100;
	hh = (time - mm - ss) / 10000;

	return (hh * 3600) + (mm * 60) + ss;
}

void checkerror(const char *mqcalltype, MQLONG compcode, MQLONG reason, char *resource)

{
	if ((compcode != MQCC_OK) || (reason != MQRC_NONE))
	{
		printf("MQSeries error with %s on %s - compcode = %d, reason = %d\n",
				mqcalltype, resource, compcode, reason);
	}
}

void captureString(char * value, char * arg, const size_t maxlen)

{
	if (strlen(arg) < maxlen)
	{
		strcpy(value, arg);
	}
	else
	{
		/* issue an error message */
		printf("*****Value of constant exceeds maximum length\n");

		/* Set the return code based on the message number */
		err = 94;
	}
}

int getArgPointer(int argc,
				  char *argv[],
				  int index,
				  char ** parmData,
				  const char * parmDesc)

{
	if (strlen(argv[index]) > 2)
	{
		/* point to the argument data */
		(*parmData) = argv[index] + 2;
	}
	else
	{
		index++;

		if (index < argc)
		{
			/* point to the argument data */
			(*parmData) = argv[index];
		}
		else
		{
			/* issue an error message */
			printf("*****%s parameter is missing value\n", parmDesc);
			
			/* Set the return code to indicate the error */
			err = 95;

			/*point to something valid */
			(*parmData) = argv[index - 1] + 2;
		}
	}


	return index;
}

int processNumArg(int argc,
				  char *argv[], 
				  int index, 
				  int * value, 
				  const char * parmDesc)

{
	char *argData;

	index = getArgPointer(argc, argv, index, &argData, parmDesc);

	if (index < argc)
	{
		/* convert to an integer */
		(*value) = atoi(argData);
	}

	return index;
}

int processIndArg(int argc, 
				  char *argv[], 
				  int index, 
				  char * parm, 
				  size_t parmsize, 
				  const char * parmDesc)

{
	char *argData;

	index = getArgPointer(argc, argv, index, &argData, parmDesc);

	if (index < argc)
	{
		/* value is included with parameter */
		captureString(parm, argData, parmsize);
	}

	return index;
}

void processArgs(int argc, char **argv)

{
	int		i;
	int		foundit;
	char	ch;

	i = 1;
	while ((i < argc) && (0 == err))
	{
		foundit = 0;

		if ('-' == argv[i][0])
		{
			ch = toupper(argv[i][1]);

			/* check for option */
			if ((0 == foundit) && ('Q' == ch))
			{
				foundit = 1;

				i = processIndArg(argc, 
								  argv, 
								  i,
								  qname, 
								  sizeof(qname), 
								  "queue name (-q)");
			}

			/* check for option */
			if ((0 == foundit) && ('M' == ch))
			{
				foundit = 1;

				i = processIndArg(argc, 
								  argv, 
								  i,
								  qmname, 
								  sizeof(qmname), 
								  "queue manager name (-m)");
			}

			/* check for option */
			if ((0 == foundit) && ('B' == ch))
			{
				foundit = 1;

				i = processNumArg(argc,
								  argv, 
								  i,
								  &batchSize, 
								  "Messages within unit of work (-b)");

				if ((batchSize < 1) || (batchSize > MAX_SYNC_ALLOW))
				{
					batchSize = MAX_SYNC;
				}

				printf("Sync point interval set to %d\n", batchSize);
			}
		}

		/* did we recognize the parameter? */
		if (0 == foundit)
		{
			/* issue an error message */
			printf("*****Unrecognized command line parameter (%c) found\n", argv[i][1]);

			/* Set the return code based on the message number */
			err = 93;
		}

		i++;
	}
}

void printHelp(char *pgmName)

{
	printf("MQTimes V1.11 - Performance measurement tool\n");
	printf("%s\n", Level);
	printf("format is:\n");
	printf("   %s -q queue <-m queue manager> <-b unit of work batch size>\n", pgmName);
}

int main(int argc, char **argv)

{
	int		msgcount=0;
	int		totcount=0;
	int		maxrate=0;
	int		firstsec=0;
	int		secondcount=0;
	int		totalbytes=0;
	int		uow=0;
	int		firstTime=0;
	int		secondTime=0;
	int		lastTime=0;
	int		prevLastTime=0;
	int		avgbytes;
	double	avgrate;
	MQHCONN	qm=0;
	MQHOBJ	q=0;
	MQLONG	compcode;
	MQLONG	reason;
	MQOD	objdesc = {MQOD_DEFAULT};
	MQMD	msgdesc = {MQMD_DEFAULT};
	MQMD	replydesc = {MQMD_DEFAULT};
	MQLONG	openopt = 0;
	MQGMO	mqgmo = {MQGMO_DEFAULT};
	MQPMO	mqpmo = {MQPMO_DEFAULT};
	MQLONG	datalen=0;
	char	*msgdata;
	size_t	prevtime_secs;
	size_t	currtime_secs;
	char	prevtime[9];
	char	currtime[9];

	printf("MQTIMES V1.11 program start\n");

	/* print the copyright statement */
	printf("Copyright (c) IBM Corp., 2001/2002/2003/2004\n");

	/* allocate a buffer for the message */
	msgdata = (char *)malloc(MAX_MESSAGE_LENGTH);

	/* initialize the work areas */
	memset(qmname, 0, sizeof(qmname));
	memset(qname, 0, sizeof(qname));
	memset(msgdata, 0, sizeof(msgdata));
	memset(prevtime, 0, sizeof(prevtime));
	memset(currtime, 0, sizeof(currtime));

	prevtime_secs = 0;
	currtime_secs = 0;

	/* check for too few input parameters */
	if (argc < 2)
	{
		printHelp(argv[0]);
		exit(99);
	}

	/* check for help request */
	if ((argv[1][0] == '?') || (argv[1][1] == '?'))
	{
		printHelp(argv[0]);
		exit(0);
	}

	/* process any command line arguments */
	processArgs(argc, argv);

	if (err != 0)
	{
		printHelp(argv[0]);
		exit(99);
	}

	/* Connect to the queue manager */
	printf("connecting to queue manager %s\n",qmname);
	MQCONN(qmname, &qm, &compcode, &reason);

	/* check for errors */
	checkerror((char *)"MQCONN", compcode, reason, qmname);
	if (compcode != MQCC_OK)
	{
		return 98;
	}

	/* set the queue open options */
	strncpy(objdesc.ObjectName, qname, MQ_Q_NAME_LENGTH);
	openopt = MQOO_INPUT_SHARED + MQOO_FAIL_IF_QUIESCING;

	/* open the queue for input */
	printf("opening queue %s for input\n", qname);
	MQOPEN(qm, &objdesc, openopt, &q, &compcode, &reason);

	/* check for errors */
	checkerror((char *)"MQOPEN", compcode, reason, qname);
	if (compcode != MQCC_OK)
	{
		return 97;
	}

	/* enter get message loop */
	while (MQCC_OK == compcode)
	{
		/* set the get message options */
		mqgmo.Options = MQGMO_NO_WAIT | MQGMO_FAIL_IF_QUIESCING | MQGMO_SYNCPOINT;
		mqgmo.MatchOptions = MQGMO_NONE;

		/* reset the msgid and correlid */
		memcpy(msgdesc.MsgId, MQMI_NONE, sizeof(msgdesc.MsgId));
		memcpy(msgdesc.CorrelId, MQCI_NONE, sizeof(msgdesc.CorrelId));

		/* perform the MQGET */
		MQGET(qm, q, &msgdesc, &mqgmo, MAX_MESSAGE_LENGTH, msgdata, &datalen, &compcode, &reason);

		/* check if we got to the end of the queue */
		/* check for errors, except for no more messages in queue */
		if (reason != 2033)
		{
			checkerror((char *)"MQGET", compcode, reason, qname);
		}

		if (compcode == MQCC_OK)
		{
			uow++;

			if (uow > batchSize)
			{
				MQCMIT(qm, &compcode, &reason);
				uow = 0;
			}

			/* only do this step if the MQGET worked */
			/* get the message time from the MQMD */
			memcpy(currtime, msgdesc.PutTime, sizeof(currtime));

			/* check if the time is the same or not */
			/* only check down to the seconds position */
			currtime_secs = atol(currtime) / 100;

			if (0 == prevtime_secs)
			{
				/* capture the time of the first message */
				firstTime = currtime_secs;
			}
			else
			{
				if (0 == secondTime)
				{
					/* capture the second time */
					secondTime = currtime_secs;
				}

				prevLastTime = lastTime;
				lastTime = currtime_secs;
			}

			/* count the total number of messages */
			totcount++;

			if ((prevtime_secs == 0) || (currtime_secs <= prevtime_secs ))
			{
				msgcount++;
			}
			else
			{
				/* write out the number of messages in this second */
				printf("%6.6s %d\n", prevtime, msgcount);

				/* is this the first time through? */
				if (firstsec == 0)
				{
					/* remember count in first second */
					firstsec = msgcount;
				}

				/* keep track of the maximum message rate */
				if (msgcount > maxrate)
				{
					maxrate = msgcount;
				}

				/* reset the messages in second counter */
				msgcount = 1;

				/* count the number of individual seconds with at least one message */
				secondcount++;
			}

			if (currtime_secs > prevtime_secs )
			{
				memcpy(prevtime, currtime, sizeof(currtime));
				prevtime_secs = currtime_secs;
			}

			/* calculate the total bytes in the message */
			totalbytes += datalen;
		}
	}

	/* count the last interval */
	secondcount++;

	/* dump out the last time interval */
	printf("%6.6s %d\n", prevtime, msgcount);

	/* keep track of the maximum message rate */
	if (msgcount > maxrate)
	{
		maxrate = msgcount;
	}

	/* check if we have a uow open */
	if (uow > 0)
	{
		MQCMIT(qm, &compcode, &reason);
		checkerror((char *)"MQCMIT", compcode, reason, qmname);
	}

	/* close the input queue */
	printf("closing the input queue\n");
	MQCLOSE(qm, &q, MQCO_NONE, &compcode, &reason);

	checkerror((char *)"MQCLOSE", compcode, reason, qname);

	/* Disconnect from the queue manager */
	printf("disconnecting from the queue manager\n");
	MQDISC(&qm, &compcode, &reason);

	checkerror((char *)"MQDISC", compcode, reason, qmname);

	/* dump out the total message count */
	printf("Total messages %d\n", totcount);

	/* give the total and average message size */
	if (totcount > 0)
	{
		avgbytes = totalbytes / totcount;
		printf("total bytes in all messages %d\n", totalbytes);
		printf("average message size %d\n", avgbytes);
	}

	/* indicate the number of seconds with at least one message */
	printf("Total number of seconds with at least one message %d\n", secondcount);

	/* write out the average message rate, ignoring the first and last intervals */
	if (secondcount > 2)
	{
		printf("lastTime %d firstTime %d secs %d\n", lastTime, firstTime, getSecs(lastTime) - getSecs(firstTime));
		avgrate = (float)(totcount - firstsec - msgcount) / (getSecs(prevLastTime) - getSecs(secondTime));
		printf("Average message rate except first and last intervals %7.2f\n", avgrate);
	}

	/* print out the maximum rate */
	printf("Peak message rate %d\n", maxrate);

	/******************************************************************/
	/*                                                                */
	/* END OF PROGRAM                                                 */
	/*                                                                */
	/******************************************************************/
	printf("MQTIMES program ended\n");

	return 0;
}
