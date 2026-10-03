#include "qtbenchmarkservices.h"
#include "Windows/System.h"

#ifdef _WIN32
// FM.cpp startup state, 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// The Qt startup does not enable LockMemory privilege; its setting is disabled.
// Retain the original default for Bench's large-page diagnostic branch.
bool g_LargePagesMode = false;
#endif

bool QtBenchmarkServices::GetRamSize(size_t &size) const {
    return NWindows::NSystem::GetRamSize(size);
}
HRESULT QtBenchmarkServices::Run(const CExternalCodecs *providers, IBenchPrintCallback *print,
                                 IBenchCallback *results, const CObjectVector<CProperty> &props,
                                 UInt32 iterations, bool multiDict, IBenchFreqCallback *frequency) {
    return Bench(providers, print, results, props, iterations, multiDict, frequency);
}
