import ctypes

# Load the shared scalar mult library into c types.
scalarlib = ctypes.CDLL("shared/shmem_scalar_lib1.so")

# Call the main method to see if it works properly
main_func = scalarlib.main()

# Load SHMEM library into ctypes
shmemlib = ctypes.CDLL("shared/shmem_lib1.so")

#