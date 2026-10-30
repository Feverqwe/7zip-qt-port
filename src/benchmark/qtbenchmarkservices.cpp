#include "benchmark/qtbenchmarkservices.h"
#include "Windows/System.h"

bool IQtBenchmarkServices::GetProcessAffinity(NWindows::NSystem::CProcessAffinity &affinity) const {
    return affinity.Get() != FALSE;
}

bool QtBenchmarkServices::GetRamSize(size_t &size) const {
    return NWindows::NSystem::GetRamSize(size);
}
HRESULT QtBenchmarkServices::Run(const CExternalCodecs *providers, IBenchPrintCallback *print,
                                 IBenchCallback *results, const CObjectVector<CProperty> &props,
                                 UInt32 iterations, bool multiDict, IBenchFreqCallback *frequency) {
    return Bench(providers, print, results, props, iterations, multiDict, frequency);
}
