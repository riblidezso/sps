
#change this for your opencl header path
CL_H = /home/ribli/tools/AMDAPPSDK-3.0/include/
#/usr/local/cuda-7.5/targets/x86_64-linux/include/

#change this for your opencl library path
CL_L = /home/ribli/tools/AMDAPPSDK-3.0/lib/x86_64/


# Compilers
CXX = g++
LINK = $(CXX)

# Flags
CXXFLAGS = -O3 -std=c++11 -I$(IDIR) -I$(CL_H) -W -Wall -fPIC
ifeq ($(shell uname), Darwin) # Apple
    SHARED_LIB_FLAG= -dynamiclib 
else       # Linux
    SHARED_LIB_FLAG= -shared 
endif

#Opencl library place (Apple)
ifeq ($(shell uname), Darwin) # Apple
    LIBOPENCL=-framework OpenCL
else       # Linux
    LIBOPENCL= -lOpenCL -L $(CL_L) 
endif


# Paths
BIN = bin
PYWRAP = python_wrapper
ODIR = bin/obj
SRC = src
IDIR = include


# all target
all: $(BIN)/generate_spec $(BIN)/generate_spec_cpu $(BIN)/test_spectrum_generators $(BIN)/fit_spectrum $(BIN)/fit_spectrum_cpu $(PYWRAP)/sps_fast_utils.so $(PYWRAP)/sps_fast_utils_cpu.so

# cpu only target
cpu_only: $(BIN)/generate_spec_cpu $(BIN)/fit_spectrum_cpu $(PYWRAP)/sps_fast_utils_cpu.so



#Build rules

#Building spectrum generator
$(BIN)/generate_spec : $(ODIR)/generate_spec.o  $(ODIR)/spectrum_generator.o $(ODIR)/sps_data.o  $(ODIR)/sps_write.o $(ODIR)/sps_options.o
	$(LINK) -o $@ $^ $(LIBOPENCL)

$(ODIR)/generate_spec.o : $(SRC)/generate_spec.cpp$
	$(CXX) -c -o  $@ $< $(CXXFLAGS)

$(ODIR)/sps_options.o : $(SRC)/sps_options.cpp $(IDIR)/sps_options.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS)

$(ODIR)/sps_data.o : $(SRC)/sps_data.cpp $(IDIR)/sps_data.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS)

$(ODIR)/spectrum_generator.o : $(SRC)/spectrum_generator.cpp $(IDIR)/spectrum_generator.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS)

$(ODIR)/sps_write.o : $(SRC)/sps_write.cpp $(IDIR)/sps_write.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS)


#Building spectrum generator cpu only version
$(BIN)/generate_spec_cpu : $(ODIR)/generate_spec_cpu.o  $(ODIR)/spectrum_generator_cpu.o $(ODIR)/sps_data.o  $(ODIR)/sps_write.o $(ODIR)/sps_options.o
	$(LINK) -o $@ $^

$(ODIR)/generate_spec_cpu.o : $(SRC)/generate_spec_cpu.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS)

$(ODIR)/spectrum_generator_cpu.o : $(SRC)/spectrum_generator_cpu.cpp $(IDIR)/spectrum_generator_cpu.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS)


#Building test
$(BIN)/test_spectrum_generators : $(ODIR)/test_spectrum_generators.o  $(ODIR)/spectrum_generator_cpu.o $(ODIR)/spectrum_generator.o $(ODIR)/sps_data.o  $(ODIR)/sps_options.o $(ODIR)/sps_write.o
	$(LINK) -o $@ $^ $(LIBOPENCL)

$(ODIR)/test_spectrum_generators.o : $(SRC)/test_spectrum_generators.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS)



#Building fitter
$(BIN)/fit_spectrum : $(ODIR)/fit_spectrum.o $(ODIR)/spectrum_generator.o $(ODIR)/mcmc.o $(ODIR)/sps_data.o  $(ODIR)/sps_write.o $(ODIR)/sps_options.o
	$(LINK) -o $@ $^ $(LIBOPENCL)

$(ODIR)/fit_spectrum.o : $(SRC)/fit_spectrum.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS)

$(ODIR)/mcmc.o : $(SRC)/mcmc.cpp $(IDIR)/mcmc.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS)


#Building cpu only fitter
$(BIN)/fit_spectrum_cpu: $(ODIR)/fit_spectrum_cpu.o $(ODIR)/spectrum_generator_cpu.o $(ODIR)/mcmc.o $(ODIR)/sps_data.o  $(ODIR)/sps_write.o $(ODIR)/sps_options.o
	$(LINK) -o $@ $^ 

$(ODIR)/fit_spectrum_cpu.o : $(SRC)/fit_spectrum_cpu.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS)


#Builing the python wrapper
$(PYWRAP)/sps_fast_utils.so: $(PYWRAP)/sps_fast_utils.o $(ODIR)/spectrum_generator.o $(ODIR)/sps_data.o  $(ODIR)/sps_write.o $(ODIR)/sps_options.o
	$(LINK) -o  $@ $^ $(LIBOPENCL) $(SHARED_LIB_FLAG)


$(PYWRAP)/sps_fast_utils.o : $(PYWRAP)/utils.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS)


#Builing the cpu only python wrapper
$(PYWRAP)/sps_fast_utils_cpu.so: $(PYWRAP)/sps_fast_utils_cpu.o $(ODIR)/spectrum_generator_cpu.o $(ODIR)/sps_data.o  $(ODIR)/sps_write.o $(ODIR)/sps_options.o
	$(LINK) -o  $@ $^ $(SHARED_LIB_FLAG)

$(PYWRAP)/sps_fast_utils_cpu.o : $(PYWRAP)/utils_cpu.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS)



.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(PYWRAP)/*.o $(PYWRAP)/*.so $(BIN)/generate_spec  $(BIN)/generate_spec_cpu $(BIN)/test_spectrum_generators  $(BIN)/fit_spectrum $(BIN)/fit_spectrum_cpu 


