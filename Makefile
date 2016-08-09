
#change this for your opencl header path
CL_H = /home/ribli/tools/AMDAPPSDK-3.0/include/
#/usr/local/cuda-7.5/targets/x86_64-linux/include/

#change this for your opencl library path
CL_L = /home/ribli/tools/AMDAPPSDK-3.0/lib/x86_64/


# Compilers
CXX = g++
LINK = $(CXX)

# Flags
CXXFLAGS = -O3 -std=c++11 -I$(IDIR) -I$(CL_H) -W -Wall

#Opencl library place (Apple)
ifeq ($(shell uname), Darwin) # Apple
    LIBOPENCL=-framework OpenCL
else       # Linux
    LIBOPENCL= -lOpenCL -L $(CL_L) 
endif


# Paths
BIN = bin
ODIR = bin/obj
SRC = src
IDIR = include




# all target
all: $(BIN)/generate_spec $(BIN)/generate_spec_cpu $(BIN)/test_spectrum_generators $(BIN)/fit_spectrum $(BIN)/fit_spectrum_cpu



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


#Buildingi cpu only fitter
$(BIN)/fit_spectrum_cpu: $(ODIR)/fit_spectrum_cpu.o $(ODIR)/spectrum_generator_cpu.o $(ODIR)/mcmc.o $(ODIR)/sps_data.o  $(ODIR)/sps_write.o $(ODIR)/sps_options.o
	$(LINK) -o $@ $^ 

$(ODIR)/fit_spectrum_cpu.o : $(SRC)/fit_spectrum_cpu.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS)



#Building converter
#$(BIN)/convert_ascii_2_bin : $(ODIR)/main_convert_ascii_2_bin.o  $(ODIR)/convert_ascii_2_bin.o
#	$(LINK) -o $@ $^

#$(ODIR)/main_convert_ascii_2_bin.o : $(SRC)/main_convert_ascii_2_bin.cpp
#	$(CXX) -c -o  $@ $< $(CXXFLAGS)

#$(ODIR)/convert_ascii_2_bin.o : $(SRC)/convert_ascii_2_bin.cpp $(IDIR)/convert_ascii_2_bin.h
#	$(CXX) -c -o  $@ $< $(CXXFLAGS)


.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(BIN)/sps_c++ $(BIN)/sps_openCL


