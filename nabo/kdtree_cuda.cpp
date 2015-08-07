// CUDA kdtree implementation cpp file

#ifdef HAVE_CUDA

#include "nabo_private.h"
#include "index_heap.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <limits>
#include <queue>
#include <algorithm>
// #include <map>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/limits.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>

namespace Nabo
{
	template<typename T>
	CUDASearch<T>::CUDASearch(const Matrix& cloud, const Index dim, const unsigned creationOptionFlags):
		NearestNeighbourSearch<T>::NearestNeighbourSearch(cloud, dim, creationOptionFlags),
		//deviceType(deviceType),
		//context(contextManager.createContext(deviceType))
	{
	}
	
	
};
