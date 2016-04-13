
#ifndef __JACOBI_H__
#define __JACOBI_H__

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>

/**
 * @file Jacobi.h
 * @brief The header containing the most relevant functions for the Jacobi solver
 */

// =========
// Constants
// =========

/**
 * Setting this to 1 makes the application use only single-precision floating-point data. Set this to 
 * 0 in order to use double-precision floating-point data instead.
 */
#define USE_FLOAT 0		
#define DEFAULT_DOMAIN_SIZE 4096	
#define MIN_DOM_SIZE 3		

/**
 * This is the environment variable which allows the reading of the local rank of the current MPI
 * process before the MPI environment gets initialized with MPI_Init(). This is necessary when running
 * the CUDA-aware MPI version of the Jacobi solver, which needs this information in order to be able to
 * set the CUDA device for the MPI process before MPI environment initialization. If you are using MVAPICH2, 
 * set this constant to "MV2_COMM_WORLD_LOCAL_RANK"; for Open MPI, use "OMPI_COMM_WORLD_LOCAL_RANK".  
 */
#define ENV_LOCAL_RANK "MV2_COMM_WORLD_LOCAL_RANK"
#define MPI_MASTER_RANK 0		
#define	JACOBI_TOLERANCE 1.0E-5F	
#define JACOBI_MAX_LOOPS 1000	

#define DIR_TOP		0
#define DIR_RIGHT	1
#define DIR_BOTTOM	2
#define DIR_LEFT	3

/**
 * This is the status value that indicates a successful operation.
 */
#define STATUS_OK 			0

/**
 * This is the status value that indicates an error.
 */
#define STATUS_ERR			-1

#if USE_FLOAT
X	#define real	float
	#define MPI_CUSTOM_REAL		MPI_FLOAT
#else
	#define real	double
	#define MPI_CUSTOM_REAL		MPI_DOUBLE
#endif		

#define uint64					unsigned long long

#define SafeCudaCall(call) 		CheckCudaCall(call, #call, __FILE__, __LINE__)
#define SafeHostFree(block)		{ if (block) free(block); }
#define SafeDevFree(block)		{ if (block) SafeCudaCall(cudaFree(block)); }
#define OnePrintf(allow, ...)		{ if (allow) printf(__VA_ARGS__); }
#define OneErrPrintf(allow, ...)	{ if (allow) fprintf(stderr, __VA_ARGS__); }
#define HasNeighbor(neighbors, dir)	(neighbors[dir] != MPI_PROC_NULL)

// =============
// Host routines
// =============

void Initialize(int * argc, char *** argv, int * rank, int * size);
void Finalize(real * devBlocks[2], real * devSideEdges[2], real * devHaloLines[2], real * hostSendLines[2], real * hostRecvLines[2], real * devResidue, cudaStream_t copyStream);

int ParseCommandLineArguments(int argc, char ** argv, int rank, int size, int2 * domSize, int2 * topSize, int * useFastSwap);
int ApplyTopology(int * rank, int size, const int2 * topSize, int * neighbors, int2 * topIndex, MPI_Comm * cartComm);
void InitializeDataChunk(int topSizeY, int topIdxY, const int2 * domSize, const int * neighbors, cudaStream_t * copyStream, real * devBlocks[2], 
		real * devSideEdges[2], real * devHaloLines[2], real * hostSendLines[2], real * hostRecvLines[2], real ** devResidue);

void PreRunJacobi(MPI_Comm cartComm, int rank, int size, double * timerStart);
void RunJacobi(MPI_Comm cartComm, int rank, int size, const int2 * domSize, const int2 * topIndex, const int * neighbors, int useFastSwap, 
		real * devBlocks[2], real * devSideEdges[2], real * devHaloLines[2], real * hostSendLines[2], real * hostRecvLines[2], real * devResidue,
		cudaStream_t copyStream, int * iterations, double * avgTransferTime);
void PostRunJacobi(MPI_Comm cartComm, int rank, int size, const int2 * topSize, const int2 * domSize, int iterations, int useFastSwap, double timerStart, double avgTransferTime);

void SetDeviceBeforeInit();
void SetDeviceAfterInit(int rank);
void SafeCheckMPIStatus(MPI_Status * status, int expectedElems);
void ExchangeHalos(MPI_Comm cartComm, real * devSend, real * hostSend, real * hostRecv, real * devRecv, int neighbor, int elemCount);

// ===============
// Device wrappers
// ===============

#ifdef __cplusplus
extern "C" 
{
#endif

void CheckCudaCall(cudaError_t command, const char * commandName, const char * fileName, int line);
real CallJacobiKernel(real * devBlocks[2], real * devResidue, const int4 * bounds, const int2 * size);
void CopyDeviceBlock(real * devBlocks[2], const int4 * bounds, const int2 * size);
void CopyDevHalosToBlock(real * devBlock, const real * devHaloLineLeft, const real * devHaloLineRight, const int2 * size, const int * neighbors);
void CopyDevSideEdgesFromBlock(const real * devBlock, real * devSideEdges[2], const int2 * size, const int * neighbors, cudaStream_t copyStream);

#ifdef __cplusplus
}	// extern "C"
#endif

#endif	// __JACOBI_H__
