// Narrow engine/platform boundary for the original BenchmarkDialog workflow.
#pragma once
#include "Windows/System.h"
#include "benchmark/Bench.h"

class IQtBenchmarkServices {
  public:
    virtual ~IQtBenchmarkServices() = default;
    virtual bool GetRamSize(size_t &size) const = 0;
    virtual bool GetProcessAffinity(NWindows::NSystem::CProcessAffinity &affinity) const;
    virtual HRESULT Run(const CExternalCodecs *providers, IBenchPrintCallback *print,
                        IBenchCallback *results, const CObjectVector<CProperty> &props,
                        UInt32 iterations, bool multiDict, IBenchFreqCallback *frequency) = 0;
};

class QtBenchmarkServices final : public IQtBenchmarkServices {
  public:
    bool GetRamSize(size_t &size) const override;
    HRESULT Run(const CExternalCodecs *providers, IBenchPrintCallback *print,
                IBenchCallback *results, const CObjectVector<CProperty> &props, UInt32 iterations,
                bool multiDict, IBenchFreqCallback *frequency) override;
};
