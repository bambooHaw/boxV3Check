CC := arm-none-linux-gnueabi-gcc
FLAGS += -Wall

c_files += main.c
obj_file := stm8swimlog

all:
	$(CC) $(c_files) $(FLAGS) -o $(obj_file)

clean:
	rm -rf *.o a.out $(obj_file)
