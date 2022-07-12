#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

#define BOUNDARY 25.0
#define ROWS 50
#define COLS 50

void intitialize(float *array, int rows, int cols);
void Write2File(float *C, int rows, int cols, char file_name[]);
void calculate_next_step(float* current, float* next, float* current_upper, float* current_lower, int rows, int cols);

int main(int argc, char** argv)
{
    int rank;
    int size;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int q = 20; // number of iterations between each break condition check
    int count = 0;
    int rows_segment = ROWS / size;
    float* current = (float*) malloc((ROWS * COLS) * sizeof(float));
    float* current_segment = (float*) malloc((rows_segment * COLS) * sizeof(float));
    float* next_segment = (float*) malloc((rows_segment * COLS) * sizeof(float));

    if (rank == 0)
    {
        intitialize(current, ROWS, COLS);
        Write2File(current, ROWS, COLS, "output/out_0");
    }
    MPI_Scatter(current, rows_segment * COLS, MPI_FLOAT, current_segment, rows_segment * COLS, MPI_FLOAT, 0, MPI_COMM_WORLD);

    float* current_upper = (float*) malloc(COLS * sizeof(float));
    float* current_lower = (float*) malloc(COLS * sizeof(float));
    float max_dif, max_dif_segment;
    float tolerance = 0.0001;
    while (1)
    {
        count += 1;
        // Commmunicate upper row
        if (rank == 0)
        {
            printf("I am process %d, count: %d\n", rank, count);
            for (int i = 0; i < COLS; i++)
            {
                *(current_upper + i) = BOUNDARY; 
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
                *(current_lower + i) = BOUNDARY; 
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
        calculate_next_step(current_segment, next_segment, current_upper, current_lower, rows_segment, COLS);

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
        
        MPI_Reduce(&max_dif_segment, &max_dif, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD);
        if (count % q == 0)
        {
            printf("I am process %d, it's morbing time, count:%d\n", rank, count);
            printf("I am process %d, local max: %f\n", rank, max_dif);
            MPI_Bcast(&max_dif, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
            printf("I am process %d, Max: %f\n", rank, max_dif);
            if (max_dif <= tolerance)
            {
                break;
            }
        }
        // for printing the output in each iteration
        // MPI_Gather(current_segment, rows_segment * COLS, MPI_FLOAT, current, rows_segment * COLS, MPI_FLOAT, 0, MPI_COMM_WORLD);
        // printf("I am process %d, Finished gathering\n", rank);
        // if (rank == 0)
        // {
        //     char file_name[10] = "out_";
        //     char count_str[10];
        //     sprintf(count_str, "%i", count);
        //     printf("%s\n", count_str);
        //     strcat(file_name, count_str);
        //     printf("%s\n", file_name);
        //     Write2File(current, ROWS, COLS, file_name);
        //     printf("I am process %d, Finished writing\n", rank);
        // }
    }
    printf("I am process %d, Out of loop\n", rank);
    MPI_Gather(current_segment, rows_segment * COLS, MPI_FLOAT, current, rows_segment * COLS, MPI_FLOAT, 0, MPI_COMM_WORLD);
    printf("I am process %d, Finished gathering\n", rank);
    if (rank == 0)
    {
        char file_name[20] = "output/out_";
        char count_str[10];
        sprintf(count_str, "%d", count);
        strcat(file_name, count_str);
        Write2File(current, ROWS, COLS, file_name);
        printf("I am process %d, Finished writing\n", rank);
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
                *(array + i * cols + j) = 80.0;
            else
                *(array + i * cols + j) = 25.0;
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
            float west = (j == 0) ? BOUNDARY : *(current + i * cols + j - 1);
            float east = (j == cols - 1) ? BOUNDARY : *(current + i * cols + j + 1);

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