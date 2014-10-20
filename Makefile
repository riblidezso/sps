# Compilers
CXX = g++
LINK = $(CXX)

# Flags
CXXFLAGS = -std=c++11 -I$(IDIR) -I../NVIDIA_GPU_COMPUTING_SDK/OpenCL/common/inc

#Opencl library place (Apple)
ifeq ($(shell uname), Darwin) # Apple
    LIBOPENCL=-framework OpenCL
else       # Linux
    LIBOPENCL=-lOpenCL
endif

# Paths
BIN = bin
ODIR = bin/obj
SRC = src
IDIR = include

#Build rules
# kicsit fura de ez van
 
#Building converter
$(BIN)/convert_ascii_2_bin : $(ODIR)/main_convert_ascii_2_bin.o  $(ODIR)/convert_ascii_2_bin.o 
	$(LINK) -o $@ $^

$(ODIR)/main_convert_ascii_2_bin.o : $(SRC)/main_convert_ascii_2_bin.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS) 

$(ODIR)/convert_ascii_2_bin.o : $(SRC)/convert_ascii_2_bin.cpp $(IDIR)/convert_ascii_2_bin.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS)


 
#Building spectrum generator
$(BIN)/spec_gen : $(ODIR)/main_spec_gen.o  $(ODIR)/spec_gen.o $(ODIR)/sps_read.o 
	$(LINK) -o $@ $^

$(ODIR)/main_spec_gen.o : $(SRC)/main_spec_gen.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS) 

$(ODIR)/spec_gen.o : $(SRC)/spec_gen.cpp $(IDIR)/spec_gen.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS) 

$(ODIR)/sps_read.o : $(SRC)/sps_read.cpp $(IDIR)/sps_read.h 
	$(CXX) -c -o  $@ $< $(CXXFLAGS) 


#Building fitter
$(BIN)/fit_sdss : $(ODIR)/main_fit_sdss.o $(ODIR)/opencl_fit_w_err.o $(ODIR)/sps_read.o  $(ODIR)/sps_write.o
	$(LINK) -o $@ $^ $(LIBOPENCL) 

$(ODIR)/main_fit_sdss.o : $(SRC)/main_fit_sdss.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS) $(LIBOPENCL)


$(ODIR)/opencl_fit_w_err.o : $(SRC)/opencl_fit_w_err.cpp $(IDIR)/opencl_fit_w_err.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS) $(LIBOPENCL)


$(ODIR)/sps_read.o : $(SRC)/sps_read.cpp $(IDIR)/sps_read.h 
	$(CXX) -c -o  $@ $< $(CXXFLAGS) 

$(ODIR)/sps_write.o : $(SRC)/sps_write.cpp $(IDIR)/sps_write.h 
	$(CXX) -c -o  $@ $< $(CXXFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(BIN)/sps_c++ $(BIN)/sps_openCL


