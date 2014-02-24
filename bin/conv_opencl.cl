#pragma OPENCL EXTENSION cl_khr_fp64 : enable
__kernel void conv_opencl ( __global double* rawdata ,
					__global double* time, 
					__global double* output , 
					__global double* age, 
					__const int nspecsteps, 
					__const int ntimesteps,  
					__const int offset,  
					__const int nfillingdata,  
					__const double tau, 
					__const int n_ages)

{
	const int idx = get_global_id(0);
	int i,k;
	double temp;

	if ( idx < nspecsteps )
	{
		for(k=0;k<n_ages;k++)
		{
			temp=0;
			for(i=1; ( time[i] <= age[k] ) && ( i<ntimesteps ) ;i++)
			{
				temp+= (time[i]-time[i-1]) * ( rawdata[offset+idx+i*(nspecsteps+nfillingdata+2)] * exp((time[i]-age[k])/tau) + rawdata[offset+idx+(i-1)*(nspecsteps+nfillingdata+2)] * exp((time[i-1]-age[k])/tau) );
			}
			output[idx+k*nspecsteps]=temp/(2*tau);
		} 
	}
}

