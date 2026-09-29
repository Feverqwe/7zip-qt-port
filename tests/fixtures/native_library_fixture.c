#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
int sevenzip_fixture_value(void) {
    return 42;
}
