#ifndef __NABO_H
#define __NABO_H

#include "Eigen/Core"
#include "Eigen/Array"
#include "index_heap.h"
#include <vector>
#include <cstdatomic>

namespace Nabo
{
	// Euclidean distance
	template<typename T, typename A, typename B>
	inline T dist2(const A& v0, const B& v1)
	{
		return (v0 - v1).squaredNorm();
	}

	// Nearest neighbor search interface, templatized on scalar type
	template<typename T>
	struct NearestNeighborSearch
	{
		typedef typename Eigen::Matrix<T, Eigen::Dynamic, 1> Vector;
		typedef typename Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> Matrix; // each entry is a col, matrix has dim rows
		typedef int Index;
		typedef typename Eigen::Matrix<Index, Eigen::Dynamic, 1> IndexVector;
		typedef typename Eigen::Matrix<Index, Eigen::Dynamic, Eigen::Dynamic> IndexMatrix;
		
		const Matrix& cloud;
		const size_t dim;
		const Vector minBound;
		const Vector maxBound;
		
		struct Statistics
		{
			Statistics():lastQueryVisitCount(0),totalVisitCount(0) {}
			std::atomic_uint lastQueryVisitCount;
			std::atomic_uint totalVisitCount;
		};
		
		enum SearchOptionFlags
		{
			ALLOW_SELF_MATCH = 1,
			SORT_RESULTS = 2
		};
		
		NearestNeighborSearch(const Matrix& cloud);
		virtual IndexVector knn(const Vector& query, const Index k = 1, const T epsilon = 0, const unsigned optionFlags = 0) = 0;
		virtual IndexMatrix knnM(const Matrix& query, const Index k = 1, const T epsilon = 0, const unsigned optionFlags = 0);
		const Statistics& getStatistics() const { return statistics; }
		
	protected:
		Statistics statistics;
	};

	// Brute-force nearest neighbor
	template<typename T>
	struct BruteForceSearch: public NearestNeighborSearch<T>
	{
		typedef typename NearestNeighborSearch<T>::Vector Vector;
		typedef typename NearestNeighborSearch<T>::Matrix Matrix;
		typedef typename NearestNeighborSearch<T>::Index Index;
		typedef typename NearestNeighborSearch<T>::IndexVector IndexVector;

		BruteForceSearch(const Matrix& cloud);
		virtual IndexVector knn(const Vector& query, const Index k, const T epsilon, const unsigned optionFlags);
	};
	
	// KDTree, balanced, points in nodes
	template<typename T>
	struct KDTreeBalancedPtInNodes:public NearestNeighborSearch<T>
	{
		typedef typename NearestNeighborSearch<T>::Vector Vector;
		typedef typename NearestNeighborSearch<T>::Matrix Matrix;
		typedef typename NearestNeighborSearch<T>::Index Index;
		typedef typename NearestNeighborSearch<T>::IndexVector IndexVector;
		
	protected:
		struct BuildPoint
		{
			Vector pos;
			size_t index;
			BuildPoint(const Vector& pos =  Vector(), const size_t index = 0): pos(pos), index(index) {}
		};
		typedef std::vector<BuildPoint> BuildPoints;
		typedef typename BuildPoints::iterator BuildPointsIt;
		typedef typename BuildPoints::const_iterator BuildPointsCstIt;
		
		struct CompareDim
		{
			size_t dim;
			CompareDim(const size_t dim):dim(dim){}
			bool operator() (const BuildPoint& p0, const BuildPoint& p1) { return p0.pos(dim) < p1.pos(dim); }
		};
		
		struct Node
		{
			Vector pos;
			int dim; // -1 == leaf, -2 == invalid
			Index index;
			Node(const Vector& pos = Vector(), const int dim = -2, const Index index = 0):pos(pos), dim(dim), index(index) {}
		};
		typedef std::vector<Node> Nodes;
		
		Nodes nodes;
		
		inline size_t childLeft(size_t pos) const { return 2*pos + 1; }
		inline size_t childRight(size_t pos) const { return 2*pos + 2; }
		inline size_t parent(size_t pos) const { return (pos-1)/2; }
		size_t getTreeSize(size_t size) const;
		IndexVector cloudIndexesFromNodesIndexes(const IndexVector& indexes) const;
		void buildNodes(const BuildPointsIt first, const BuildPointsIt last, const size_t pos);
		void dump(const Vector minValues, const Vector maxValues, const size_t pos) const;
		
	protected:
		KDTreeBalancedPtInNodes(const Matrix& cloud);
	};
	
	// KDTree, balanced, points in nodes, priority queue
	template<typename T>
	struct KDTreeBalancedPtInNodesPQ: public KDTreeBalancedPtInNodes<T>
	{
		typedef typename NearestNeighborSearch<T>::Vector Vector;
		typedef typename NearestNeighborSearch<T>::Matrix Matrix;
		typedef typename NearestNeighborSearch<T>::Index Index;
		typedef typename NearestNeighborSearch<T>::IndexVector IndexVector;
		typedef typename KDTreeBalancedPtInNodes<T>::Node Node;
		typedef typename KDTreeBalancedPtInNodes<T>::Nodes Nodes;
		
		using NearestNeighborSearch<T>::statistics;
		using KDTreeBalancedPtInNodes<T>::nodes;
		using KDTreeBalancedPtInNodes<T>::childLeft;
		using KDTreeBalancedPtInNodes<T>::childRight;
		
	protected:
		struct SearchElement
		{
			size_t index;
			T minDist;
			
			SearchElement(const size_t index, const T minDist): index(index), minDist(minDist) {}
			// invert test as std::priority_queue shows biggest element at top
			friend bool operator<(const SearchElement& e0, const SearchElement& e1) { return e0.minDist > e1.minDist; }
		};
		
	public:
		KDTreeBalancedPtInNodesPQ(const Matrix& cloud);
		virtual IndexVector knn(const Vector& query, const Index k, const T epsilon, const unsigned optionFlags);
	};
	
	// KDTree, balanced, points in nodes, stack
	template<typename T>
	struct KDTreeBalancedPtInNodesStack: public KDTreeBalancedPtInNodes<T>
	{
		typedef typename NearestNeighborSearch<T>::Vector Vector;
		typedef typename NearestNeighborSearch<T>::Matrix Matrix;
		typedef typename NearestNeighborSearch<T>::Index Index;
		typedef typename NearestNeighborSearch<T>::IndexVector IndexVector;
		typedef typename KDTreeBalancedPtInNodes<T>::Node Node;
		typedef typename KDTreeBalancedPtInNodes<T>::Nodes Nodes;
		
		using NearestNeighborSearch<T>::statistics;
		using KDTreeBalancedPtInNodes<T>::nodes;
		using KDTreeBalancedPtInNodes<T>::childLeft;
		using KDTreeBalancedPtInNodes<T>::childRight;
		
		typedef IndexHeapSTL<Index, T> Heap;
		
	protected:
		void recurseKnn(const Vector& query, const size_t n, T rd, Heap& heap, Vector& off, const T maxError, const bool allowSelfMatch);
		
	public:
		KDTreeBalancedPtInNodesStack(const Matrix& cloud);
		virtual IndexVector knn(const Vector& query, const Index k, const T epsilon, const unsigned optionFlags);
	};
	
	
	//  KDTree, balanced, points in leaves, stack
	template<typename T>
	struct KDTreeBalancedPtInLeavesStack: public NearestNeighborSearch<T>
	{
		typedef typename NearestNeighborSearch<T>::Vector Vector;
		typedef typename NearestNeighborSearch<T>::Matrix Matrix;
		typedef typename NearestNeighborSearch<T>::Index Index;
		typedef typename NearestNeighborSearch<T>::IndexVector IndexVector;
		
		using NearestNeighborSearch<T>::statistics;
		using NearestNeighborSearch<T>::cloud;
		using NearestNeighborSearch<T>::minBound;
		using NearestNeighborSearch<T>::maxBound;
		
	protected:
		struct BuildPoint
		{
			Vector pos;
			size_t index;
			BuildPoint(const Vector& pos =  Vector(), const size_t index = 0): pos(pos), index(index) {}
		};
		typedef std::vector<BuildPoint> BuildPoints;
		typedef typename BuildPoints::iterator BuildPointsIt;
		typedef typename BuildPoints::const_iterator BuildPointsCstIt;
		
		struct CompareDim
		{
			size_t dim;
			CompareDim(const size_t dim):dim(dim){}
			bool operator() (const BuildPoint& p0, const BuildPoint& p1) { return p0.pos(dim) < p1.pos(dim); }
		};
		
		typedef IndexHeapSTL<Index, T> Heap;
		
		struct Node
		{
			int dim; // -1 == invalid, <= -2 = index of pt
			T cutVal;
			Node(const int dim = -1, const T cutVal = 0):
				dim(dim), cutVal(cutVal) {}
		};
		typedef std::vector<Node> Nodes;
		
		Nodes nodes;
		
		inline size_t childLeft(size_t pos) const { return 2*pos + 1; }
		inline size_t childRight(size_t pos) const { return 2*pos + 2; }
		inline size_t parent(size_t pos) const { return (pos-1)/2; }
		size_t getTreeSize(size_t size) const;
		void buildNodes(const BuildPointsIt first, const BuildPointsIt last, const size_t pos, const Vector minValues, const Vector maxValues, const bool balanceVariance);
		void recurseKnn(const Vector& query, const size_t n, T rd, Heap& heap, Vector& off, const T maxError, const bool allowSelfMatch);
		
	public:
		KDTreeBalancedPtInLeavesStack(const Matrix& cloud, const bool balanceVariance);
		virtual IndexVector knn(const Vector& query, const Index k, const T epsilon, const unsigned optionFlags);
	};
	
	//  KDTree, unbalanced, points in leaves, stack, implicit bounds, ANN_KD_SL_MIDPT
	template<typename T, typename Heap>
	struct KDTreeUnbalancedPtInLeavesImplicitBoundsStack: public NearestNeighborSearch<T>
	{
		typedef typename NearestNeighborSearch<T>::Vector Vector;
		typedef typename NearestNeighborSearch<T>::Matrix Matrix;
		typedef typename NearestNeighborSearch<T>::Index Index;
		typedef typename NearestNeighborSearch<T>::IndexVector IndexVector;
		typedef typename NearestNeighborSearch<T>::IndexMatrix IndexMatrix;
		
		using NearestNeighborSearch<T>::statistics;
		using NearestNeighborSearch<T>::cloud;
		using NearestNeighborSearch<T>::minBound;
		using NearestNeighborSearch<T>::maxBound;
		
	protected:
		struct BuildPoint
		{
			Vector pos;
			size_t index;
			BuildPoint(const Vector& pos =  Vector(), const size_t index = 0): pos(pos), index(index) {}
		};
		typedef std::vector<BuildPoint> BuildPoints;
		typedef typename BuildPoints::iterator BuildPointsIt;
		typedef typename BuildPoints::const_iterator BuildPointsCstIt;
		
		struct CompareDim
		{
			size_t dim;
			CompareDim(const size_t dim):dim(dim){}
			bool operator() (const BuildPoint& p0, const BuildPoint& p1) { return p0.pos(dim) < p1.pos(dim); }
		};
		
		struct Node
		{
			enum
			{
				INVALID_CHILD = 0xffffffff,
				INVALID_PT = 0xffffffff
			};
			unsigned dim;
			unsigned rightChild;
			union
			{
				T cutVal;
				unsigned ptIndex;
			};
			
			Node(const int dim, const T cutVal, unsigned rightChild):
				dim(dim), rightChild(rightChild), cutVal(cutVal) {}
			Node(const unsigned ptIndex = INVALID_PT):
				dim(0), rightChild(INVALID_CHILD), ptIndex(ptIndex) {}
		};
		typedef std::vector<Node> Nodes;
		
		Nodes nodes;
		
		unsigned buildNodes(const BuildPointsIt first, const BuildPointsIt last, const Vector minValues, const Vector maxValues);
		void recurseKnn(const Vector& query, const unsigned n, T rd, Heap& heap, Vector& off, const T maxError, const bool allowSelfMatch);
		
	public:
		KDTreeUnbalancedPtInLeavesImplicitBoundsStack(const Matrix& cloud);
		virtual IndexVector knn(const Vector& query, const Index k, const T epsilon, const unsigned optionFlags);
		virtual IndexMatrix knnM(const Matrix& query, const Index k, const T epsilon, const unsigned optionFlags);
	};
	
	//  KDTree, unbalanced, points in leaves, stack, implicit bounds, ANN_KD_SL_MIDPT, optimised
	template<typename T, typename Heap>
	struct KDTreeUnbalancedPtInLeavesImplicitBoundsStackOpt: public NearestNeighborSearch<T>
	{
		typedef typename NearestNeighborSearch<T>::Vector Vector;
		typedef typename NearestNeighborSearch<T>::Matrix Matrix;
		typedef typename NearestNeighborSearch<T>::Index Index;
		typedef typename NearestNeighborSearch<T>::IndexVector IndexVector;
		typedef typename NearestNeighborSearch<T>::IndexMatrix IndexMatrix;
		
		using NearestNeighborSearch<T>::statistics;
		using NearestNeighborSearch<T>::cloud;
		using NearestNeighborSearch<T>::minBound;
		using NearestNeighborSearch<T>::maxBound;
		
	protected:
		typedef std::vector<Index> BuildPoints;
		typedef typename BuildPoints::iterator BuildPointsIt;
		typedef typename BuildPoints::const_iterator BuildPointsCstIt;
		
		struct Node
		{
			enum
			{
				INVALID_CHILD = 0xffffffff,
				INVALID_PT = 0
			};
			Index dim; // also index for point
			unsigned rightChild;
			union
			{
				T cutVal;
				const T* pt;
			};
			
			Node(const Index dim, const T cutVal, unsigned rightChild):
				dim(dim), rightChild(rightChild), cutVal(cutVal) {}
			Node(const Index index = 0, const T* pt = 0):
				dim(index), rightChild(INVALID_CHILD), pt(pt) {}
		};
		typedef std::vector<Node> Nodes;
		
		Nodes nodes;
		const int dimCount;
		
		std::pair<T,T> getBounds(const BuildPointsIt first, const BuildPointsIt last, const unsigned dim);
		unsigned buildNodes(const BuildPointsIt first, const BuildPointsIt last, const Vector minValues, const Vector maxValues);
		
		template<bool allowSelfMatch>
		void recurseKnn(const T* query, const unsigned n, T rd, Heap& heap, std::vector<T>& off, const T maxError);
		
	public:
		KDTreeUnbalancedPtInLeavesImplicitBoundsStackOpt(const Matrix& cloud);
		virtual IndexVector knn(const Vector& query, const Index k, const T epsilon, const unsigned optionFlags);
		virtual IndexMatrix knnM(const Matrix& query, const Index k, const T epsilon, const unsigned optionFlags);
	};
	
	//  KDTree, unbalanced, points in leaves, stack, explicit bounds, ANN_KD_SL_MIDPT
	template<typename T>
	struct KDTreeUnbalancedPtInLeavesExplicitBoundsStack: public NearestNeighborSearch<T>
	{
		typedef typename NearestNeighborSearch<T>::Vector Vector;
		typedef typename NearestNeighborSearch<T>::Matrix Matrix;
		typedef typename NearestNeighborSearch<T>::Index Index;
		typedef typename NearestNeighborSearch<T>::IndexVector IndexVector;
		
		
		using NearestNeighborSearch<T>::statistics;
		using NearestNeighborSearch<T>::cloud;
		using NearestNeighborSearch<T>::minBound;
		using NearestNeighborSearch<T>::maxBound;
		
	protected:
		struct BuildPoint
		{
			Vector pos;
			size_t index;
			BuildPoint(const Vector& pos =  Vector(), const size_t index = 0): pos(pos), index(index) {}
		};
		typedef std::vector<BuildPoint> BuildPoints;
		typedef typename BuildPoints::iterator BuildPointsIt;
		typedef typename BuildPoints::const_iterator BuildPointsCstIt;
		
		struct CompareDim
		{
			size_t dim;
			CompareDim(const size_t dim):dim(dim){}
			bool operator() (const BuildPoint& p0, const BuildPoint& p1) { return p0.pos(dim) < p1.pos(dim); }
		};
		
		typedef IndexHeapSTL<Index, T> Heap;
		
		struct Node
		{
			int dim; // <= -1 = index of pt
			unsigned rightChild;
			T cutVal;
			T lowBound;
			T highBound;
			Node(const int dim = -1, const T cutVal = 0, const T lowBound = 0, const T highBound = 0, unsigned rightChild = 0):
				dim(dim), rightChild(rightChild), cutVal(cutVal), lowBound(lowBound), highBound(highBound) {}
		};
		typedef std::vector<Node> Nodes;
		
		Nodes nodes;
		
		unsigned buildNodes(const BuildPointsIt first, const BuildPointsIt last, const Vector minValues, const Vector maxValues);
		void recurseKnn(const Vector& query, const size_t n, T rd, Heap& heap, const T maxError, const bool allowSelfMatch);
		
	public:
		KDTreeUnbalancedPtInLeavesExplicitBoundsStack(const Matrix& cloud);
		virtual IndexVector knn(const Vector& query, const Index k, const T epsilon, const unsigned optionFlags);
	};
}

#endif // __NABO_H