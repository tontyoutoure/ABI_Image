#include "image_dei_pl.h"

const char *argp_program_bug_address = "tontyoutoure@gmail.com";
const char *argp_program_version = "version 2.0";
/*static int poisson_noise(double *p, long size, long mean);*/
static inline int compare(const void *a, const void *b);
void *makeimage_1(void *ARG);

int main(int argc, char *argv[]){
	FILE *fout = NULL;
	pthread_t tid[NoT];
	pthread_mutex_t mutexi = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mutexo = PTHREAD_MUTEX_INITIALIZER;
	char input_fn[FILENAME_MAX], output_fn_mid[FILENAME_MAX];
	uint32_t i, j, k, m, n, number_of_positions = 0, LRC, X, Y;
	double fwhm, *image, *image_out, *pho, *RC[2] = {NULL, NULL}, *pos = NULL, *pos_out, *inte = NULL, sum3, *photon_number = NULL;
	time_t timer;
	size_t dims[3], number_of_photons;
	uint16_t file_number = 0;
	struct argp_option options[] = {
		{"size", 's', "XSize_YSize", 0, "Specialize specimen size, default value is 0.01m"},
		{"distance", 'd', "Distance", 0, "Specialize distance from specimen to detector, default value is 0"},
		{"output", 'o', "OutFileName", 0, "Specialize the output file name, should not be omitted"},
		{"pixel", 615, "PixelSize", 0, "Specialize pixel size, default value is 0.000009999999999"},
		{"log", 'l', "LogFileName", 0, "Specialize log file, optional"},
		{"position", 'p', "[Position][PositionFileNames]", 0, "Specialize position, or position file"},
		{"no-attenuation", 616, 0, 0, "No attenuation effect will be accounted"},
		{"no-crystal", 617, 0, 0, "No Analyser crystal will be used"},
		{"rocking_curve", 'r', "RCFileName", 0, "Specialize the double crystal rocking curve file"},
		{"pos-abs", 620, 0, 0, "Using seconds as the unit of the position instead of fwhm"},
		{"multiplier", 'm', "Multiplier", 0, "How much photons a file include for a single pixel"},
		{"interval", 'i', "Numbers", 0, "Output for every what numbers of files"},
		{"verbose level", 'v', "Level", 0, "Set the verbose level, default value is 0(silenced)"},
		{"sigma", 'g', "SigmaX_SigmaY", 0, "Sigma of point dispersion function on detector, default value is 0_0"},
		{0}
	};
	makeimage_t ARG;
	struct argp argp = {options, ParseOpt, "[PhotonFileNames]", "A program transform photons into images.\v"};
	struct argp_input input = {0.01, 0.01, 0, 0.0000099999999, 0, FLAG_SPEC_CRY|FLAG_SPEC_ATT, 0, NULL, {0}, {0}, {0}, {0}, 1, 0, 0, 0, 0};
	mat_t *mat;
	matvar_t *matvar_output, *matvar_pos = 0, *matvar_RC = 0;
	argp_parse(&argp, argc, argv, 0, 0, &input);

	fout = fopen(input.output_fn, "w");
	if(!fout) {
		fprintf(stderr, "Output file cannot be created\n");
		exit(EXIT_FAILURE);
	}
	fclose(fout);

	if (input.a <=0 || input.b <= 0) {
		fprintf(stderr, "Specimen size specification error\n");
		exit(EXIT_FAILURE);
	}

	if (input.pixel_size <= 0) {
		fprintf(stderr, "Pixel size specification error\n");
		exit(EXIT_FAILURE);
	}

	if (input.flag_spec & FLAG_SPEC_CRY) {
		/*Input Rocking curve from mat file*/
		mat = Mat_Open(input.rc_fn, MAT_ACC_RDONLY);
		matvar_RC = Mat_VarRead(mat, "RC");
		Mat_Close(mat);
		if (!matvar_RC) {
			fprintf(stderr, "Rocking curve file error\n");
			exit(EXIT_FAILURE);
		}
		RC[0] = matvar_RC->data;
		RC[1] = (double *)matvar_RC->data + *(matvar_RC->dims);
		LRC = *(matvar_RC->dims);

		if (input.flag_spec & FLAG_SPEC_POSF) {
			mat = Mat_Open(input.pos_fn, MAT_ACC_RDONLY);
			matvar_pos = Mat_VarRead(mat, "pos");
			Mat_Close(mat);
			if(!matvar_pos) {
				fprintf(stderr, "Position file specification error\n");
				exit(EXIT_FAILURE);
			}
			pos = matvar_pos->data; 
			number_of_positions = *(matvar_pos->dims); 
			inte = matvar_pos->data+sizeof(double)*number_of_positions; 
		} 

		else if (input.flag_spec & FLAG_SPEC_POS) {
			pos = &(input.pos);
			number_of_positions = 1;
		}

		else {
			fprintf(stderr, "Position on the rocking curve not specialized\n");
			exit(EXIT_FAILURE);
		}

		for (i = 0, m = 0; i < LRC; i++) {
			if(RC[1][i] > RC[1][m])
				m = i;
		}
		for(i=0,j=0,k=0;i<LRC;i++){
			if(RC[1][i]>RC[1][m]/2){
				if(j==0)
					j=i;
				k=i;
			}
		}
		fwhm=1;//find the fwhm
		ARG.pos = pos;
		ARG.number_of_positions = number_of_positions;
		ARG.RC = RC;
		ARG.LRC = LRC;
		ARG.fwhm = fwhm;
	}
	ARG.fSigmaX = input.fSigmaX;
	ARG.fSigmaY = input.fSigmaY;
	ARG.pixel_size = input.pixel_size;

	if (input.input_n == 0) {
		fprintf(stderr, "No input files\n");
		exit(EXIT_FAILURE);
	}
	time(&timer);

	X = input.a/input.pixel_size, Y = input.b/input.pixel_size;	
	image=(double *)malloc(X*Y*(2+number_of_positions)*sizeof(double));
	for(i=0;i<X*Y*(2+number_of_positions);i++)
		image[i]=0;
	
	/*begin making file with each pho file*/
	pho = NULL;// OK, I surrendered

	do {
		file_number++;
		if(input.verbose_level > 0)
			printf("%d\n", file_number);
		input.input_pt = FilenamePop(input.input_pt, input_fn);	
		pho = ReadPho(&number_of_photons, input_fn);
		
		/*thread arguments initialization*/
		ARG.mutexpi = &mutexi;
		ARG.mutexpo = &mutexo;
		ARG.pho = pho;
		for (i = 0, sum3 = 0; i < number_of_positions; i++) {
			sum3 += inte[i];
		}
		photon_number = malloc(sizeof(double)*number_of_positions);
		for (i = 0; i < number_of_positions; i++) {
			photon_number[i] = inte[i]/sum3*number_of_photons;
		}
		ARG.photon_number= photon_number;
		ARG.image = image;
		n = 0;
		ARG.pn = &n;
		ARG.a = input.a;
		ARG.b = input.b;
		ARG.number_of_photons = number_of_photons;
		ARG.X = X;
		ARG.Y = Y;

		if (input.flag_spec & FLAG_SPEC_ATT)
			ARG.att = 1;
		else
			ARG.att = 0;

		if (input.flag_spec & FLAG_SPEC_CRY)
			ARG.cry = 1;
		else
			ARG.cry = 0;

		ARG.dis = input.dis;

		if (input.flag_spec & FLAG_SPEC_POSA)
			ARG.posa = 1;
		else
			ARG.posa = 0;

		for (i = 0; i < NoT; i++)
			pthread_create(tid+i, NULL, makeimage_1, &ARG);
		for (i = 0; i < NoT; i++) 
			pthread_join(tid[i], NULL); 

		free(pho);
		if(input.interval && !(file_number%input.interval)) {
			sprintf(output_fn_mid, "%d_%s", file_number*input.ppppf, input.output_fn);
			mat = Mat_CreateVer(output_fn_mid, NULL, MAT_FT_MAT5);
			if(input.flag_spec & FLAG_SPEC_CRY) {
				dims[0] = number_of_positions, dims[1] = 1, dims[2] = 0;
				pos_out = malloc(number_of_positions*sizeof(double));
				memcpy(pos_out, pos, number_of_positions*sizeof(double));
				matvar_pos = Mat_VarCreate("pos", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, pos_out, 0);
				Mat_VarWrite(mat, matvar_pos, MAT_COMPRESSION_NONE);
				Mat_VarFree(matvar_pos);

				Mat_VarWrite(mat, matvar_RC, MAT_COMPRESSION_NONE);

				dims[0] = 1, dims[1] = 1, dims[2] = 0;
				matvar_output = Mat_VarCreate("fwhm", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, &fwhm, 0);
				Mat_VarWrite(mat, matvar_output, MAT_COMPRESSION_NONE);
				Mat_VarFree(matvar_output);
				
				image_out = malloc(X*Y*number_of_positions*sizeof(double));
				for (i = 0; i < number_of_positions; i++) {
					for (j = 0; j < Y; j++) {
						for (k = 0; k < X; k++) {
					//		image_out[k*Y*number_of_positions+j*number_of_positions+i] = image[i*X*Y+j*X+k];
							image_out[i*X*Y+k*Y+j] = image[i*X*Y+j*X+k];
						}
					}
				}
				dims[0] = Y;
				dims[1] = X;
				dims[2] = number_of_positions;
				matvar_output = Mat_VarCreate("image", MAT_C_DOUBLE, MAT_T_DOUBLE, 3, dims, image_out, 0);
				Mat_VarWrite(mat, matvar_output, MAT_COMPRESSION_NONE);
				Mat_VarFree(matvar_output);
				free(image_out);
			}

			dims[0] = 1, dims[1] = 1, dims[2] = 0;
			matvar_output = Mat_VarCreate("dis", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, &(input.dis), 0);
			Mat_VarWrite(mat, matvar_output, MAT_COMPRESSION_NONE);
			Mat_VarFree(matvar_output);

			image_out = malloc(X*Y*sizeof(double));
			for (i = 0; i < X; i++) 
				for (j = 0; j < Y; j++)
					image_out[i*Y+j] = image[i+j*X+number_of_positions*X*Y];
			dims[0] = Y, dims[1] = X, dims[2] = 0;	
			matvar_output = Mat_VarCreate("nocry", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, image_out, 0);
			Mat_VarWrite(mat, matvar_output, MAT_COMPRESSION_NONE);
			Mat_VarFree(matvar_output);
			free(image_out);

			image_out = malloc(X*Y*sizeof(double));
			for (i = 0; i < X; i++) 
				for (j = 0; j < Y; j++)
					image_out[i*Y+j] = image[i+j*X+(number_of_positions+1)*X*Y];
			dims[0] = Y, dims[1] = X, dims[2] = 0;	
			matvar_output = Mat_VarCreate("count", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, image_out, 0);
			Mat_VarWrite(mat, matvar_output, MAT_COMPRESSION_NONE);
			Mat_VarFree(matvar_output);
			free(image_out);
			Mat_Close(mat);
		}
	}
	while(input.input_pt);

			//begin output
		
	mat = Mat_CreateVer(input.output_fn, NULL, MAT_FT_MAT5);
	if(input.flag_spec & FLAG_SPEC_CRY) {

		dims[0] = number_of_positions, dims[1] = 1, dims[2] = 0;
		matvar_pos = Mat_VarCreate("pos", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, pos, 0);
		Mat_VarWrite(mat, matvar_pos, MAT_COMPRESSION_NONE);
		Mat_VarFree(matvar_pos);

		Mat_VarWrite(mat, matvar_RC, MAT_COMPRESSION_NONE);
		Mat_VarFree(matvar_RC);

		dims[0] = 1, dims[1] = 1, dims[2] = 0;
		matvar_output = Mat_VarCreate("fwhm", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, &fwhm, 0);
		Mat_VarWrite(mat, matvar_output, MAT_COMPRESSION_NONE);
		Mat_VarFree(matvar_output);
		
		image_out = malloc(X*Y*number_of_positions*sizeof(double));
		for (i = 0; i < number_of_positions; i++) {
			for (j = 0; j < Y; j++) {
				for (k = 0; k < X; k++) {
			//		image_out[k*Y*number_of_positions+j*number_of_positions+i] = image[i*X*Y+j*X+k];
					image_out[i*X*Y+k*Y+j] = image[i*X*Y+j*X+k];
				}
			}
		}
		dims[0] = Y;
		dims[1] = X;
		dims[2] = number_of_positions;
		matvar_output = Mat_VarCreate("image", MAT_C_DOUBLE, MAT_T_DOUBLE, 3, dims, image_out, 0);
		Mat_VarWrite(mat, matvar_output, MAT_COMPRESSION_NONE);
		Mat_VarFree(matvar_output);
		free(image_out);
	}

	dims[0] = 1, dims[1] = 1, dims[2] = 0;
	matvar_output = Mat_VarCreate("dis", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, &(input.dis), 0);
	Mat_VarWrite(mat, matvar_output, MAT_COMPRESSION_NONE);
	Mat_VarFree(matvar_output);

	image_out = malloc(X*Y*sizeof(double));
	for (i = 0; i < X; i++) 
		for (j = 0; j < Y; j++)
			image_out[i*Y+j] = image[i+j*X+number_of_positions*X*Y];
	dims[0] = Y, dims[1] = X, dims[2] = 0;	
	matvar_output = Mat_VarCreate("nocry", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, image_out, 0);
	Mat_VarWrite(mat, matvar_output, MAT_COMPRESSION_NONE);
	Mat_VarFree(matvar_output);
	free(image_out);

	image_out = malloc(X*Y*sizeof(double));
	for (i = 0; i < X; i++) 
		for (j = 0; j < Y; j++)
			image_out[i*Y+j] = image[i+j*X+(number_of_positions+1)*X*Y];
	dims[0] = Y, dims[1] = X, dims[2] = 0;	
	matvar_output = Mat_VarCreate("count", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, image_out, 0);
	Mat_VarWrite(mat, matvar_output, MAT_COMPRESSION_NONE);
	Mat_VarFree(matvar_output);
	free(image_out);
	Mat_Close(mat);



	free(image);
	return 0;
}

/*
static int poisson_noise(double *p, long size, long mean) {
	gsl_rng * gsl_rp = gsl_rng_alloc(gsl_rng_mt19937);
	long i;
	
	gsl_rng_set(gsl_rp, time(NULL));
	if (mean > 0) {
		for (i=0; i<size; i++)
			p[i]*=((double)gsl_ran_poisson(gsl_rp, mean)/mean);
	}
	else {
		for (i=0; i<size; i++)
			p[i]*=((double)gsl_ran_poisson(gsl_rp, floor(p[i]))/floor(p[i]));
	}
	gsl_rng_free(gsl_rp);
	return 0;

}*/
static inline int compare(const void *a, const void *b){
	return (*(double *)a-*(double *)b>0)-(*(double *)a-*(double *)b<0);
}
/*inline double grey(double x){
	double z;
	if(x-floor(x))
		z=(x-floor(x))*RC[1][(int)floor(x)]+(ceil(x)-x)*RC[1][(int)ceil(x)];
	else
		z=RC[1][(int)x];
		
	return(z);
}
*/
void *makeimage_1(void *ARG) {
	double thetay, index, intensity, dis;
	uint64_t x, y, i;
	uint32_t n;
	uint16_t posi;
	double *IM, *nocry, *count;
	pthread_mutex_t *mutexpi = ((makeimage_t *)ARG) -> mutexpi;
	pthread_mutex_t *mutexpo = ((makeimage_t *)ARG) -> mutexpo;

	gsl_interp *itp;
	gsl_interp_accel *acc = gsl_interp_accel_alloc();


	double a = ((makeimage_t *)ARG) -> a;
	double b = ((makeimage_t *)ARG) -> b;
	int X = ((makeimage_t *)ARG) -> X;
	int Y = ((makeimage_t *)ARG) -> Y;
	int att = ((makeimage_t *)ARG) -> att;
	int	cry = ((makeimage_t *)ARG) -> cry;
	int posa = ((makeimage_t *)ARG) -> posa;
	double * pho = ((makeimage_t *)ARG) -> pho;
	double *image = ((makeimage_t *)ARG) -> image;
	double *pos = ((makeimage_t *)ARG) -> pos;
	double **RC = ((makeimage_t *)ARG) -> RC;
	uint32_t number_of_positions = ((makeimage_t *)ARG) -> number_of_positions;
	long LRC = ((makeimage_t *)ARG) -> LRC;
	double fwhm = ((makeimage_t *)ARG) -> fwhm;
	dis = ((makeimage_t *)ARG) -> dis;
	double *photon_number = ((makeimage_t *)ARG) -> photon_number;
	double pixel_size = ((makeimage_t *)ARG) -> pixel_size;
	double fSigmaX = ((makeimage_t *)ARG) -> fSigmaX;
	double fSigmaY = ((makeimage_t *)ARG) -> fSigmaY;
	if(cry) {
		itp = gsl_interp_alloc(gsl_interp_linear, LRC);
		gsl_interp_init(itp, RC[0], RC[1], LRC);
		IM = malloc(X*Y*number_of_positions*sizeof(double));
		for (i = 0; i < X*Y*number_of_positions;i++)
			IM[i] = 0;
	}
	count = malloc(X*Y*sizeof(double));
	nocry = malloc(X*Y*sizeof(double));
	for (i = 0; i < X*Y; i++)
		count[i] = 0, nocry[i] = 0;
	while (1) {
		pthread_mutex_lock(mutexpi);
		n = (*(((makeimage_t *)ARG) -> pn))++;
		pthread_mutex_unlock(mutexpi);
			

		if (n >= ((makeimage_t *)ARG) -> number_of_photons)
			break;


		if (dis)
			x = floor((pho[n*5+0]+pho[n*5+2]*dis)/a*X), y = floor(Y-(pho[n*5+1]+pho[n*5+3]*dis)/b*Y);
		else {
			x = floor(pho[n*5+0]/a*X), y = floor(Y-pho[n*5+1]/b*Y);
			}
		if(x < 0 || x >= X || y < 0 || y >= Y)
			continue;
			
		if(cry) {
			posi = getpos(n, photon_number, number_of_positions);
			thetay = asin(pho[n*5+3]);
			if (posa) 
				index = thetay-pos[posi];
			else 
				index = thetay-pos[posi]*fwhm;

			if (index > RC[0][0] && index < RC[0][LRC-1]) {
				intensity = gsl_interp_eval(itp, RC[0], RC[1], index, acc) * (att*pho[n*5+4]+!att);
				GaussianPDF(IM+posi*X*Y, y*X+x, X, Y, fSigmaX, fSigmaY, pixel_size, intensity/photon_number[posi]);
			}
		}
		count[y*X+x] += 1;
		GaussianPDF(nocry, y*X+x, X, Y, fSigmaX, fSigmaY, pixel_size, pho[n*5+4]);
	}

	pthread_mutex_lock(mutexpo);
	if(cry) {
		for(i = 0; i < X*Y*number_of_positions; i++)
			image[i] += IM[i];
		gsl_interp_free(itp);
		free(IM);
	}

	for(i = 0; i < X*Y; i++) {
		image[i+X*Y*number_of_positions] += nocry[i];
		image[i+X*Y*(number_of_positions+1)] += count[i];
	}
	pthread_mutex_unlock(mutexpo);
	free(nocry);
	free(count);
	gsl_interp_accel_free(acc);

	return 0;
}

/*

void *na_square(void *input){
	long i,j,k,l;
	long *pos=(long *)input;
	long p=0;
	double sum,sum2;
	
		
	while(1){
	
		pthread_mutex_lock(&mutex);
		p=++(*pos);
		i=p/(X-2*para1),j=p%(X-2*para1);			
		pthread_mutex_unlock(&mutex);
		if(p>=(X-2*para1)*(Y-2*para1))
			break;
		for(k=0,sum=0;k<=2*para1;k++){
			for(l=0;l<=2*para1;l++){
				sum+=cache[(i+k)*X+j+l];
								
			}
		}
		
		image[i*(X-2*para1)+j]=sum/pow(para1*2+1,2);
	}
	return NULL;
}
int gauss(long x, long y, double win, fftw_complex *p){
	long i,j,k;
	double ret=(double)1/2/win/win;

	for(i=0;i<x*y;i++){
		j=i%x-x/2,k=i/x-y/2;
		p[i][0]=exp(-ret*j*j-ret*k*k);
		p[i][1]=p[i][0];
	}

	return 0;
}

int blpf(long x,long y,double win, double n, fftw_complex *p){
	long i,j,k;
	for(i=0;i<x*y;i++){
		j=i%x-x/2,k=i/x-y/2;
		p[i][0]=1/(1+pow(sqrt(j*j+k*k)/win,2*n));
	}
}*/
