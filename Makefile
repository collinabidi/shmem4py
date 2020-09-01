$(info Note: make sure that you have added the location of the openmpi and ucx binaries to your PATH. Mine, for example, are located at /home/collin/ompi/install/bin and /home/collin/ucx/install/bin)

all: shmem_scalar_lib1.o shmem_scalar_lib1.so shmem_lib
	mv shmem_scalar_lib1.o shared
	mv shmem_scalar_lib1.so shared
	@echo "== DONE MAKING ALL =="

shmem_scalar_lib1.so: shmem_scalar_lib1.o
	oshcc -shared -fPIC -o shmem_scalar_lib1.so shmem_scalar_lib1.o scalar_mult.c utils.c
	@echo "== DONE MAKING shmem_scalar_lib1.so =="

shmem_scalar_lib1.o: scalar_mult.c
	oshcc -shared -fPIC -o shmem_scalar_lib1.o scalar_mult.c utils.c shmem.h
	@echo "== DONE MAKING shmem_scalar_lib1.o =="

shmem_lib:
	oshcc -shared -fPIC -o shmem_lib1.o utils.c
	oshcc -shared -fPIC -o shmem_lib1.so shmem_lib1.o utils.c
	mv shmem_lib1.so shared
	mv shmem_lib1.o shared
	@echo "== DONE MAKING shmem_lib1.o and shmem_lib1.so =="
