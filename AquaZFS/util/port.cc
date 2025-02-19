//
// Created by chiro on 23-5-25.
//

#include "port.h"
#include <cassert>

#if defined(__i386__) || defined(__x86_64__)

#include <cpuid.h>

#endif

#include <cerrno>
#include <sched.h>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <string>

#include "util/string_util.h"

namespace aquafs::port {
const bool kDefaultToAdaptiveMutex = false;

static int PthreadCall(const char *label, int result) {
  if (result != 0 && result != ETIMEDOUT && result != EBUSY) {
    fprintf(stderr, "pthread %s: %s\n", label, errnoStr(result).c_str());
    abort();
  }
  return result;
}

Mutex::Mutex(bool adaptive) {
  (void) adaptive;
#ifdef ROCKSDB_PTHREAD_ADAPTIVE_MUTEX
  if (!adaptive) {
    PthreadCall("init mutex", pthread_mutex_init(&mu_, nullptr));
  } else {
    pthread_mutexattr_t mutex_attr;
    PthreadCall("init mutex attr", pthread_mutexattr_init(&mutex_attr));
    PthreadCall("set mutex attr", pthread_mutexattr_settype(
                                      &mutex_attr, PTHREAD_MUTEX_ADAPTIVE_NP));
    PthreadCall("init mutex", pthread_mutex_init(&mu_, &mutex_attr));
    PthreadCall("destroy mutex attr", pthread_mutexattr_destroy(&mutex_attr));
  }
#else
  PthreadCall("init mutex", pthread_mutex_init(&mu_, nullptr));
#endif  // ROCKSDB_PTHREAD_ADAPTIVE_MUTEX
}

Mutex::~Mutex() { PthreadCall("destroy mutex", pthread_mutex_destroy(&mu_)); }

void Mutex::Lock() {
  PthreadCall("lock", pthread_mutex_lock(&mu_));
#ifndef NDEBUG
  locked_ = true;
#endif
}

void Mutex::Unlock() {
#ifndef NDEBUG
  locked_ = false;
#endif
  PthreadCall("unlock", pthread_mutex_unlock(&mu_));
}

bool Mutex::TryLock() {
  bool ret = PthreadCall("trylock", pthread_mutex_trylock(&mu_)) == 0;
#ifndef NDEBUG
  if (ret) {
    locked_ = true;
  }
#endif
  return ret;
}

void Mutex::AssertHeld() {
#ifndef NDEBUG
  assert(locked_);
#endif
}

CondVar::CondVar(Mutex *mu) : mu_(mu) {
  PthreadCall("init cv", pthread_cond_init(&cv_, nullptr));
}

CondVar::~CondVar() { PthreadCall("destroy cv", pthread_cond_destroy(&cv_)); }

void CondVar::Wait() {
#ifndef NDEBUG
  mu_->locked_ = false;
#endif
  PthreadCall("wait", pthread_cond_wait(&cv_, &mu_->mu_));
#ifndef NDEBUG
  mu_->locked_ = true;
#endif
}

bool CondVar::TimedWait(uint64_t abs_time_us) {
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(abs_time_us / 1000000);
  ts.tv_nsec = static_cast<suseconds_t>((abs_time_us % 1000000) * 1000);

#ifndef NDEBUG
  mu_->locked_ = false;
#endif
  int err = pthread_cond_timedwait(&cv_, &mu_->mu_, &ts);
#ifndef NDEBUG
  mu_->locked_ = true;
#endif
  if (err == ETIMEDOUT) {
    return true;
  }
  if (err != 0) {
    PthreadCall("timedwait", err);
  }
  return false;
}

void CondVar::Signal() { PthreadCall("signal", pthread_cond_signal(&cv_)); }

void CondVar::SignalAll() {
  PthreadCall("broadcast", pthread_cond_broadcast(&cv_));
}

RWMutex::RWMutex() {
  PthreadCall("init mutex", pthread_rwlock_init(&mu_, nullptr));
}

RWMutex::~RWMutex() {
  PthreadCall("destroy mutex", pthread_rwlock_destroy(&mu_));
}

void RWMutex::ReadLock() {
  PthreadCall("read lock", pthread_rwlock_rdlock(&mu_));
}

void RWMutex::WriteLock() {
  PthreadCall("write lock", pthread_rwlock_wrlock(&mu_));
}

void RWMutex::ReadUnlock() {
  PthreadCall("read unlock", pthread_rwlock_unlock(&mu_));
}

void RWMutex::WriteUnlock() {
  PthreadCall("write unlock", pthread_rwlock_unlock(&mu_));
}

int PhysicalCoreID() {
#if defined(ROCKSDB_SCHED_GETCPU_PRESENT) && defined(__x86_64__) && \
    (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 22))
  // sched_getcpu uses VDSO getcpu() syscall since 2.22. I believe Linux offers
  // VDSO support only on x86_64. This is the fastest/preferred method if
  // available.
  int cpuno = sched_getcpu();
  if (cpuno < 0) {
    return -1;
  }
  return cpuno;
#elif defined(__x86_64__) || defined(__i386__)
  // clang/gcc both provide cpuid.h, which defines __get_cpuid(), for x86_64 and
  // i386.
  unsigned eax, ebx = 0, ecx, edx;
  if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
    return -1;
  }
  return ebx >> 24;
#else
  // give up, the caller can generate a random number or something.
  return -1;
#endif
}

void InitOnce(OnceType *once, void (*initializer)()) {
  PthreadCall("once", pthread_once(once, initializer));
}

void Crash(const std::string &srcfile, int srcline) {
  fprintf(stdout, "Crashing at %s:%d\n", srcfile.c_str(), srcline);
  fflush(stdout);
  kill(getpid(), SIGTERM);
}

int GetMaxOpenFiles() {
#if defined(RLIMIT_NOFILE)
  struct rlimit no_files_limit;
  if (getrlimit(RLIMIT_NOFILE, &no_files_limit) != 0) {
    return -1;
  }
  // protect against overflow
  if (static_cast<uintmax_t>(no_files_limit.rlim_cur) >=
      static_cast<uintmax_t>(std::numeric_limits<int>::max())) {
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(no_files_limit.rlim_cur);
#endif
  return -1;
}

void *cacheline_aligned_alloc(size_t size) {
#if __GNUC__ < 5 && defined(__SANITIZE_ADDRESS__)
  return malloc(size);
#elif (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600 || defined(__APPLE__))
  void *m;
  errno = posix_memalign(&m, CACHE_LINE_SIZE, size);
  return errno ? nullptr : m;
#else
  return malloc(size);
#endif
}

void cacheline_aligned_free(void *memblock) { free(memblock); }

static size_t GetPageSize() {
#if defined(OS_LINUX) || defined(_SC_PAGESIZE)
  long v = sysconf(_SC_PAGESIZE);
  if (v >= 1024) {
    return static_cast<size_t>(v);
  }
#endif
  // Default assume 4KB
  return 4U * 1024U;
}

const size_t kPageSize = GetPageSize();

void SetCpuPriority(ThreadId id, CpuPriority priority) {
#ifdef OS_LINUX
  sched_param param;
  param.sched_priority = 0;
  switch (priority) {
    case CpuPriority::kHigh:
      sched_setscheduler(id, SCHED_OTHER, &param);
      setpriority(PRIO_PROCESS, id, -20);
      break;
    case CpuPriority::kNormal:
      sched_setscheduler(id, SCHED_OTHER, &param);
      setpriority(PRIO_PROCESS, id, 0);
      break;
    case CpuPriority::kLow:
      sched_setscheduler(id, SCHED_OTHER, &param);
      setpriority(PRIO_PROCESS, id, 19);
      break;
    case CpuPriority::kIdle:
      sched_setscheduler(id, SCHED_IDLE, &param);
      break;
    default:
      assert(false);
  }
#else
  (void)id;
  (void)priority;
#endif
}

int64_t GetProcessID() { return getpid(); }

bool GenerateRfcUuid(std::string *output) {
  output->clear();
  std::ifstream f("/proc/sys/kernel/random/uuid");
  std::getline(f, /*&*/ *output);
  if (output->size() == 36) {
    return true;
  } else {
    output->clear();
    return false;
  }
}
}