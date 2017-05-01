#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/* Matrix size and process number */
#define MAT_SIZE 8192
#define PROCS 16

int main(int argc, char *argv[])
{
	/* Store total of processes and rank of this process */
	int size, rank;
	/* Store a constant which is used frequently */
	const int width = MAT_SIZE / PROCS;
	/*
	 * Create sub-matrix for this process to
	 * be transposed. Note that the matrix has
	 * been divided into PROCS parts, thus size
	 * of each part is MAT_SIZE * width.
	 *
	 * Attention:
	 * Assume that the matrix is divided by COLUMNs.
	 */
	int *src_mat = (int*)malloc(MAT_SIZE * width * sizeof(int));
	int *des_mat = (int*)malloc(MAT_SIZE * width * sizeof(int));
	/* Used for timing */
	double begin, end;

	/*
	 * Initialize MPI and get rank number of this
	 * process and total number of processes.
	 */
	MPI_Init (&argc, &argv);
	MPI_Comm_size (MPI_COMM_WORLD, &size);
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);

	/* Check if process number is PROC_NUM, that is, 16 */
	if (size != PROCS) {
		if (rank == 0)
			printf ("Run with option 'mpirun -np %d'!", PROCS);
		MPI_Finalize ();
		return 1;
	}

	/* Initialize values in the matrix */
	for (int i = 0; i < MAT_SIZE; i++)
		for (int j = 0; j < width; j++)
			/*
			 * Assign unique values for each element which
			 * will be convenient for checking afterwards.
			 *
			 * Map the whole matrix to a 1-Dimensional array,
			 * and the unique value for each element equals
			 * its subscript in the 1-D array.
			 */
			src_mat[i * width + j] =
			    i * MAT_SIZE + rank * width + j;

	/* Begin timing */
	begin = MPI_Wtime();

	/*
	 * Here in this process we send data blocks to
	 * all processes in MPI_COMM_WORLD. And we'll
	 * receive date blocks from all processes as well.
	 */
	MPI_Alltoall(src_mat, width * width, MPI_INT,
	             des_mat, width * width, MPI_INT,
	             MPI_COMM_WORLD);

	/*
	 * After get data blocks from other processes,
	 * it's time to transpose each of them (each
	 * block is a sub-matrix).
	 */
	int exch_1, exch_2, temp;
	for (int i = 0; i < PROCS; i++)
		for (int j = 0; j < width; j++)
			for (int k = 0; k < j; k++) {
				exch_1 = i * width * width + j * width + k;
				exch_2 = i * width * width + k * width + j;
				temp = des_mat[exch_1];
				des_mat[exch_1] = des_mat[exch_2];
				des_mat[exch_2] = temp;
			}

	/* End timing */
	end = MPI_Wtime();

	/*
	 * Check whether the result is right in every
	 * process. Once there's some value that does
	 * not match, error message will be printed out
	 * and process would be aborted.
	 */
	for (int i = 0; i < MAT_SIZE; i++)
		for (int j = 0; j < width; j++)
			/*
			 * Note that if transposition is successful,
			 * now the element in row i, column j should
			 * be from row j, column i. originally. Take
			 * rank into consideration, then column
			 * changes from rank * width + j to i.
			 */
			if (des_mat[i * width + j] !=
			        MAT_SIZE * (rank * width + j) + i ) {
				printf("Element wrong.\n");
				MPI_Abort (MPI_COMM_WORLD, 1);
				return 1;
			}

	/* If no problem, then the first process prints result out */
	if (rank == 0)
		printf ("Result correct.\n");

	/* Only the first process prints the time */
	if (rank == 0)
		printf("The main time-cost is %lf s.\n", end - begin);

	/* Free memory */
	free(src_mat);
	free(des_mat);
	src_mat = des_mat = NULL;

	MPI_Finalize ();

	return 0;
}