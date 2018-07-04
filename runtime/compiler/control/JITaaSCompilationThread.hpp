#ifndef JITaaS_COMPILATION_THREAD_H
#define JITaaS_COMPILATION_THREAD_H

#include <unordered_map>
#include "control/CompilationThread.hpp"
#include "rpc/J9Client.hpp"
#include "env/PersistentCollections.hpp"
#include "env/j9methodServer.hpp"

class TR_PersistentClassInfo;
class TR_IPBytecodeHashTableEntry;

using IPTable_t = PersistentUnorderedMap<uint32_t, TR_IPBytecodeHashTableEntry*>;

class ClientSessionData
   {
   public:
   struct ClassInfo
      {
      J9ROMClass *romClass; // romClass content exists in persistentMemory at the server
      J9Method *methodsOfClass;
      // Fields meaningful for arrays
      TR_OpaqueClassBlock *baseComponentClass; 
      int32_t numDimensions;
      PersistentUnorderedMap<TR_RemoteROMStringKey, std::string> *_remoteROMStringsCache; // cached strings from the client
      PersistentUnorderedMap<int32_t, std::string> *_fieldOrStaticNameCache;
      TR_OpaqueClassBlock *parentClass;
      };

   struct J9MethodInfo
      {
      J9ROMMethod *_romMethod; // pointer to local/server cache
      // The following is a hashtable that maps a bcIndex to IProfiler data
      // The hashtable is created on demand (nullptr means it is missing)
      IPTable_t *_IPData;
      };

   TR_PERSISTENT_ALLOC(TR_Memory::ClientSessionData)
   ClientSessionData(uint64_t clientUID);
   ~ClientSessionData();

   void setJavaLangClassPtr(TR_OpaqueClassBlock* j9clazz) { _javaLangClassPtr = j9clazz; }
   TR_OpaqueClassBlock * getJavaLangClassPtr() const { return _javaLangClassPtr; }
   void * getSystemClassLoader() const { return _systemClassLoader; }
   void   setSystemClassLoader(void * cl) { _systemClassLoader = cl; }
   PersistentUnorderedMap<TR_OpaqueClassBlock*, TR_PersistentClassInfo*> & getCHTableClassMap() { return _chTableClassMap; }
   PersistentUnorderedMap<J9Class*, ClassInfo> & getROMClassMap() { return _romClassMap; }
   PersistentUnorderedMap<J9Method*, J9MethodInfo> & getJ9MethodMap() { return _J9MethodMap; }
   PersistentUnorderedMap<std::string, TR_OpaqueClassBlock*> & getSystemClassByNameMap() { return _systemClassByNameMap; }
   void processUnloadedClasses(const std::vector<TR_OpaqueClassBlock*> &classes);
   TR::Monitor *getROMMapMonitor() { return _romMapMonitor; }
   TR::Monitor *getSystemClassMapMonitor() { return _systemClassMapMonitor; }
   TR_IPBytecodeHashTableEntry *getCachedIProfilerInfo(TR_OpaqueMethodBlock *method, uint32_t byteCodeIndex, bool *methodInfoPresent);
   bool cacheIProfilerInfo(TR_OpaqueMethodBlock *method, uint32_t byteCodeIndex, TR_IPBytecodeHashTableEntry *entry);

   void incInUse() { _inUse++; }
   void decInUse() { _inUse--; TR_ASSERT(_inUse >= 0, "_inUse=%d must be positive\n", _inUse); }
   bool getInUse() const { return _inUse; }

   void updateTimeOfLastAccess();
   int64_t getTimeOflastAccess() const { return _timeOfLastAccess; }

   void printStats();

   private:
   uint64_t _clientUID; // for RAS
   int64_t  _timeOfLastAccess; // in ms
   TR_OpaqueClassBlock *_javaLangClassPtr; // nullptr means not set
   void *              _systemClassLoader; // declared as void* so that we don't try to dereference it
   PersistentUnorderedMap<TR_OpaqueClassBlock*, TR_PersistentClassInfo*> _chTableClassMap; // cache of persistent CHTable
   PersistentUnorderedMap<J9Class*, ClassInfo> _romClassMap;
   // Hashtable for information related to one J9Method
   PersistentUnorderedMap<J9Method*, J9MethodInfo> _J9MethodMap;
   // The following hashtable caches <classname> --> <J9Class> mappings
   // All classes in here are loaded by the systemClassLoader so we know they cannot be unloaded
   PersistentUnorderedMap<std::string, TR_OpaqueClassBlock*> _systemClassByNameMap;
 
   TR::Monitor *_romMapMonitor;
   TR::Monitor *_systemClassMapMonitor;
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
   ClientSessionData * findClientSession(uint64_t clientUID);
   void purgeOldDataIfNeeded();
   void printStats();

   private:
   PersistentUnorderedMap<uint64_t, ClientSessionData*> _clientSessionMap;

   uint64_t _timeOfLastPurge;
   const int64_t TIME_BETWEEN_PURGES; // ms; this defines how often we are willing to scan for old entries to be purged
   const int64_t OLD_AGE;// ms; this defines what an old entry means
                         // This value must be larger than the expected life of a JVM
   }; // ClientSessionHT

size_t methodStringLength(J9ROMMethod *);
std::string packROMClass(J9ROMClass *, TR_Memory *);
bool handleServerMessage(JITaaS::J9ClientStream *, TR_J9VM *);
TR_MethodMetaData *remoteCompile(J9VMThread *, TR::Compilation *, TR_ResolvedMethod *,
      J9Method *, TR::IlGeneratorMethodDetails &, TR::CompilationInfoPerThreadBase *);
void remoteCompilationEnd(TR::IlGeneratorMethodDetails &details, J9JITConfig *jitConfig,
      TR_FrontEnd *fe, TR_MethodToBeCompiled *entry, TR::Compilation *comp);
void printJITaaSMsgStats(J9JITConfig *);
void printJITaaSCHTableStats(J9JITConfig *, TR::CompilationInfo *);
void printJITaaSCacheStats(J9JITConfig *, TR::CompilationInfo *);
class JITaaSHelpers
   {
   public:
   static void cacheRemoteROMClass(ClientSessionData *clientSessionData, J9Class *clazz, J9ROMClass *romClass,
      J9Method *methods, TR_OpaqueClassBlock *baseComponentClass, int32_t numDimensions, 
      TR_OpaqueClassBlock *parentClass);
   static J9ROMClass *getRemoteROMClassIfCached(ClientSessionData *clientSessionData, J9Class *clazz);
   };

#endif
