# --------------------------------------------------------------------------- #
# Econet FileStore server and gateway                                         #
#                                                                             #
# (c) Eelco Huininga 2017-2019                                                #
# --------------------------------------------------------------------------- #

# --------------------------------------------------------------------------- #
# First we'll set all the variables                                           #
# --------------------------------------------------------------------------- #
# Define which compiler we want to use                                        #
# --------------------------------------------------------------------------- #
CC = g++

# --------------------------------------------------------------------------- #
# Set the compiler flags:                                                     #
#  -g            adds debugging information to the executable file            #
#  -Wall         turns on most, but not all, compiler warnings                #
#  -Wall -Wextra turns on almost all compiler warnings                        #
#  -O2           do some code optimizations when compiling                    #
#  -std=c++11    turns on compiler and library support for the 2011 C++       #
#                standard (needed for thread support)                         #
# --------------------------------------------------------------------------- #
CFLAGS = -g -O2 -std=c++11 -Wall -Wextra -Wpedantic

# define any directories containing header files other than /usr/include
INCLUDES = 

# define library paths in addition to /usr/lib
LDFLAGS = 

# define any libraries to link into executable:
LIBS =	 -lpthread -lreadline -lcrypto -lssl

# define the C source files
MAIN_SRCS = \
	main.cpp \
	aun.cpp \
	cli.cpp \
	debug.cpp \
	econet.cpp \
	errorhandler.cpp \
	adfs.cpp \
	nativefs.cpp \
	netfs.cpp \
	settings.cpp \
	stations.cpp \
	users.cpp \
	platforms/linux.cpp \
	platforms/strlcpy.cpp \
	platforms/strtoupper.cpp \
	platforms/kbhit.cpp \
	platforms/adapters/none.cpp

# define the C object files
OBJS	= $(MAIN_SRCS:.cpp=.o)

# define the executable file
MAIN	= FileStore
MAIN_LINK	= main_link
CERTS	= certs



# --------------------------------------------------------------------------- #
# The following part of the makefile is generic; it can be used to            #
# build any executable just by changing the definitions above and by          #
# deleting dependencies appended to the file from 'make depend'               #
# --------------------------------------------------------------------------- #
# this is a suffix replacement rule for building .o's from .c's it uses       #
# automatic variables $<: the name of the prerequisite of the rule(a .c file) #
# and $@: the name of the target of the rule (a .o file) (see the gnu make    #
# manual section about automatic variables)                                   #
# --------------------------------------------------------------------------- #
%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

all:	$(MAIN) $(MAIN_LINK) $(CERTS)
	@echo
	@echo Done! Your Econet FileStore gateway server is compiled.
	
$(MAIN): $(OBJS)

main_link:
	@echo Linking $(MAIN)...
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LDFLAGS) $(LIBS)

install:
	@echo Installing $(MAIN)...
	@install -m 755 $(MAIN) ..
	@strip ../$(MAIN)
	@echo
	@echo Done! Your Econet FileStore gateway server is installed.

certs:
	@mkdir -p "../conf/keys/"
	@if [ ! -f ../conf/keys//client.cert ]; then \
		echo Generating server certificate... ; \
		openssl req -x509 -SHA512 -newkey rsa:4096 -days 3650 -nodes -subj "/C=UK/ST=Cambridgeshire/L=Cambridge/O=Acorn" -keyout ../conf/keys//server.key -out ../conf/keys//server.cert ; \
	fi
	@chmod 600 ../conf/keys//server.key
	@chmod 600 ../conf/keys//server.cert
	@if [ ! -f ../conf/keys//client.cert ]; then \
		echo Generating client certificate... ; \
		openssl req -x509 -SHA512 -newkey rsa:4096 -days 3650 -nodes -subj "/C=UK/ST=Cambridgeshire/L=Cambridge/O=Acorn" -keyout ../conf/keys//client.key -out ../conf/keys//client.cert ; \
	fi
	@chmod 600 ../conf/keys//client.key
	@chmod 600 ../conf/keys/client.cert

clean:
	@$(RM) $(OBJS) $(MAIN) config.h
	@$(RM) *~
	@$(RM) -rf autom4te.cache
	@$(RM) config.log config.h config.status

certclean:
	@$(RM) ../conf/keys//server.key ../conf/keys//server.cert
	@$(RM) ../conf/keys//client.key ../conf/keys//client.cert

dependclean:
	cd pigpio && $(MAKE) clean

cleanall: clean certclean dependclean

depend: $(MAIN_SRCS)
	makedepend $(INCLUDES) $^

help:
	@echo Options
	@echo "   make"
	@echo "      Compile the sourcecode."
	@echo ""
	@echo "   make install"
	@echo "      Install the compiled sourcecode."
	@echo ""
	@echo "   make clean"
	@echo "      Remove all like object (.o) files and the compiled binaries (FileStore and client)."
	@echo ""
	@echo "   make cleanall"
	@echo "      Remove all object (.o) files, certificates and keys (.key and .cert) and the compiled binaries (FileStore and client)."
	@echo ""
	@echo "   make clean"
	@echo "      Remove all certificates and keys (.key and .cert)."
	@echo ""
	@echo "   make certs"
	@echo "      Generate new certificate and private key pairs for the FileStore server and the debugging client."
	@echo ""
	@echo "   make help"
	@echo "      Show this help screen."

