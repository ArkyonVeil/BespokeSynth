// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppPossiblyUninitializedMember


// Created by ArkyonVeil on 15/05/2026.
//
//Who knew C++ multicThreaded programming could be so safe.

#pragma once
#include <vector>
#include "SynthGlobals.h"

/// WHAT IS THIS?
///
/// SyncVariables are a form of variables designed for multicThreaded work.
/// One cThread writes/reads, another just reads. The ISyncVarsHandler ensures they remain
/// synced without race-condition dangers.

/// To use:
/// 1. Inherit ISyncVarsHandler
/// 2. Call Sync() ONCE in the Draw cThread, and Sync() ONCE in the Audio cThread
/// 3. That's it. Your syncVars<T> should be okay now. Have fun!
/// -Ark

//Debug only guards.
#if DEBUG || BESPOKE_NIGHTLY
#define rule(x) assert(x)
#else
#define rule(x) ((void)0)
#endif

#define cThread IsAudioThread() ? 1 : 0

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

class ISyncVarsHandler;
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
   bool ThreadOwnsVar() const { return cThread == mThreadOwner; }
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
      rule(mThreadOwner == cThread); //Writing to a synced var from the non owning cThread is a contractual violation.
      if (queuedChanges.size() != 1)
         queuedChanges.resize(1);
      queuedChanges[0] = change;
      hasQueue = true;
   }
   void QueueChange(T value)
   {
      rule(mThreadOwner == cThread); //Thou must only write in the owning cThread.
      queuedChanges.push_back(value);
      hasQueue = true;
   }

   void CommitChanges() override { committedChanges = std::move(queuedChanges); }
   void FlushCommits() override { committedChanges.clear(); };

protected:
   std::vector<syncOperation<T>> queuedChanges;
   std::vector<syncOperation<T>> committedChanges;
};

template <typename T>
class syncVariable : public ISyncVarType<T>
{
public:
   T operator=(const T& value)
   {
      this->SetChange(syncOperation<T>(opWrite, value));
      var[cThread] = value;
      return var[cThread];
   };
   operator T() const { return *var[cThread]; }

   void MergeChanges() override { var[!cThread] = this->committedChanges[0].mValue; }

private:
   T mMainVar;
   T mAudioVar;
   T* var[2]{ &mMainVar, &mAudioVar };
};

template <typename T>
class syncVector : public ISyncVarType<T>
{
public:
   syncVector(ISyncVarsHandler* handler, char threadOwner = 0)
   : ISyncVarType<T>(handler, threadOwner)
   {}

   //Direct access is blocked to avoid misuse. Use get/set
   T& operator[](size_t i) = delete;

   size_t size() { return vec[cThread]->size(); };
   bool empty() { return vec[cThread]->empty(); };

   void push_back(T var)
   {
      vec[cThread]->push_back(var);
      this->QueueChange(opPush, var);
   }
   void pop_back()
   {
      vec[cThread]->pop_back();
      this->QueueChange(opPop, nullptr);
   }
   void clear()
   {
      vec[cThread]->clear();
      this->QueueChange(opClear, nullptr);
   }
   void insert(size_t i, T var)
   {
      auto& v = vec[cThread];
      auto pos = v->begin();
      v->insert(pos + i, var);
      this->QueueChange(opInsert, var, i);
   }
   void erase(size_t i)
   {
      auto& v = vec[cThread];
      auto pos = v->begin();
      v->erase(pos + i);
      this->QueueChange(opErase, nullptr, i);
   }
   //Implementation of the handy RemoveFromVector(), insert a type, if it matches, its removed from the vector.
   bool removeFrom(T& match)
   {
      auto& v = vec[cThread];
      for (int i = 0; i < v->size(); ++i)
      {
         if (v[i] == match)
         {
            v->erase(v->begin() + i);
            this->QueueChange(opErase, nullptr, i);
            return true;
         }
      }
      return false;
   }
   T& get(size_t i)
   {
      return vec[cThread][i];
   }
   void set(size_t i, T value)
   {
      vec[cThread][i] = value;
      this->QueueChange(opWrite, value, i);
   }

   template <typename T>
   class borrowedVector
   {
      borrowedVector(std::vector<T>* data, syncVector* owner)
      : mData(data)
      , mOwner(owner)
      {
         rule(mOwner ? (mOwner->mThreadOwner == cThread) : true); //Can only borrow on owning thread.
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

   //Get the internal vector for batch operations.
   //Automatically returned at scope end.
   borrowedVector<T> borrow() { return borrowedVector<T>(vec[cThread], this); } //Only allowed on owning thread,
   borrowedVector<T> borrowRead() const { return borrowedVector<T>(vec[cThread], nullptr); } //Allowed on any thread.

   void MergeChanges() override
   {
      auto& merge = *vec[!cThread];
      for (auto& op : this->committedChanges)
      {
         switch (op.mOpType)
         {
            case opWrite:
               merge[op.mIdx] = op.mValue;
               break;
            case opClear:
               merge.clear();
               break;
            case opInsert:
               merge.insert(merge.begin() + op.mIdx, op.mValue);
               break;
            case opPush:
               merge.push_back(op.mValue);
               break;
            case opPop:
               merge.pop_back();
               break;
            case opSnapshot:
               merge = std::move(op.mSnapshot);
               break;
            case opErase:
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

//To use, make sure to call Sync() once on the Draw cThread, and once on the Process() cThread.
//Everything else is handled.
class ISyncVarsHandler
{
public:
   //Call twice, once on the Draw() and another on Process()
   void Sync() const
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
   void InternalSyncVariableRegister(ISyncVarBase* var)
   {
      mManagedVariables.push_back(var);
   }

private:
   std::vector<ISyncVarBase*> mManagedVariables;
};

/*
 * Kind of work In Progress, to be complete whenever Ark feels like it. (Probably never)
 *
template <typename T>
class syncArray : public ISyncVarType<T>
{
};
*/
//Intellisense failed me. -Ark