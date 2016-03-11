# Kompilator (gcc) och diagnostikfilter (gccfilter)
CCC = g++
# CCC = gccfilter -a -c g++

# Kompilatorflaggor, lägg till '-g' om kompilering för avlusning ska göras.
CCFLAGS += -std=c++11 -Wpedantic -Wall -Wextra

# Länkflaggor för att fixa problem med nuvarande versioner av Solaris och 
# C++ länkbibliotek.
LDFLAGS += -L/sw/gcc-${GCC4_V}/lib -static-libstdc++

ninnyproxy: nin_proxy.cc Makefile
	$(CCC) $(CCFLAGS) $(LDFLAGS) -o ninnyproxy nin_proxy.cc
	@echo "Kör med kommandot 'ninnyproxy <port att lyssna på>'"

#ninnyverbose: nin_proxy_verbose.cc Makefile
#	$(CCC) $(CCFLAGS) $(LDFLAGS) -o ninnyproxy nin_proxy_verbose.cc
#	@echo "Kör med kommandot 'ninnyproxy <port att lyssna på>'"

