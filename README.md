## Compilation

- If you have the opencl header and libs, please set their location in the Makefile. After that you can make everything.

```
make all
```

- If you dont want to use the opencl version, you can just make the cpu only codes.

```
make cpu_only
```


## Usage 


### Preparations


#### Enviroment variables

- First please export the following enviroment variable. The programs use binary model inputs and opencl kernel which are placed in specific locations.

```
export SPSFAST_PATH=/path/to/sps/root/
```

- Make sure dynimaic opencl library can be found 

```
export LD_LIBRARY_PATH=/path/to/opencl/shared/lib/:LD_LIBRARY_PATH
```

- If you want to use the python bindings also export its location to python path:

```
export PYTOHONPATH=/path/to/sps/root/python_wrapper/:PYTOHNPATH
```

#### Model data

- You need to download binary models from this link: https://drive.google.com/open?id=0B1mTQzI2DYe-UmdMUXJLbGxnTTA extract the files an copy them to the input/bin/ directory


### Generating spectra in command line

- with opencl 

```
bin/generate_spec input/example_config_file.cfg input/example_params.tsv results.tsv
```

- only on cpu

```
bin/generate_spec_cpu input/example_config_file.cfg input/example_params.tsv results.tsv
```

### Fitting spectrum with MCMC

- with opencl 

```
bin/fit_spectrum input/example_fitter_config_file.cfg result_prefix
```

- only on cpu

```
bin/fit_spectrum_cpu input/example_fitter_config_file.cfg result_prefix
```


### Python wrapper

You can use the python wrapper to convieniently generate spectra, please check out the notebook for a demonstration.


