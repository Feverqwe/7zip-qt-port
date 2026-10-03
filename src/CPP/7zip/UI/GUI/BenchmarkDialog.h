// BenchmarkDialog.h

#ifndef ZIP7_INC_BENCHMARK_DIALOG_H
#define ZIP7_INC_BENCHMARK_DIALOG_H

// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "7zip/Common/CreateCoder.h"
#include "7zip/UI/Common/Property.h"
class QWidget;
class IQtBenchmarkServices;

const UInt32 k_NumBenchIterations_Default = 10;

HRESULT Benchmark(DECL_EXTERNAL_CODECS_LOC_VARS const CObjectVector<CProperty> &props,
                  UInt32 numIterations, QWidget *hwndParent = NULL,
                  IQtBenchmarkServices *services = nullptr);

#endif
