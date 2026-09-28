# Borrowed 7-Zip sources

Source: official 7-Zip sibling checkout, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

The following files are copied byte for byte, preserving upstream paths.
Their dependency headers are included so the numeric parser and portable CPU/RAM discovery can compile on all
three supported platforms. They are not formatted with the Qt application's rules.

- `C/7zTypes.h`
- `C/7zWindows.h`
- `C/Compiler.h`
- `C/Precomp.h`
- `C/Threads.h`
- `CPP/Common/Common.h`
- `CPP/Common/Common0.h`
- `CPP/Common/Defs.h`
- `CPP/Common/MyVector.h`
- `CPP/Common/MyGuidDef.h`
- `CPP/Common/MyTypes.h`
- `CPP/Common/MyWindows.h`
- `CPP/Common/NewHandler.h`
- `CPP/Common/StdAfx.h`
- `CPP/Common/StringToInt.cpp`
- `CPP/Common/StringToInt.h`
- `CPP/Windows/StdAfx.h`
- `CPP/Windows/System.cpp`
- `CPP/Windows/System.h`
- `License.txt`

See `License.txt` for the upstream license and `docs/upstream-port-audit.md`
for the adapted functions outside this directory.
