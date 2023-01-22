#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <x86intrin.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

inline auto start_tsc(){
	_mm_lfence();
	auto tsc = __rdtsc();
	_mm_lfence();
	return tsc;
}

inline auto stop_tsc(){
	unsigned int aux;
	auto tsc = __rdtscp(&aux);
	_mm_lfence();
	return tsc;
}

extern "C" void *calcSum_i(void *vp){
	double * info = (double*)vp;
	double low_bound = info[0];
	double interval = info[1];
	int n_i = (int)info[2];
	unsigned int seed = (unsigned int) info[3];
	
	double sum = 0;
	for(int j = 0; j < n_i; j++){
		double rndm = double(rand_r(&seed)) / RAND_MAX;
		double rand_x = low_bound + rndm*interval;
		double y = sin(rand_x);
		if (rand_x == 0){
			y /= 1;
		}
		else{
			y /= rand_x;
		}
		sum += y;
	}
	
	info[4] = sum;
	pthread_exit((void*)info);
}
 
double integrate(double a, double b, long long n, int t, int timeMode, double freq){
	auto begin = start_tsc(); //start time calculation

	int sign = 1; //init positive
	if (a > b) { //a must be less than b
		sign = -1;
		double tmp = a;
		a = b;
		b = tmp;
	}
	
	double interval = b-a;
	int q = n / t;
	int remainder = n - q*t;
	pthread_t tids[20]; //should scale well upto 10 cores
	for (int i = 0; i < t; i++){
		int r = (remainder > 0)? 1 : 0;
		remainder--;
		int n_i = q + r;
		double *args = new double[5];
		args[0]=a + (interval/t)*i; //low_bound
		args[1]=interval/t; //interval
		args[2]=n_i; //num of samples for thread i
		args[3]=(i+1)*20; //seed for thread i
		pthread_t tid; 
		pthread_create(&tid, NULL, calcSum_i, (void*) args);
		tids[i] = tid;
	}
	
	double sum = 0;
	for (int i = 0; i < t; i++){
		void *vp;
		pthread_join(tids[i], &vp);
		double *info = (double *)vp;
		sum += info[4]; //sum from each thread i
		delete info;
	}
	sum *= sign * interval / n ; 
	auto end = stop_tsc(); //end time calculation

	double time = (end-begin)/ freq;
	if (timeMode) {
		return time;
	}
	else{
		return sum;
	}
}

int main(int argc, char **argv){
	if (argc != 5)
		return -1;
	double a = atof(argv[1]); 
	double b = atof(argv[2]);
	if (a == b){ // integral from a to a = 0
		printf("0\n");
		return 0;
	}
	int t = atoi(argv[4]);

	long long n = 0;
	int timeMode = 0;
	double timeLim = 0;
	int n_len = strlen(argv[3]);
	if (!isdigit(argv[3][n_len-1])){ //n arg ends in s
		printf("extra credit mode initiating...\n");
		timeMode = 1;
		argv[3][n_len-1] = '\0';
		timeLim = atoi(argv[3]);
	}
	else{
		n = atoi(argv[3]);
	}

	//To calculate clock frequency, (end cycle - start cycle) / 1 sec
	auto zero = start_tsc();
	sleep(1);
	auto one = stop_tsc();
	double freq = one - zero;
	
	if (timeMode){ //time limit mode
		//find greatest n that will complete below time limit
		long long low = 1000; 
		while (integrate(a, b, 2*low, t, timeMode, freq) < timeLim){
			low *= 2;
		}
		long long high = 2*low;
		printf("...tuning, pls hold...\n");
		while (low < high){
			long long mid = (low + high)/2;
			double midTime = integrate(a, b, mid, t, timeMode, freq);
			if (midTime <= timeLim)
				low = mid; //low always completes in at most timeLim seconds
			else
				high = mid - 1;
		}
		n = low; 
	}

	printf("%.16f\n", integrate(a, b, n, t, 0, freq)); //regular execution mode
}
