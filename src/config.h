#ifndef __CONFIG_H
#define __CONFIG_H

#include <limits.h>

#define LOGIN_PASSWD_MAX 16384 // i think this is enough for a password?

#define PERM_ROOT 1 << 0		 /* will be the root user */
#define PERM_USER 1 << 1		 /* will be a custom specified user */
#define PERM_PASSWD 1 << 2	 /* password is required */
#define PERM_NOPASSWD 1 << 3 /* password is not required */

struct user_permissions
{
	const char name[LOGIN_NAME_MAX];
	const char target_user[LOGIN_NAME_MAX];
	unsigned char permissions;
};

const static struct user_permissions permissions[]
		= { { .name = "lain",
					.target_user = "\0",
					.permissions = PERM_ROOT | PERM_NOPASSWD } };

#endif
