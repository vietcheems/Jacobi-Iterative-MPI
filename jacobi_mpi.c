#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

// change hyperparams here
#define TOP_BOUNDARY 100
#define BOTTOM_BOUNDARY 0
#define LEFT_BOUNDARY 0
#define RIGHT_BOUNDARY 100
#define ROWS 50
#define COLS 50
#define Q 20 // number of iterations between each break condition check
#define TOLERANCE 0.0001

void intitialize(float *array, int rows, int cols);
void Write2File(float *C, int rows, int cols, char file_name[]);
void calculate_next_step(float* current, float* next, float* current_upper, float* current_lower, int rows, int cols);

int main(int argc, char** argv)
{
    // initialize MPI
    int rank;
    int size;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Decompose M x N array into M/Ms arrays of size Ms x N and send to Ms processes
    int count = 0;
    int rows_segment = ROWS / size;
    float* current = (float*) malloc((ROWS * COLS) * sizeof(float));
    float* current_segment = (float*) malloc((rows_segment * COLS) * sizeof(float));
    float* next_segment = (float*) malloc((rows_segment * COLS) * sizeof(float));

    if (rank == 0)
    {
        // initialize the matrix
        intitialize(current, ROWS, COLS);
        Write2File(current, ROWS, COLS, "output/out_0");
    }
    MPI_Scatter(current, rows_segment * COLS, MPI_FLOAT, current_segment, rows_segment * COLS, MPI_FLOAT, 0, MPI_COMM_WORLD);

    float* current_upper = (float*) malloc(COLS * sizeof(float));
    float* current_lower = (float*) malloc(COLS * sizeof(float));
    float max_dif, max_dif_segment;
    while (1)
    {
        count += 1;
        // Commmunicate upper row
        if (rank == 0)
        {
            for (int i = 0; i < COLS; i++)
            {
                *(current_upper + i) = TOP_BOUNDARY;
            }
            MPI_Send(current_segment + (rows_segment - 1) * COLS, COLS, MPI_FLOAT,
            rank + 1, rank, MPI_COMM_WORLD);
        }
        else if (rank == size - 1)
        {
            MPI_Recv(current_upper, COLS, MPI_FLOAT, rank - 1, rank - 1, MPI_COMM_WORLD, &status);
        }
        else
        {
            MPI_Send(current_segment + (rows_segment - 1) * COLS, COLS, MPI_FLOAT,
            rank + 1, rank, MPI_COMM_WORLD);
            MPI_Recv(current_upper, COLS, MPI_FLOAT, rank - 1, rank - 1, MPI_COMM_WORLD, &status);
        }

        // Communicate lower row
        if (rank == size - 1)
        {
            for (int i = 0; i < COLS; i++)
            {
                *(current_lower + i) = BOTTOM_BOUNDARY;
            }
            MPI_Send(current_segment, COLS, MPI_FLOAT, rank - 1, rank, MPI_COMM_WORLD);
        }
        else if (rank == 0)
        {
            MPI_Recv(current_lower, COLS, MPI_FLOAT, rank + 1, rank + 1, MPI_COMM_WORLD, &status);
        }
        else
        {
            MPI_Send(current_segment, COLS, MPI_FLOAT, rank - 1, rank, MPI_COMM_WORLD);
            MPI_Recv(current_lower, COLS, MPI_FLOAT, rank + 1, rank + 1, MPI_COMM_WORLD, &status);
        }
        // calculate next iteration according to the formula
        calculate_next_step(current_segment, next_segment, current_upper, current_lower, rows_segment, COLS);

        // calculate local max difference between 2 iterations 
        max_dif_segment = 0;
        for (int i = 0; i < rows_segment; i++)
        {
            for (int j = 0; j < COLS; j++)
            {
                float dif = fabs(*(next_segment + i * COLS + j) - *(current_segment + i * COLS + j));
                if (dif > max_dif_segment)
                {
                    max_dif_segment = dif;
                }
                *(current_segment + i * COLS + j) = *(next_segment + i * COLS + j);
            }
        }
        if (count % Q == 0)
        {
            // calculate global max difference and broadcast it to all processes
            MPI_Allreduce(&max_dif_segment, &max_dif, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
            // check stop condition
            if (max_dif <= TOLERANCE)
            {
                break;
            }
            // saving the output in each iteration, dont run this in reality
            MPI_Gather(current_segment, rows_segment * COLS, MPI_FLOAT, current, rows_segment * COLS, MPI_FLOAT, 0, MPI_COMM_WORLD);
            if (rank == 0)
            {
                char file_name[30] = "output/out_";
                char count_str[10];
                sprintf(count_str, "%i", count);
                strcat(file_name, count_str);
                Write2File(current, ROWS, COLS, file_name);
            }
        }
    }
    // save the final output
    MPI_Gather(current_segment, rows_segment * COLS, MPI_FLOAT, current, rows_segment * COLS, MPI_FLOAT, 0, MPI_COMM_WORLD);
    if (rank == 0)
    {
        char file_name[30] = "output/out_";
        char count_str[10];
        sprintf(count_str, "%d", count);
        strcat(file_name, count_str);
        Write2File(current, ROWS, COLS, file_name);
    }
    MPI_Finalize();
    return 0;
}

void intitialize(float *array, int rows, int cols)
{
    int i, j;
    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < cols; j++)
        {
            if (i >= (rows / 2 - 5) && i < (rows / 2 + 5) && j >= (cols / 2 - 5) && j < (cols / 2 + 5))
                *(array + i * cols + j) = 0;
            else
                *(array + i * cols + j) = 0;
        }
    }
}

void calculate_next_step(float* current, float* next, float* current_upper, float* current_lower, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            float north = (i == 0) ? *(current_upper + j) : *(current + (i - 1) * cols + j);
            float south = (i == rows - 1) ? *(current_lower + j) : *(current + (i + 1) * cols + j);
            float west = (j == 0) ? LEFT_BOUNDARY : *(current + i * cols + j - 1);
            float east = (j == cols - 1) ? RIGHT_BOUNDARY : *(current + i * cols + j + 1);

            *(next + i * cols + j) = (north + south + west + east) / 4;
        }
    }
}

void Write2File(float *C, int rows, int cols, char file_name[])
{
    FILE *result = fopen(file_name, "w");
    int i,j;

    for(i = 0; i <rows; i++) 
    {
        for(j = 0; j < cols; j++) 
        { 
            fprintf(result, "%lf\t", *(C+i*cols+j));
        }
        fprintf(result, "\n");
    }

    fclose(result);
}