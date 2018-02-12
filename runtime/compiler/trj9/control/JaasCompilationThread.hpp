#ifndef JAAS_COMPILATION_THREAD_H
#define JAAS_COMPILATION_THREAD_H

#include <unordered_map>
#include "control/CompilationThread.hpp"
#include "rpc/J9Client.h"

template<typename T, typename U>
using PersistentUnorderedMapAllocator = TR::typed_allocator<std::pair<const T, U>, TR::PersistentAllocator&>;
template<typename T, typename U>
using PersistentUnorderedMap = std::unordered_map<T, U, std::hash<T>, std::equal_to<T>, PersistentUnorderedMapAllocator<T, U>>;

class TR_PersistentClassInfo;

class ClientSessionData
   {
   public:
   TR_PERSISTENT_ALLOC(TR_Memory::ClientSessionData)
   ClientSessionData();

   void setJavaLangClassPtr(TR_OpaqueClassBlock* j9clazz) { _javaLangClassPtr = j9clazz; }
   TR_OpaqueClassBlock * getJavaLangClassPtr() const { return _javaLangClassPtr; }
   PersistentUnorderedMap<TR_OpaqueClassBlock*, TR_PersistentClassInfo*> & getCHTableClassMap() { return _chTableClassMap; }

   void incInUse() { _inUse++; }
   void decInUse() { _inUse--; TR_ASSERT(_inUse >= 0, "_inUse=%d must be positive\n", _inUse); }
   bool getInUse() const { return _inUse; }

   void updateTimeOfLastAccess();
   int64_t getTimeOflastAccess() const { return _timeOfLastAccess; }

   private:
   int64_t  _timeOfLastAccess; // in ms
   TR_OpaqueClassBlock *_javaLangClassPtr; // nullptr means not set
   PersistentUnorderedMap<TR_OpaqueClassBlock*, TR_PersistentClassInfo*> _chTableClassMap; // cache of persistent CHTable
   int8_t  _inUse;  // Number of concurrent compilations from the same client 
                    // Accessed with compilation monitor in hand
   }; // ClientSessionData

// Hashtable that maps clientUID to a pointer that points to ClientSessionData
// This indirection is needed so that we can cache the value of the pointer so
// that we can access client session data without going through the hashtable.
// Accesss to this hashtable must be protected by the compilation monitor.
// Compilation threads may purge old entries periodically at the beginning of a 
// compilation. Entried with inUse > 0 must not be purged.
class ClientSessionHT
   {
   public:
   ClientSessionHT();
   ~ClientSessionHT();
   static ClientSessionHT* allocate(); // allocates a new instance of this class
   ClientSessionData * findOrCreateClientSession(uint64_t clientUID);
   void purgeOldDataIfNeeded();

   private:
   PersistentUnorderedMap<uint64_t, ClientSessionData*> _clientSessionMap;

   uint64_t _timeOfLastPurge;
   const int64_t TIME_BETWEEN_PURGES; // ms; this defines how often we are willing to scan for old entries to be purged
   const int64_t OLD_AGE;// ms; this defines what an old entry means
                         // This value must be larger than the expected life of a JVM
   }; // ClientSessionHT

size_t methodStringLength(J9ROMMethod *);
std::string packROMClass(J9ROMClass *, TR_Memory *);
bool handleServerMessage(JAAS::J9ClientStream *, TR_J9VM *);
TR_MethodMetaData *remoteCompile(J9VMThread *, TR::Compilation *, TR_ResolvedMethod *,
      J9Method *, TR::IlGeneratorMethodDetails &, TR::CompilationInfoPerThreadBase *);
void remoteCompilationEnd(TR::IlGeneratorMethodDetails &details, J9JITConfig *jitConfig,
      TR_FrontEnd *fe, TR_MethodToBeCompiled *entry, TR::Compilation *comp);
void printJaasMsgStats(J9JITConfig *);
void printJaasCHTableStats(J9JITConfig *, TR::CompilationInfo *);

#endif
