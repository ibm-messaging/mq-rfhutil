#
# Copyright (c) IBM Corporation 2000, 2019
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Contributors:
# Jim MacNair - Initial Contribution
#
# This Makefile compiles the mqperf programs for Linux.

DIR = \
  mqcapone \
  mqcapsub \
  mqcapture \
  mqlatency \
  mqput2 \
  mqreply \
  mqtest \
  mqtimes2 \
  mqtimes3

# Where the output goes
OUTDIR=../bin/linux

APPS = $(foreach dir, $(DIR), $(OUTDIR)/$(dir))

CFLAGS=-I/opt/mqm/inc -L/opt/mqm/lib64 -lmqm -I./CommonSubs 
WARNINGS=-Wno-implicit-function-declaration

# mqputs is the same as mqput2 but with an extra -D option.
# mqtimes does not use the common subroutine files
#
# Building is fast, so we force all apps to be rebuilt every time for simplicity
all: $(OUTDIR) $(DIR) mqputs mqtimes

$(OUTDIR): clean
	mkdir -p $(OUTDIR)

$(DIR): $(OUTDIR)
	$(CC) -o $(OUTDIR)/$@ $@/$@.c ./CommonSubs/*.c $(CFLAGS) $(WARNINGS)

mqputs: $(OUTDIR)
	$(CC) -o $(OUTDIR)/$@ mqput2/mqput2.c ./CommonSubs/*.c $(CFLAGS) $(WARNINGS) -DNOTUNE

mqtimes: $(OUTDIR)
	$(CC) -o $(OUTDIR)/$@ mqtimes/mqtimes.c $(CFLAGS) $(WARNINGS)

clean:
	rm -f $(OUTDIR)/*
