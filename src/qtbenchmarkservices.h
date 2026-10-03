// Narrow engine/platform boundary for the original BenchmarkDialog workflow.
#pragma once
#include "CPP/7zip/UI/Common/Bench.h"

class IQtBenchmarkServices {
  public:
    virtual ~IQtBenchmarkServices() = default;
    virtual bool GetRamSize(size_t &size) const = 0;
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
