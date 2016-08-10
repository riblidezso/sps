from ctypes import *
import numpy as np
import os

#load lib
libcpu = cdll.LoadLibrary(os.environ['SPSFAST_PATH']+'python_wrapper/sps_fast_utils_cpu.so')

#specify function args and return types 
libcpu.spectrum_generator_cpu_new.restype = c_void_p
    
libcpu.generate_spec_cpu_exp.argtypes  = [c_void_p,c_double,c_double,c_double,c_double,c_double,c_double,c_void_p]
libcpu.generate_spec_cpu_exp.restype = c_void_p

libcpu.generate_spec_cpu_sfr_vec.argtypes  = [c_void_p,c_double,c_double,c_double,c_double,c_double,c_double,c_void_p,c_void_p]
libcpu.generate_spec_cpu_sfr_vec.restype = c_void_p

libcpu.freeme.argtypes = c_void_p,
libcpu.freeme.restype = None

libcpu.free_spec_gen_cpu.argtypes = c_void_p,
libcpu.free_spec_gen_cpu.restype = None


class Spectrum_generator_cpu(object):
    """Cpu only spectrum generator"""
    
    def __init__(self,measurement_file='../input/testspec1.csv',imf='chabrier'):
        """
        Create a new spectrum generator instance.
            -free it later?
        
        """
        # get a spectrum generator class
        self.obj = libcpu.spectrum_generator_cpu_new(measurement_file,imf)
    
    def generate_spec(self,sfr=None,age=1e9,vdisp=150, metall=0.001,sfr_tau=1e8,
                      dust_tau_v=1.0,dust_mu=0.7):
        """Generate spectrum."""
        if sfr is None:
            return self.generate_spec_exp(age,vdisp, metall,sfr_tau,dust_tau_v,dust_mu)
        else:
            return self.generate_spec_sfr(sfr,age,vdisp, metall,sfr_tau,dust_tau_v,dust_mu)
        
    def generate_spec_exp(self,age,vdisp, metall,sfr_tau,dust_tau_v,dust_mu):
        """Generate spectrum."""
        #generate spectrum
        result_size=c_int()
        res_pointer=libcpu.generate_spec_cpu_exp(self.obj,age,vdisp, metall,sfr_tau,dust_tau_v,dust_mu,byref(result_size))
        
        #spectrum as numpy array
        res=np.ctypeslib.as_array(cast(res_pointer,POINTER(c_double)),shape=(result_size.value,))
        
        #unforutnately i have to make a copy of the array to
        #force it to take ownership of the data and get python
        #delete the data when it deletes the array
        res=np.array(res,copy=True)
        
        #free memory
        libcpu.freeme(res_pointer)
        
        return res
    
    def generate_spec_sfr(self,sfr,age,vdisp, metall,sfr_tau,dust_tau_v,dust_mu):
        """Generate spectrum."""
        #generate specrtrum
        result_size=c_int()
        res_pointer=libcpu.generate_spec_cpu_sfr_vec(self.obj,age,vdisp, metall,sfr_tau,dust_tau_v,dust_mu,c_void_p(sfr.ctypes.data),byref(result_size),)
        
        #spectrum as numpy array
        res=np.ctypeslib.as_array(cast(res_pointer,POINTER(c_double)),shape=(result_size.value,))
    
        #unfortunately i have to make a copy of the array to
        #force it to take ownership of the data and get python
        #delete the data when it deletes the array
        res=np.array(res,copy=True)
        
        #free memory
        libcpu.freeme(res_pointer)
        
        return res
    
    def __del__(self):
        """Free the memory of the spectrum generator class."""
        libcpu.free_spec_gen_cpu(self.obj)