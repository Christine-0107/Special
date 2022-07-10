#include <iostream>
#include <fstream>
#include <sstream>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>
#include "mpi.h"
#define tag 0
#define comm MPI_COMM_WORLD
using namespace std;
const int col = 2700,row = 60000;
int n, m1, m2;
int count_eliminated = 0;
struct bitset {
	unsigned int bits[col];
	int lp = -1;
	void addNum(int num) 
	{
		int n1 = num / 32, n2 = num % 32;
		bits[n1] += (0x80000000 >> n2);
		lp = max(lp, num);
		n = max(n, n1 + 1);
	}
};
bitset R[row], E[row];
void print(bitset& b, ofstream& of) {
	if (b.lp == -1) {
		of << endl;
		return;
	}
	for (int i = n; i >= 0; --i) {
		for (unsigned int temp = 1, j = 31; temp != 0; temp <<= 1, --j)
			if (temp & b.bits[i])
				of << i * 32 + j << " ";
	}
	of << endl;
}
void Xor(bitset& b1, bitset& b2) {
	for (int i = n; i >= 0; --i) {
		b1.bits[i] ^= b2.bits[i];
	}
	int i = n, j = 31;
	while (b1.bits[i] == 0 && i >= 0)--i;
	if (i < 0) {
		b1.lp = -1;
		return;
	}
	unsigned temp = 1;
	while ((b1.bits[i] & temp) == 0)temp <<= 1, --j;
	b1.lp = 32 * i + j;
}
void readData() {
	string file1 = "2.txt", file2 = "1.txt";
	fstream fs1(file1);
	string line;
	while (getline(fs1, line)) {
		stringstream ss(line);
		int index, num;
		ss >> index;
		R[index].addNum(index);
		while (ss >> num)
			R[index].addNum(num);
		++m1;
	}
	fs1.close();
	fs1.open(file2);
	while (getline(fs1, line)) {
		stringstream ss(line);
		int num;
		while (ss >> num)
			E[m2].addNum(num);
		++m2;
	}
	cout << m2;
	fs1.close();
}

bool ifPrint = false;

const int NUM_THREADS = 3;
sem_t sem_main;
sem_t sem_workstart[NUM_THREADS];
sem_t sem_workend[NUM_THREADS];

void grobner() {
	for (int i = 0; i < m2; ++i) {
		while (E[i].lp != -1) {
			if (R[E[i].lp].lp != -1) {
				Xor(E[i], R[E[i].lp]);
			}

			else {
				R[E[i].lp] = E[i];
				break;
			}
		}
	}
}
void mpi_grobner() {
	MPI_Init(NULL, NULL);
	int rank, size,begin,end;
	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &size);

	begin = rank * (m2 / size);
	if (rank == size - 1)end = m2;
	else end = (rank + 1) * (m2 / size);

	while (count_eliminated < m2) {
		for (int i = max(count_eliminated,begin); i < end; ++i) {
			while (E[i].lp != -1)
				if (R[E[i].lp].lp != -1) {
					Xor(E[i], R[E[i].lp]);
				}
				else break;
		}
		

		MPI_Barrier(comm);
		
		int sendRank = min(count_eliminated / (m2 / size), size - 1);
		if (rank == sendRank) {
			while (E[count_eliminated].lp != -1) {
				if (R[E[count_eliminated].lp].lp != -1) {
					Xor(E[count_eliminated], R[E[count_eliminated].lp]);
				}
				else {
					R[E[count_eliminated].lp] = E[count_eliminated];
					break;
				}
			}
		}
		MPI_Bcast(&E[count_eliminated], sizeof(bitset), MPI_BYTE, sendRank, comm);
		MPI_Bcast(&R[E[count_eliminated].lp], sizeof(bitset), MPI_BYTE, sendRank, comm);
		++count_eliminated;//the order of this three line matters
		
	}
	
	if (rank != size - 1)
		MPI_Send(&E[begin], (end-begin)* sizeof(bitset), MPI_BYTE, size - 1, tag, comm);
	else {
		for (int i = 0; i < size - 1; ++i)
			MPI_Recv(&E[i * (m2 / size)], (end - begin) * sizeof(bitset), MPI_BYTE, i,tag, comm, MPI_STATUS_IGNORE);
		ifPrint = true;
	}
	//关键在于每个消元行和消元子的结合顺序不能变
	MPI_Finalize();
}
struct threadParam {
	int t_id;
	int begin;
	int end;
};
void* threadFuncXor(void* param) {
	threadParam* p = (threadParam*)param;
	int t_id = p->t_id;
	int begin = p->begin;
	int end = p->end;
	while (count_eliminated < m2) {
		sem_wait(&sem_workstart[t_id]);
		for (int i = max(count_eliminated, begin); i < end; i+=NUM_THREADS) {
			while (E[i].lp != -1)
				if (R[E[i].lp].lp != -1) {
					Xor(E[i], R[E[i].lp]);
				}
				else break;
		}
		sem_post(&sem_main);
		sem_wait(&sem_workend[t_id]);
	}
	pthread_exit(NULL);
	return nullptr;
}
void mpi_pthread_grobner() {
	MPI_Init(NULL, NULL);
	int rank, size, begin, end;
	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &size);

	begin = rank * (m2 / size);
	if (rank == size - 1)end = m2;
	else end = (rank + 1) * (m2 / size);

	sem_init(&sem_main, 0, 0);
	for (int i = 0; i < NUM_THREADS; ++i) {
		sem_init(&sem_workstart[i], 0, 0);
		sem_init(&sem_workend[i], 0, 0);
	}
	pthread_t handles[NUM_THREADS];
	threadParam params[NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; ++i) {
		params[i].t_id = i;
		params[i].begin = begin;
		params[i].end = end;
		pthread_create(&handles[i], NULL, threadFuncXor, &params[i]);
	}
	

	while (count_eliminated < m2) {
		

		for (int i = 0; i < NUM_THREADS; ++i)
			sem_post(&sem_workstart[i]);

		for (int i = 0; i < NUM_THREADS; ++i)
			sem_wait(&sem_main);

		int sendRank = min(count_eliminated / (m2 / size), size - 1);
		if (rank == sendRank) {
			while (E[count_eliminated].lp != -1) {
				if (R[E[count_eliminated].lp].lp != -1) {
					Xor(E[count_eliminated], R[E[count_eliminated].lp]);
				}
				else {
					R[E[count_eliminated].lp] = E[count_eliminated];
					break;
				}
			}
		}
		MPI_Bcast(&E[count_eliminated], sizeof(bitset), MPI_BYTE, sendRank, comm);
		MPI_Bcast(&R[E[count_eliminated].lp], sizeof(bitset), MPI_BYTE, sendRank, comm);
		++count_eliminated;

		for (int i = 0; i < NUM_THREADS; ++i)
			sem_post(&sem_workend[i]);
	}

	if (rank != size - 1)
		MPI_Send(&E[begin], (end - begin) * sizeof(bitset), MPI_BYTE, size - 1, tag, comm);
	else {
		for (int i = 0; i < size - 1; ++i)
			MPI_Recv(&E[i * (m2 / size)], (end - begin) * sizeof(bitset), MPI_BYTE, i, tag, comm, MPI_STATUS_IGNORE);
		ifPrint = true;
	}
	//关键在于每个消元行和消元子的结合顺序不能变
	MPI_Finalize();
}
void mpi_omp_grobner() {
	MPI_Init(NULL, NULL);
	int rank, size, begin, end;
	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &size);

	begin = rank * (m2 / size);
	if (rank == size - 1)end = m2;
	else end = (rank + 1) * (m2 / size);

	while (count_eliminated < m2) {
#pragma omp parallel for num_threads(NUM_THREADS)
		for (int i = max(count_eliminated, begin); i < end; ++i) {
			while (E[i].lp != -1)
				if (R[E[i].lp].lp != -1) {
					Xor(E[i], R[E[i].lp]);
				}
				else break;
		}


		MPI_Barrier(comm);

		int sendRank = min(count_eliminated / (m2 / size), size - 1);
		if (rank == sendRank) {
			while (E[count_eliminated].lp != -1) {
				if (R[E[count_eliminated].lp].lp != -1) {
					Xor(E[count_eliminated], R[E[count_eliminated].lp]);
				}
				else {
					R[E[count_eliminated].lp] = E[count_eliminated];
					break;
				}
			}
		}
		MPI_Bcast(&E[count_eliminated], sizeof(bitset), MPI_BYTE, sendRank, comm);
		MPI_Bcast(&R[E[count_eliminated].lp], sizeof(bitset), MPI_BYTE, sendRank, comm);
		++count_eliminated;//the order of this three line matters

	}

	if (rank != size - 1)
		MPI_Send(&E[begin], (end - begin) * sizeof(bitset), MPI_BYTE, size - 1, tag, comm);
	else {
		for (int i = 0; i < size - 1; ++i)
			MPI_Recv(&E[i * (m2 / size)], (end - begin) * sizeof(bitset), MPI_BYTE, i, tag, comm, MPI_STATUS_IGNORE);
		ifPrint = true;
	}
	//关键在于每个消元行和消元子的结合顺序不能变
	MPI_Finalize();
}
int main() 
{
	readData();
	double time;
	struct timeval t1,t2;
	gettimeofday(&t1,NULL);
	mpi_grobner();
	gettimeofday(&t2,NULL);
	time=(double)(t2.tv_sec-t1.tv_sec)*1000.0 + (double)(t2.tv_usec-t1.tv_usec)/1000.0;
	cout<<time<<endl;
	ofstream of("output.txt");
	for (int i = 0; i < m2; ++i)
		print(E[i], of);
	of.close();
}