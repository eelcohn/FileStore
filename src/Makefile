# --------------------------------------------------------------------------- #
# Econet FileStore server and gateway                                         #
#                                                                             #
# (c) Eelco Huininga 2017-2018                                                #
# --------------------------------------------------------------------------- #

# --------------------------------------------------------------------------- #
# First we'll set all the variables                                           #
# --------------------------------------------------------------------------- #
# Define which compiler we want to use                                        #
# --------------------------------------------------------------------------- #
CC = g++

# compiler flags:
#  -g            adds debugging information to the executable file
#  -Wall         turns on most, but not all, compiler warnings
#  -Wall -Wextra turns on almost all compiler warnings
#  -O2           do some code optimizations when compiling
#  -std=c++11    turns on compiler and library support for the 2011 C++ standard (needed for thread support)
CFLAGS = -g -Wall -O3 -std=c++11

# define any directories containing header files other than /usr/include
INCLUDES = ./wiringPi

# define library paths in addition to /usr/lib
LDFLAGS = ./wiringPi

# define any libraries to link into executable:
LIBS =	-lwiringPi \
	-lcrypt \
	-lcrypto \
	-lcurses \
	-lpthread \
	-lrt \
	-lssl

# define the C source files
SRCS =	main.cpp \
	api.cpp \
	debug.cpp \
	econet.cpp \
	ethernet.cpp \
	errorhandler.cpp \
	gpio.cpp \
	dtls.cpp \
	commands/commands.cpp \
	commands/star_access.cpp \
	commands/star_cat.cpp \
	commands/star_cdir.cpp \
	commands/star_clock.cpp \
	commands/star_configure.cpp \
	commands/star_date.cpp \
	commands/star_delete.cpp \
	commands/star_discs.cpp \
	commands/star_dismount.cpp \
	commands/star_exit.cpp \
	commands/star_help.cpp \
	commands/star_mount.cpp \
	commands/star_netmon.cpp \
	commands/star_newuser.cpp \
	commands/star_notify.cpp \
	commands/star_pass.cpp \
	commands/star_printtest.cpp \
	commands/star_priv.cpp \
	commands/star_remuser.cpp \
	commands/star_time.cpp \
	commands/star_users.cpp \
	platforms/linux.cpp

# define the C object files
OBJS =	$(SRCS:.cpp=.o)

# define the executable file
MAIN =	../FileStore
DTLSCLIENT =	../client

# --------------------------------------------------------------------------- #
# The following part of the makefile is generic; it can be used to            #
# build any executable just by changing the definitions above and by          #
# deleting dependencies appended to the file from 'make depend'               #
# --------------------------------------------------------------------------- #

.PHONY: depend clean

all:    $(MAIN)
	@echo
	@echo Generating certificates...
	@echo
#	@openssl req -x509 -newkey rsa:4096 -days 3650 -nodes -keyout ../client-key.pem -out ../client-cert.pem
#	@openssl req -x509 -newkey rsa:4096 -days 3650 -nodes -keyout ../server-key.pem -out ../server-cert.pem
	@echo
	@echo Done! Your Econet FileStore gateway server is compiled.

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDES) -o $(MAIN) $(OBJS) -L$(LDFLAGS) $(LIBS)

$(DTLSCLIENT):
	$(CC) $(CFLAGS) -I$(INCLUDES) -o $(DTLSCLIENT) dtls_client.cpp dtls.cpp -lssl -lcrypto

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
%.o: %.cpp
	$(CC) $(CFLAGS) -I$(INCLUDES) -c $< -o $@

clean:
	$(RM) $(OBJS)
	$(RM) *~
	$(RM) ../client-key.pem ../client-cert.pem ../server-key.pem ../server-cert.pem ../client
	$(RM) $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

