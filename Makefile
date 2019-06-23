# Makefile
#
# Author: Luigi Pertoldi - pertoldi@pd.infn.it
# Created: Tue 18 Jun 2019
#
CXX      = c++ -Wall -std=c++11 -O3
CXXFLAGS = $$(root-config --cflags)
LIBS     = $$(root-config --libs) -lMinuit -lTreePlayer

all: bin/gerda-fake-gen bin/gerda-factory

bin :
	mkdir -p bin

bin/gerda-fake-gen : src/gerda-fake-gen.cc src/GerdaFactory.cc src/GerdaFactory.h src/utils.hpp bin
	$(CXX) $(CXXFLAGS) -o $@ $< src/GerdaFactory.cc $(LIBS)

bin/gerda-factory : src/gerda-factory.cc src/GerdaFactory.cc src/GerdaFactory.h src/utils.hpp bin
	$(CXX) $(CXXFLAGS) -o $@ $< src/GerdaFactory.cc $(LIBS)

clean :
	-rm -r bin

.PHONY : clean dirs
