// Adapted from 7-Zip CPP/7zip/UI/Common/CompressCall2.cpp.
// Baseline 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17, Benchmark entry only.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "../GUI/BenchmarkDialog.h"
#include "Common/ComTry.h"
#include "CompressCall.h"

HRESULT Benchmark(bool totalMode, const CExternalCodecs *externalCodecs, QWidget *parent,
                  IQtBenchmarkServices *services) {
    COM_TRY_BEGIN
    // The caller supplies the confirmed catalog instead of CREATE_CODECS and
    // LOAD_EXTERNAL_CODECS globals, and retains it until the source worker joins.
    CObjectVector<CProperty> props;
    if (totalMode) {
        CProperty prop;
        prop.Name = "m";
        prop.Value = "*";
        props.Add(prop);
    }
    return Benchmark(externalCodecs, props, k_NumBenchIterations_Default, parent, services);
    COM_TRY_END
}
