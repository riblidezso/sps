#include "spectrum_generator_cpu.h"

spectrum_generator_cpu::spectrum_generator_cpu(sps_data& input_data){
    //copy and move data from model
    copy_and_move_data(input_data);
}


/////////////////////////////////////////////////////////////////////////////
// functions for initialization
/////////////////////////////////////////////////////////////////////////////


/*
 copies and moves data from sps data
 */
int spectrum_generator_cpu::copy_and_move_data(sps_data& input_data){
    //copy data sizes
    this->ntimesteps=(int) input_data.time.size();
    this->mes_nspecsteps=(int) input_data.mes_spec_wavel.size();
    
    //copy (and there is type conversion too) small data, that wont be modified
    this->time=std::vector<double>(input_data.time.begin(),input_data.time.end());
    this->mes_spec=std::vector<double>(input_data.mes_spec.begin(),input_data.mes_spec.end());
    this->mes_spec_wavel=std::vector<double>(input_data.mes_spec_wavel.begin(),input_data.mes_spec_wavel.end());
    this->mes_spec_mask=std::vector<double>(input_data.mes_spec_mask.begin(),input_data.mes_spec_mask.end());
    this->mes_spec_err=std::vector<double>(input_data.mes_spec_err_h.begin(),input_data.mes_spec_err_h.end());
    
    //copy model data
    this->resampled_model=std::vector<double>(input_data.resampled_model_cont.begin(),input_data.resampled_model_cont.end());
    
    //initialize model which will be used for metallicity interpolation
    this->model.resize(this->ntimesteps*this->mes_nspecsteps);
    
    //initalize results
    result_no_vel.resize(this->mes_nspecsteps);
    result.resize(this->mes_nspecsteps);
    
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// functions during operation
/////////////////////////////////////////////////////////////////////////////

/*
 set model parameters
 */
int spectrum_generator_cpu::set_params( std::map<std::string,double>& parameters ){
    //set params if they are in the input map
    std::map<std::string,double>::iterator it;
    it= parameters.find("dust_tau_v");
    if(it != parameters.end()){
        this->dust_tau_v = parameters["dust_tau_v"];
    }
    it= parameters.find("dust_mu");
    if(it != parameters.end()){
        this->dust_mu = parameters["dust_mu"];
    }
    it= parameters.find("sfr_tau");
    if(it != parameters.end()){
        this->sfr_tau = parameters["sfr_tau"];
    }
    it= parameters.find("age");
    if(it != parameters.end()){
        this->age = parameters["age"];
    }
    it= parameters.find("metall");
    if(it != parameters.end()){
        this->metall = parameters["metall"];
    }
    it= parameters.find("vdisp");
    if(it != parameters.end()){
        this->vdisp = parameters["vdisp"];
    }

    
    //initalize results
    for( int i=0;i<this->mes_nspecsteps;i++) {
        //result[i]=0;
    }
    
	return 0;
}

/////////////////////////////////////////////////////////////////////////////

/*
 generate spectrum
 */
int spectrum_generator_cpu::generate_spectrum(){
    for(int i=0;i<this->mes_nspecsteps;i++){
        metall_interpol(i);
    }
    for(int i=0;i<this->mes_nspecsteps;i++){
        conv_model_w_sfh(i);
    }
    for(int i=0;i<this->mes_nspecsteps;i++){
        convol_vel_disp(i);
    }

    return 0;
}

/*
 interpolate models in metallicity
 */
int spectrum_generator_cpu::metall_interpol(int wave){
    //get the first higher metallicty
    double model_metal[6]={0.0001,0.0004,0.004,0.008,0.02,0.05};
    int modelno;
    for(int j=0;j<6;j++){
        if(model_metal[j] > this->metall){
            modelno=j-1;
            break;
        }
    }
    
    //calculate interploation weights
    double wlower,whigher,delta;
    delta=log( model_metal[modelno+1] / model_metal[modelno] );
    wlower=log(model_metal[modelno+1]/metall) / delta;
    whigher=log(metall/model_metal[modelno]) / delta;
    
    //linear interpol in log(z)
    int place,place1,place2; //pointers
    //loop over timsteps
    for(int i=0; i<this->ntimesteps ;i++){
        place=wave+i*this->mes_nspecsteps;
        place1=modelno*this->mes_nspecsteps*this->ntimesteps + place;
        place2=place1+this->mes_nspecsteps*this->ntimesteps;
        model[place]= wlower*resampled_model[place1] + whigher*resampled_model[place2] ;
    }
    
    return 0;
}

/*
 calculate the convolution of the star formation history an ssp models
 */
int spectrum_generator_cpu::conv_model_w_sfh(int wave){
    int i;
    
    //first period until 1e7
    double temp=0;
    temp+= this->time[0] * this->model[wave] * exp((this->time[0]-age)/this->sfr_tau);
    for(i=1; ( this->time[i] <= 1e7 ) && ( this->time[i] <= this->age ) ;i++){
        temp+= (this->time[i]-this->time[i-1]) * this->model[ i*this->mes_nspecsteps + wave] * exp((this->time[i]-this->age)/this->sfr_tau);
    }
    
    //second period
    float temp1=0;
    for(; ( this->time[i] < this->age ) && ( (i+1) <this->ntimesteps ) ;i++){
        temp1+= (this->time[i]-this->time[i-1]) * this->model[ i*this->mes_nspecsteps + wave] * exp((this->time[i]-this->age)/this->sfr_tau);
    }
    temp1+= (this->age-this->time[i-1]) *  this->model[i*this->mes_nspecsteps + wave]  ;
    
    //part of dust exponent is constant for a wavel
    double  exponent=pow(this->mes_spec_wavel[wave]/5500.0,-0.7);
    
    //add the 2 periods
    this->result_no_vel[wave] = temp * exp(-exponent*this->dust_tau_v) + temp1 * exp(-exponent*this->dust_tau_v*this->dust_mu);
    
    return 0;
}

/*
 convolute result with a gaussian kernel for velocity dispersion
*/
int spectrum_generator_cpu::convol_vel_disp(int wave){
    //vdisp convolution window
    #define WINDOW 5
    
    //current wavelength
    double curr_waveleng=mes_spec_wavel[wave];
    
    //width of gaussian in wavelengths
    //speed of light hardcoded, but no problem, that wont change soon
    //vdisp is in km/sec
    double sig_lam = curr_waveleng * this->vdisp/ 299792.458 ;
    
    //weight, now it is not normalized
    double w;
    //sum of weights for normalizing
    double sumw=0;
    
    //do the convolution
    this->result[wave]=0;
    for (int i=std::max(wave-WINDOW,0);i<=std::min(wave+WINDOW,this->mes_nspecsteps-1);i++){
        w= exp(-(this->mes_spec_wavel[i]- curr_waveleng) * (this->mes_spec_wavel[i]- curr_waveleng) / (2 * sig_lam * sig_lam));
        sumw+=w;
        this->result[wave]+= w * this->result_no_vel[i];
    }
    
    //normalize
    this->result[wave]=this->result[wave]/sumw;

    return 0;
}

/////////////////////////////////////////////////////////////////////////////

/*
 compare generated spectrum to measurement
 */
int spectrum_generator_cpu::compare_to_measurement(){
    //get factor to pull the generated spec to the measurement
    double scale_factor=get_factor_to_scale_spectra_to_measurement();
    
    //multiply with scale factor
    for(int i=0;i<this->mes_nspecsteps;i++){
        this->result[i]=this->result[i] * scale_factor;
    }
    
    //get factor to pull the generated spec to the measurement
    this->chi=get_chi_square(scale_factor);

	return 0;
}

/*
 calculate the scaliung factor to pull spectra to measurement
 */
double spectrum_generator_cpu::get_factor_to_scale_spectra_to_measurement(){
    //weighted squared errors
    std::vector<double> factor1(this->mes_nspecsteps);
    std::vector<double> factor2(this->mes_nspecsteps);
    
    //loop over wavelenghts
    for( int wave=0 ; wave<this->mes_nspecsteps;wave++){
        //counting the factor with err
        if ( this->mes_spec_mask[wave] == 0 && this->mes_spec_err[wave]!=0  ){
            factor1[wave]= (this->mes_spec[wave] * this->result[wave])  / (this->mes_spec_err[wave] * this->mes_spec_err[wave] );
            factor2[wave]= (this->result[wave] * this->result[wave] ) / (this->mes_spec_err[wave] * this->mes_spec_err[wave]);
        }
        else { //problems in mes
            factor1[wave]=0;
            factor2[wave]=0;
        }
    }
    
    //temp1, temp2 are sums of vectors factor1,factor2
    //they are use to calculate the "factor" that pulls together observed
    //and model spectra
    double temp_1,temp_2,scale_factor;
    
    //get factors
    
    //summing factors, to pull spectra together
    //summing is sequential on CPU paralell summing would an overkill for 3-4000 numbers i guess
    temp_1=0;
    temp_2=0;
    for(int i=0;i<this->mes_nspecsteps;i++){
        temp_1+=factor1[i];
        temp_2+=factor2[i];
    }
    scale_factor=temp_1/temp_2;
    
    return scale_factor;
}

/*
 calculate the sum of weighted squared errors
 */
double spectrum_generator_cpu::get_chi_square(double scale_factor){
    //weighted squared errors
    std::vector<double> chis(this->mes_nspecsteps);
    
    //count chi
    for (int wave=0;wave<this->mes_nspecsteps;wave++){
        if (  this->mes_spec_mask[wave] == 0 && this->mes_spec_err[wave]!=0 ){
            chis[wave]= ( this->mes_spec[wave] - this->result[wave]) * ( this->mes_spec[wave] - this->result[wave]) / (2 * this->mes_spec_err[wave] * this->mes_spec_err[wave]);
        }
        else{
            chis[wave]=0;
        }
    }
    
    //summing chi squares in host
    double chi=0;
    for(int i=0;i<mes_nspecsteps;i++){
        chi+=chis[i];
    }
    
    return chi;
    
}

/////////////////////////////////////////////////////////////////////////////

/*
return the modeled spectrum
 */
std::vector<double> spectrum_generator_cpu::get_result(){
    return this->result;
}


/////////////////////////////////////////////////////////////////////////////
// write results
/////////////////////////////////////////////////////////////////////////////


/*
 write spectra to tsv file
 */
int spectrum_generator_cpu::write_specs(std::vector< std::vector<double> >& results,
                                    std::string out_fname){
    std::vector<std::vector <double> > output;
    output.push_back(std::vector<double>(this->mes_spec_wavel.begin(),this->mes_spec_wavel.end()));
    for (auto res : results){
        output.push_back(std::vector<double>(res.begin(),res.end()));
    }
    write_table_col(output,out_fname);
    return 0;
}