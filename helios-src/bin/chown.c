#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>

int main(int argc, char ** argv) {
	/* XXX Not sure why this isn't working */
	if(argc > 2) {
		char * group = strchr(argv[1],':') + 1;
		char owner[256];
		memset(owner, 0, 256);
		memcpy(owner, argv[1], group-argv[1] - 1);

		struct passwd * p_owner = getpwnam(owner);

		chown(argv[2], p_owner->pw_uid, p_owner->pw_uid);
	} else {
		fprintf(stderr, "%s: usage: %s [owner]:[group] [file]\n", argv[0], argv[0]);
		exit(EXIT_FAILURE);
	}
	return 0;
}

