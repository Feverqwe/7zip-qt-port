# Native shell server locks — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed complete `CPP/7zip/UI/Explorer/DllExportsExplorer.cpp`, its factory,
exports, registration paths and shared ContextMenu module count, supporting
MyCom/Win32 interlocked declarations and the actual native shell COM consumers.
The source LockServer body is a success-only stub; no other implementation was
found in the upstream CPP checkout. This correction stays in that original
method and its DllCanUnloadNow consumer, without introducing another factory or
changing archive/menu algorithms, class identity or registration behavior.

[IClassFactory::LockServer](https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iclassfactory-lockserver)
requires balanced lock/unlock counts that keep the server resident. The copied
stub ignored those calls, so DllCanUnloadNow could return S_OK after the factory
was released even with outstanding server locks. The production-DLL regression
actually reproduced that result after two successful lock calls.

LockServer now increments/decrements the existing interlocked module-owner
count, alongside factory and context owners. DllCanUnloadNow reads it with the
matching interlocked operation. Each lock must be balanced independently; one
unlock does not retire another client's lock. Source object creation,
aggregation/error handling, interfaces, module symbols and dependencies remain
unchanged, with no source branch omitted. The only new behavior is the missing
COM server-lifetime contract.

The regression uses exported DllGetClassObject/DllCanUnloadNow and IClassFactory
on the actual built shell DLL. It checks residency across factory lifetimes,
partial unlock and final unload eligibility. Its scope guard balances locks and
releases the private QLibrary load even after a failed assertion. Existing tests
also retain child Explorer commands after root/factory release and unload the
DLL after the last owner. No system registration, user association change,
allocation injection or standalone Qt reproducer is involved.

Windows x64 / Qt 6.12 rebuilt the production shell DLL and selected consumers,
then passed native_shell_com, native_shell_data, native_system_menu,
native_menu_presentation, native_language_host and shell_context (6/6, 13.39 s),
run `20261018T132543Z-c78e6b14`. The initial baseline returned S_OK instead of
S_FALSE in the new lock test; its early-return QLibrary cleanup was corrected
before final acceptance so the failure cannot contaminate subsequent unload
checks. Existing incremental builds and compiler caches were preserved.
This Windows-only export change does not establish macOS/Linux runtime,
full/slow-suite or visual acceptance.
