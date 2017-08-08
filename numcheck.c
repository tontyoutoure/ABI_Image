/*
 * =====================================================================================
 *
 *       Filename:  numcheck.c
 *
 *    Description:  check if a string is a number, and what kind of number it is.
 *
 *        Version:  1.0
 *        Created:  05/06/2015 05:55:56 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ifstrnum
 *  Description:  retrun -1 if the string is not a valid number, return 0 if it is a
 				  intergal, return 1 if it is a simple float number, return 2 if it is
				  a scientifc notation number.
 * =====================================================================================
 */

int16_t ifstrnum(const char *str) {
	int16_t i = 0;
	if (str[i] == '-')
		i++;
	if (str[i] < '0' || str[i] > '9')
		return -1;
	while (str[i] >= '0' && str[i] <= '9')
		i++;
	if (str[i] == '\0')
		return 0;  
	else if (str[i] == '.') {
		i++;
		if (str[i] == '\0') 
			return 1;
		else if (str[i] >= '0' && str[i] <= '9') {
			while (str[i] >= '0' && str[i] <= '9')
				i++;
			if (str[i] == '\0')
				return 1;
		}
		
		if (str[i] != 'e' && str[i] != 'E')
			return -1;
	}
	else if (str[i] != 'e' && str[i] != 'E')
		return -1;
	i++;
	if (str[i] == '-')
		i++;
	if (str[i] < '0' || str[i] > '9')
		return -1;
	while (str[i] >= '0' && str[i] <= '9')
		i++;
	if (str[i] == '\0')
		return 2;
	else 
		return -1;
}		/* -----  end of function ifstrnum  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  eatof
 *  Description:  A function used for turning string into double presice float number,
 				  which is compatible with scientific notation. If a invalid string is
				  inputted, the function would return 15498.
 * =====================================================================================
 */
double eatof(const char *str) {
	double a;
	int16_t n, expo;
	int8_t *cp;
	int8_t buf[FILENAME_MAX];
	n = ifstrnum(str);
	switch (n) {
		case 0:
			return atof(str);
		case 1:
			return atof(str);
		case 2:
			strcpy(buf, str);
			cp = strchr(buf, 'e');
			if (cp == NULL)
				cp = strchr(buf, 'E');
			*cp = '\0';
			cp++;
			expo = atoi(cp);
			a = atof(buf);
			if (expo >= 0) {
				while (expo > 0) {
					a *= 10;
					expo--;
				}
			}
			else {
				while (expo < 0) {
					a /= 10;
					expo++;
				}
			}
			return a;
		default:
			return NAN;

	}
}		/* -----  end of function eatoi  ----- */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
/*int main(int argc, char *argv[]) {
	char buf[FILENAME_MAX];
	while (1) {
		scanf("%s", buf);
		printf("%1.53lf\n", eatof(buf));
	}
	return EXIT_SUCCESS;
}*/				/* ----------  end of function main  ---------- */

