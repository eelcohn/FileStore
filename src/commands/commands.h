/* commands.h
 * Header file for all * commands
 *
 * (c) Eelco Huininga 2017-2018
 */

#ifndef ECONET_COMMANDS_HEADER
#define ECONET_COMMANDS_HEADER

#include "../api.h"                    // High level API calls to Econet

#include "../netfs.h"
#include "star_cat.h"
#include "star_cdir.h"
#include "star_clock.h"
#include "star_configure.h"
#include "star_date.h"
#include "star_delete.h"
#include "star_discs.h"
#include "star_dismount.h"
#include "star_exit.h"
#include "star_help.h"
#include "star_mount.h"
#include "star_netmon.h"
#include "star_newuser.h"
#include "star_notify.h"
#include "star_pass.h"
#include "star_printtest.h"
#include "star_priv.h"
#include "star_remuser.h"
#include "star_time.h"
#include "star_users.h"

extern const char	*cmds[][2];
extern int		(*cmds_jumptable[]) (char **);

int totalNumOfCommands(void);

#endif

