# Makefile
#
# Author: Luigi Pertoldi - pertoldi@pd.infn.it
# Created: Tue 18 Jun 2019
#
CXX      = c++ -Wall -Wextra -std=c++11 -g -O3
CXXFLAGS = $$(root-config --cflags)
LIBS     = $$(root-config --libs) -lMinuit -lTreePlayer
PREFIX   = /usr/local
EXE      = bin/gerda-factory bin/gerda-fake-gen

all: dirs | $(EXE)

dirs :
	@mkdir -p bin

bin/gerda-fake-gen : src/gerda-fake-gen.cc src/GerdaFactory.cc src/GerdaFactory.h src/utils.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< src/GerdaFactory.cc $(LIBS)

bin/gerda-factory : src/gerda-factory.cc src/GerdaFastFactory.cc src/GerdaFastFactory.h src/utils.hpp
	$(CXX) $(CXXFLAGS) -o $@ $< src/GerdaFastFactory.cc $(LIBS)

clean :
	-rm -f $(EXE)

install : $(EXE)
	install -d $(PREFIX)/bin
	install $^ $(PREFIX)/bin

.PHONY : clean install
