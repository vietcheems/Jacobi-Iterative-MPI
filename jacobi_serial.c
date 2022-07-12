#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define BOUNDARY 25.0

void intitialize(float *array, int rows, int cols);
void Write2File(float *C, int rows, int cols);
void calculate_next_step(float* current, float* next, int rows, int cols);

int main(void)
{
    int rows = 50;
    int cols = 50;
    float* current = (float*) malloc((rows * cols) * sizeof(float));
    intitialize(current, rows, cols);
    float* next = (float*) malloc((rows * cols) * sizeof(float));

    float tolerance = 0.0001;
    int count = 0;
    float max_dif;
    do
    {
        calculate_next_step(current, next, rows, cols);
        max_dif = 0;
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                float dif = fabs(*(next + i * cols + j) - *(current + i * cols + j));
                if (dif > max_dif)
                {
                    max_dif = dif;
                }
                *(current + i * cols + j) = *(next + i * cols + j);
            }
        }
        count++;
    } while (max_dif > tolerance);

    Write2File(current, rows, cols);
    printf("Number of iterations: %d\n", count);
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

void calculate_next_step(float* current, float* next, int rows, int cols)
{
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            float north = (i == 0) ? BOUNDARY : *(current + (i - 1) * cols + j);
            float south = (i == rows - 1) ? BOUNDARY : *(current + (i + 1) * cols + j);
            float west = (j == 0) ? BOUNDARY : *(current + i * cols + j - 1);
            float east = (j == cols - 1) ? BOUNDARY : *(current + i * cols + j + 1);

            *(next + i * cols + j) = (north + south + west + east) / 4;
        }
    }
}

void Write2File(float *C, int rows, int cols)
{
    FILE *result = fopen("output/result_serial.txt", "w");
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