# Makefile
#
# Author: Luigi Pertoldi - pertoldi@pd.infn.it
# Created: Tue 18 Jun 2019
#
CXX      = c++ -Wall -std=c++11 -O3
CXXFLAGS = $$(root-config --cflags)
LIBS     = $$(root-config --libs) -lMinuit -lTreePlayer

all: bin/gerda-fake-gen

dirs :
	mkdir -p bin

bin/gerda-fake-gen : src/gerda-fake-gen.cc src/GerdaFastFactory.cc src/GerdaFastFactory.h src/utils.hpp dirs
	$(CXX) $(CXXFLAGS) -o $@ $< src/GerdaFastFactory.cc $(LIBS)

clean :
	-rm -r bin

.PHONY : clean dirs
