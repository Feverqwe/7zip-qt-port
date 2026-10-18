# Object-vector copy exception ownership — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed the complete `CPP/Common/MyVector.h`: record storage, object allocation
and registration, constructors, assignment/append, removal, sorting and
destruction. Also reviewed its Common/MyString dependencies, unchanged direct
header consumers (Windows/System and 7zip/Common stream/progress/memory headers),
and nested owners such as compression/extraction properties and ZIP extra blocks.
The original header was byte-identical to that upstream commit before adaptation.

The copy constructor allocated pointer storage and then copied objects into it.
If a later object copy threw, previously registered objects were not destroyed:
only the member pointer array's destructor ran. Those objects could retain
buffers, strings or COM owners. The copy constructor now delegates to the
existing default constructor before its unchanged allocation/copy loop. A
completed target construction makes exception unwinding invoke the existing
object-vector destructor. This reuses the original destruction algorithm;
assignment, append and successful copy semantics remain unchanged. No source
body, dependency or branch is omitted.

The sole implementation moves to `src/CPP/Common/MyVector.h`. Its Common.h
dependency is rooted through the existing CPP include path. Project source
include roots let unchanged upstream headers continue to use their original
plain, one-level and two-level relative spellings after the move. This avoids
vendor forwarding copies or independent container implementations. The include
change affects production modules and standalone tests, so runtime validation
must cover the whole default target graph. Borrowed formatting is preserved
explicitly; the constructor adaptation was formatted narrowly.

The standalone `object_vector_lifetime` regression copies entries that retain a
resource and throws from the second entry copy. It checks propagation, continued
use and copying of the original, and release of the resource after all valid
owners leave scope. This reproduced a retained-resource leak before the fix and
passed afterwards on macOS. It uses public container behavior without private
fields, source-text assertions or pinning allocation counts.

The external official engine/module binaries retain their separate upstream
implementation. This adaptation applies to the project's compiled owners and
does not change the read-only upstream checkout or patch those external images.

The regression and direct consumers passed on macOS arm64, Linux x64 and Windows
x64 / Qt 6.12. The focused application checks passed eighteen suites on macOS,
nineteen on Linux (`20261018T104509Z-456be6c1`), and nineteen on Windows
(`20261018T104113Z-08a7e8e0`). The Windows batch's additional native_shell_com
suite passed its functional cases and then crashed during shutdown; that separate
module-lifetime issue remains open and is not accepted by this change.

Because the header and include roots affect the whole target graph, validation
also attempted the full default gate. Formatting stopped on a previous reopen
call and concurrent localization work; the reopen line was formatted narrowly.
Separate all-target builds completed on macOS/Linux. Their default runs passed
313/338 and 332/340 suites respectively, with outstanding localization, geometry,
dialog-timeout and window-lifetime failures. Windows stopped in a concurrent
shell-test change that used UInt32 before including its source type declaration;
the focused follow-up compiled that consumer after the include was corrected.
Thus no full gate is claimed. Opt-in slow matrices were excluded throughout.

An initial local harness exported a disposable Git index into tests with their
own temporary repositories, invalidating the Git-fixture results. Those two
suites passed when rerun with the normal environment. The canonical source
verifier passed separately with an isolated index representing the concurrent
parser/ErrorMsg moves; those changes remain outside this commit. Existing
incremental builds and compiler caches were preserved.
