﻿#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <stdlib.h>

#define dimension 1 //размерность декартовой решетки

//вычисления максимального произведения соответствующих элементов строк
int calc_sum(int* a, int* b, int len)
{
	int sum = 0;
	for (int i = 0; i < len; i++)
		sum = sum + (a[i] * b[i]);
	return sum;
}

int main(int argc, char* argv[])
{
	int i, j, n, rank, rank_pred, rank_next, min, current;
	int dims[dimension], periods[dimension], new_coords[dimension];
	MPI_Comm new_comm;
	MPI_Status st;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &n);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	//Обнуляем массив dims и заполняем массив periods для топологии "кольцо" 

	for (i = 0; i < dimension; i++)
	{
		dims[i] = 0;
		periods[i] = 1;
	}
	MPI_Dims_create(n, dimension, dims);
	/*из методички:
		функция MPI_Dims_create позволяет выбрать сбалансированное
		распределение процессов по направлению координат, в зависимости от числа процессов в группе, и
		необязательных ограничений, которые могут быть определены пользователем

		Входные параметры:
			size - количество узлов в решетке;
			quantity_dims - размерность декартовой топологии.
		Выходные параметры:
			dims - целочисленный массив, определяющий количество узлов в каждой размерности.
	*/
	MPI_Cart_create(MPI_COMM_WORLD, dimension, dims, periods, 0, &new_comm);
	/* Поподробнее:
	MPI_COMM_WORLD - входной (старый) коммуникатор;
	quantity_dims - количество измерений в декартовой топологии;
	dims - целочисленный массив размером ndims, определяющий количество процессов в каждом измерении;
	periods - массив размером ndims логических значений, определяющих периодичность (true) или нет (false) в каждом измерении;
	reorder	- ранги могут быть перенумерованы (true) или нет (false).
	&comm - новый коммуникатор
	*/
	MPI_Cart_coords(new_comm, rank, dimension, new_coords);
	MPI_Cart_shift(new_comm, 0, -1, &rank_pred, &rank_next);

	int* A, * B;
	A = (int*)(malloc(sizeof(int) * n)); // размер строки = n
	B = (int*)(malloc(sizeof(int) * n)); // размер стролбца = n

	char str[300] = "A[", buf[5];
	_itoa(rank, buf, 10); strncat(str, buf, strlen(buf));
	strncat(str, "] = [", 5);
	//заполнение строки  А
	srand(time(NULL) + rank);
	for (i = 0; i < n; i++)
	{
		A[i] = rand() % 10;
		_itoa(A[i], buf, 10); strncat(str, buf, strlen(buf)); strncat(str, ", ", 2);
	}
	strncat(str, "]\nB[", 5); _itoa(rank, buf, 10); strncat(str, buf, strlen(buf)); strncat(str, "] = [", 5);
	//заполнение столбца B
	for (i = 0; i < n; i++)
	{
		B[i] = rand() % 10;
		_itoa(B[i], buf, 10); strncat(str, buf, strlen(buf)); strncat(str, "  ", 2);
	}
	strncat(str, "]\nResults:\n   sum(A[", 22); _itoa(rank, buf, 10); strncat(str, buf, strlen(buf));
	strncat(str, ",k]*B[k,0]) = ", 14);
	current = calc_sum(A, B, n);	// вычисление суммы произведений элементов для сформированных строк,
	min = current;				// берётся за текущий минимум
	_itoa(current, buf, 10); strncat(str, buf, strlen(buf)); strncat(str, "\n", 2);
	for (j = 1; j < n; j++) // расчёт для остальных столбцов матрицы В
	{
		MPI_Sendrecv_replace(B, n, MPI_INT, rank_next, 2, rank_pred, 2, new_comm, &st); //пересылка и получение нового столбца В
		current = calc_sum(A, B, n); // вычисление суммы произведений
		if (current <= min)
			min = current; // вычисление минимума

		// печать sum(A[i,k]*B[k,j]) = current
		strncat(str, "   sum(A[", 9); _itoa(rank, buf, 10); strncat(str, buf, strlen(buf));
		strncat(str, ",k]*B[k,", 8); _itoa(j, buf, 10); strncat(str, buf, strlen(buf));
		strncat(str, "]) = ", 5); _itoa(current, buf, 10); strncat(str, buf, strlen(buf)); strncat(str, "\n", 2);
	}
	// печать результата
	strncat(str, " Minimum = ", 11); _itoa(min, buf, 10); strncat(str, buf, strlen(buf)); strncat(str, "\n", 2); 
	printf("%s\n", str);

	MPI_Comm_free(&new_comm);
	free(A);
	free(B);
	MPI_Finalize();
	return 0;
}