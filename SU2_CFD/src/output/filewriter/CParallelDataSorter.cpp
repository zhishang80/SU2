#include "../../../include/output/filewriter/CParallelDataSorter.hpp"

CParallelDataSorter::CParallelDataSorter(CConfig *config, unsigned short nFields){
  
  rank = SU2_MPI::GetRank();
  size = SU2_MPI::GetSize();
  
  GlobalField_Counter = nFields;
  
  nParallel_Hexa = 0;
  nParallel_Line = 0;
  nParallel_Quad = 0;
  nParallel_Tetr = 0;
  nParallel_Pris = 0;
  nParallel_Pyra = 0;
  nParallel_Tria = 0;
  
  Conn_Line_Par = NULL;
  Conn_Hexa_Par = NULL;
  Conn_Pris_Par = NULL;
  Conn_Quad_Par = NULL;
  Conn_Tetr_Par = NULL;
  Conn_Tria_Par = NULL;
  Conn_Pyra_Par = NULL;
  
  Parallel_Data = NULL;
  
  nPoint_Send  = NULL;
  nPoint_Recv  = NULL;
  Index        = NULL;
  connSend     = NULL;
  idSend       = NULL;
  nSends = 0;
  nRecvs = 0;
  
  nLocalPoint_Sort  = 0;
  nGlobalPoint_Sort = 0;
  
  nPoint_Send = new int[size+1]; nPoint_Send[0] = 0;
  nPoint_Recv = new int[size+1]; nPoint_Recv[0] = 0;
  
  for (int ii=0; ii < size; ii++) {
    nPoint_Send[ii] = 0;
    nPoint_Recv[ii] = 0;
  }
  nPoint_Send[size] = 0; nPoint_Recv[size] = 0;

}

CParallelDataSorter::~CParallelDataSorter(){
  
  if (nPoint_Send != NULL) delete [] nPoint_Send;
  if (nPoint_Recv != NULL) delete [] nPoint_Recv;
  
  DeallocateConnectivity();
  
  DeallocateData();
  
}

void CParallelDataSorter::CreateLinearPartition(unsigned long nGlobalPoint){
    
  /*--- Now that we know the actual number of points we need to output,
   compute the number of points that will be on each processor.
   This is a linear partitioning with the addition of a simple load
   balancing for any remainder points. ---*/

  beg_node = new unsigned long[size];
  end_node = new unsigned long[size];

  nPoint_Lin = new unsigned long[size];
  nPoint_Cum = new unsigned long[size+1];

  unsigned long total_points = 0;
  for (int ii = 0; ii < size; ii++) {
    nPoint_Lin[ii] = nGlobalPoint/size;
    total_points  += nPoint_Lin[ii];
  }

  /*--- Get the number of remainder points after the even division. ---*/

  unsigned long remainder = nGlobalPoint - total_points;
  for (unsigned long ii = 0; ii < remainder; ii++) {
    nPoint_Lin[ii]++;
  }

  /*--- Store the local number of nodes on each proc in the linear
   partitioning, the beginning/end index, and the linear partitioning
   within an array in cumulative storage format. ---*/

  beg_node[0] = 0;
  end_node[0] = beg_node[0] + nPoint_Lin[0];
  nPoint_Cum[0] = 0;
  for (int ii = 1; ii < size; ii++) {
    beg_node[ii]   = end_node[ii-1];
    end_node[ii]   = beg_node[ii] + nPoint_Lin[ii];
    nPoint_Cum[ii] = nPoint_Cum[ii-1] + nPoint_Lin[ii-1];
  }
  nPoint_Cum[size] = nGlobalPoint;
  
}

unsigned short CParallelDataSorter::FindProcessor(unsigned long global_index){
  
  unsigned short iProcessor = global_index/nPoint_Lin[0];
  if (iProcessor >= (unsigned long)size)
    iProcessor = (unsigned long)size-1;
  if (global_index >= nPoint_Cum[iProcessor])
    while(global_index >= nPoint_Cum[iProcessor+1]) iProcessor++;
  else
    while(global_index <  nPoint_Cum[iProcessor])   iProcessor--;
  
  return iProcessor;
}

unsigned long CParallelDataSorter::GetnElem(GEO_TYPE type){
  
  switch (type) {
    case LINE:
      return nParallel_Line;
      break;
    case TRIANGLE:
      return nParallel_Tria;
      break;
    case QUADRILATERAL:
      return nParallel_Quad;
      break;
    case TETRAHEDRON:
      return nParallel_Tetr;
      break;
    case HEXAHEDRON:
      return nParallel_Hexa;
      break;
    case PRISM:
      return nParallel_Pris;
      break;
    case PYRAMID:
      return nParallel_Pyra;
      break;
    default:
      break;
  }
  
  SU2_MPI::Error("GEO_TYPE not found", CURRENT_FUNCTION);
  
  return 0;
}


void CParallelDataSorter::PrepareSendBuffers(std::vector<unsigned long>& globalID){
  
  unsigned long iPoint;
  unsigned short iProcessor;
  
  int VARS_PER_POINT = GlobalField_Counter;
  
  /*--- We start with the grid nodes distributed across all procs with
   no particular ordering assumed. We need to loop through our local partition
   and decide how many nodes we must send to each other rank in order to
   have all nodes sorted according to a linear partitioning of the grid
   nodes, i.e., rank 0 holds the first ~ nGlobalPoint()/nProcessors nodes.
   First, initialize a counter and flag. ---*/
  
  for (iPoint = 0; iPoint < nLocalPoint_Sort; iPoint++ ) {
    
    iProcessor = FindProcessor(globalID[iPoint]);
    
    /*--- If we have not visited this node yet, increment our
       number of elements that must be sent to a particular proc. ---*/
    
    nPoint_Send[iProcessor+1]++;
  }
  
  /*--- Communicate the number of nodes to be sent/recv'd amongst
   all processors. After this communication, each proc knows how
   many cells it will receive from each other processor. ---*/
  
#ifdef HAVE_MPI
  SU2_MPI::Alltoall(&(nPoint_Send[1]), 1, MPI_INT,
                    &(nPoint_Recv[1]), 1, MPI_INT, MPI_COMM_WORLD);
#else
  nPoint_Recv[1] = nPoint_Send[1];
#endif
  
  /*--- Prepare to send coordinates. First check how many
   messages we will be sending and receiving. Here we also put
   the counters into cumulative storage format to make the
   communications simpler. ---*/
  
  nSends = 0; nRecvs = 0;

  for (int ii = 0; ii < size; ii++) {
    if ((ii != rank) && (nPoint_Send[ii+1] > 0)) nSends++;
    if ((ii != rank) && (nPoint_Recv[ii+1] > 0)) nRecvs++;
    
    nPoint_Send[ii+1] += nPoint_Send[ii];
    nPoint_Recv[ii+1] += nPoint_Recv[ii];
  }
  
  /*--- Allocate memory to hold the connectivity that we are
   sending. ---*/
  
  connSend = NULL;
  connSend = new su2double[VARS_PER_POINT*nPoint_Send[size]];
  for (int ii = 0; ii < VARS_PER_POINT*nPoint_Send[size]; ii++)
    connSend[ii] = 0;
  
  /*--- Allocate arrays for sending the global ID. ---*/
  
  idSend = new unsigned long[nPoint_Send[size]];
  for (int ii = 0; ii < nPoint_Send[size]; ii++)
    idSend[ii] = 0;
  
  /*--- Create an index variable to keep track of our index
   positions as we load up the send buffer. ---*/
  
  unsigned long *index = new unsigned long[size];
  for (int ii=0; ii < size; ii++) index[ii] = VARS_PER_POINT*nPoint_Send[ii];
  
  unsigned long *idIndex = new unsigned long[size];
  for (int ii=0; ii < size; ii++) idIndex[ii] = nPoint_Send[ii];
  
  Index = new unsigned long[nLocalPoint_Sort];

  /*--- Loop through our elements and load the elems and their
   additional data that we will send to the other procs. ---*/
  
  for (iPoint = 0; iPoint < nLocalPoint_Sort; iPoint++) {
    
    iProcessor = FindProcessor(globalID[iPoint]);

    /*--- Load the global ID (minus offset) for sorting the
         points once they all reach the correct processor. ---*/
    
    unsigned long nn = idIndex[iProcessor];
    idSend[nn] = globalID[iPoint] - beg_node[iProcessor];
    
    /*--- Store the index this point has in the send buffer ---*/
    
    Index[iPoint] = index[iProcessor]; 
    
    /*--- Increment the index by the message length ---*/
    
    index[iProcessor]  += VARS_PER_POINT;
    idIndex[iProcessor]++;


  }
  
  /*--- Free memory after loading up the send buffer. ---*/
  
  delete [] index;
  delete [] idIndex;
}

unsigned long CParallelDataSorter::GetElem_Connectivity(GEO_TYPE type, unsigned long iElem, unsigned long iNode) {
  
  switch (type) {
    case LINE:
      return Conn_Line_Par[iElem*N_POINTS_LINE + iNode];
      break;
    case TRIANGLE:
      return Conn_Tria_Par[iElem*N_POINTS_TRIANGLE + iNode];
      break;
    case QUADRILATERAL:
      return Conn_Quad_Par[iElem*N_POINTS_QUADRILATERAL + iNode];
      break;
    case TETRAHEDRON:
      return Conn_Tetr_Par[iElem*N_POINTS_TETRAHEDRON + iNode];
      break;
    case HEXAHEDRON:
      return Conn_Hexa_Par[iElem*N_POINTS_HEXAHEDRON + iNode];
      break;
    case PRISM:
      return Conn_Pris_Par[iElem*N_POINTS_PRISM + iNode];
      break;
    case PYRAMID:
      return Conn_Pyra_Par[iElem*N_POINTS_PYRAMID + iNode];
      break;
    default:
      break;
  }
  
  SU2_MPI::Error("GEO_TYPE not found", CURRENT_FUNCTION);
  
  return 0;
}

void CParallelDataSorter::DeallocateConnectivity() {
  
  /*--- Deallocate memory for connectivity data on each processor. ---*/
  
  if (nParallel_Line > 0 && Conn_Line_Par != NULL) delete [] Conn_Line_Par;
  if (nParallel_Tria > 0 && Conn_Tria_Par != NULL) delete [] Conn_Tria_Par;
  if (nParallel_Quad > 0 && Conn_Quad_Par != NULL) delete [] Conn_Quad_Par;
  if (nParallel_Tetr > 0 && Conn_Tetr_Par != NULL) delete [] Conn_Tetr_Par;
  if (nParallel_Hexa > 0 && Conn_Hexa_Par != NULL) delete [] Conn_Hexa_Par;
  if (nParallel_Pris > 0 && Conn_Pris_Par != NULL) delete [] Conn_Pris_Par;
  if (nParallel_Pyra > 0 && Conn_Pyra_Par != NULL) delete [] Conn_Pyra_Par;
  
  
}

void CParallelDataSorter::DeallocateData() {
  
  /*--- Deallocate memory for solution data ---*/
  
  for (unsigned short iVar = 0; iVar < GlobalField_Counter; iVar++) {
    if (Parallel_Data[iVar] != NULL) delete [] Parallel_Data[iVar];
  }
  if (Parallel_Data != NULL) delete [] Parallel_Data;

}