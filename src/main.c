#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pwd.h>
#include <shadow.h>
#include <unistd.h>

#include <termios.h>

#include "config.h"

void
print_usage ()
{
	puts ("usage: beroot command [args]");
}

char *
get_current_user_name ()
{
	struct passwd *pwd = getpwuid (getuid ());
	if (!pwd)
		{
			puts ("couldn't obtain your pwd");
			return NULL;
		}

	return pwd->pw_name;
}

const struct user_permissions *
user_in_permitted_users (char *name)
{
	int total_users
			= (int)(sizeof (permissions) / sizeof (struct user_permissions));

	for (int i = 0; i < total_users; i++)
		{
			const struct user_permissions *current_user = &permissions[i];
			if (!current_user)
				continue;

			if (strncmp (name, current_user->name, LOGIN_NAME_MAX) == 0)
				return current_user;
		}

	return NULL;
}

struct spwd *
obtain_target_swpd (const char *name)
{
	if (!name)
		return NULL;

	return getspnam (name);
}

_Bool
authenticate_user (char *password, char *entered_password)
{
	// the entered password ends with a \n, we remove it here
	entered_password[strcspn (entered_password, "\n")] = '\0';
	size_t password_len = strlen (password);

	char *encrypted_password = crypt (entered_password, password);
	int result = strncmp (password, encrypted_password, password_len);
	if (result == 0)
		return 1;

	return 0;
}

void
execute (char **command)
{
	size_t total_commands = 0;
	size_t maximum_commands = 10;
	char **commands = calloc (sizeof (char *), maximum_commands);
	if (!commands)
		{
			fputs ("Couldn't parse the commands\n", stderr);
			return;
		}

	while (*command != NULL)
		{
			if (total_commands >= maximum_commands)
				{
					maximum_commands *= 2;
					commands = realloc (commands, sizeof (char *) * maximum_commands);
				}

			commands[total_commands] = *command;

			command++;
			total_commands++;
		}

	commands[total_commands] = NULL;
	execvp (commands[0], commands);

	free (commands);
	commands = NULL;
}

void
get_password (char *password)
{
	if (!password)
		return;

	static struct termios old_t = { 0 }, new_t = { 0 };
	tcgetattr (STDIN_FILENO, &old_t);
	new_t = old_t;
	new_t.c_lflag &= ~(ECHO);
	tcsetattr (STDIN_FILENO, TCSANOW, &new_t);

	int i = 0;
	char c = '\0';
	while ((c = getchar ()) != '\n' && c != EOF && i < LOGIN_PASSWD_MAX)
		{
			password[i++] = c;
		}
	password[i] = '\0';

	tcsetattr (STDIN_FILENO, TCSANOW, &old_t);
	puts (""); // the \n character is not printed anymore
}

int
main (int argc, char *argv[])
{
	if (argc < 2)
		{
			print_usage ();
			return EXIT_FAILURE;
		}

	char *user_name = get_current_user_name ();
	if (!user_name)
		return EXIT_FAILURE;

	const struct user_permissions *user = user_in_permitted_users (user_name);
	if (!user)
		{
			fprintf (stderr, "%s is not allowed to run commands, aborting\n",
							 user_name);
			return EXIT_FAILURE;
		}

	struct spwd *target_swpd = user->permissions & PERM_ROOT
																 ? obtain_target_swpd ("root")
																 : obtain_target_swpd (user->target_user);
	if (!target_swpd)
		{
			fputs ("couldn't obtain the target user's shadow\n", stderr);
			return EXIT_FAILURE;
		}

	struct passwd *target_passwd = getpwnam (target_swpd->sp_namp);
	if (!target_passwd)
		{
			fputs ("couldn't obtain the passwd for the target user\n", stderr);
			return EXIT_FAILURE;
		}

	// TODO: Give the user 3 attempts
	_Bool is_authenticated = 0;
	if (user->permissions & PERM_NOPASSWD)
		{
			is_authenticated = 1;
		}
	else if (user->permissions & PERM_PASSWD)
		{
			printf ("Enter the %s password: ", user->target_user);
			char *entered_password = calloc (sizeof (char), LOGIN_PASSWD_MAX);
			get_password (entered_password);
			is_authenticated
					= authenticate_user (target_swpd->sp_pwdp, entered_password);

			free (entered_password);
		}
	else
		{
			fputs (
					"Please specify an authentication way, either PASSWD or NOPASSWD\n",
					stderr);
			return 0;
		}

	if (!is_authenticated)
		{
			fputs ("Couldn't authenticate you, please try again\n", stderr);
			return EXIT_FAILURE;
		}

	if (setuid (target_passwd->pw_uid) != 0)
		{
			fputs ("Couldn't switch to the target user\n", stderr);
			return EXIT_FAILURE;
		}

	execute (argv + 1);

	return EXIT_SUCCESS;
}
