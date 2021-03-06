/* vim: tabstop=4 shiftwidth=4 noexpandtab
 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (C) 2013-2014 Kevin Lange
 */

#ifndef HELIOS_SRC_USR_INCLUDE_SECURITY_HELIOS_AUTH_H_
#define HELIOS_SRC_USR_INCLUDE_SECURITY_HELIOS_AUTH_H_

int helios_auth(char * usr, char * pass);
void helios_auth_set_vars(void);

#endif /* HELIOS_SRC_USR_INCLUDE_SECURITY_HELIOS_AUTH_H_ */
