# location of the Python header files
 
PYTHON_VERSION = 3.5m
PYTHON_INCLUDE = /usr/include/python$(PYTHON_VERSION)
 
# location of the Boost Python include files and library
 
BOOST_INC = /usr/include/boost/python3.5
BOOST_LIB = /usr/lib
 
# compile mesh classes
TARGET = supercache
 
$(TARGET).so: $(TARGET).o
	g++ -shared  -Wl,--export-dynamic $(TARGET).o -lmurmur3 -L$(BOOST_LIB) -lboost_python3 -L/usr/lib/python$(PYTHON_VERSION)/config -lpython$(PYTHON_VERSION) -o $(TARGET).so
 
$(TARGET).o: $(TARGET).cpp
	g++ -Ofast -fuse-linker-plugin -flto -march=native -I. -I$(PYTHON_INCLUDE) -I$(BOOST_INC) -fPIC -c $(TARGET).cpp
