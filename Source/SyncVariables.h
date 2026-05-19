// ReSharper disable CppNonExplicitConversionOperator
// ReSharper disable CppPossiblyUninitializedMember


// Created by ArkyonVeil on 15/05/2026.
//
//Who knew C++ multithreaded programming could be so safe.

#pragma once
#include "SynthGlobals.h"
#include <vector>
#include <type_traits>
#include <queue>
#include <optional>

/// WHAT IS THIS?
///
/// SyncVariables are a variable wrapper designed for multithreaded work.
/// One thread writes/reads, another just reads. The ISyncVarsHandler ensures they remain
/// synced without race-condition dangers.

/// HOW TO USE:
/// 1. Module inherits ISyncVarsHandler
/// 2. Call SyncVars() in Poll(), and SyncVars() in Process(). So they run once every frame. (Do you lack a Process() override? See table below)
/// 3. That's it. Your synced variables should be okay now. Have fun!
/// -Ark


/// Methods for deploying SyncVars() calls.
/// Main thread.
/// - Poll(): runs every frame, even offscreen.
/// Audio thread.
/// - Process(): every buffer tick, about 187.5 times per sec on default 48kh/256(buffer size).
/// - OnTimeEvent(): developer's choice.
/// - OnTransportAdvanced(): same as Process()

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


//Debug only guards.
#if DEBUG || BESPOKE_NIGHTLY
#define rule(x) assert(x)
#else
#define rule(x) ((void)0)
#endif


class ISyncVarsHandler;
namespace isv
{
   class ISyncVarBase;
   class ISyncPipeBase;
   class ISyncVarsHandler
   {
   public:
      void SyncVars() const;
      void InternalSyncVariableRegister(ISyncVarBase* var);
      void InternalSyncVariableRegister(ISyncPipeBase* var);

   private:
      std::vector<ISyncVarBase*> mManagedVariables;
      std::vector<ISyncPipeBase*> mManagedPipes;
   };

   template <typename>
   struct is_unique_ptr : std::false_type
   {
   };
   template <typename U, typename D>
   struct is_unique_ptr<std::unique_ptr<U, D>> : std::true_type
   {
   };

   //TODO investigate thread_local storage for a possible x50 performance increase
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

   class ISyncPipeBase
   {
   public:
      explicit ISyncPipeBase(ISyncVarsHandler* handler)
      {
         if (handler)
            handler->InternalSyncVariableRegister(this);
      }
      ISyncPipeBase() = default; //Can work unmanaged, but needs manual "pumping" via UpdatePipes to push queues along.

      std::atomic<bool> mNeedsUpdate{ false };
      virtual void UpdatePipes() = 0;
      virtual bool CheckNeedsUpdate() = 0;
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
         static_assert(!std::is_pointer_v<T>, "Raw pointers are not supported in SyncVariables. Use shared_ptr instead.");
         static_assert(!is_unique_ptr<std::remove_cv_t<std::remove_reference_t<T>>>::value,
                       "unique_ptr is not supported in SyncVariables. Use shared_ptr instead.");
         static_assert(std::is_copy_constructible_v<T>, "Non-copyable types are not supported in SyncVariables.");
         handler->InternalSyncVariableRegister(this);
         mThreadOwner = threadOwner;
      }
      void OverrideWithChanges(syncOperation<T> change)
      {
         rule(mThreadOwner == threadId()); //Writing to a synced var from the non owning thread is a contractual violation.
         if (queuedChanges.size() != 1)
            queuedChanges.resize(1);
         queuedChanges[0] = change;
         hasQueue = true;
      }
      void QueueChange(syncOperation<T> change)
      {
         rule(mThreadOwner == threadId()); //Writing to a synced var from the non owning thread is a contractual violation.
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

/// Safely synced type across threads.
/// Supports copy-constructible types. (ex: int, float, most classes)
///
/// Does not support T*(memory leaks), nor unique_ptr<T>(implementation quirks), if you need references use shared_ptr.
///
/// NOTE: shared_ptr objects ARE shared across threads. The shared_ptr itself is not. (ex: if you nullptr the shared_ptr, it will be null in owning thread
/// and become nullptr next sync on the other thread)
///
/// Requires brief implementation setup. See the SyncVariables.h header.
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

/// Like a vector<T>, but safely synced across threads.
/// Supports copy-constructible types. (ex: int, float, most classes)
///
/// Does not support T*(memory leaks), nor unique_ptr<T>(implementation quirks), if you need references use shared_ptr.
///
/// NOTE: shared_ptr objects ARE shared across threads, the vector is not.
///
/// Requires brief implementation setup. See the SyncVariables.h header.
template <typename T>
class syncVector : public isv::ISyncVarType<T>
{
public:
   syncVector(ISyncVarsHandler* handler, char threadOwner = 0)
   : isv::ISyncVarType<T>(handler, threadOwner)
   {}

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
      this->OverrideWithChanges(isv::opClear, {});
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
   //Returns true on success.
   bool removeThis(T& match)
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
   const T& get(size_t i)
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
      borrowedVector(std::vector<T>* data, syncVector* owner, bool batch)
      : mData(data)
      , mOwner(owner)
      , mBatch(batch)
      {
         rule(mOwner ? (mOwner->mThreadOwner == isv::threadId()) : true); //Can only borrow write on owning thread.
      }
      std::vector<T>* mData;
      syncVector* mOwner;
      bool mBatch{ false };

   public:
      ~borrowedVector()
      {
         if (!mOwner)
            return;

         this->mOwner->OverrideWithChanges(syncOperation<T>(std::vector<T>(mData)));
      }
      auto begin() { return mData->begin(); }
      auto end() { return mData->end(); }
      const T& get(size_t i) const { return (*mData)[i]; }
      void set(size_t i, T value)
      {
         mOwner->queuedChanges.push_back(isv::syncOperation<T>(isv::opWrite, value, i));
         mData[i] = value;
      }
      size_t size() const { return mData->size(); }
      void push_back(T var)
      {
         mOwner->queuedChanges.push_back(isv::syncOperation<T>(isv::opPush, var));
         mData->push_back(var);
      }
      void pop_back()
      {
         mOwner->queuedChanges.push_back(isv::syncOperation<T>(isv::opPop));
         mData->pop_back();
      }
      void erase(size_t i)
      {
         mOwner->queuedChanges.push_back(isv::syncOperation<T>(isv::opErase, {}, i));
         auto pos = mData->begin();
         mData->erase(pos + i);
      }
      void clear()
      {
         mOwner->queuedChanges.resize(1);
         mOwner->queuedChanges[0] = isv::syncOperation<T>(isv::opClear, {});
         mData->clear();
      }

      //Fast, does not queue individual actions. Best for borrowBatch
      void pop_back_f() { mData->pop_back(); }
      //Fast, does not queue individual actions. Best for borrowBatch
      void push_back_f(T var) { mData->push_back(var); }
      //Fast, does not queue individual actions. Best for borrowBatch
      void erase_f(size_t i) { mData->erase(mData->begin() + i); }
      //Fast, does not queue individual actions. Best for borrowBatch
      void set_f(size_t i, T value) { mData[i] = value; }

      //Consts
      auto begin() const { return mData->begin(); }
      auto end() const { return mData->end(); }
      const T& operator[](size_t i) const { return (*mData)[i]; }
   };

   //Get the internal vector for batch operations. Both Read and Write.
   //Changes are automatically handled on scope end.
   //Only allowed on owning thread,
   borrowedVector borrowWrite() { return borrowedVector(vec[isv::threadId()], this, false); }
   //Get the internal vector for batch operations. Both Read and Write.
   //Best for full vector rewrites and _f use function use. (Fully copies, rather than individual changes)
   //Only allowed on owning thread,
   borrowedVector borrowBatch() { return borrowedVector(vec[isv::threadId()], this, true); }
   //Get the internal vector for batch operations. Only Read.
   //No sync overhead.
   //Allowed on any thread.
   //
   //WARNING: C++ allows you to modify objects within a borrowRead via method calls.
   //borrowRead() only protects against vector modifications. Object changes will NOT be synced, and
   //writes in shared object variables may cause data-races. Be careful!
   borrowedVector borrowRead() const { return borrowedVector(vec[isv::threadId()], nullptr, false); }

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

/// Bidirectional set of pipes. Can easily, and safely transfer data from one thread to another and vise versa.
/// Push stuff in, extract the other side. First-In-First-Out
/// Can queue up multiple transfers.
///
/// TIP: If you need to move a lot of data, fill a vector and push it through.
///
/// TIP2: Unlike most syncVars. syncPipes work ok unmanaged.
/// But UpdatePipes() needs to be called manually before pushing.
template <typename T>
class syncPipe : isv::ISyncPipeBase
{
   syncPipe() = default;
   struct pipeMessage
   {
      pipeMessage(int destination, T var)
      {
         targetThread = destination;
         message = var;
      }
      int targetThread;
      T message;
   };

public:
   //Insert a value into the pipe to be delivered async to the other thread.
   //Inserting while its full, enqueues it and is delivered after the next sync.
   void push(T value)
   {
      int destination = !isv::threadId();
      char thread = isv::threadId();

      if (!mHasIncoming[destination])
      {
         if (mHasOutgoing[thread])
         {
            mOutgoing[thread].push_back(pipeMessage(destination, value));
            UpdatePipes();
         }
         else
         {
            mIncoming[destination].push(value);
            mHasIncoming[destination] = true;
         }
      }
      else
      {
         mOutgoing[thread].push_back(pipeMessage(destination, value));
         mHasOutgoing[thread] = true;
         mNeedsUpdate = true;
      }
   }
   //With no arguments, simply pushes the queue into the delivery pipe if empty.
   void push()
   {
      UpdatePipes();
   }
   //Push into the pipe queue, without sending it.
   //Do not forget to send it by calling push() later.
   //
   //TIP: If you need to send a lot of data at once, use vectors.
   void queue(T value)
   {
      char thread = isv::threadId();
      mOutgoing[thread].push_back(value);
      mHasOutgoing[thread] = true;
      mNeedsUpdate = true;
   }

   //Checks to see if the pipe has any data going out.
   //Sees queued outgoing content.
   bool hasContentOutgoing()
   {
      return mHasOutgoing[isv::threadId()];
   }
   //Checks to see if the pipe has any data going in.
   //Checks the content of the incoming pipe.
   //Returns the number of entries.
   size_t hasContentIncoming()
   {
      char thread = isv::threadId();
      if (!mHasIncoming[thread])
         return 0;
      return mIncoming[thread].size();
   }

   //Returns the incoming data in the pipe as a pointer.
   //Does not empty the pipe.
   //Thread relative.
   T* peek()
   {
      char thread = isv::threadId();
      if (mHasIncoming[thread])
         return &mIncoming[thread].front();
      return nullptr;
   }
   //Returns the incoming data in the pipe as a copy.
   //Empties the pipe by one. Returns nothing if empty.
   //Thread relative.
   std::optional<T> extract()
   {
      char thread = isv::threadId();
      if (!mHasIncoming[thread])
         return std::nullopt;

      auto out = std::make_optional(mIncoming[thread]->front());
      mIncoming[thread]->pop();
      mHasIncoming[thread] = !mIncoming->empty();
      return out;
   }
   //Empties our side of the pipe.
   //Disposes incoming pipe.
   void flushIncoming()
   {
      char thread = isv::threadId();
      auto& pipe = mIncoming[thread];
      const int sz = pipe.size();
      for (int i = 0; i < sz; ++i)
         pipe.pop();
      mHasIncoming[thread] = false;
   }
   //Empties our side of the pipe.
   //Disposes outgoing pipe and queues.
   void flushOutgoing()
   {
      mOutgoing[isv::threadId()].clear();
      mHasOutgoing[isv::threadId()] = false;
   }
   //Push forward queues.
   void UpdatePipes() override
   {
      char thread = isv::threadId();
      if (auto& hasQueue = mHasOutgoing[thread])
      {
         std::array<bool, kThreads> threadsToFlagFull{ false };
         auto& queueVec = mOutgoing[thread];
         for (size_t i = 0; i < queueVec.size(); ++i)
         {
            pipeMessage item = queueVec[i];
            if (!mHasIncoming[item.targetThread])
            {
               mIncoming[item.targetThread].push(item.message);
               threadsToFlagFull[item.targetThread] = true;
               queueVec.erase(queueVec.begin() + i);
               --i;
               if (queueVec.empty())
               {
                  hasQueue = false;
                  break;
               }
            }
         }
         for (int i = 0; i < kThreads; ++i)
         {
            if (threadsToFlagFull[i])
               mHasIncoming[i] = true;
         }
      }
   }
   bool CheckNeedsUpdate() override
   {
      for (int i = 0; i < kThreads; ++i)
      {
         if (mHasOutgoing[i])
            return false;
      }
      mNeedsUpdate = true;
      return true;
   };


private:
   static constexpr int kThreads{ 2 }; //0 main, 1 audio.

   std::atomic<bool> mHasIncoming[kThreads]{ false };
   std::atomic<bool> mHasOutgoing[kThreads]{ false };

   //They're kinda like mailboxes.
   std::queue<T> mIncoming[kThreads];
   std::vector<pipeMessage> mOutgoing[kThreads];
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
      for (const auto var : mManagedPipes)
      {
         //Check if pipes need management, generally they're okay. But if any pipe queues, it needs updating.
         if (var->mNeedsUpdate)
         {
            var->UpdatePipes();
            var->CheckNeedsUpdate();
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
   void InternalSyncVariableRegister(isv::ISyncPipeBase* var)
   {
      mManagedPipes.push_back(var);
   }

private:
   std::vector<isv::ISyncVarBase*> mManagedVariables;
   std::vector<isv::ISyncPipeBase*> mManagedPipes;
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