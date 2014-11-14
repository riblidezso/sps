-----------------------------------------------------------------
-----------------------------------------------------------------
-----------------------------------------------------------------
For the impatient:
	-I suppose you have a working openCL implementation.

-----------------------------------------------------------------
LINUX
	-I suppose you have openCL header file, and icd loader
	library in the  path (if not you can modify the Makefile).

	#go to "sps" directory then:
	>make
	#go ti bin directory (now the exec van only be run from there)
	>cd bin
	#run example
	#(test.cfg in config file, 40000 is iteration steps)
	>./fit_sdss test.cfg 40000	
	# you will have to  interact with the application
		#to specify the measurement file, 
			#(there are some testfiles)
		#and choose OpenCL implementation
	
	#interpreting the output will be adressed later	

-----------------------------------------------------------------
OSX
	-I suppose you have Xcode installed 

	#go to "sps" directory then:
	>make
	#go to bin directory (now the exec van only be run from there)
	>cd bin
	#run example
	#(test.cfg in config file, 40000 is iteration steps)
	>./fit_sdss test.cfg 40000	
	# you will have to  interact with the application
		#to specify the measurement file, 
			#(there are some testfiles)
		#and choose OpenCL implementation
	
	#interpreting the output will be adressed later	

-----------------------------------------------------------------
WINDOWS
	#go to sps/bin directory (now the exec van only be run from there)
	>cd sps/bin
	#run example
	#(test.cfg in config file, 40000 is iteration steps)
	>./sps_win_float test.cfg 40000	
	# you will have to  interact with the application
		#to specify the measurement file, 
			#(there are some testfiles)
		#and choose OpenCL implementation
	
	#interpreting the output will be adressed later	


-----------------------------------------------------------------
-----------------------------------------------------------------
-----------------------------------------------------------------






IF THE ABOVE DOES NOT WORK.

-----------------------------------------------------------------
-----------------------------------------------------------------
Installing opencl implementation

(first check your harware supports OpenCL (GPU,CPU))

-----------------------------------------------------------------
LINUX
	GPU:
		NVIDIA:
			install the nvidia proprietary driver
			it has opencl support

		AMD:
			should check, but proprietary driver
			might be enough
	
		INTEL integrated:
			-you will need HD4000+ to have opencl
			supported
			-no official supprt for linux yet,
			try beignet
	CPU:
		INTEL:
			install the Intel OpenCL SDK from
			intels website, you will need to
			fill some form but it is free
-----------------------------------------------------------------
WINDOWS
	GPU:
		NVIDIA:
			install the nvidia driver
			it has opencl support

		AMD:
			should check, but driver
			should be enough, if not, 
			install AMD SDK from AMD website
	
		INTEL integrated:
			-you will need HD4000+ to have opencl
			supported
			-intel driver has opencl support
	CPU:
		INTEL:
			install the Intel OpenCL SDK from
			intels website, you will need to
			fill some form but it is free
-----------------------------------------------------------------
OSX
	Install Xcode
-----------------------------------------------------------------

If you still have OpenCL related problems, consult the internet
some helpful links:
	http://wiki.tiker.net/OpenCLHowTo




-----------------------------------------------------------------
-----------------------------------------------------------------
Compiling the program
-----------------------------------------------------------------
LINUX
	#go to "sps" directory, then:
	>make

	the C++ code was tested with g++ 4.8.2 on Ubuntu 14.04
	the code uses C11 features, this might be a problem with older
	compiler versions
	
	To compile opencl_fit_w_err.cpp you will need the OpenCL
	header file. These are usually distributed wiht opencl
	implementations (drivers).
	If you cant find is, you can install it from Khronos webpage.
	(OpenCL versions are backwards compatible, so it does not
	matter which version you have)	

	To link the object files you will need the so called "icd loader"
	library. This is just a layer that forwards yout API calls
	to  your OpenCL implemantation.
	ICD loaders are also distributed with implementations.
	You can use whatever ICD loader you want independent of
	which OpenCL platform implementation you use.
	API calls are standardized. 
	(Eg.: you can use the AMD-s ICD loader to compile code,
	that will run on an Nvidia GPU using nvidias Opencl 
	implmentation)
	If you dont have an ICD loader on you machine, you
	can download an SDK and use the ICD loader.
	Every company distributes OpenCL icd loaders in their SDK-s.

	On linux you (might/have to) have an icd registry
	in /etc/OpenCL/vendors. These files will point to
	the ICD loaders. (I'm not sure about its purpose)

-----------------------------------------------------------------
WINDOWS
	
	Compilation was tested under Visual Studio Express 2012	

	To compile opencl_fit_w_err.cpp you will need the OpenCL
	header file. These are usually distributed wiht opencl
	implementations (drivers).
	If you cant find it, you can install it from Khronos webpage.
	(OpenCL versions are backwards compatible, so it does not
	matter which version you have)	

	To link the object files you will need the so called "icd loader"
	library. This is just a layer that forwards yout API calls
	to  your OpenCL implemantation.
	ICD loaders are also distributed with implementations.
	You can use whatever ICD loader you want independent of
	which OpenCL platform implementation you use.
	API calls are standardized. 
	(Eg.: you can use the AMD-s ICD loader to compile code,
	that will run on an Nvidia GPU using nvidias Opencl 
	implmentation)
	If you dont have an ICD loader on you machine, you
	can download an SDK and use the ICD loader.
	Every company distributes OpenCL icd loaders in their SDK-s.


-----------------------------------------------------------------
OSX
	Compilation was tested with Xcode
	You just need  install it

	#go to "sps" directory, then:
	>make

	Thats it. The OpenCL framework is included in XCode
	by default.
	

-----------------------------------------------------------------
-----------------------------------------------------------------
Running the application
	
	!!!Please keep the directory structure
	
	Now you can only run the executable from sps/bin
	it reads ther kernel source code from there
	some input paths are hard codoed

	cmdline args
		First command line argument is a config file
		it describes the fitting parameters
		
		Second command line argument is the number of 
		iterations in the Markov chain Monte Carlo.
		It should be somewhere above 10000, there is
		no upper limit, but going above a million is
		not tested.
		Recommended for initial testing is around 40000	
	
	interaction with the application
		Choose openCL platfrom for listed

		Choose device from listed
	
		Choose input sdss spectra to fit	
		!!!!Type path relative to "sps/input/"!!!

	Best fitting parameteres are listed at the end

	Output files are listed at the end
	They will be in sps/output



-----------------------------------------------------------------
-----------------------------------------------------------------
Interpreting the output

output files:
	there are files named by the parameters (age.dat, etc..)
	these store the points pf the markov chain
	(each file has the same no of rows corresponding to steps)

	there are file names "sigmas_evol*"
	these store the evolution of the stepsize of one parameter
	binned to 500 steps
	they are usefulis for tuning, and diagnostics

	there are file names "acc_ratios_evol*"
	these store the evolution of the  acceptance rates
	of changed parameters in the markov chain
	binned to 500 steps
	they are usefulis for tuning, and diagnostics

	ignore other output files

plotting the output:
	There are some simple python matplotlib scripts
	for the visualization of the results
	
	!!!run theese scripts from "sps/output"
	plots parameter histograms,
	parameter evolutions
	2parameter covariances
	fitted spectrum	
	>python matplotlib_scripts/plot.py


	>python matplotlib_scripts/plot_acc.py
	>python matplotlib_scripts/plot_sig.py
	>python matplotlib_scripts/plot_logp.py


	
-----------------------------------------------------------------
-----------------------------------------------------------------
Common problems:

	You get: Warning sigma > 1
		This is not software bug
		One of the parameters behave strangely when fitting
		and it starts to fluctuate heavily.
		Program will run, you can check the problem in
		output sigmas_evol files.
		
		Recommended to fix the malbehaving parameter

	
	You get: ERROR CAN'T OPEN FILE: ../input/bin/time.bin
		Most probably you just downloaded the source from
		github and you dont have the necessary binary input models
		Solve this by downloading a release package

		Other problem can be: you tried to run the exec not in
		sps/bin

	
