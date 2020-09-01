# shmem4py

<div align="center"> 
  <img src="images/nsf-shrec.png", width="300">
</div>
<div align="center">
  <img src="images/p3-logo.png", width="80">
</div>


This is a Python wrapper for the C-based SHMEM library that is part of the OpenMPI 4.0.3 package.

It is authored by [Collin Abidi](https://github.com/collinabidi) as a part of the **High-Performance Computing** 
group at [**NSF SHREC**](https://nsf-shrec.org/).

The script *shmemtest.py* uses the **ctypes** library to access functions from a **.so** file. **.so** files and **.o** files are stored in the /shared directory.

*shmemtest.py* simply accesses a method in the *shmem_scalar_lib1.so* file, which contains the code from *scalar_mult.c*, a benchmarking script that is a part of the [SHREC-Parallel-Kernels](https://github.com/aljo242/SHREC-Parallel-Kernels) repository. *scalar_mult.c*'s main method, which is called in *shmemtest.py* performs a benchmarking test run of the scalar multiply BLAS kernel with MPI, MPI RMA, and SHMEM implementations, producing timing information on each.

Simply calling

```python shmemtest.py``` 

will execute the benchmark run on your device. Make sure that you have MPI with RMA accessibility and SHMEM on your machine. Always check your PATH.
