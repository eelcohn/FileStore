/* cli.cpp
 * Handler for all Command Line Interpreter commands
 *
 * (c) Eelco Huininga 2017-2019
 */

#include <cstdio>			// NULL, printf(), FILE*, fopen(), fgets(), feof() and fclose()
#include <cstdlib>			// strtol()
#include <cstring>			// strlen(), strncpy()
#include <unistd.h>			// usleep()
#include <termios.h>			// struct termios
#include <ctime>			// time_t tm
#include <readline/readline.h>		// rl_attempted_completion_over, rl_completion_matches()
#include "cli.h"
#include "config.h"			// DEBUG_BUILD
#include "debug.h"			// debug::*
#include "econet.h"			// econet::netmon and econet::Frame
#include "main.h"			// bye, strtoupper() and STARTUP_MESSAGE
#include "netfs.h"			// netfs::*
#include "settings.h"			// settings::*
#include "stations.h"			// stations::*
#include "users.h"			// MAX_PASSWORD_LENGTH, users::newUser()
#include "platforms/platform.h"
#if (FILESTORE_HAS_KBHIT == 0)
#include "platforms/kbhit.h"
#endif


extern std::atomic<bool>	bye;
extern FILE			*fp_volume;

const char *modules[]={"OS", "NetFS", "Debug", NULL};

Command commands[] {
#if (FILESTORE_DEBUG == 1) && (FILESTORE_ADAPTER == FILESTORE_ADAPTER_REMA)
	{debug::read,		"DEBUG",	"READ",		"<reg>"},
	{debug::write,		"DEBUG",	"WRITE",	"<reg> <value>"},
	{debug::d,		"DEBUG",	"D",		"<0..7> <0|1>"},
	{debug::rs,		"DEBUG",	"RS",		"<0..3>"},
	{debug::rw,		"DEBUG",	"RW",		"<R|W>"},
	{debug::cs,		"DEBUG",	"CS",		"<ON|OFF>"},
	{debug::rst,		"DEBUG",	"RST",		"<ON|OFF>"},
	{debug::phi2,		"DEBUG",	"PHI2",		"<ON|OFF>"},
#endif
	{cli::access,		"NETFS",	"ACCESS",	"<filename> (RWL)"},
	{cli::logout,		"OS",		"BYE",		""},
	{cli::cat,		"NETFS",	"CAT",		""},
	{cli::cdir,		"NETFS",	"CDIR",		"<dir>"},
	{cli::clock,		"OS",		"CLOCK",	"<ON|OFF|AUTO>"},
	{cli::clockspeed,	"OS",		"CLOCKSPEED",	""},
	{cli::configure,	"OS",		"CONFIGURE",	"<keyword> (value)"},
	{cli::date,		"OS",		"DATE",		""},
	{cli::del,		"NETFS",	"DELETE",	"<fsp>"},
	{cli::discs,		"OS",		"DISCS",	""},
	{cli::dismount,		"NETFS",	"DISMOUNT",	""},
	{cli::ex,		"NETFS",	"EX",		"<filename>"},
	{cli::exit,		"OS",		"EXIT",		""},
	{cli::help,		"OS",		"HELP",		"(command)"},
	{cli::login,		"OS",		"LOGIN",	"<username> (password)"},
	{cli::logout,		"OS",		"LOGOUT",	""},
	{cli::info,		"NETFS",	"INFO",		"<fsp>"},
	{cli::mount,		"NETFS",	"MOUNT",	"<filename>"},
	{cli::netmon,		"OS",		"NETMON",	""},
	{cli::newuser,		"OS",		"NEWUSER",	"<username> <password>"},
	{cli::notify,		"OS",		"NOTIFY",	"<station id> <message>"},
	{cli::pass,		"OS",		"PASS",		"<username> <password>"},
	{cli::printtest,	"OS",		"PRINTTEST",	""},
	{cli::priv,		"OS",		"PRIV",		"<username> (S)"},
	{cli::remuser,		"OS",		"REMUSER",	"<username>"},
	{cli::rename,		"NETFS",	"RENAME",	"<filename>"},
	{cli::sessions,		"OS",		"SESSIONS",	""},
	{cli::stations,		"OS",		"STATIONS",	""},
	{cli::star_time,	"OS",		"TIME",		""},
	{cli::users,		"OS",		"USERS",	"(mask)"},
	{NULL,			NULL,		NULL,		NULL}
};

using namespace std;



namespace cli {
	unsigned int	user_id = 0;		// User ID of the user currently logged into the CLI

	int access(int argv, char **args) {
		if (argv == 3) {
			return (netfs::access(args[1], args[2]));
		} else {
			return(-2);
		}
		return(0);
	}

	int cat(int argv, char **args) {
		FSDirectory dir[ECONET_MAX_DIRENTRIES];
		char access[FILESTORE_MAX_ATTRIBS];
		int i, result;

		if ((argv == 1) || (argv == 2)) {
			if (argv == 1) {
				result = netfs::catalogue(0x00, dir, "", 0, ECONET_MAX_DIRENTRIES);
			} else {
				result = netfs::catalogue(0x00, dir, args[1], 0, ECONET_MAX_DIRENTRIES);
			}
			for (i = 0; i < result; i++) {
				netfs::attribtostr(&dir->fsp[i].attrib, access);
				printf("%-10s %08X %08X %06X %s\n", dir->fsp[i].name, dir->fsp[i].loadaddr, dir->fsp[i].execaddr, dir->fsp[i].length, access); /* TODO replace %-10s by ECONET_MAX_FILENAME_LEN */
			}
		} else {
			return(-2);
		}
		return(0);
	}

	int cdir(int argv, char **args) {
		if (argv == 2) {
			return (netfs::cdir(args[1]));
		} else {
			return(-2);
		}
		return(0);
	}

	int clock(int argv, char **args) {
		if (argv != 2) {
			return(-2);
		} else {
			strtoupper(args[1]);
			if (strcmp(args[1], "ON") == 0) {
				api::startClock();
				printf("Internal clock is now on (%iHz %i%%).\n", settings::clockspeed, settings::dutycycle);
			} else if (strcmp(args[1], "OFF") == 0) {
				api::stopClock();
				printf("Internal clock is now off.\n");
			} else if (strcmp(args[1], "AUTO") == 0) {
				printf("CLOCK AUTO - Not yet implemented\n");
			} else {
				return(-2);
			}
		}
		return(0);
	}

	int clockspeed(__attribute__((__unused__))int argv, __attribute__((__unused__))char **args) {
		printf("Measured clockspeed is %i Hz\n", api::getClockSpeed());
		return(0);
	}

	int configure(int argv, char **args) {
		int value;
		FILE *fp_printer;

		if (argv == 1) {
			printf("STATION         %i\n", settings::econet_station);
			printf("NETWORK         %i\n", settings::econet_network);
			printf("AUNNETWORK      %i\n", settings::aun_network);
			printf("AUTOLEARN       %i\n", settings::autolearn);
			printf("VOLUME          %s\n", settings::volume);
			printf("PRINTQUEUE      %s\n", settings::printqueue);
			if (settings::clock == true)
				printf("CLOCK           ON\n");
			else
				printf("CLOCK           OFF\n");
			printf("CLOCKSPEED      %iHz\n", settings::clockspeed);
			printf("DUTYCYCLE       %i%%\n", settings::dutycycle);
			printf("ONERROR         %s\n", settings::onError);
		} else if ((argv == 2) || (argv == 3)) {
			strtoupper(args[1]);

			if (strcmp(args[1], "STATION") == 0) {
				value = strtol(args[2], NULL, 10);
				if ((value < 1) || (value > 254)) {
					printf("Error: %s is an invalid station ID\n", args[2]);
					return(0x000000FD);
				} else {
					settings::econet_station = value;
					printf("Econet station ID set to %i\n", value);
				}
			} else if (strcmp(args[1], "NETWORK") == 0) {
				value = strtol(args[2], NULL, 10);
				if ((value < 1) || (value > 254)) {
					printf("Error: %s is an invalid network ID\n", args[2]);
					return(0x000000FD);
				} else {
					settings::econet_network = value;
					printf("Econet network ID set to %i\n", value);
				}
			} else if (strcmp(args[1], "AUNNETWORK") == 0) {
				value = strtol(args[2], NULL, 10);
				if ((value < 1) || (value > 254)) {
					printf("Error: %s is an invalid network ID\n", args[2]);
					return(0x000000FD);
				} else {
					settings::aun_network = value;
					printf("AUN network ID set to %i\n", value);
				}
			} else if (strcmp(args[1], "PRINTQUEUE") == 0) {
				if (!(fp_printer = fopen(args[2], "w"))) {
					printf("Error: Could not open %s\n", args[2]);
					return(0x000000FD);
				} else {
					fclose (fp_printer);
					settings::printqueue = (unsigned char *)args[2];
					printf("Printqueue set to %s\n", args[2]);
				}
			} else if (strcmp(args[1], "CLOCKSPEED") == 0) {
				value = strtol(args[2], NULL, 10);
				if ((value < 50000) || (value > 1000000)) {
					printf("Error: %s is an invalid clock speed value\n", args[2]);
					return(0x000000FD);
				} else {
					settings::clockspeed = value;
					printf("Clockspeed set to %i Hz\n", value);
				}
			} else if (strcmp(args[1], "DUTYCYCLE") == 0) {
				value = strtol(args[2], NULL, 10);
				if ((value < 1) || (value > 99)) {
					printf("Error: %s is an invalid duty cycle value\n", args[2]);
					return(0x000000FD);
				} else {
					settings::dutycycle = value;
					printf("Dutycycle set to %i%%\n", value);
				}
			} else if (strcmp(args[1], "ONERROR") == 0) {
				if (strlen(args[2]) > 256) {
					printf("Error: String too long\n");
					return(0x000000FD);
				} else {
					strlcpy((char *)settings::onError, args[2], sizeof(settings::onError));
					printf("ON ERROR handler set to %s\n", args[2]);
				}
			} else
				return(0x000000FE);
		} else
			return(-2);

		return(0);
	}

	int date(__attribute__((__unused__))int argv, __attribute__((__unused__))char **args) {
		char buffer[128];
		time_t rawtime;
		struct tm * timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime (buffer, BUFFER_LENGTH, "Today is %A the %eth of %B %Y\n", timeinfo);

		puts(buffer);
		return(0);
	}

	int del(int argv, char **args) {
		if (argv == 2) {
			return (netfs::del(args[1]));
		} else {
			return(-2);
		}
		return(0);
	}

	int discs(__attribute__((__unused__))int argv, __attribute__((__unused__))char **args) {
		int i;

		printf("Slot  Disc title  Filename\n");
		for (i = 0; i < ECONET_MAX_DISCDRIVES; i++) {
//			if (discs[i] == NULL)
				printf("%i\n", i);
//			else
//				printf("%i  %s\n", discs[i].image->title);
		}

		return(0);
	}

	int dismount(int argv, char **args) {
		if (argv == 2) {
			return (netfs::dismount(args[1]));
		} else {
			return(-2);
		}
		return(0);
	}

	int ex(int argv, char **args) {
		if (argv == 2) {
			return (netfs::ex(args[1]));
		} else {
			return(-2);
		}
		return(0);
	}

	int exit(__attribute__((__unused__))int argv, __attribute__((__unused__))char **args) {
		bye = true;
		return(0);
	}

	int help(int argv, char **args) {
		size_t i;

		printf(STARTUP_MESSAGE " v" FILESTORE_VERSION_MAJOR "." FILESTORE_VERSION_MINOR "." FILESTORE_VERSION_PATCHLEVEL "\n");

		if(argv == 2) {
			strtoupper(args[1]);

			for (i = 0; i < (totalNumOfCommands()); i++) {
				if (strcmp(args[1], commands[i].module) == 0)
					printf("   %s %s\n", commands[i].command, commands[i].help);
			}
			printf("\n");
		} else if (argv == 1) {
			for (i = 0; i < 3; i++) {
				printf("   %s\n", modules[i]);
			}
			printf("\n");
		}

		return(0);
	}

	int info(int argv, char **args) {
		if (argv == 2) {
			return (netfs::info(args[1]));
		} else {
			return(-2);
		}
		return(0);
	}

	int login(int argv, char **args) {
		int result, user_id;
		char *password;

		if (argv == 2) {
			if ((user_id = users::getUserID(args[1])) != -1) {
				password = (char *) malloc(MAX_PASSWORD_LENGTH);

				if (getPassword("Enter password: ", password, sizeof(password)) != 0) {
					free(password);
					return(0x000000FF);
				}

				if ((result = users::login(user_id, password, 0, 0)) == 0) {
					printf("Login succesfull\n");
					cli::user_id = user_id;
					free(password);
					return(0);
				} else {
					free(password);
					return(result);
				}
			} else {
				return(0x000000BC);
			}
		}

		if (argv == 3) {
			if ((user_id = users::getUserID(args[1])) != -1) {
				if ((result = users::login(user_id, args[2], 0, 0)) == 0) {
					cli::user_id = user_id;
					printf("Login succesfull\n");
					cli::user_id = user_id;
					return(0);
				} else {
					return(result);
				}
			} else {
				return(0x000000BC);
			}
		}

		return (-2);
	}

	int logout(__attribute__((__unused__))int argv, __attribute__((__unused__))char **args) {
		int result;

		if ((result = users::logout(cli::user_id, 0, 0)) == 0) {
			cli::user_id = -1;
			return (0);
		} else {
			return (result);
		}
	}

	int mount(int argv, char **args) {
		if (argv == 3) {
			return (netfs::mount(strtol(args[1], NULL, 10), args[2]));
		} else {
			return(-2);
		}
		return(0);
	}

	int netmon(__attribute__((__unused__))int argv, __attribute__((__unused__))char **args) {
		struct termios oflags, nflags;

		/* Get current values */
		if (tcgetattr(fileno(stdin), &nflags) == -1) {
			fprintf(stderr, "cli::netmon: tcgetattr error\n");
			return -1;
		}
		oflags = nflags;			/* Preserve original values */

		/* Disable echo-ing of input characters */
		nflags.c_lflag &= ~ECHO;
		if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
			fprintf(stderr, "cli::netmon: tcsetattr disable echo error\n");
			return -1;
		}

		econet::netmon = true;
		printf("Netmon started; press <ENTER> key to exit.\n");

		// The econet::pollNetworkReceive(), econet::sendFrame(), ethernet::pollNetworkReceive() and ethernet::sendFrame() will check if econet::netmon is set to true.
		// If it is, they will print the sent or received frame to stdin
		while (bye == false) {
			if (kbhit()) {
				getc(stdin);
				bye = true;
			}
			/* Ease down on the CPU a bit */
			usleep(250);
		}

		/* Restore echo */
		if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
			fprintf(stderr, "cli::netmon: tcsetattr enable echo error\n");
			return -1;
		}

		econet::netmon = false;
		bye = false;
		printf("\n");
		return(0);
	}

	int newuser(int argv, char **args) {
		char *password1, *password2;

		if (argv == 2) {
			if (users::getUserID(args[1]) == -1) {
				password1 = (char *) malloc(MAX_PASSWORD_LENGTH);
				password2 = (char *) malloc(MAX_PASSWORD_LENGTH);

				if (getPassword("Enter password: ", password1, sizeof(password1)) != 0) {
					free(password1);
					return(0);
				}

				if (getPassword("Re-type password: ", password2, sizeof(password2)) != 0) {
					free(password1);
					free(password2);
					return(0);
				}

				if (strcmp(password1, password2) != 0) {
					free(password1);
					free(password2);
					return 0x00000030;
				}

				if ((users::newUser(args[1], password1)) == true) {
					printf("Added new user %s\n", args[1]);
					free(password1);
					free(password2);
					return(0);
				} else {
					printf("Error adding user %s\n", args[1]);
					free(password1);
					free(password2);
					return(-2);
				}
			} else {
				printf("User %s already exists.\n", args[1]);
				return(-1);
			}
		}
		if (argv == 3) {
			if ((users::newUser(args[1], args[2])) == true) {
				printf("Added new user %s\n", args[1]);
				return(0);
			} else {
				printf("Error adding user %s\n", args[1]);
				return(-2);
			}
		}

		return (-2);
	}

	int notify(int argv, char **args) {
		/* Temporary code to prevent -Wunused-parameter for now */
		printf("%i %s\n", argv, args[1]);
		return(0);
	}

	int pass(int argv, char **args) {
		int user_id;

		if (argv == 4) {
			if ((user_id = users::getUserID(args[1])) != -1) {
				if (users::users[cli::user_id].flags.s == true) {
					if (users::users[cli::user_id].flags.p == true) {
						if (users::changePassword(user_id, args[2], args[3]) == 0) {
							printf("Password for user %s successfully changed.\n", args[1]);
							return(0);
						} else {
							fprintf(stderr, "Error trying to change password.\n");
							return(0x12345678);
						}
					} else {
						return(0x000000C3);
					}
				} else {
					return(0x000000BA);
				}
			} else {
				return(0x000000BC);
			}
		} else
			return (-2);

	}

	int printtest(__attribute__((__unused__))int argv, __attribute__((__unused__))char **args) {
		return(0);
	}

	int priv(int argv, char **args) {
		if (argv == 3) {
			/* Temporary code to prevent -Wunused-parameter for now */
			printf("%i %s\n", argv, args[1]);
			return(0);
		} else
			return (-2);
	}

	int remuser(int argv, char **args) {
		if (argv == 2) {
			printf("Deleted user %s\n", args[1]);
			return(0);
		} else
			return (-2);
	}

	int rename(int argv, char **args) {
		if (argv == 3) {
			return (netfs::rename(args[1], args[2]));
		} else {
			return(-2);
		}
		return(0);
	}

	int sessions(int argv, char **args) {
		char flags[MAX_USER_FLAGS];
		char buffer[BUFFER_LENGTH];
		struct tm *timeinfo;
		size_t i;

		if ((argv == 1) || (argv == 2)) {
			/* Temporary code to prevent -Wunused-parameter for now */
			printf("List all users. Mask = %s\n", args[1]);

			printf("UsrId  Net:Stn  Username    Flags   Login time\n");
			for (i = 0; i < users::totalSessions; i++) {
				users::getUserFlags(users::sessions[i].user_id, flags);
				timeinfo = localtime (&users::sessions[i].login_time);
				strftime(buffer, BUFFER_LENGTH, "%a %d %b %Y %H:%M:%S", timeinfo);
				printf("%5d  %3d:%3d  %-10s  %-6s  %s\n", users::sessions[i].user_id, users::sessions[i].network, users::sessions[i].station, users::users[users::sessions[i].user_id].username, flags, buffer);
			}
		} else {
			return(-2);
		}

		return(0);
	}

	int star_time(__attribute__((__unused__))int argv, __attribute__((__unused__))char **args) {
		char buffer[BUFFER_LENGTH];
		time_t rawtime;
		struct tm *timeinfo;

		time(&rawtime);
		timeinfo = localtime(&rawtime);
		if (timeinfo->tm_hour < 6)
			strftime(buffer, BUFFER_LENGTH, "Good night! It's %H:%M:%S\n", timeinfo);
		else if (timeinfo->tm_hour < 12)
			strftime(buffer, BUFFER_LENGTH, "Good morning! It's %H:%M:%S\n", timeinfo);
		else if (timeinfo->tm_hour < 18)
			strftime(buffer, BUFFER_LENGTH, "Good afternoon! It's %H:%M:%S\n", timeinfo);
		else
			strftime(buffer, BUFFER_LENGTH, "Good evening! It's %H:%M:%S\n", timeinfo);

		puts(buffer);
		return(0);
	}

	int stations(__attribute__((__unused__))int argv, __attribute__((__unused__))char **args) {
		char ipstr[BUFFER_LENGTH];
		int n, s;

		if (argv == 1) {
			printf("Net:Stn  Type      IP Address                  Port   Fingerprint\n");
			for (n = 0; n < 126; n++) {
				for (s = 0; s < 255; s++) {
//					if (stations::stations[n][s].type != STATION_UNUSED) {
					switch (stations::stations[n][s].type) {
						case STATION_UNUSED :
							continue;
							break;

						case STATION_IPV4 :
							inet_ntop(AF_INET, &stations::stations[n][s].ipv4, ipstr, sizeof(ipstr));
							break;

						case STATION_IPV6 :
							inet_ntop(AF_INET6, &stations::stations[n][s].ipv6, ipstr, sizeof(ipstr));
							break;

						default :
							strlcpy(ipstr, "-", sizeof(ipstr));
							break;
					}
					printf("%3d:%3d  %-8s  %-26s  %-5d  %s\n", n, s, station_type[stations::stations[n][s].type], ipstr, stations::stations[n][s].port, stations::stations[n][s].fingerprint);
				}
			}
		} else {
			return(-2);
		}

		return(0);
	}

	int users(int argv, char **args) {
		char flags[MAX_USER_FLAGS];
		char buffer[BUFFER_LENGTH];
		char bootopt[5];
		struct tm *timeinfo;
		size_t i;

		if ((argv == 1) || (argv == 2)) {
			/* Temporary code to prevent -Wunused-parameter for now */
			printf("List all users. Mask = %s\n", args[1]);

			printf("UsrId  Username    Flags   Boot  Last login time\n");
			for (i = 0; i < users::totalUsers; i++) {
				users::getUserFlags(users::sessions[i].user_id, flags);
				users::getBootOption(users::users[i].bootoption, bootopt);
				timeinfo = localtime (&users::sessions[i].login_time);
				strftime(buffer, BUFFER_LENGTH, "%a %d %b %Y %H:%M:%S", timeinfo);
				printf("%5lu  %-10s  %-6s  %-4s  %s\n", i, users::users[i].username, flags, bootopt, buffer);
			}
		} else {
			return(-2);
		}

		return(0);
	}

	char **command_completion(const char *text, __attribute__((__unused__))int start, __attribute__((__unused__))int end) {
		/* Disable the default filename completion */
		rl_attempted_completion_over = 1;
		return rl_completion_matches(strtoupper((char *)text), cli::command_generator);
	}

	char *command_generator(const char *text, int state) {
		static int list_index, len;
		char *cmd;

		if (!state) {
			list_index = 0;
			len = strlen(text);
		}

		if (strncmp(rl_line_buffer, "HELP", 4) == 0) {
			while ((cmd = (char *)modules[list_index++])) {
				strtoupper(cmd);
				if (strncmp(cmd, text, len) == 0) {
					return strdup(cmd);
				}
			}
		} else {
			while ((cmd = (char *)commands[list_index++].command)) {
				strtoupper(cmd);
				if (strncmp(cmd, text, len) == 0) {
					return strdup(cmd);
				}
			}
		}

		return NULL;
	}

	int getPassword(const char *prompt, char *password, size_t pwlen) {
		struct termios oflags, nflags;

		/* Get current values */
		if (tcgetattr(fileno(stdin), &nflags) == -1) {
			fprintf(stderr, "cli::getPassword: tcgetattr error\n");
			return -1;
		}
		oflags = nflags;			/* Preserve original values */

		/* Disable echo-ing of input characters */
		nflags.c_lflag &= ~ECHO;
		if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0) {
			fprintf(stderr, "cli::getPassword: tcsetattr disable echo");
			return -1;
		}

		printf(prompt);
		if (fgets(password, pwlen, stdin) == NULL) {
			fprintf(stderr, "cli::getPassword: Aborted\n");
			return(-1);
		}

		/* Restore echo */
		if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0) {
			fprintf(stderr, "cli::getPassword: tcsetattr enable echo");
			return -1;
		}

		/* Remove any trailing LF, CR, CRLF or LFCR's */
		strtok(password, "\n\r");

		return(0);
	}
}

/* Hex-dump a frame to stdout */
void netmonPrintFrame(const char *interface, bool tx, econet::Frame *frame, int size) {
	const uint8_t chars_per_line = 16;
	int i, offset;

	printf("          Offs  Tr Po Ct Rt -Sequence--\n");

	tx ? printf("Tx") : printf("Rx");

//	printf("  %s  %s  dst=%02X:%02X  src=%02X:%02X  ctrl=%02X  port=%02X  size=%i bytes\n", interface, (frame->flags | ECONET_FRAME_INVALID) ? "v" : ".", frame->econet.dst_network, frame->econet.dst_station, frame->econet.src_network, frame->econet.src_station, frame->control, frame->port, size);
	printf(" %s %s  ", interface, (frame->flags | ECONET_FRAME_INVALID) ? "v" : ".");

	offset = 0;
	while ((size - offset) > 0) {
		if (offset < chars_per_line)
			printf("%04X  ", offset);
		else
			printf("          %04X  ", offset);

		for (i = 0; i < chars_per_line; i++) {
			if ((offset + i) < size)
				printf("%02X ", frame->rawdata[offset + i]);
			else
				printf("   ");
		}
		printf(" ");
		for (i = 0; i < chars_per_line; i++) {
			if ((offset + i) < size) {
				if ((frame->rawdata[offset + i] > 31) && (frame->rawdata[offset + i] < 127))
					printf("%c", frame->rawdata[offset + i]);
				else
					printf(".");
			}
		}
		printf("\n");
		offset += chars_per_line;
	}
}

size_t totalNumOfCommands(void) {
	return ((sizeof(commands) / sizeof(char *) / 4) - 1);
}

