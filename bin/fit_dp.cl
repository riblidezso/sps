//#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void fit_dp_a ( 
					__global double* model_without_dust,
					__global double* time, 
					__global double* wavelengths,
					__global double* sample_spec,

					__global double* model,
					__global double* result,
					__global double* factor1,
					__global double* factor2,

					__const int nspecsteps, 
					__const int ntimesteps,

					__const double dust_tau_v,
					__const double dust_mu,
					__const double sfr_tau,
					__const double age,
					__const double metall,
					__const int modelno
					)
				
{
	const int wave = get_global_id(0);
	int modelsize=nspecsteps*ntimesteps;

	int i,j,place,place1,place2;
	double exponent,dust_tau;

	//interpolating
	//weigths
	double wd,wu,delta;
	double model_metal[6]={0.0001,0.0004,0.004,0.008,0.02,0.05};
	delta=log( model_metal[modelno+1] / model_metal[modelno] );
	wd=log(model_metal[modelno+1]/metall) / delta;
	wu=log(metall/model_metal[modelno]) / delta;

	//linear interpol in log(z)
	for(i=0; i<ntimesteps ;i++)
	{
		place=wave+i*nspecsteps;
		place1=modelno*modelsize+place;
		place2=place1+modelsize;
		model[place]= wd*model_without_dust[place1] + wu*model_without_dust[place2] ;
	}

	//dust modification
	
	//exponent is constant for a wavel
	exponent=pow(wavelengths[wave]/5500,-0.7);

	for(j=0;j<ntimesteps;j++)
	{
		//dust_tau depends on the age of the stellar pop
		if(time[j]<= 10000000)
			dust_tau= -dust_tau_v;
		else
			dust_tau= -dust_mu*dust_tau_v;

		place= wave + j*nspecsteps;
		model[place]= exp(exponent*dust_tau) * model_without_dust[place];
	}


/*convol*/
	double temp=0;

	for(i=1; ( time[i] <= age ) && ( i<ntimesteps ) ;i++)
	{
		place=i*nspecsteps + wave;
		temp+= (time[i]-time[i-1]) * (model[place] * exp((time[i]-age)/sfr_tau));
	}
	temp=temp/sfr_tau;
	result[wave]=temp;
	

//counting the factor
	//no error yet

	factor1[wave]=sample_spec[wave] * temp;
	factor2[wave]=temp*temp;
	
	return;
}


__kernel void fit_dp_b ( 
					__global double* sample_spec,
					__global double* result,
					__global double* chi,
					__const double factor
					)			
{
	const int wave = get_global_id(0);
//modifying data due to factor
	result[wave]= (result[wave] * factor);

//counting chi[]
	//no error yet
	chi[wave]= ( sample_spec[wave] - result[wave] )*( sample_spec[wave] - result[wave] ) / sample_spec[wave];

	return;
}