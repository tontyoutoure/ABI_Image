/* function.c */
int ParseOpt(int32_t key, char *arg, struct argp_state *state);
double *ReadPho(size_t *number, const char *input_fn);
void *FilenamePush(filename_element *pt, const char *filename);
void *FilenamePop(filename_element *pt, char *filename);
uint16_t getpos(uint32_t n, double *photon_number, uint32_t number_of_positions);
/* numcheck.c */
int16_t ifstrnum(const char *str);
double eatof(const char *str);
