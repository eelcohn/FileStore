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

# --------------------------------------------------------------------------- #
# Set the compiler flags:                                                     #
#  -g            adds debugging information to the executable file            #
#  -Wall         turns on most, but not all, compiler warnings                #
#  -Wall -Wextra turns on almost all compiler warnings                        #
#  -O2           do some code optimizations when compiling                    #
#  -std=c++11    turns on compiler and library support for the 2011 C++       #
#                standard (needed for thread support)                         #
# --------------------------------------------------------------------------- #
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
MAIN_SRCS = \
	main.cpp \
	api.cpp \
	debug.cpp \
	dtls.cpp \
	econet.cpp \
	ethernet.cpp \
	errorhandler.cpp \
	gpio.cpp \
	stations.cpp \
	users.cpp \
	commands/commands.cpp \
	netfs.cpp \
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

CLIENT_SRCS = \
	client.cpp \
	dtls.cpp

# define the C object files
OBJS		= $(MAIN_SRCS:.cpp=.o)
CLIENT_OBJS	= $(CLIENT_SRCS:.cpp=.o)

# define the executable file
MAIN	= ../FileStore
CLIENT	= ../client

# --------------------------------------------------------------------------- #
# Define the OpenSSL parameters                                               #
# --------------------------------------------------------------------------- #
OPENSSL_KEYSIZE		= 4096
# Switch to EC curve x25519 once OpenSSL 1.1.0 is released
OPENSSL_ECDHCURVE	= x25519
OPENSSL_EXPIRE		= 3650
OPENSSL_CA_DIR		= ../conf/keys/
OPENSSL_SUBJ		= "/C=UK/ST=Cambridgeshire/L=Cambridge/O=Acorn"
CLIENT_KEY		= client.key
CLIENT_CERT		= client.cert
SERVER_KEY		= server.key
SERVER_CERT		= server.cert



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
	$(CC) $(CFLAGS) -I$(INCLUDES) -c $< -o $@

all:	$(MAIN) $(CLIENT) certs
	@echo
	@echo Done! Your Econet FileStore gateway server is compiled.
	
$(MAIN): $(OBJS)
	@echo Linking $(MAIN)...
	$(CC) $(CFLAGS) -I$(INCLUDES) -o $(MAIN) $(OBJS) -L$(LDFLAGS) $(LIBS)

$(CLIENT): $(CLIENT_OBJS)
	@echo Linking $(CLIENT)...
	$(CC) $(CFLAGS) -I$(INCLUDES) -o $(CLIENT) $(CLIENT_OBJS) -lssl -lcrypto

certs:
	@echo Generating certificates...
	@mkdir -p ../conf/keys/
	@openssl req -x509 -sha512 -newkey rsa:${OPENSSL_KEYSIZE} -days ${OPENSSL_EXPIRE} -nodes -subj ${OPENSSL_SUBJ} -keyout ${OPENSSL_CA_DIR}/${CLIENT_KEY} -out ${OPENSSL_CA_DIR}/${CLIENT_CERT}
#	@openssl req -x509 -sha512 -newkey ec -pkeyopt ec_paramgen_curve:${OPENSSL_ECDHCURVE} -days ${OPENSSL_EXPIRE} -nodes -subj ${OPENSSL_SUBJ} -keyout ${OPENSSL_CA_DIR}/${CLIENT_KEY} -out ${OPENSSL_CA_DIR}/${CLIENT_CERT}
	@chmod 600 ${OPENSSL_CA_DIR}/${CLIENT_KEY}
	@chmod 600 ${OPENSSL_CA_DIR}/${CLIENT_CERT}
#	@openssl req -x509 -sha512 -newkey ec -pkeyopt ec_paramgen_curve:${OPENSSL_ECDHCURVE} -days ${OPENSSL_EXPIRE} -nodes -subj ${OPENSSL_SUBJ} -keyout ${OPENSSL_CA_DIR}/${SERVER_KEY} -out ${OPENSSL_CA_DIR}/${SERVER_CERT}
	@openssl req -x509 -sha512 -newkey rsa:${OPENSSL_KEYSIZE} -days ${OPENSSL_EXPIRE} -nodes -subj ${OPENSSL_SUBJ} -keyout ${OPENSSL_CA_DIR}/${SERVER_KEY} -out ${OPENSSL_CA_DIR}/${SERVER_CERT}
	@chmod 600 ${OPENSSL_CA_DIR}/${SERVER_KEY}
	@chmod 600 ${OPENSSL_CA_DIR}/${SERVER_CERT}

clean:
	$(RM) $(OBJS) $(MAIN)
	$(RM) $(CLIENT_OBJS) $(CLIENT)
	$(RM) *~

certclean:
	$(RM) ${OPENSSL_CA_DIR}/${SERVER_KEY} ${OPENSSL_CA_DIR}/${SERVER_CERT}
	$(RM) ${OPENSSL_CA_DIR}/${CLIENT_KEY} ${OPENSSL_CA_DIR}/${CLIENT_CERT}

cleanall: clean certclean

depend: $(MAIN_SRCS)
	makedepend $(INCLUDES) $^

help:
	@echo Options
	@echo "   make"
	@echo "      Compile the sourcecode."
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

