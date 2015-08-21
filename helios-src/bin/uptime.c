/*
 * uptime.c
 *
 *  Created on: Aug 21, 2015
 *      Author: miguel
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <getopt.h>
#include <sys/time.h>

#define MINUTE (60)
#define HOUR (60 * MINUTE)
#define DAY (24 * HOUR)
void print_seconds(int seconds) {
	if (seconds > DAY) {
		int days = seconds / DAY;
		seconds -= DAY * days;
		printf("%d day%s, ", days, days != 1 ? "s" : "");
	}
	if (seconds > HOUR) {
		int hours = seconds / HOUR;
		seconds -= HOUR * hours;
		int minutes = seconds / MINUTE;
		printf("%2d:%02d", hours, minutes);
		return;
	} else if (seconds > MINUTE) {
		int minutes = seconds / MINUTE;
		printf("%d minute%s,  ", minutes, minutes != 1 ? "s" : "");
		seconds -= MINUTE * minutes;
	}

	printf("%2d second%s", seconds, seconds != 1 ? "s" : "");
}

void print_uptime(void) {
	FILE * f = fopen("/proc/uptime", "r");
	if (!f) return;

	int seconds, subseconds;
	fscanf(f, "%d.%2d", &seconds, &subseconds);
	printf("up ");
	print_seconds(seconds);
	printf("\n");
}

int main(int argc, char * argv[]) {
	print_uptime();
	return 0;
}



