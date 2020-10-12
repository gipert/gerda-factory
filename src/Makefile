# Makefile
#
# Author: Luigi Pertoldi - pertoldi@pd.infn.it
# Created: Tue 18 Jun 2019
#
CXX      = c++ -Wall -Wextra -std=c++11 -g -O3
CXXFLAGS = $$(root-config --cflags)
LIBS     = $$(root-config --libs) -lMinuit -lTreePlayer
PREFIX   = /usr/local
EXE      = gerda-factory

all: $(EXE)

#gerda-fake-gen : gerda-fake-gen.cc GerdaFactory.cc GerdaFactory.h utils.hpp
#	$(CXX) $(CXXFLAGS) -o $@ $< GerdaFactory.cc $(LIBS)

gerda-factory : gerda-factory.cc GerdaFastFactory.cc GerdaFastFactory.h utils.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< GerdaFastFactory.cc $(LIBS)

#gerda-factory : gerda-factory.cc GerdaFactory.cc GerdaFactory.h utils.hpp
#        $(CXX) $(CXXFLAGS) -o $@ $< GerdaFactory.cc $(LIBS)


clean :
	-rm -r $(EXE)

install : $(EXE)
	install -d $(PREFIX)/bin
	install $^ $(PREFIX)/bin

.PHONY : clean install
