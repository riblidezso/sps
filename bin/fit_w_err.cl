#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void fit_w_err_1 ( 
					__global double* res_model,
					__global double* time, 
					__global double* wavelengths,
					
					__global double* mes_spec,
					__global double* mes_spec_err,
					__global double* mes_spec_mask,

					__global double* model,
					__global double* result_no_vel,
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
	double model_metal[12]={0.0001,0.0004,0.004,0.008,0.02,0.05,0.0001,0.0004,0.004,0.008,0.02,0.05};

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
	exponent=pow(wavelengths[wave]/5500,-0.7);

	double temp=0;
	temp+= time[0] * model[wave] * exp((time[0]-age)/sfr_tau);
	for(i=1; ( time[i] <= 10e7 ) && ( time[i] <= age ) ;i++)
	{
		temp+= (time[i]-time[i-1]) * model[ i*nspecsteps + wave] * exp((time[i]-age)/sfr_tau);
	}

	double temp1=0;
	for(; ( time[i] < age ) && ( (i+1) <ntimesteps ) ;i++)
	{
		temp1+= (time[i]-time[i-1]) * model[ i*nspecsteps + wave] * exp((time[i]-age)/sfr_tau);
	}

	//the last step foor smoothness with linear interpol
	temp1+= ((age-time[i-1]) * ( (time[i]-age) * model[(i-1)*nspecsteps + wave] + (age-time[i-1]) * model[i*nspecsteps + wave] )) / (time[i]-time[i-1]) ;

	result_no_vel[wave]= temp*exp(-exponent*dust_tau_v) + temp1*exp(-exponent*dust_tau_v*dust_mu);
	
	return;
}


__kernel void mask_veloc_disp ( 
					__global double* wavelengths,
					
					__global double* mes_spec,
					__global double* mes_spec_err,
					__global double* mes_spec_mask,

					__global double* result_no_vel,
					__global double* result,
					__global double* factor1,
					__global double* factor2,

					__const int nspecsteps, 

					__const double vsig
					)
				
{
	const int wave = get_global_id(0);
	//current wavelength
	double curr_waveleng=wavelengths[wave];

	//width of gaussian in wavelengths
	double sig_lam = curr_waveleng * vsig;

	//result
	double loc_result=0;

	//weigth, now it is not normalized
	double weigth;
	//sum of weigths for normalizing
	double sumweigth=0;

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
		factor1[wave]= (mes_spec[wave] * result[wave]) / (mes_spec_err[wave] * mes_spec_err[wave] );
		factor2[wave]= (result[wave] * result[wave] ) / (mes_spec_err[wave] * mes_spec_err[wave]);
	}
	else 
	{
		//problems in mes
		factor1[wave]=0;
		factor2[wave]=0;
	}

	return;
}

__kernel void fit_w_err_2 ( 
					__global double* mes_spec,
					__global double* mes_spec_err,
					__global double* mes_spec_mask,

					__global double* result,
					__global double* chi,
					__const double factor,

					__global double* wavelengths
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
