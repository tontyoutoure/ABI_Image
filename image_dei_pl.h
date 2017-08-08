/*
 * =====================================================================================
 *
 *       Filename:  image_dei.h
 *
 *    Description:  Header for the program turning pho into images
 *
 *        Version:  1.0
 *        Created:  05/10/2015 12:19:01 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#if 1
	#define DPRINTF(A) printf(A);fflush(stdout);
	#define DPRINTF2(A, B) printf(A, B); fflush(stdout);
#else
	#define DPRINTF(A)
	#define DPRINTF2(A, B)
#endif

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_interp.h>
#include <argp.h>
#include <matio.h>


#define NoT 1 // number of threads
#define SQRT2PI 2.50662827463
#define FORMAT_PNG 1
#define FORMAT_TIF 2
#define FORMAT_MAT 0

#define POSITION_FLAG 1

#define FLAG_SPEC_SIZE 1
#define FLAG_SPEC_POS 2
#define FLAG_SPEC_POSF 4
#define FLAG_SPEC_CRY 8
#define FLAG_SPEC_RC 64
#define FLAG_SPEC_DIS 128
#define FLAG_SPEC_LOG 256
#define FLAG_SPEC_POSA 512
#define FLAG_SPEC_ATT 1024

typedef struct {
	pthread_mutex_t *mutexpi;/*Mutex lock for input*/
	pthread_mutex_t *mutexpo;/*Mutex lock for output*/
	double a;/*size of the detector, x direction*/
	double b;/*size of the detector, y direction*/
	uint16_t X;/*size of the detector, x direction*/
	uint16_t Y;/*size of the detector, y direction*/
	int8_t att;
	int8_t cry;
	int8_t posa;
	double *pho;/*Photon data pointer*/
	double *image;/*Raw image*/
	double *pos;/*positions on the rocking curve*/
	double *photon_number;/*photon number for each position*/
	double **RC;/*Rocking_curve*/
	uint32_t number_of_positions;
	uint64_t number_of_photons;
	uint32_t LRC;
	uint32_t *pn;/*photon being calculated*/
	double fwhm;
	double dis;/*distance between the sample and the detector*/
} makeimage_t;

struct argp_input{
    double a; /*size of the specimen, x direction*/
	double b; /*size of the specimen, y direction*/
	double dis; /*distance from the specimen to the detector*/
	double pixel_size; /*size of the detector pixel*/
	double pos; /*position of the image on the rocking curve*/
	uint16_t flag_spec; /*Kinds of binary flags*/
	uint16_t input_n; /*Numbers of input files*/
	void *input_pt; /*Pointer to the top element of the filename stack*/
	char output_fn[FILENAME_MAX]; /*The output filename*/
	char log_fn[FILENAME_MAX]; /*The log filename*/
	char rc_fn[FILENAME_MAX]; /*The rocking curve filename*/
	char pos_fn[FILENAME_MAX]; /*The position filename*/
	uint32_t ppppf; /*How many photons a file include*/
	uint32_t interval; /*Output interval*/
	uint32_t verbose_level;
};

typedef struct {
	void *next;
	char filename[FILENAME_MAX];
}filename_element;

#include "prototype.h"

