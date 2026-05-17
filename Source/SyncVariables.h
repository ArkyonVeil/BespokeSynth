// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppPossiblyUninitializedMember


// Created by ArkyonVeil on 15/05/2026.
//
//Who knew C++ multithreaded programming could be so safe.

#pragma once
#include <vector>
#include "SynthGlobals.h"

/// WHAT IS THIS?
///
/// SyncVariables are a variable wrapper designed for multithreaded work.
/// One thread writes/reads, another just reads. The ISyncVarsHandler ensures they remain
/// synced without race-condition dangers.

/// HOW TO USE:
/// 1. Module inherits ISyncVarsHandler
/// 2. Call SyncVars() in Poll(), and SyncVars() in Process(). So they run once every frame. (Don't use Process()? See table below)
/// 3. That's it. Your synced variables should be okay now. Have fun!
/// -Ark


/// DETAILS FOR NERDS:
/// synced variables are assigned to a handler (usually a module) on creation and internally keep two copies.
/// One is the authoritative copy, the other is the reader copy. SyncVars() calls propagate changes across threads
/// while ensuring that threads never stall for this sharing of information.
/// This is useful for avoiding data-races, common in multithreaded work. And this implementation should
/// cause less stalls than mutexes.
///
/// However, due the way it works. SyncVars() is recommended to be called at most once per frame.
/// This is because it batches changes, and commits them at a SyncVars() call. The pipe is considered "full" at this point
/// and must be drained first by the other thread for new changes to be committed. (This avoids mutex stalls)
/// Do not worry about this though, changes are still queued even with a full pipe. They'll only be propagated a cycle
/// later.
///
/// This pipe check-in, check-out cycle is also why changes across threads, while fast, aren't instant. Multithreaded work
/// is messy, sorry.
///
/// Anyway, this is meant to make your life much easier but writing off a ton of pain from syncing (and forgetting to sync)
/// stuff manually. Cheers -ArkyonVeil


/// Common Methods for Sync table
/// Main thread.
/// - Poll(): runs every frame, even offscreen.
/// Audio thread.
/// - Process(): every buffer tick, about 187.5 times per sec on default 48kh/256(buffer size).
/// - OnTimeEvent(): developer's choice.
/// - OnTransportAdvanced(): same as Process()


//Debug only guards.
#if DEBUG || BESPOKE_NIGHTLY
#define rule(x) assert(x)
#else
#define rule(x) ((void)0)
#endif


class ISyncVarsHandler;
namespace isv
{
   inline char threadId() { return IsAudioThread() ? 1 : 0; }
   enum OperationType
   {
      opWrite = 0,
      opPush = 1,
      opPop = 2,
      opInsert = 3,
      opClear = 4,
      opSnapshot = 5,
      opErase = 6
   };

   template <typename T>
   struct syncOperation
   {
      syncOperation(OperationType opType, T value, int idx = 0)
      {
         mOpType = opType;
         mValue = value;
         mIdx = idx;
      }
      syncOperation(std::vector<T>& snapshot)
      {
         mOpType = opSnapshot;
         mSnapshot = snapshot;
      }

      OperationType mOpType{ opWrite };
      int mIdx{ 0 };
      T mValue;
      std::vector<T> mSnapshot;
   };

   //Thread 0 = Main/UI , 1 = Audio
   class ISyncVarBase
   {
   public:
      explicit ISyncVarBase(ISyncVarsHandler* handler, char threadOwner = 0)
      : mThreadOwner(threadOwner)
      {}
      std::atomic<bool> hasCommits{ false };
      std::atomic<bool> hasQueue{ false };
      bool ThreadOwnsVar() const { return threadId() == mThreadOwner; }
      virtual ~ISyncVarBase() = default;
      virtual void CommitChanges() = 0; //Moves changes from the queue to commits.
      virtual void MergeChanges() = 0; //Formalizes merged changes.
      virtual void FlushCommits() = 0;

   protected:
      char mThreadOwner;
   };

   template <typename T>
   class ISyncVarType : public ISyncVarBase
   {
   public:
      ISyncVarType(ISyncVarsHandler* handler, char threadOwner)
      : ISyncVarBase(handler, threadOwner)
      {
         static_assert(!std::is_pointer_v<T>, "Raw pointers are not supported in SyncVariables. Use smart pointers instead.");
         handler->InternalSyncVariableRegister(this);
         mThreadOwner = threadOwner;
      }
      void SetChange(syncOperation<T> change)
      {
         rule(mThreadOwner == threadId()); //Writing to a synced var from the non owning thread is a contractual violation.
         if (queuedChanges.size() != 1)
            queuedChanges.resize(1);
         queuedChanges[0] = change;
         hasQueue = true;
      }
      void QueueChange(syncOperation<T> change)
      {
         rule(mThreadOwner == threadId()); //Thou can only write in the owning thread.
         queuedChanges.push_back(change);
         hasQueue = true;
      }

      void CommitChanges() override { committedChanges = std::move(queuedChanges); }
      void FlushCommits() override { committedChanges.clear(); };

   protected:
      std::vector<syncOperation<T>> queuedChanges;
      std::vector<syncOperation<T>> committedChanges;
   };
}

template <typename T>
class syncVariable : public isv::ISyncVarType<T>
{
public:
   syncVariable(ISyncVarsHandler* handler, char threadOwner)
   : isv::ISyncVarType<T>(handler, threadOwner)
   {}
   T operator=(const T& value)
   {
      this->SetChange(syncOperation<T>(isv::opWrite, value));
      var[isv::threadId()] = value;
      return var[isv::threadId()];
   };
   operator T() const { return *var[isv::threadId()]; }

   void MergeChanges() override { var[!isv::threadId()] = this->committedChanges[0].mValue; }

private:
   T mMainVar;
   T mAudioVar;
   T* var[2]{ &mMainVar, &mAudioVar };
};

template <typename T>
class syncVector : public isv::ISyncVarType<T>
{
public:
   syncVector(ISyncVarsHandler* handler, char threadOwner = 0)
   : isv::ISyncVarType<T>(handler, threadOwner)
   {}

   //Direct access is blocked to avoid misuse. Use get/set
   T& operator[](size_t i) = delete;

   size_t size() { return vec[isv::threadId()]->size(); };
   bool empty() { return vec[isv::threadId()]->empty(); };

   void push_back(T var)
   {
      vec[isv::threadId()]->push_back(var);
      this->QueueChange(isv::opPush, var);
   }
   void pop_back()
   {
      vec[isv::threadId()]->pop_back();
      this->QueueChange(isv::opPop, {});
   }
   void clear()
   {
      vec[isv::threadId()]->clear();
      this->QueueChange(isv::opClear, {});
   }
   void insert(size_t i, T var)
   {
      auto& v = vec[isv::threadId()];
      auto pos = v->begin();
      v->insert(pos + i, var);
      this->QueueChange(isv::opInsert, var, i);
   }
   void erase(size_t i)
   {
      auto& v = vec[isv::threadId()];
      auto pos = v->begin();
      v->erase(pos + i);
      this->QueueChange(isv::opErase, {}, i);
   }
   //Implementation of the handy RemoveFromVector(), insert a type, if it matches, its removed from the vector.
   bool removeFrom(T& match)
   {
      auto& v = vec[isv::threadId()];
      for (int i = 0; i < v->size(); ++i)
      {
         if (v[i] == match)
         {
            v->erase(v->begin() + i);
            this->QueueChange(isv::opErase, {}, i);
            return true;
         }
      }
      return false;
   }
   T& get(size_t i)
   {
      return vec[isv::threadId()][i];
   }
   void set(size_t i, T value)
   {
      vec[isv::threadId()][i] = value;
      this->QueueChange(isv::opWrite, value, i);
   }

   class borrowedVector
   {
      borrowedVector(std::vector<T>* data, syncVector* owner)
      : mData(data)
      , mOwner(owner)
      {
         rule(mOwner ? (mOwner->mThreadOwner == isv::threadId()) : true); //Can only borrow on owning thread.
      }
      std::vector<T>* mData;
      syncVector* mOwner;

   public:
      ~borrowedVector()
      {
         if (!mOwner)
            return;
         this->mOwner->SetChange(syncOperation<T>(std::vector<T>(mData)));
      }
      auto begin() { return mData->begin(); }
      auto end() { return mData->end(); }
      T& operator[](size_t i) { return (*mData)[i]; }
      size_t size() const { return mData->size(); }

      auto begin() const { return mData->begin(); }
      auto end() const { return mData->end(); }
      const T& operator[](size_t i) const { return (*mData)[i]; }
   };

   //Get the internal vector for batch operations. Both Read and Write.
   //Changes are automatically handled on scope end.
   //Only allowed on owning thread,
   borrowedVector borrow() { return borrowedVector(vec[isv::threadId()], this); }
   //Get the internal vector for batch operations. Only Read.
   //No sync overhead.
   //Allowed on any thread,
   borrowedVector borrowRead() const { return borrowedVector(vec[isv::threadId()], nullptr); }

   void MergeChanges() override
   {
      auto& merge = *vec[!isv::threadId()];
      for (auto& op : this->committedChanges)
      {
         switch (op.mOpType)
         {
            case isv::opWrite:
               merge[op.mIdx] = op.mValue;
               break;
            case isv::opClear:
               merge.clear();
               break;
            case isv::opInsert:
               merge.insert(merge.begin() + op.mIdx, op.mValue);
               break;
            case isv::opPush:
               merge.push_back(op.mValue);
               break;
            case isv::opPop:
               merge.pop_back();
               break;
            case isv::opSnapshot:
               merge = std::move(op.mSnapshot);
               break;
            case isv::opErase:
               merge.erase(merge.begin() + op.mIdx);
               break;
            default:
               rule(false); //Unsupported
         }
      }
   }

private:
   std::vector<T> mMainVector;
   std::vector<T> mAudioVector;
   std::vector<T>* vec[2]{ &mMainVector, &mAudioVector };
};

//To use, make sure to call SyncVars() once on the Poll() method, and once on the Process() thread.
//Everything else is handled.
class ISyncVarsHandler
{
public:
   //Call twice, once on Poll() and another on Process()
   void SyncVars() const
   {
      for (const auto var : mManagedVariables)
      {
         if (var->hasQueue)
         {
            //Can only push when tunnel is ready.
            //Otherwise we need a mutex, which may cause audio glitches.
            if (!var->hasCommits && var->ThreadOwnsVar())
            {
               var->CommitChanges();
               var->hasQueue = false;
               var->hasCommits = true;
            }
         }
         else if (var->hasCommits)
         {
            if (!var->ThreadOwnsVar())
            {
               var->MergeChanges();
               var->FlushCommits();
               var->hasCommits = false;
            }
         }
      }
      /// Write Change - Adds to the queue
      /// Sync()-owner - Commits a batch of changes. Queue is cleared.
      /// Sync()-reader - Reads commits, merges them.
      /// Flags pipe as ready for further changes.
   }
   //Done automatically, calling this is a bug.
   void InternalSyncVariableRegister(isv::ISyncVarBase* var)
   {
      mManagedVariables.push_back(var);
   }

private:
   std::vector<isv::ISyncVarBase*> mManagedVariables;
};

/*
 * Kind of a work in progress, to be complete whenever Ark feels like it. (Probably never)
 *
template <typename T>
class syncArray : public ISyncVarType<T>
{
};
*/
//Intellisense failed me. -Ark