@echo off

rem Run the RFHUTILC program with an MQ environment setting and
rem the trace options set - both this program, and MQ

set curdir=%CD%
set pd=c:\Program Files\IBM\MQ

rem setmqenv resets "echo" value
call "%pd%"\bin\setmqenv.cmd -s


@echo off

set dd=%MQ_DATA_PATH%

cd bin\Release

rem Clear environment variables in case we comment out the next couple of lines
set RFHUTIL_TRACE_FILE=
set RFHUTIL_VERBOSE_TRACE=

rem We seem to get an error exit when running with trace. Will need
rem to look at that at some point.
set RFHUTIL_TRACE_FILE=c:\temp\rfhutil_trace.txt
set RFHUTIL_VERBOSE_TRACE=Yes

rem Clear the trace files
del /q %RFHUTIL_TRACE_FILE%
del /q "%dd%"\trace\*.TRC

strmqtrc

rem Execute the real program - this is the client variation
rfhutilc

endmqtrc

cd %curdir%