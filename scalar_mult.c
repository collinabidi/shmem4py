#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <shmem.h>
#include "utils.h"

int         pWrk_int[SHMEM_REDUCE_SYNC_SIZE];
float       pWrk_float[SHMEM_REDUCE_SYNC_SIZE];
double      pWrk_double[SHMEM_REDUCE_SYNC_SIZE];
long        pSync[SHMEM_BCAST_SYNC_SIZE];

int GetPartitionSize(const int local_size, const int numPartitions);
int GetLastPartitionSize(const int local_size, const int numPartitions);
void PrintPartitionInfo(const int size, const int local_size, const int masterLocal,
const int numPartitions, const int partitionSize, const int lastPartitionSize);

// functions to perform operation
double int_scalar_mult_overlap(const int size, const int numPartitions, const int local_size, const int partitionSize, const int masterLocal);
double int_scalar_mult(const int size, const int local_size);
double float_scalar_mult_overlap(const int size, const int numPartitions, const int local_size, const int partitionSize, const int masterLocal);
double float_scalar_mult(const int size, const int local_size);
double double_scalar_mult_overlap(const int size, const int numPartitions, const int local_size, const int partitionSize, const int masterLocal);
double double_scalar_mult(const int size, const int local_size);

// function to run timing tests on the different data types
void partitions_sweep(const int size, const int startPartition, const int stopPartition, const int iterations);
void size_sweep(const int numPartitions, const int startSize, const int stopSize, const int iterations);
void fixed_test(const int size, const int iterations, const int numPartitions);

#define MASTER 0

#define PARTITION_DIVIDER 2
#define __PARTITION_DIVISION_SCHEME__ 0

int main(int argc, char **argv) {

    for (int i = 0; i < SHMEM_BCAST_SYNC_SIZE; ++i) {
        pSync[i] = SHMEM_SYNC_VALUE;
    }

    int world_size, world_rank;
    shmem_init();
    world_size = shmem_n_pes();
    world_rank = shmem_my_pe();

	double out_time;
	const int numPartitions = 10;
	const int startSize = 100;
	const int stopSize = 10000;
	const int iterations = 100;
	
	// Run axpy test
	//size_sweep(numPartitions, startSize, stopSize, iterations);
	//fixed_test(stopSize, iterations, numPartitions);
    partitions_sweep(stopSize, 1, 10, iterations);

    shmem_finalize();

    return 0;
}

void partitions_sweep(const int size, const int startPartition, const int stopPartition, const int iterations)
{
    int world_size, world_rank;
    world_size = shmem_n_pes();
    world_rank = shmem_my_pe();

    const int partitionDivision = (stopPartition) / iterations;

    for (int i = startPartition; i <= stopPartition; ++i)
    {
        fixed_test(size, iterations, i);
    }
}
 

void size_sweep(const int numPartitions, const int startSize, const int stopSize, const int iterations)
{
    int world_size, world_rank;
    world_size = shmem_n_pes();
    world_rank = shmem_my_pe();

    const int sizeStep = (stopSize) / iterations;

    for (int i = 0; i < iterations; ++i)
    {
        const int size = (i * sizeStep) + startSize;
        fixed_test(size, iterations, numPartitions);
    }
}


void fixed_test(const int size, const int iterations, const int numPartitions)
{
    int world_size, world_rank;
    world_size = shmem_n_pes();
    world_rank = shmem_my_pe();

    int local_size = 0;
    int partitionSize = 0;
    int lastPartitionSize = 0; // only use if modulo is nonzero
    int masterLocal = 0;
    
    local_size = size / world_size;
    partitionSize = GetPartitionSize(local_size, numPartitions);
    lastPartitionSize = GetLastPartitionSize(local_size, numPartitions);
    masterLocal = (size / world_size) + (size % world_size) + (lastPartitionSize * (world_size - 1)); // any extas go to master nodes

    if (world_rank == MASTER) 
    {
        //printf("Running with %d SHMEM processes.\n", world_size);
        //PrintPartitionInfo(size, (local_size - lastPartitionSize), masterLocal, numPartitions, partitionSize, lastPartitionSize);
        local_size = masterLocal;
    }
    else
    {
        local_size -= lastPartitionSize;
    }

    double intOverlapAvg = 0;
    double intAvg = 0;
    double floatOverlapAvg = 0;
    double floatAvg = 0;
    double doubleOverlapAvg = 0;
    double doubleAvg = 0;

    for (int i = 0; i < iterations; ++i)
    {
        intOverlapAvg += int_scalar_mult_overlap(size, numPartitions, local_size, partitionSize, masterLocal);
        intAvg += int_scalar_mult(size, local_size);

        floatOverlapAvg += float_scalar_mult_overlap(size, numPartitions, local_size, partitionSize, masterLocal);
        floatAvg += float_scalar_mult(size, local_size);

        doubleOverlapAvg += double_scalar_mult_overlap(size, numPartitions, local_size, partitionSize, masterLocal);
        doubleAvg += double_scalar_mult(size, local_size);
    }

    if (world_rank == MASTER)
    {
        printf("shmem, scalar_mult, %d, %d, int, %d, %.9lf\n", world_size, numPartitions, size, intAvg/(double)iterations);
        printf("shmem, scalar_mult, %d, %d, int_overlap, %d, %.9lf\n", world_size, numPartitions, size, intOverlapAvg/(double)iterations);
        printf("shmem, scalar_mult, %d,  %d, double, %d, %.9lf\n", world_size, numPartitions, size, doubleAvg/(double)iterations);
        printf("shmem, scalar_mult, %d,  %d, double_overlap, %d, %.9lf\n", world_size, numPartitions, size, doubleOverlapAvg/(double)iterations);
        printf("shmem, scalar_mult, %d,  %d, float, %d, %.9lf\n", world_size, numPartitions, size, floatAvg/(double)iterations);
        printf("shmem, scalar_mult, %d, %d, float_overlap, %d, %.9lf\n", world_size, numPartitions, size, floatOverlapAvg/(double)iterations);
    }
}

int GetPartitionSize(const int local_size, const int numPartitions)
{
    return local_size / numPartitions;
}

int GetLastPartitionSize(const int local_size, const int numPartitions)
{
    return local_size % numPartitions;
}

void PrintPartitionInfo(const int size, const int local_size,  const int masterLocal, 
    const int numPartitions, const int partitionSize, const int lastPartitionSize)
{
    printf("---------------------------------------\n");
    printf("Size: \t\t\t%d\n", size);
    printf("Local Size: \t\t%d\n", local_size);
    printf("Master Local Size: \t%d\n", masterLocal);
    printf("Num Partitions: \t%d\n", numPartitions);
    printf("Partition Size: \t%d\n", partitionSize);
    printf("Last Partition: \t%d\n", lastPartitionSize);
    printf("---------------------------------------\n");
}


double int_scalar_mult_overlap(const int size, const int numPartitions, const int local_size, const int partitionSize, const int masterLocal)
{
    int world_size, world_rank;
    world_size = shmem_n_pes();
    world_rank = shmem_my_pe();

    static int a_symmetric = 0;

    /*  Create Vector and Scalar  */
    int* main = (int *) shmem_malloc(size*sizeof(int));

    // must allocate all with size (masterLocal >= local_size) 
    // shmem_malloc MUST be called collectively by ALL PEs and all arguments must be the same
    // otherwise, the standard defines this as UNDEFINED BEHAVIOR
    int* local = (int *) shmem_malloc(masterLocal * sizeof (int));
    int* test_main = NULL; 

    struct timespec start, end;

    int a = 0;
    a_symmetric = (world_rank+1) * rand() % 10;

    if (world_rank == MASTER)
    {
        test_main = (int *) malloc(size*sizeof(int));
        a = a_symmetric;
        for (int i = 0; i < size; ++i)
        {
            main[i] = rand() % 10;
            test_main[i] = main[i] * a;
        }
    }
    shmem_barrier_all();

    /* BEGIN TIMING */
    clock_gettime(CLOCK, &start);

    if (world_rank != MASTER)
    {
        const int offsetMaster = (world_rank) * masterLocal;
        shmem_get(local, main + offsetMaster, partitionSize, 0);
        a = shmem_g(&a_symmetric, 0);

        for (int iter = 1; iter <= numPartitions; ++iter)
        {
            // ensure previous data transfer complete so it can be computed on
            shmem_quiet();
            // first begin the data transfer for the next iteration
            if (iter != numPartitions)
            {
                const int offset = offsetMaster + (partitionSize * (iter));
                const int localOffset = partitionSize * (iter);
                shmem_get_nbi(local + localOffset, main + offset, partitionSize, 0);
            }
            const int computeStart = (iter - 1) * partitionSize;
            const int computeEnd = (iter) * partitionSize; 

            for (int i = computeStart; i < computeEnd; ++i) 
            {
                local[i] *= a;
            }
        }
    }

    else // world_rank == MASTER
    {
        for (int i = 0; i < local_size; ++i)
        {
            local[i] = main[i] * a;
        }
    }
    shmem_barrier_all();

    shmem_collect32(main, local, local_size, 0, 0, world_size, pSync); // 32 for 32bit int
    // fixed collect because size is the same for all PEs
    

    /* END TIMING */
    clock_gettime(CLOCK, &end);

    if (world_rank  == MASTER)
    {
        for (int i = 0; i < size; ++i)
        {
            const int diff = test_main[i] - main[i];
            if (diff != 0)
            {
                printf("%d nonzero at index %d\n", diff, i);
            }
        }
        free(test_main);
    }

    shmem_free(main);
    shmem_free(local);


    return elapsed_time(start, end);
}


double int_scalar_mult(const int size, const int local_size)
{
    int world_size, world_rank;
    world_size = shmem_n_pes();
    world_rank = shmem_my_pe();

    static int a_symmetric = 0;

    /*  Create Vector and Scalar  */
    int* main = (int *) shmem_malloc(size*sizeof(int));
    int* local = (int *) shmem_malloc(local_size * sizeof (int));


    struct timespec start, end;

    if (world_rank == MASTER)
    {
        for (int i = 0; i < size; ++i)
        {
            main[i] = rand() % 10;
        }
    }
    shmem_barrier_all();

    clock_gettime(CLOCK, &start);


    int a = 0;
    a_symmetric = (world_rank+1) * rand() % 10;

    if (world_rank != MASTER)
    {
        int offset = world_rank * local_size;
        shmem_get(local, main + offset, local_size, 0);
        a = shmem_g(&a_symmetric, 0);
    }
    else
    {
        a = a_symmetric;
    }

    /* perform scalar product */
    if (world_rank != MASTER)
    {
        for (int i = 0; i < local_size; i++) 
        {
            local[i] *= a;
        }
    }
    else
    {
        for (int i = 0; i < local_size; i++) 
        {
            main[i] *= a;
        } 
    }
    shmem_barrier_all();


    if (world_size > 1) // only broadcast if we actually should
    {
        shmem_collect32(main, local, local_size, 0, 0, world_size, pSync); // 32 for 32bit int
        // fixed collect because size is the same for all PEs
    }

    clock_gettime(CLOCK, &end);

    shmem_free(main);
    shmem_free(local);

    return elapsed_time(start, end);
}


double float_scalar_mult_overlap(const int size, const int numPartitions, const int local_size, const int partitionSize, const int masterLocal)
{
    int world_size, world_rank;
    world_size = shmem_n_pes();
    world_rank = shmem_my_pe();

    static float a_symmetric = 0.0f;

    /*  Create Vector and Scalar  */
    float* main = (float *) shmem_malloc(size*sizeof(float));

    // must allocate all with size (masterLocal >= local_size) 
    // shmem_malloc MUST be called collectively by ALL PEs and all arguments must be the same
    // otherwise, the standard defines this as UNDEFINED BEHAVIOR
    float* local = (float *) shmem_malloc(masterLocal * sizeof (float));
    float* test_main = NULL; 

    struct timespec start, end;

    float a = 0.0f;    
    a_symmetric = (world_rank+1) * rand() / 10.0f;

    if (world_rank == MASTER)
    {
        test_main = (float *) malloc(size*sizeof(float));
        a = a_symmetric;
        for (int i = 0; i < size; ++i)
        {
            main[i] = rand() / 10.0f;
            test_main[i] = main[i] * a;
        }
    }
    shmem_barrier_all();


    /* BEGIN TIMING */
    clock_gettime(CLOCK, &start);


    if (world_rank != MASTER)
    {
        const int offsetMaster = (world_rank) * masterLocal;
        shmem_get(local, main + offsetMaster, partitionSize, 0);
        a = shmem_g(&a_symmetric, 0);

        for (int iter = 1; iter <= numPartitions; ++iter)
        {
            // ensure previous data transfer complete so it can be computed on
            shmem_quiet();
            // first begin the data transfer for the next iteration
            if (iter != numPartitions)
            {
                const int offset = offsetMaster + (partitionSize * (iter));
                const int localOffset = partitionSize * (iter);
                shmem_get_nbi(local + localOffset, main + offset, partitionSize, 0);
            }
            const int computeStart = (iter - 1) * partitionSize;
            const int computeEnd = (iter) * partitionSize; 

            for (int i = computeStart; i < computeEnd; ++i) 
            {
                local[i] *= a;
            }
        }
    }

    else // world_rank == MASTER
    {
        for (int i = 0; i < local_size; ++i)
        {
            local[i] = main[i] * a;
        }
    }
    shmem_barrier_all();

    shmem_collect32(main, local, local_size, 0, 0, world_size, pSync); // 32 for 32bit float

    /* END TIMING */
    clock_gettime(CLOCK, &end);

    if (world_rank  == MASTER)
    {
        free(test_main);
    }

    shmem_free(main);
    shmem_free(local);

    return elapsed_time(start, end);
}



double float_scalar_mult(const int size, const int local_size)
{
    int world_size, world_rank;
    world_size = shmem_n_pes();
    world_rank = shmem_my_pe();

    static float a_symmetric = 0.0f;

    /*  Create Vector and Scalar  */
    float* main = (float *) shmem_malloc(size*sizeof(float));
    float* local = (float *) shmem_malloc(local_size * sizeof (float));


    struct timespec start, end;

    if (world_rank == MASTER)
    {
        for (int i = 0; i < size; ++i)
        {
            main[i] = rand() / 10.0f;
        }
    }
    shmem_barrier_all();

    clock_gettime(CLOCK, &start);


    float a = 0.0f;
    a_symmetric = (world_rank+1) * rand() / 10.0f;

    if (world_rank != MASTER)
    {
        int offset = world_rank * local_size;
        shmem_get(local, main + offset, local_size, 0);
        a = shmem_g(&a_symmetric, 0);
    }
    else
    {
        a = a_symmetric;
    }


    /* perform scalar product */
    if (world_rank != MASTER)
    {
        for (int i = 0; i < local_size; i++) 
        {
            local[i] *= a;
        }
    }
    else
    {
        for (int i = 0; i < local_size; i++) 
        {
            main[i] *= a;
        } 
    }
    shmem_barrier_all();

    if (world_size > 1) // only broadcast if we actually should
    {
        shmem_fcollect32(main, local, local_size, 0, 0, world_size, pSync); // 32 for 32bit int
        // fixed collect because size is the same for all PEs
    }

    clock_gettime(CLOCK, &end);

    shmem_free(main);
    shmem_free(local);

    return elapsed_time(start, end);
}


double double_scalar_mult_overlap(const int size, const int numPartitions, const int local_size, const int partitionSize, const int masterLocal)
{
    int world_size, world_rank;
    world_size = shmem_n_pes();
    world_rank = shmem_my_pe();

    static double a_symmetric = 0.0;

    /*  Create Vector and Scalar  */
    double a = 0.0;
    a_symmetric = (world_rank+1) * rand() / 10.0;

    double* main = (double *) shmem_malloc(size*sizeof(double));

    // must allocate all with size (masterLocal >= local_size) 
    // shmem_malloc MUST be called collectively by ALL PEs and all arguments must be the same
    // otherwise, the standard defines this as UNDEFINED BEHAVIOR
    double* local = (double *) shmem_malloc(masterLocal * sizeof (double));


    struct timespec start, end;

    if (world_rank == MASTER)
    {
        for (int i = 0; i < size; ++i)
        {
            main[i] = rand() / 10.0;
        }
    }
    shmem_barrier_all();

     /* BEGIN TIMING */
    clock_gettime(CLOCK, &start);


    if (world_rank != MASTER)
    {
        const int offsetMaster = (world_rank) * masterLocal;
        shmem_get(local, main + offsetMaster, partitionSize, 0);
        a = shmem_g(&a_symmetric, 0);

        for (int iter = 1; iter <= numPartitions; ++iter)
        {
            // ensure previous data transfer complete so it can be computed on
            shmem_quiet();
            // first begin the data transfer for the next iteration
            if (iter != numPartitions)
            {
                const int offset = offsetMaster + (partitionSize * (iter));
                const int localOffset = partitionSize * (iter);
                shmem_get_nbi(local + localOffset, main + offset, partitionSize, 0);
            }
            const int computeStart = (iter - 1) * partitionSize;
            const int computeEnd = (iter) * partitionSize; 

            for (int i = computeStart; i < computeEnd; ++i) 
            {
                local[i] *= a;
            }
        }
    }

    else // world_rank == MASTER
    {
        for (int i = 0; i < local_size; ++i)
        {
            local[i] = main[i] * a;
        }
    }
    shmem_barrier_all();

    shmem_collect64(main, local, local_size, 0, 0, world_size, pSync); // 64 for 46bit double

    /* END TIMING */
    clock_gettime(CLOCK, &end);

    shmem_free(main);
    shmem_free(local);

    return elapsed_time(start, end);
}



double double_scalar_mult(const int size, const int local_size)
{
    int world_size, world_rank;
    world_size = shmem_n_pes();
    world_rank = shmem_my_pe();

    static double a_symmetric = 0.0;

    /*  Create Vector and Scalar  */
    double a = 0.0;
    a_symmetric = (world_rank+1) * rand() / 10.0;

    double* main = (double *) shmem_malloc(size*sizeof(double));
    double* local = (double *) shmem_malloc(local_size * sizeof (double));

    struct timespec start, end;

    if (world_rank == MASTER)
    {
        for (int i = 0; i < size; ++i)
        {
            main[i] = rand() / 10.0;
        }
    }
    shmem_barrier_all();

    /* BEGIN TIMING */
    clock_gettime(CLOCK, &start);


    if (world_rank != MASTER)
    {
        int offset = world_rank * local_size;
        shmem_get(local, main + offset, local_size, 0);
        a = shmem_g(&a_symmetric, 0);
    }
    else
    {
        a = a_symmetric;
    }

    /* perform scalar product */
    if (world_rank != MASTER)
    {
        for (int i = 0; i < local_size; i++) 
        {
            local[i] *= a;
        }
    }
    else
    {
        for (int i = 0; i < local_size; i++) 
        {
            main[i] *= a;
        } 
    }
    shmem_barrier_all();

    shmem_collect64(main, local, local_size, 0, 0, world_size, pSync); // 64 for 64bit double

    /* END TIMING */
    clock_gettime(CLOCK, &end);

    shmem_free(main);
    shmem_free(local);

    return elapsed_time(start, end);
}
