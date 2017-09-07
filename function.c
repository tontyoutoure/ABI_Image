/*
 * =====================================================================================
 *
 *       Filename:  function.c
 *
 *    Description:  functions used in image_dei
 *
 *        Version:  1.0
 *        Created:  05/06/2015 01:47:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  tontyoutoure 
 *   Organization:  
 *
 * =====================================================================================
 */

#include "image_dei_pl.h"

int ParseOpt(int32_t key, char *arg, struct argp_state *state) {
	char buf[FILENAME_MAX];
	char *cp;
	int16_t n;
	struct argp_input *input = state->input;
	switch (key) {
		case 's':
			cp = strchr(arg, '_')+1;
			n = ifstrnum(cp);
			if (n >= 0)
				input->a = eatof(cp);
			else {
				fprintf(stderr, "Size input error\n");
				exit(-1);
			}
			strcpy(buf, arg);
			cp = strchr(buf, '_');
			*cp = '\0';
			n = ifstrnum(buf);
			if (n >= 0)
				input->b = eatof(buf);
			else {
				fprintf(stderr, "Size input error\n");
				exit(-1);
			}
			break;
		case 'g':
			cp = strchr(arg, '_')+1;
			n = ifstrnum(cp);
			if (n >= 0)
				input->fSigmaX = eatof(cp);
			else {
				fprintf(stderr, "Sigma input error\n");
				exit(-1);
			}
			strcpy(buf, arg);
			cp = strchr(buf, '_');
			*cp = '\0';
			n = ifstrnum(buf);
			if (n >= 0)
				input->fSigmaY = eatof(buf);
			else {
				fprintf(stderr, "Sigma input error\n");
				exit(-1);
			}
			break;

		case 'd':	
			if (ifstrnum(arg) >= 0 )
				input->dis = eatof(arg);
			else {
				fprintf(stderr, "Distance from specimen to detector specification error, will use 0\n");
				input->dis = 0;
			}
			break;

		case 'o':	
			cp = strrchr(arg, '.');
			if (cp == NULL) {
				fprintf(stderr, "Output file specification error, should be a mat file\n");
				exit(-1);
			}
			cp++;
			if (strcmp("mat", cp)) {
				fprintf(stderr, "Output file specification error, should be a mat file\n");
				exit(-1);
			}
			strcpy(input->output_fn, arg);
			break;

		case 615:	
			if (ifstrnum(arg) >= 0)
				input->pixel_size = eatof(arg);
			else {
				fprintf(stderr, "Pixel size specification error, will use 0.00000999999999\n");
				input->pixel_size = 0.00000999999999;
			}
			break;

		case 'l':	
			strcpy(input->log_fn, arg);
			input->flag_spec |= FLAG_SPEC_LOG;
			break;

		case 'p':	
			if (ifstrnum(arg) >= 0 ) {
				input->pos = eatof(arg);
				input->flag_spec |= FLAG_SPEC_POS;
			}
			else {
				strcpy(input->pos_fn, arg);
				input->flag_spec |= FLAG_SPEC_POSF;
			}
			break;

		case 616:	
			input->flag_spec &= ~FLAG_SPEC_ATT;
			break;

		case 617:	
			input->flag_spec &= ~FLAG_SPEC_CRY;
			break;

		case 'r':
			strcpy(input->rc_fn, arg);
			input->flag_spec |= FLAG_SPEC_RC;
			break;
		case 620:
			input->flag_spec |= FLAG_SPEC_POSA;
			break;
		case 'm':
			input->ppppf = eatof(arg);
			break;
		case 'i':
			input->interval = eatof(arg);
			break;
		case 'v':
			input->verbose_level = eatof(arg);
			break;
		case ARGP_KEY_ARG:
			(input->input_n)++;
			input->input_pt = FilenamePush(input->input_pt, arg);
			break;
		case ARGP_KEY_END:
			break;
		default:	
			break;
	}			
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  readpho
 *  Description:  read photon file, returns numbers of the photons.
 * =====================================================================================
 */
double *ReadPho(size_t *number, const char *input_fn) {
	FILE *fp = fopen(input_fn, "r");
	double *pho;
	if (fp) {
		fseek(fp, 0, SEEK_END);
		*number = ftell(fp);
		rewind(fp);
		pho = malloc(*number);
		fread(pho, *number, 1, fp);
		*number = *number/40;
		fclose(fp);
		return pho;
	}
	else {
		fprintf(stderr, "Photon file %s open failed\n", input_fn);
		return NULL;
	}
}		/* -----  end of function readpho  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  FilenamePush
 *  Description:  Push element into filename stack
 * =====================================================================================
 */
void *FilenamePush(filename_element *pt, const char *filename) {
	filename_element *return_pointer = malloc(sizeof(filename_element));
	strcpy(return_pointer->filename, filename);
	return_pointer->next = pt;
	fflush(stdout);
	return return_pointer;
}		/* -----  end of function FilenamePush  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  FilenamePoP
 *  Description:  Pop element from filename stack
 * =====================================================================================
 */
void *FilenamePop(filename_element *pt, char *filename) {
	void *return_pointer;
	strcpy(filename, pt->filename);
	return_pointer = pt->next;
	free(pt);
	fflush(stdout);
	return return_pointer;
}		/* -----  end of function FilenamePoP  ----- */



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getpos
 *  Description:  get the current postion on the rocking curve based
 * =====================================================================================
 */
uint16_t getpos(uint32_t n, double *photon_number, uint32_t number_of_positions) {
	static double sum;
	uint16_t i;
	for ( i = 0, sum=0; i < number_of_positions; i++ ) {
		sum += photon_number[i];
		if (n<=sum)
			break;
	}
	return i;
}		/* -----  end of function getpos  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GaussianPDF
 *  Description:  disperse photon on detector. We assume outside 6 sigma, the dispersion could be emitted
 * =====================================================================================
 */
uint16_t GaussianPDF(double *image, uint64_t index, uint32_t iX, uint32_t iY, double fSigmaX, double fSigmaY, double fPixelSize, double fIntensity) {
	uint32_t iHalfWindowX, iHalfWindowY;
	int64_t i, j;
	iHalfWindowX = ceil(fSigmaX/fPixelSize*6), iHalfWindowY = ceil(fSigmaY/fPixelSize*6);
	for (j = (int64_t)floor(index/iX)-iHalfWindowY; j <= (int64_t)floor(index/iX)+iHalfWindowY; j++) {
		for (i = (int64_t)(index%iX)-iHalfWindowX; i <= (int64_t)(index%iY)+iHalfWindowX; i++) {
			if (i >= 0 && i < iX && j >=0 && j <iY) {
				image[j*iX+i] = image[j*iX+i]+ fIntensity*exp(-pow(((double)i-index%iX)*fPixelSize, 2)/2/pow(fSigmaX, 2))*exp(-pow(((double)j-floor((double)index/iX))*fPixelSize, 2)/2/pow(fSigmaY, 2));
			}
		}
	}
	
	return EXIT_SUCCESS;
}		/* -----  end of function GaussianPDF  ----- */
