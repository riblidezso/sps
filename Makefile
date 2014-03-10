# Compilers
CXX = g++
LINK = $(CXX)

# Flags
CXXFLAGS = -I$(IDIR) -I../NVIDIA_GPU_COMPUTING_SDK/OpenCL/common/inc

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
$(BIN)/spec_gen : $(ODIR)/main_spec_gen.o  $(ODIR)/spec_gen.o $(ODIR)/read.o 
	$(LINK) -o $@ $^

$(ODIR)/main_spec_gen.o : $(SRC)/main_spec_gen.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS) 

$(ODIR)/spec_gen.o : $(SRC)/spec_gen.cpp $(IDIR)/spec_gen.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS) 

$(ODIR)/read.o : $(SRC)/read.cpp $(IDIR)/read.h 
	$(CXX) -c -o  $@ $< $(CXXFLAGS) 


#Building fitter
$(BIN)/fit_dp : $(ODIR)/main_sps_fit_dp.o $(ODIR)/read.o 
	$(LINK) -o $@ $^ -lOpenCL

$(ODIR)/main_sps_fit_dp.o : $(SRC)/main_sps_fit_dp.cpp
	$(CXX) -c -o  $@ $< $(CXXFLAGS) -lOpenCL


.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(BIN)/sps_c++ $(BIN)/sps_openCL


