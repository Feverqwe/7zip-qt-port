#pragma once
#include "benchmark/qtbenchmarkservices.h"
#include <atomic>
#include <chrono>
#include <string>
#include <thread>

class FixtureBenchmarkServices : public IQtBenchmarkServices {
  public:
    size_t Ram = size_t(32) << 20;
    HRESULT Result = S_OK;
    bool Hold = false;
    std::string Output = "Fixture native benchmark output";
    std::atomic<unsigned> Calls{0}, Active{0}, Canceled{0};
    bool GetRamSize(size_t &size) const override {
        size = Ram;
        return true;
    }
    HRESULT Run(const CExternalCodecs *, IBenchPrintCallback *print, IBenchCallback *results,
                const CObjectVector<CProperty> &, UInt32, bool, IBenchFreqCallback *) override {
        ++Calls;
        ++Active;
        struct Exit {
            std::atomic<unsigned> &Active;
            ~Exit() { --Active; }
        } exit{Active};
        if (Result != S_OK)
            return Result;
        CBenchInfo info;
        info.GlobalTime = info.GlobalFreq = info.UserFreq = 1000;
        info.UserTime = 500;
        info.UnpackSize = UInt64(1) << 20;
        info.PackSize = UInt64(1) << 18;
        info.NumIterations = 1;
        if (print) {
            print->Print(Output.c_str());
            print->NewLine();
        }
        do {
            const HRESULT status =
                results ? results->SetEncodeResult(info, !Hold) : print->CheckBreak();
            if (status != S_OK) {
                ++Canceled;
                return status;
            }
            if (!Hold)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } while (true);
        return results ? results->SetDecodeResult(info, true) : S_OK;
    }
};
