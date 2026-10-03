// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Partial CompressCall.h scope: Benchmark declaration only; see upstream-port-audit.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "7zip/Common/CreateCoder.h"
class QWidget;
class IQtBenchmarkServices;
HRESULT Benchmark(bool totalMode, const CExternalCodecs *externalCodecs, QWidget *parent,
                  IQtBenchmarkServices *services = nullptr);
