OBJS    = main.o HDC2080.o BMP280.o VEML7700.o i2c_helper.o
SOURCE  = main.cpp BMP280.cpp VEML7700.cpp HDC2080.cpp i2c_helper.cpp
HEADER  = HDC2080.h i2c_helper.h VEML7700.h BMP280.h
OUT     = OutsideSensor
CC       = g++
FLAGS    = -g3 -c -Wall
LFLAGS   = -lpthread

all: $(OBJS)
        $(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

main.o: main.cpp
        $(CC) $(FLAGS) main.cpp

HDC2080.o: HDC2080.cpp
        $(CC) $(FLAGS) HDC2080.cpp

i2c_helper.o: i2c_helper.cpp
        $(CC) $(FLAGS) i2c_helper.cpp


clean:
        rm -f $(OBJS) $(OUT)

debug: $(OUT)
        valgrind $(OUT)

valgrind: $(OUT)
        valgrind $(OUT)

valgrind_leakcheck: $(OUT)
        valgrind --leak-check=full $(OUT)

valgrind_extreme: $(OUT)
        valgrind --leak-check=full --show-leak-kinds=all --leak-resolution=high --track-origins=yes --vgdb=yes $(OUT)