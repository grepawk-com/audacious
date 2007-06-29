/*
 * Audtool2
 * Copyright (c) 2007 Audacious development team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <mowgli.h>
#include <locale.h>
#include "libaudclient/audctrl.h"
#include "audtool.h"

void get_volume(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_main_volume(dbus_proxy);

	audtool_report("%d", i);
}

void set_volume(gint argc, gchar **argv)
{
	gint i, current_volume;

	if (argc < 2)
	{
		audtool_whine("invalid parameters for %s.", argv[0]);
		audtool_whine("syntax: %s <level>", argv[0]);
		return;
	}

	current_volume = audacious_remote_get_main_volume(dbus_proxy);
	switch (argv[1][0]) 
	{
		case '+':
		case '-':
			i = current_volume + atoi(argv[1]);
			break;
		default:
			i = atoi(argv[1]);
			break;
	}

	audacious_remote_set_main_volume(dbus_proxy, i);
}

void mainwin_show(gint argc, gchar **argv)
{
	if (argc > 1)
	{
		if (!strncmp(argv[1],"on",2)) {
			audacious_remote_main_win_toggle(dbus_proxy, TRUE);
			return;
		}
		else if (!strncmp(argv[1],"off",3)) {
			audacious_remote_main_win_toggle(dbus_proxy, FALSE);
			return;
		}
	}
	audtool_whine("invalid parameter for %s.", argv[0]);
	audtool_whine("syntax: %s <on/off>", argv[0]);
}

void playlist_show(gint argc, gchar **argv)
{
	if (argc > 1)
	{
		if (!strncmp(argv[1],"on",2)) {
			audacious_remote_pl_win_toggle(dbus_proxy, TRUE);
			return;
		}
		else if (!strncmp(argv[1],"off",3)) {
			audacious_remote_pl_win_toggle(dbus_proxy, FALSE);
			return;
		}
	}
	audtool_whine("invalid parameter for %s.", argv[0]);
	audtool_whine("syntax: %s <on/off>", argv[0]);
}

void equalizer_show(gint argc, gchar **argv)
{
	if (argc > 1)
	{
		if (!strncmp(argv[1],"on",2)) {
			audacious_remote_eq_win_toggle(dbus_proxy, TRUE);
			return;
		}
		else if (!strncmp(argv[1],"off",3)) {
			audacious_remote_eq_win_toggle(dbus_proxy, FALSE);
			return;
		}
	}
	audtool_whine("invalid parameter for %s.", argv[0]);
	audtool_whine("syntax: %s <on/off>", argv[0]);
}

void show_preferences_window(gint argc, gchar **argv)
{
	audacious_remote_show_prefs_box(dbus_proxy);
}

void show_jtf_window(gint argc, gchar **argv)
{
	audacious_remote_show_jtf_box(dbus_proxy);
}

void shutdown_audacious_server(gint argc, gchar **argv)
{
	audacious_remote_quit(dbus_proxy);
}

void get_handlers_list(gint argc, gchar **argv)
{
	gint i;

	for (i = 0; handlers[i].name != NULL; i++)
	{
		if (!g_strcasecmp("<sep>", handlers[i].name))
			audtool_report("%s%s:", i == 0 ? "" : "", handlers[i].desc);
		else
			audtool_report("   %-34s - %s", handlers[i].name, handlers[i].desc);
	}

	audtool_report("Handlers may be prefixed with `--' (GNU-style long-options) or not, your choice.");
	audtool_report("Report bugs to http://bugs-meta.atheme.org/");
}
