CC=gcc  -g
CC=gcc  -g -DSTREAM
INC=-I/usr/local/include -I.
COMEDILIB= -L/usr/local/lib -lcomedi
OLIB=-lpthread 
ACQEXE=mxacq mxrecord
EXE=mxgplot mxscan mxasci mxdonline mxponline
SCRIPTS=observe.pl average.pl lstclock.pl ponline.pl Chart.pm mxobs.pl

ACQOBJ=acq.o 
RECOBJ=record.o 
PLOTOBJ=plot.o 
SOBJ=scan.o 
ASOBJ=asci.o 
ONLINEOBJ=getonline.o 
ONLINEOBJ=getonline.o 

OBJ=$(PLOTOBJ) $(ASOBJ) $(SOBJ) $(ONLINEOBJ)

AOBJ=$(ACQOBJ) $(RECOBJ)

required: $(EXE)
	/bin/mkdir -p $(HOME)/bin
	/bin/cp -d -f $(EXE) $(SCRIPTS) $(HOME)/bin

all:   required $(ACQEXE)
	/bin/mkdir -p $(HOME)/bin
	/bin/cp -f $(ACQEXE) $(HOME)/bin

.c.o: 
	$(CC) $(INC) -c  $*.c

record.c: record.h acq.h acqcommon.h server.h
acq.o   : acq.h acq.h acqcommon.h 
scan.o  : scanmain.c cmd.c  cmd.h server.h
record.o: stream.c server.h
plot.o:   cmd.c server.h
getonline.o: cmd.c server.h getonline.h plotgnu.c writeonline.c scaledata.c

mxacq:  $(ACQOBJ)
	$(CC) -o $@ $(ACQOBJ)  $(COMEDILIB) $(OLIB)

mxrecord: $(RECOBJ) 
	$(CC) -o $@   $(RECOBJ)  $(OLIB)

mxscan: $(SOBJ)
	$(CC) -o $@ $(SOBJ) 

mxasci: $(ASOBJ)
	$(CC) -o $@ $(ASOBJ) 

mxgplot: $(ONLINEOBJ)
	$(CC) -o $@ $(ONLINEOBJ)  $(OLIB) -lm

mxdonline:
	ln -sf mxgplot mxdonline

mxponline:
	ln -sf mxgplot mxponline


$(RECOBJ): acqcommon.h record.h record.c recordthread.c

clean: 
	rm -rf $(EXE) $(OBJ)

allclean: clean
	rm -rf $(ACQEXE) $(AOBJ)


jplot: jchart.h jplot.c jchart.c
	$(CC) -o $@ jplot.c jchart.c `pkg-config --cflags gtk+-2.0`  `pkg-config --libs gtk+-2.0`
