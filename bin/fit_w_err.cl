#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void spec_gen ( 
					__global float* res_model,
					__global float* time, 
					__global float* wavelengths,
					
					__global float* mes_spec,
					__global float* mes_spec_err,
					__global float* mes_spec_mask,

					__global float* model,
					__global float* result_no_vel,
					__global float* factor1,
					__global float* factor2,

					__const int nspecsteps, 
					__const int ntimesteps,

					__const float dust_tau_v,
					__const float dust_mu,
					__const float sfr_tau,
					__const float age,
					__const float metall,
					__const int modelno
					)
				
{
	const int wave = get_global_id(0);

	int modelsize=nspecsteps*ntimesteps;

	int i,j,place,place1,place2;
	float exponent,dust_tau;

	//interpolating
	//weigths
	float wd,wu,delta;
	float model_metal[12]={0.0001,0.0004,0.004,0.008,0.02,0.05,0.0001,0.0004,0.004,0.008,0.02,0.05};

	delta=log( model_metal[modelno+1] / model_metal[modelno] );
	wd=log(model_metal[modelno+1]/metall) / delta;
	wu=log(metall/model_metal[modelno]) / delta;

	//linear interpol in log(z)
	for(i=0; i<ntimesteps ;i++)
	{
		place=wave+i*nspecsteps;
		place1=modelno*modelsize+place;
		place2=place1+modelsize;
		model[place]= wd*res_model[place1] + wu*res_model[place2] ;
	}
	

/*convol*/

	//part of dust exponent is constant for a wavel
	exponent=pow(wavelengths[wave]/5500.0,-0.7);

	float temp=0;
	temp+= time[0] * model[wave] * exp((time[0]-age)/sfr_tau);
	for(i=1; ( time[i] <= 10e7 ) && ( time[i] <= age ) ;i++)
	{
		temp+= (time[i]-time[i-1]) * model[ i*nspecsteps + wave] * exp((time[i]-age)/sfr_tau);
	}

	float temp1=0;
	for(; ( time[i] < age ) && ( (i+1) <ntimesteps ) ;i++)
	{
		temp1+= (time[i]-time[i-1]) * model[ i*nspecsteps + wave] * exp((time[i]-age)/sfr_tau);
	}

	//the last step foor smoothness with linear interpol
//old one
//	temp1+= ((age-time[i-1]) * ( (time[i]-age) * model[(i-1)*nspecsteps + wave] + (age-time[i-1]) * model[i*nspecsteps + wave] )) / (time[i]-time[i-1]) ;

//test
	temp1+= (age-time[i-1]) *  model[i*nspecsteps + wave]  ;

	result_no_vel[wave]= temp*exp(-exponent*dust_tau_v) + temp1*exp(-exponent*dust_tau_v*dust_mu);
	
	return;
}


__kernel void mask_veloc_disp ( 
					__global float* wavelengths,
					
					__global float* mes_spec,
					__global float* mes_spec_err,
					__global float* mes_spec_mask,

					__global float* result_no_vel,
					__global float* result,
					__global float* factor1,
					__global float* factor2,

					__const int nspecsteps, 

					__const float vsig
					)
				
{
	const int wave = get_global_id(0);
	//current wavelength
	float curr_waveleng=wavelengths[wave];

	//width of gaussian in wavelengths
	float sig_lam = curr_waveleng * vsig;

	//result
	float loc_result=0;

	//weigth, now it is not normalized
	float weigth;
	//sum of weigths for normalizing
	float sumweigth=0;

	int i;

	//i do not do it at the very ends
	if (wave>5  && wave <nspecsteps-5){
		for (i=-5;i<=5;i++){
			weigth= exp(-(wavelengths[wave+i]- curr_waveleng) * (wavelengths[wave+i]- curr_waveleng) / (2 * sig_lam * sig_lam));
			sumweigth+=weigth;
			loc_result+= weigth * result_no_vel[wave+i];}
		//normalizing
		result[wave]=loc_result/sumweigth;}

	else 
		result[wave]=result_no_vel[wave];


//counting the factor with err
	if ( mes_spec_mask[wave] == 0 && mes_spec_err[wave]!=0  ) //
	{
		factor1[wave]=  result_no_vel[wave]; 
		//factor1[wave]= (mes_spec[wave] * result[wave])  / (mes_spec_err[wave] * mes_spec_err[wave] );
		factor2[wave]= (result[wave] * result[wave] ); // / (mes_spec_err[wave] * mes_spec_err[wave]);
	}
	else 
	{
		//problems in mes
		factor1[wave]=0;
		factor2[wave]=0;
	}

	return;
}

__kernel void chi_calculation ( 
					__global float* mes_spec,
					__global float* mes_spec_err,
					__global float* mes_spec_mask,

					__global float* result,
					__global float* chi,
					__const float factor,

					__global float* wavelengths
					)			
{
	const int wave = get_global_id(0);

//modifying data due to factor
	result[wave]= (result[wave] * factor);
	
//counting chi[]
	if (  mes_spec_mask[wave] == 0 && mes_spec_err[wave]!=0 ) //
		chi[wave]= ( mes_spec[wave] - result[wave] )*( mes_spec[wave] - result[wave] )/(2* mes_spec_err[wave]*mes_spec_err[wave]);
	else 
		chi[wave]=0;

	return;
}	
