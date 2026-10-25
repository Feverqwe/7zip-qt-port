// Opt-in native client fixture using the same server adapter as the product.
// Fixed credentials belong to synthetic data, never to an OS account.
#include "smbreadonlyserver.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
    if (argc != 2 && argc != 3)
        return 2;
    constexpr quint64 size = (quint64(5) << 30) + 123;
    const bool trace = argc == 3 && QString::fromUtf8(argv[2]) == "--trace";
    QList<SmbTreeEntry> entries{
        {1, 0, 0, 0, {}, true},
        {2, 1, 65536, 133800000000000007ULL, QStringLiteral("hello.txt"), false},
        {3, 1, 262144, 116444736000000000ULL, QStringLiteral("Файл с пробелом.bin"), false},
        {4, 1, size, 110000000000000000ULL, QStringLiteral("large.bin"), false}};
    SmbReadOnlyServer server(std::move(entries), QStringLiteral("szprobe"),
                             QStringLiteral("LocalProbeOnly427"),
                             [trace](quint64, quint64 offset, quint32 length) {
                                 if (trace)
                                     std::fprintf(stderr, "fixture read\n");
                                 SmbTreeRead result;
                                 result.Data.resize(length);
                                 for (quint32 i = 0; i < length; ++i)
                                     result.Data[i] = char((offset + i) % 251);
                                 return result;
                             });
    QString error;
    if (trace)
        server.setDiagnosticHandler(
            [](const QString &message) { std::fprintf(stderr, "%s\n", qPrintable(message)); });
    if (!server.validate(error)) {
        std::fprintf(stderr, "%s\n", qPrintable(error));
        return 2;
    }
    return server.serve(quint16(std::strtoul(argv[1], nullptr, 10)),
                        [](quint16 port) {
                            std::fprintf(stderr, "loopback SMB fixture ready on port %u\n", port);
                        }) == 0
               ? 0
               : 1;
}
