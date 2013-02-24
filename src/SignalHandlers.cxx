/*
 * Copyright (C) 2003-2011 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "SignalHandlers.hxx"

#define SIGACTION (_POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE)
#ifdef WIN32
#  undef SIGACTION
#endif

#if SIGACTION

#include "Log.hxx"
#include "Main.hxx"
#include "event/Loop.hxx"
#include "GlobalEvents.hxx"
#include "mpd_error.h"

#include <glib.h>

#include <signal.h>
#include <errno.h>
#include <string.h>

static void exit_signal_handler(G_GNUC_UNUSED int signum)
{
	GlobalEvents::Emit(GlobalEvents::SHUTDOWN);
}

static void reload_signal_handler(G_GNUC_UNUSED int signum)
{
	GlobalEvents::Emit(GlobalEvents::RELOAD);
}

static void
x_sigaction(int signum, const struct sigaction *act)
{
	if (sigaction(signum, act, NULL) < 0)
		MPD_ERROR("sigaction() failed: %s", strerror(errno));
}

static void
handle_reload_event(void)
{
	g_debug("got SIGHUP, reopening log files");
	cycle_log_files();
}

#endif

void initSigHandlers(void)
{
#if SIGACTION
	struct sigaction sa;

	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	x_sigaction(SIGPIPE, &sa);

	sa.sa_handler = exit_signal_handler;
	x_sigaction(SIGINT, &sa);
	x_sigaction(SIGTERM, &sa);

	GlobalEvents::Register(GlobalEvents::RELOAD, handle_reload_event);
	sa.sa_handler = reload_signal_handler;
	x_sigaction(SIGHUP, &sa);
#endif
}
