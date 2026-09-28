# Borrowed 7-Zip sources

Source: official 7-Zip sibling checkout, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

The following files are copied byte for byte, preserving upstream paths.
Their dependency headers are included so the numeric parser and portable CPU/RAM discovery can compile on all
three supported platforms, alongside the portable folder/COM/property contract.
They are not formatted with the Qt application's rules.

- `C/7zTypes.h`
- `C/7zWindows.h`
- `C/Compiler.h`
- `C/Precomp.h`
- `C/Threads.h`
- `C/CpuArch.h`
- `CPP/Common/Common.h`
- `CPP/Common/Common0.h`
- `CPP/Common/Defs.h`
- `CPP/Common/MyVector.h`
- `CPP/Common/MyGuidDef.h`
- `CPP/Common/MyTypes.h`
- `CPP/Common/MyWindows.h`
- `CPP/Common/MyWindows.cpp`
- `CPP/Common/MyCom.h`
- `CPP/Common/MyUnknown.h`
- `CPP/Common/MyInitGuid.h`
- `CPP/Common/ComTry.h`
- `CPP/Common/MyBuffer.h`
- `CPP/Common/MyString.cpp`
- `CPP/Common/MyString.h`
- `CPP/Common/IntToString.cpp`
- `CPP/Common/IntToString.h`
- `CPP/Common/StringConvert.cpp`
- `CPP/Common/StringConvert.h`
- `CPP/Common/UTFConvert.cpp`
- `CPP/Common/UTFConvert.h`
- `CPP/Common/NewHandler.h`
- `CPP/Common/StdAfx.h`
- `CPP/Common/StringToInt.cpp`
- `CPP/Common/StringToInt.h`
- `CPP/Windows/StdAfx.h`
- `CPP/Windows/System.cpp`
- `CPP/Windows/System.h`
- `CPP/Windows/PropVariant.cpp`
- `CPP/Windows/PropVariant.h`
- `CPP/Windows/PropVariantConv.h`
- `CPP/Windows/WinDefs.h`
- `CPP/7zip/PropID.h`
- `CPP/7zip/IDecl.h`
- `CPP/7zip/IProgress.h`
- `CPP/7zip/IStream.h`
- `CPP/7zip/UI/FileManager/IFolder.h`
- `CPP/7zip/UI/Common/ExtractMode.h`
- `License.txt`

Additional portable filesystem dependencies:

- `CPP/Common/MyLinux.h`
- `CPP/Windows/FileFind.cpp` and `FileFind.h`
- `CPP/Windows/FileIO.cpp` and `FileIO.h` (implementation built on Windows)
- `CPP/Windows/FileName.h` (adapted implementation under `src/CPP/Windows`)
- `CPP/Windows/TimeUtils.cpp` and `TimeUtils.h`

All 52 source/header files and `License.txt` (from upstream `DOC/License.txt`)
are retained byte for byte from the exact commit above.
This provenance check does not establish runtime verification on all platforms.
`.gitattributes` excludes unchanged vendored source from Git whitespace errors
so upstream trailing spaces remain byte-for-byte intact. Local documentation,
adapted source and tests keep the normal whitespace checks.

See `License.txt` for the upstream license and `docs/upstream-port-audit.md`
for the adapted functions outside this directory.
