#
# makefile for bupsc ("bahp-see":)
#

#compiler, linker, flags, libs:
CXX = g++
LD = g++
CXF = -std=c++14 -O3 -ffunction-sections -fdata-sections -Wall -pthread
LDF = -Wl,--gc-sections
LIBS = -lpthread -lz -lsqlite3

#***cpp experimental file-system lib: (kept sep. since will be in cpp17?)***
LIBS += -lstdc++fs

#for cleanup:
RM = /bin/rm -f

#libs-only - fix for non-libs
DEPENDS = libz libsqlite3

#sources:
SRC = main.cpp
SRC += bscglobals.cpp
SRC += bscglobals.h
SRC += bupsc.cpp
SRC += bupsc.h
SRC += dbmaster.cpp
SRC += dbmaster.h
SRC += monitor.cpp
SRC += monitor.h
SRC += monitors.cpp
SRC += compression.cpp
SRC += compression.h
SRC += dbsqlite3.cpp
SRC += dbsqlite3.h
SRC += dbsqlite3types.h
SRC += utilfuncs.cpp
SRC += utilfuncs.h

#objects:
OBJ = main.o
OBJ += bscglobals.o
OBJ += bupsc.o
OBJ += dbmaster.o
OBJ += monitor.o
OBJ += monitors.o
OBJ += compression.o
OBJ += dbsqlite3.o
OBJ += utilfuncs.o

#create application named:
APP = bupsc

#*artifacts created during make for clean-up:*
GUNK = $(OBJ) *.h.gch

##=====
$(APP): depends $(OBJ)
	@echo linking...
	$(LD) $(LDF) $(OBJ) $(LIBS) -o $(APP)
	@echo cleaning-up...
	@$(RM) $(GUNK)
	@echo $(APP) - done

##=====
$(OBJ): $(SRC)
	@echo compiling...
	$(CXX) $(CXF) -c $(SRC)

##=====
clean:
	@$(RM) $(APP) $(GUNK)
	@echo cleaned up

##=====
#checking for libs here, use which/find/... for other stuff as needed
depends:
	@echo checking dependencies...
	@for F in $(DEPENDS) ; do \
		/sbin/ldconfig -p | grep $$F > /tmp/xlibx ; \
		if [ ! -s /tmp/xlibx ]; \
		then \
			echo ; \
			echo "FATAL: missing dependency: $$F"; \
			echo ; \
			exit 1 ; \
		fi \
	done
