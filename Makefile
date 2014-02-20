# Compilers
CXX = g++
LINK = $(CXX)

# Flags
CXXFLAGS = -I$(IDIR)

# Paths
BIN = bin
ODIR = bin/obj
SRC = src
IDIR = include

#Build rules
# kicsit fura de ez van
 
$(BIN)/sps_c++ : $(ODIR)/main_c++.o $(ODIR)/cb_data.o $(ODIR)/table.o
	$(LINK) -o $@ $^

$(BIN)/sps_openCL : $(ODIR)/main_openCL.o $(ODIR)/cb_data.o $(ODIR)/openCL_func.o $(ODIR)/table.o
	$(LINK) -o  $@ $^ -lOpenCL 

$(ODIR)/main_c++.o : $(SRC)/main_c++.cpp $(IDIR)/cb_data.h $(IDIR)/table.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS) 

$(ODIR)/main_openCL.o : $(SRC)/main_openCL.cpp $(IDIR)/cb_data.h $(IDIR)/table.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS)

$(ODIR)/cb_data.o : $(SRC)/cb_data.cpp $(IDIR)/cb_data.h $(IDIR)/table.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS) 

$(ODIR)/openCL_func.o : $(SRC)/openCL_func.cpp $(IDIR)/cb_data.h $(IDIR)/table.h
	$(CXX) $(CXXFLAGS) -c -o $@ $< -lOpenCL  

$(ODIR)/table.o : $(SRC)/table.cpp $(IDIR)/table.h
	$(CXX) -c -o  $@ $< $(CXXFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(BIN)/sps_c++ $(BIN)/sps_openCL


