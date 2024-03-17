// Copyright 2020 Alexandr Marchenko. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Algo/BinarySearch.h"
#include "Algo/IsSorted.h"
#include "Math/IntPoint.h"
#include "Templates/Sorting.h"


/** ArrayHelpers */
namespace ArrayHelpers
{
	using BoundIdx = FIntPoint;

	template<typename T>
	void CopyFrom_EndB_To_EndA(TArray<T>& A, const TArray<T>& B, int32 MergeCount)
	{
		A.Reserve(A.Num() + MergeCount);
		checkSlow((void*)&A != (void*)&B);
		FMemory::Memcpy(A.GetData() + A.Num(), B.GetData() + B.Num() - MergeCount, sizeof(T) * MergeCount);
		A.SetNumUnsafeInternal(A.Num() + MergeCount);
	}

	template<typename T>
	void InsertFrom_BtoA(TArray<T>& A, int32 InsertAID, const TArray<T>& B, int32 B_ID, int32 InsertCount)
	{
		if (B.Num() <= 0 || InsertCount <= 0) return;

		checkSlow(InsertAID < A.Num());
		checkSlow(B_ID < B.Num());
		checkSlow(B_ID + InsertCount <= B.Num());

		checkSlow((void*)&A != (void*)&B);
		A.InsertUninitialized(InsertAID, InsertCount);
		FMemory::Memcpy(A.GetData() + InsertAID, B.GetData() + B_ID, sizeof(T) * InsertCount);
	}

	//std::max_element()
	template<typename T, typename LessPredicate>
	int32 MaxElementIDByPredicate(const TArray<T>& A, LessPredicate Predicate)
	{
		int32 ID = A.Num() ? 0 : INDEX_NONE;
		for (int32 i = 1; i < A.Num(); i++)
		{
			if (Predicate(A[ID], A[i])) //A[ID] < A[i]
			{
				ID = i;
			}
		}
		return ID;
	}

	//return start index and count relative to 0
	template<typename T, typename ValidPredicate>
	BoundIdx GetValidBounds(const TArray<T>& A, ValidPredicate Predicate)
	{
		BoundIdx Out(INDEX_NONE, INDEX_NONE);
		for (int32 i = 0; i < A.Num(); i++)
		{
			if (Out.X == INDEX_NONE && Predicate(A[i]))
			{
				Out.X = i;
			}
			if (Out.Y == INDEX_NONE && Predicate(A[A.Num() - i - 1]))
			{
				Out.Y = A.Num() - i;
			}
			if (Out.Y != INDEX_NONE && Out.X != INDEX_NONE)
			{
				break;
			}
		}
		return Out;
	}

	template<typename T, typename RemovePredicate>
	bool Filter_BreakSort(TArray<T>& Arr, RemovePredicate Predicate, bool bShrink = true)
	{
		int32 RemID = Arr.Num();
		for (int32 i = Arr.Num() - 1; i > INDEX_NONE; i--)
		{
			if (Predicate(Arr[i]))
			{
				Arr.Swap(i, --RemID);
				//FMemory::Memswap((uint8*)Arr.GetData() + (sizeof(T) * i), (uint8*)Arr.GetData() + (sizeof(T) * RemID), sizeof(T));
			}
		}
		if (RemID < Arr.Num())
		{
			Arr.RemoveAt(RemID, Arr.Num() - RemID, bShrink);
			return true;
		}
		return false;
	}

	template<typename T, typename RemovePredicate>
	bool Filter_BreakSort_V2(TArray<T>& Arr, RemovePredicate Predicate, bool bShrink = true)
	{
		int32 RemID = Arr.Num();
		for (int32 i = Arr.Num() - 1; i > INDEX_NONE; i--)
		{
			if (Predicate(Arr, i))
			{
				Arr.Swap(i, --RemID);
			}
		}
		if (RemID < Arr.Num())
		{
			Arr.RemoveAt(RemID, Arr.Num() - RemID, bShrink);
			return true;
		}
		return false;
	}

	template<typename T, typename RemovePredicate>
	bool Filter_Sorted(TArray<T>& A, RemovePredicate Predicate, bool bShrink = true)
	{
		int32 r = -1;
		for (int32 i = 0; i < A.Num(); i++)
		{
			if (!Predicate(A, i))
			{
				r++;
				A[r] = MoveTemp(A[i]);
			}
		}
		r++;
		if (r < A.Num())
		{
			A.RemoveAt(r, A.Num() - r, bShrink);
			return true;
		}
		return false;
	}

	template<typename T, typename RemovePredicate>
	bool Filter_Sorted_V2(TArray<T>& A, RemovePredicate Predicate, bool bShrink = true)
	{
		int32 r = -1;
		for (int32 i = 0; i < A.Num(); i++)
		{
			if (!Predicate(A[i]))
			{
				r++;
				A[r] = MoveTemp(A[i]);
			}
		}
		r++;
		if (r < A.Num())
		{
			A.RemoveAt(r, A.Num() - r, bShrink);
			return true;
		}
		return false;
	}

} // namespace ArrayHelpers

/** ArraySorted */
namespace ArraySorted
{
	using BoundIdx = ArrayHelpers::BoundIdx;

	inline int32 BoundCount(const BoundIdx Bound)
	{
		return (Bound.Y - Bound.X) + 1;
	}
	inline bool IsValidBound(const BoundIdx Bound, const int32 ArrayNum)
	{
		return BoundCount(Bound) > 0 && (Bound.X < ArrayNum) && (Bound.Y < ArrayNum);
	}

	template<typename ArrayA, typename ArrayB, typename SortPredicateType>
	void GetBoundAofB_P(const ArrayA& A, const ArrayB& B, SortPredicateType SortPredicate, BoundIdx& OutA, BoundIdx& OutB)
	{
		if (A.Num() > 0 && B.Num() > 0)
		{
			int32 i;
			for (i = 0; i < B.Num(); i++)
			{
				OutA.X = Algo::BinarySearch(A, B[i], SortPredicate);
				if (OutA.X != INDEX_NONE) break;
			}
			if (OutA.X != INDEX_NONE)
			{
				OutB.X = i;
				for (int32 j = B.Num() - 1; j > i; j--)
				{
					OutA.Y = Algo::BinarySearch(A, B[j], SortPredicate);
					if (OutA.Y != INDEX_NONE)
					{
						OutB.Y = j;
						return;
					}
				}
			}
		}
		OutA = {INDEX_NONE, INDEX_NONE};
		OutB = {INDEX_NONE, INDEX_NONE};
	}

	/*
	template<typename T, typename U, typename SortPredicateType>
	void GetBoundAofB(TArrayView<T> A, TArrayView<U>& B, SortPredicateType SortPredicate, TArrayView<T>& OutA, TArrayView<U>& OutB)
	{
		OutA = TArrayView<T>(nullptr, 0);
		OutB = TArrayView<U>(nullptr, 0);
		if (A.Num() > 0 && B.Num() > 0)
		{
			BoundIdx OutAp, OutBp;
			GetBoundAofB_P(A, B, SortPredicate, OutAp, OutBp);
			if (IsValidBound(OutAp, A.Num()) && IsValidBound(OutBp, B.Num()))
			{
				OutA = TArrayView<T>(A.GetData() + OutAp.X, OutAp.X - OutAp.Y);
				OutA = TArrayView<T>(A.GetData() + OutBp.X, OutBp.X - OutBp.Y);
			}
		}
	}
	*/

	/***************************************/

	template<typename T, typename SortPredicateType>
	FORCEINLINE bool Contains_SortedPredicate(const TArray<T>& A, const T& Elem, SortPredicateType SortPredicate)
	{
		return INDEX_NONE != Algo::BinarySearch(A, Elem, MoveTempIfPossible(SortPredicate));
	}


	template<typename T, typename SortPredicateType>
	int32 InsertUniqueSorted(TArray<T>& A, const T& InsertElem, SortPredicateType SortPredicate, const bool bOverride = false)
	{
		//checkSlow(Algo::IsSorted(A, SortPredicate));
		const int32 ID = Algo::LowerBound(A, InsertElem, MoveTempIfPossible(SortPredicate));
		if (A.IsValidIndex(ID))
		{
			if (A[ID] == InsertElem)
			{
				if (bOverride)
				{
					A[ID] = InsertElem;
				}
			}
			else
			{
				A.Insert(InsertElem, ID);
			}
		}
		else
		{
			A.Add(InsertElem); //
		}

		//checkSlow(Algo::IsSorted(A, SortPredicate));
		return ID;
	}

	template<typename T, typename SortPredicateType>
	int32 InsertUniqueSorted(TArray<T>& A, T&& InsertElem, SortPredicateType SortPredicate, const bool bOverride = false)
	{
		//checkSlow(Algo::IsSorted(A, SortPredicate));
		const int32 ID = Algo::LowerBound(A, InsertElem, MoveTempIfPossible(SortPredicate));
		if (A.IsValidIndex(ID))
		{
			if (A[ID] == InsertElem)
			{
				if (bOverride)
				{
					A[ID] = MoveTemp(InsertElem);
				}
			}
			else
			{
				A.Insert(MoveTemp(InsertElem), ID);
			}
		}
		else
		{
			A.Add(MoveTemp(InsertElem)); //
		}
		//checkSlow(Algo::IsSorted(A, SortPredicate));
		return ID;
	}

	template<typename T, typename SortPredicateType>
	bool RemoveSorted(TArray<T>& A, const T& Elem, SortPredicateType SortPredicate)
	{
		//checkSlow(Algo::IsSorted(A, SortPredicate));
		const int32 ID = Algo::BinarySearch(A, Elem, MoveTempIfPossible(SortPredicate));
		if (ID != INDEX_NONE)
		{
			A.RemoveAt(ID);
			return true;
		}
		return false;
	}

	/**Duplicates*/
	template<typename T>
	bool RemoveDuplicates_Sorted(TArray<T>& A, const bool bShrink = true) //Duplicates
	{
		int32 r = 0;
		for (int32 i = 1; i < A.Num(); i++)
		{
			if (!(A[i] == A[i - 1]))
			{
				r++;
				A[r] = MoveTemp(A[i]);
			}
		}
		r++;
		if (r > 0 && r < A.Num())
		{
			A.RemoveAt(r, A.Num() - r, bShrink);
			return true;
		}
		return false;
	}

	template<typename T>
	bool ContainsDuplicates_Sorted(const TArray<T>& A)
	{
		for (int32 i = 1; i < A.Num(); i++)
		{
			if (A[i - 1] == A[i])
			{
				return true;
			}
		}
		return false;
	}

	/***************************************/

	template<typename T, typename SortPredicateType>
	void Merge_SortedPredicate(TArray<T>& A, const TArray<T>& B, SortPredicateType SortPredicate, const bool bOverride = true)
	{
		if (B.Num() != 0)
		{
			//checkSlow(Algo::IsSorted(A, SortPredicate));
			//checkSlow(Algo::IsSorted(B, SortPredicate));
			//checkSlow(!ContainsDuplicates_Sorted(A));
			//checkSlow(!ContainsDuplicates_Sorted(B));

			if (A.Num() == 0)
			{
				A = B;
			}
			else if (B.Num() == 1)
			{
				InsertUniqueSorted(A, B[0], MoveTempIfPossible(SortPredicate), bOverride);
			}
			else if (SortPredicate(A.Last(), B[0]))
			{
				A.Append(B);
			}
			else if (SortPredicate(B.Last(), A[0]))
			{
				A.Insert(B, 0);
			}
			else
			{
				//todo: need check is all must work without error
				A.Reserve(A.Num() + B.Num());
				BoundIdx BoundA, BoundB;
				GetBoundAofB_P(A, B, SortPredicate, BoundA, BoundB);

				A.Insert(B, BoundA.Y); //todo: check range insert end

				int32 i = BoundA.Y - 1; //from end A
				int32 j = BoundB.Y;		//from end B
				int32 EndID = i + B.Num();
				int32 DuplicateCount = 0;
				int32 SeqA = 0;
				while (j >= 0) //duplicate //&& EndID >= BoundA.X
				{
					checkSlow(EndID >= BoundA.X);
					if (i >= BoundA.X && SortPredicate(B[j], A[i])) //A > B
					{
						SeqA++;
						//A[EndID] = MoveTemp(A[i]);
						i--;
					}
					else
					{
						if (SeqA > 0) //todo move to i - SeqA : A[EndID] = MoveTemp(A[i]);
						{
							FMemory::Memmove(A.GetData() + EndID, A.GetData() + i, SeqA);
							SeqA = 0;
						}
						if (i >= BoundA.X && A[i] == B[j]) //A == B
						{
							DuplicateCount++;
							A[EndID] = bOverride ? B[j] : MoveTemp(A[i]);
							i--;
							j--;
						}
						else //A < B
						{
							A[EndID] = B[j];
							j--;
						}
						EndID--;
					}
					if (DuplicateCount > 0)
					{
						A.RemoveAt(BoundA.X, DuplicateCount, true);
					}

					//#if WITH_EDITOR
					//				for (const auto& It : B)
					//				{
					//					check(A.Contains(It));
					//				}
					//#endif
				}
				//checkSlow(Algo::IsSorted(A, SortPredicate));
				//checkSlow(!ContainsDuplicates_Sorted(A));
			}
		}
	}

	template<typename T, typename SortPredicateType>
	void Merge_SortedPredicate(TArray<T>& A, TArray<T>&& B, SortPredicateType SortPredicate, const bool bOverride = true)
	{
		if (B.Num() != 0)
		{
			//checkSlow(Algo::IsSorted(A, SortPredicate));
			//checkSlow(Algo::IsSorted(B, SortPredicate));

			if (A.Num() == 0)
			{
				A = MoveTemp(B);
			}
			else if (B.Num() == 1)
			{
				InsertUniqueSorted(A, MoveTemp(B[0]), MoveTempIfPossible(SortPredicate), bOverride);
			}
			else if (SortPredicate(A.Last(), B[0]))
			{
				A.Append(MoveTemp(B));
			}
			else if (SortPredicate(B.Last(), A[0]))
			{
				A.Insert(MoveTemp(B), 0);
			}
			else
			{
				A.Reserve(A.Num() + B.Num());
				BoundIdx BoundA, BoundB;
				GetBoundAofB_P(A, B, SortPredicate, BoundA, BoundB);
				A.Insert(B, BoundA.Y);

				int32 i = BoundA.Y - 1; //from end A
				int32 j = B.Num() - 1;	//from end B
				int32 EndID = i + B.Num();
				int32 DuplicateCount = 0;
				while (j >= 0) //duplicate //&& EndID >= BoundA.X
				{
					checkSlow(EndID >= BoundA.X);
					if (i >= BoundA.X && A[i] == B[j])
					{
						DuplicateCount++;
						A[EndID] = bOverride ? B[j] : MoveTemp(A[i]);
						i--;
						j--;
					}
					else if (i >= BoundA.X && SortPredicate(B[j], A[i])) //A > B
					{
						A[EndID] = MoveTemp(A[i]);
						i--;
					}
					else //A < B
					{
						A[EndID] = MoveTemp(B[j]);
						j--;
					}
					EndID--;
				}
				if (DuplicateCount > 0)
				{
					A.RemoveAt(BoundA.X, DuplicateCount, true);
				}
			}
			//checkSlow(Algo::IsSorted(A, SortPredicate));
		}
	}


	/***************************************/

	/** O(M+N) ,O(BoundCount(Bound_A) + BoundCount(Bound_B))*/
	template<typename T, typename SortPredicateType>
	BoundIdx ArrayMinusArray_Linear_SortedPredicate_Check(
		TArrayView<T> A,
		TArrayView<const T> B,
		SortPredicateType SortPredicate,
		const BoundIdx Bound_A,
		const BoundIdx Bound_B)
	{
		int32 i = Bound_A.X;
		int32 j = Bound_B.X;
		int32 r = i - 1;
		for (; i <= Bound_A.Y && j <= Bound_B.Y;)
		{
			if (SortPredicate(A[i], B[j])) //A[i] < B[j]
			{
				r++;
				if (r != i)
				{
					A[r] = MoveTemp(A[i]);
				}
				i++;
			}
			else if (A[i] == B[j])
			{
				i++;
				j++;
			}
			else //if (B[j] < A[i])
			{
				j++;
			}
		}

		r++;
		return BoundIdx(r, i - r);
	}

	template<typename T, typename SortPredicateType>
	BoundIdx ArrayMinusArray_Binary_SortedPredicate_Check(
		TArrayView<T>& A,
		TArrayView<const T>& B,
		SortPredicateType SortPredicate,
		const BoundIdx Bound_A,
		BoundIdx Bound_B)
	{
		int32 ID = Bound_B.X;
		int32 i = Bound_A.X;
		int32 r = i - 1;
		//for (;i < A.Num() && ID < B.Num() && Bound_B.X < Bound_B.Y;)
		for (; i <= Bound_A.Y && ID <= Bound_B.Y && Bound_B.X <= Bound_B.Y;)
		{
			if (ID == INDEX_NONE || !(A[i] == B[ID]))
			{
				auto AvA = B.Slice(Bound_B.X, Bound_B.Y - Bound_B.X);
				ID = Algo::LowerBound(AvA, A[i], SortPredicate);
				if (AvA.IsValidIndex(ID) && AvA[ID] == A[i])
				{
					ID += Bound_B.X; //todo check
				}
				else
				{
					ID = INDEX_NONE;
				}
			}
			if (ID != INDEX_NONE)
			{
				Bound_B.X = ID;
				ID++;
			}
			else
			{
				r++;
				A[r] = MoveTemp(A[i]);
			}
			i++;
		}
		r++;
		return BoundIdx(r, i - r);
	}

	template<typename T, typename SortPredicateType>
	void ArrayMinusArray_SortedPredicate(TArray<T>& A, const TArray<T>& B, SortPredicateType SortPredicate, bool bShrink = true)
	{
		if (A.Num() == 0 || B.Num() == 0)
		{
			return;
		}
		//checkSlow(Algo::IsSorted(A, SortPredicate));
		//checkSlow(Algo::IsSorted(B, SortPredicate));

		TArrayView<const T> ViewB(B);
		TArrayView<T> ViewA(A);
		BoundIdx Bound_A, Bound_B;
		GetBoundAofB_P(ViewA, ViewB, SortPredicate, Bound_A, Bound_B);
		//ViewA = TArrayView<const T>(ViewA[Bound_A.X], Bound_A.Y - Bound_A.X);
		//ViewB = TArrayView<T>(ViewB[Bound_B.X], Bound_B.Y - Bound_B.X);

		if (IsValidBound(Bound_A, A.Num()) && IsValidBound(Bound_B, B.Num()))
		{
			BoundIdx Rem;
			if (BoundCount(Bound_A) <= BoundCount(Bound_B)) // todo:best average result?
			{
				Rem = ArrayMinusArray_Binary_SortedPredicate_Check(ViewA, ViewB, MoveTempIfPossible(SortPredicate), Bound_A, Bound_B);
			}
			else
			{
				Rem = ArrayMinusArray_Linear_SortedPredicate_Check(ViewA, ViewB, MoveTempIfPossible(SortPredicate), Bound_A, Bound_B);
			}
			if (Rem.X < A.Num() && Rem.Y != INDEX_NONE)
			{
				A.RemoveAt(Rem.X, Rem.Y + 1, bShrink);
			}
			//#if WITH_EDITOR
			//			check(Algo::IsSorted(A, SortPredicate));
			//			for (const auto& It : B)
			//			{
			//				check(!A.Contains(It))
			//			}
			//#endif

			//checkSlow(Algo::IsSorted(A, SortPredicate));
		}
	}

	/*
	//todo WIP
	template<typename T, typename SortPredicateType>
	UE_DEPRECATED(4.23, "not finished")
	BoundIdx ArrayMinusArray_Binary_Check(TArrayView<T> Arr, TArrayView<const T> Brr, SortPredicateType SortPredicate)
	{
		T* APtr = Arr.GetData();

		TArrayView<T> A = GetBoundAofB(Arr, Brr, SortPredicate);
		TArrayView<const T> B = GetBoundAofB(Brr, Arr, SortPredicate);

		const int32 ANum = A.Num();
		if (ANum && B.Num())
		{
			const int32 Delta = A.GetData() - APtr;
			int32 ID = 0;
			int32 i = 0;
			int32 r = i - 1;

			while (i < ANum && B.Num() && B.IsValidIndex(ID))
			{
				if (ID == INDEX_NONE || !(A[i] == B[ID]))
				{
					ID = Algo::LowerBound(B, A[i], SortPredicate);
					if (!(B.IsValidIndex(ID) && B[ID] == A[i]))
					{
						ID = INDEX_NONE;
					}
				}
				if (ID != INDEX_NONE)
				{
					B = B.Slice(ID, B.Num() - ID);
					ID = 0;
				}
				else
				{
					r++;
					A[r] = MoveTemp(A[i]);
				}
				i++;
			}
			r++;
			//todo + offset
			return BoundIdx(r + Delta, i - r);
		}
		return BoundIdx(-1, 0);
	}
	*/


} // namespace ArraySorted

/** HashSorted */
namespace HashSorted
{
	using BoundIdx = ArrayHelpers::BoundIdx;

	/**auto Predicate = ([&](const T& a1, const T& b1) {return GetTypeHash(a1) < GetTypeHash(b1); })*/
	/**auto Predicate = TSortHashPredicate<T>()*/
	template<typename T>
	struct TSortHashPredicate
	{
		FORCEINLINE bool operator()(const T& A, const T& B) const { return GetTypeHash(A) < GetTypeHash(B); }
	};

	template<typename T>
	struct TSortTypeHashPredicate
	{
		bool operator()(const T& A, const uint32 Hash) const { return GetTypeHash(A) < Hash; }
		bool operator()(const uint32 Hash, const T& A) const { return Hash < GetTypeHash(A); }
		//bool operator()(const uint32 HashA, const uint32 HashB ) const { return HashA < HashB; }
	};

	template<typename T>
	FORCEINLINE void ArrayMinusArray(TArray<T>& A, const TArray<T>& B, bool bShrink = true)
	{
		ArraySorted::ArrayMinusArray_SortedPredicate(A, B, TSortHashPredicate<T>(), bShrink);
	}

	template<typename T>
	FORCEINLINE void Merge(TArray<T>& A, const TArray<T>& B, const bool bOverride = true)
	{
		ArraySorted::Merge_SortedPredicate(A, B, TSortHashPredicate<T>(), bOverride);
	}
	template<typename T>
	FORCEINLINE void Merge(TArray<T>& A, TArray<T>&& B, const bool bOverride = true)
	{
		ArraySorted::Merge_SortedPredicate(A, B, TSortHashPredicate<T>(), bOverride);
	}

	/***************************************/

	template<typename T>
	void SortByHash(TArray<T>& A)
	{
		if (A.Num() > 1)
		{
			Algo::Sort(A, TSortHashPredicate<T>());
		}
	}

	template<typename T>
	bool IsSortedByHash(const TArray<T>& A)
	{
		return Algo::IsSorted(A, TSortHashPredicate<T>());
	}

	template<typename T>
	int32 BinarySearchByHash(const TArray<T>& A, const T& FindElement)
	{
		return Algo::BinarySearch(A, FindElement, TSortHashPredicate<T>());
	}

	template<typename T>
	bool Contains(const TArray<T>& A, const T& Elem)
	{
		return ArraySorted::Contains_SortedPredicate(A, Elem, TSortHashPredicate<T>());
	}


	template<typename T>
	bool Remove(TArray<T>& A, const T& Elem)
	{
		return ArraySorted::RemoveSorted(A, Elem, TSortHashPredicate<T>());
	}


	template<typename T>
	int32 InsertUnique(TArray<T>& A, const T& Insert, bool bOverride = false)
	{
		return ArraySorted::InsertUniqueSorted(A, MoveTempIfPossible(Insert), TSortHashPredicate<T>(), bOverride);
	}

	/**Duplicates*/
	template<typename T>
	FORCEINLINE bool RemoveDuplicates(TArray<T>& A)
	{
		return ArraySorted::RemoveDuplicates_Sorted(A);
	}

	template<typename T> //, typename SortPredicateType
	int32 FindInsertID_InBound(
		const TArray<T>& A,
		const T& FindElement,
		int32 StartIdx = 0,
		int32 EndNum = MAX_int32) //  ,SortPredicateType Predicate= TSortHashPredicate<T>()
	{
		if (EndNum == MAX_int32)
		{
			EndNum = A.Num();
		}
		TArrayView<const T> ArrayView = TArrayView<const T>(A).Slice(StartIdx, EndNum - StartIdx);
		return Algo::LowerBound(ArrayView, FindElement, TSortHashPredicate<T>()) + StartIdx;
	}

	template<typename T>
	FORCEINLINE int32 BinarySearch_InBound(const TArray<T>& A, const T& FindElement, int32 StartIdx = 0, int32 EndNum = MAX_int32)
	{
		if (EndNum == MAX_int32)
		{
			EndNum = A.Num();
		}
		TArrayView<const T> ArrayView = TArrayView<const T>(A).Slice(StartIdx, EndNum - StartIdx);
		return Algo::BinarySearch(ArrayView, FindElement, TSortHashPredicate<T>()) + StartIdx;
	}

	/***************************************/

	template<typename T>
	FORCEINLINE int32 BinarySearch_HashType(const TArray<T>& A, const uint32 Hash)
	{
		return Algo::BinarySearch(A, Hash, TSortTypeHashPredicate<T>());
	}

	template<typename T>
	FORCEINLINE bool Contains_HashType(const TArray<T>& A, const uint32 Hash)
	{
		if (A.Num())
		{
			if (A.Num() == 1)
			{
				return GetTypeHash(A[0]) == Hash;
			}
			//checkSlow(IsSortedByHash(A));
			return INDEX_NONE != Algo::BinarySearch(A, Hash, TSortTypeHashPredicate<T>());
		}
		return false;
	}

	template<typename T>
	FORCEINLINE int32 Remove_HashType(TArray<T>& A, const uint32 Hash, const bool bAllowShrinking = true)
	{
		const int32 ID = Algo::BinarySearch(A, Hash, TSortTypeHashPredicate<T>());
		if (ID != INDEX_NONE)
		{
			A.RemoveAt(ID, 1, bAllowShrinking);
		}
		return ID;
	}


	//sorted by Hash or binary search
	// 	template <typename T>
	// 	int32 FindInsertID_HashType(const TArray<T>& A, uint32 Hash)
	// 	{
	// 		TSortTypeHashPredicate<T> Predicate;
	//
	// 		if (A.Num() <= 0 || Predicate(Hash, A[0]))
	// 		{
	// 			return 0;
	// 		}
	// 		if (Predicate(A.Last(), Hash))
	// 		{
	// 			return A.Num();
	// 		}
	//
	// 		int32 Start = 0;
	// 		int32 Size = A.Num();
	//
	// 		while (Size > 0)
	// 		{
	// 			const int32 LeftoverSize = Size % 2;
	// 			Size = Size / 2;
	//
	// 			const int32 CheckIndex = Start + Size;
	// 			const int32 StartIfLess = CheckIndex + LeftoverSize;
	//
	// 			Start = Predicate(A[CheckIndex], Hash) ? StartIfLess : Start;
	// 		}
	// 		return Start;
	// 	}
} // namespace HashSorted
