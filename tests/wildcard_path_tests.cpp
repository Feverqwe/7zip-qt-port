#include "CPP/Common/Wildcard.h"
#include <QtTest>

class WildcardPathTests final : public QObject {
    Q_OBJECT
    static void add(NWildcard::CCensorNode &node, bool include, const QString &path, bool recursive,
                    bool files, bool directories, bool wildcards = true) {
        NWildcard::CItem item;
        SplitPathToParts(UString(path.toStdWString().c_str()), item.PathParts);
        item.Recursive = recursive;
        item.ForFile = files;
        item.ForDir = directories;
        item.WildcardMatching = wildcards;
        node.AddItem(include, item);
    }
    static void check(const NWildcard::CCensorNode &node, const QString &path, bool file,
                      bool found, bool allowed) {
        UStringVector parts;
        SplitPathToParts(UString(path.toStdWString().c_str()), parts);
        bool include = !allowed;
        QCOMPARE(node.CheckPathVect(parts, file, include), found);
        QCOMPARE(include, allowed);
    }
  private slots:
    void ancestorFallbackAndExclusion() {
        NWildcard::CCensorNode node;
        add(node, true, "*.bin", true, true, false);
        add(node, true, "deep/specific.txt", false, true, false);
        add(node, false, "deep/private/*", true, true, true);
        add(node, true, "deep/folder", false, false, true);
        check(node, "deep/specific.txt", true, true, true);
        check(node, "deep/other.txt", true, false, false);
        check(node, "deep/other.bin", true, true, true);
        check(node, "other/item.bin", true, true, true);
        check(node, "deep/private/item.bin", true, true, false);
        check(node, "deep/private/item.txt", true, true, false);
        check(node, "deep/other.bin", false, false, false);
        check(node, "deep/folder", false, true, true);
        check(node, "deep/folder", true, false, false);
        check(node, "deep/folder/child.txt", true, true, true);
    }
    void literalAndWildcardComponents() {
        NWildcard::CCensorNode node;
        add(node, true, "top/a?.bin", false, true, false);
        add(node, true, "path/literal?.bin", false, true, false, false);
        add(node, true, "nested/*/leaf.txt", false, true, false);
        add(node, true, "recursive/*/leaf.txt", true, true, false);
        check(node, "top/a1.bin", true, true, true);
        check(node, "top/a12.bin", true, false, false);
        check(node, "path/literal?.bin", true, true, true);
        check(node, "path/literalX.bin", true, false, false);
        check(node, "nested/one/leaf.txt", true, true, true);
        check(node, "nested/one/other.txt", true, false, false);
        check(node, "nested/one/two/leaf.txt", true, false, false);
        check(node, "recursive/one/two/leaf.txt", true, true, true);
    }
    void deepSelectionAndCleanup() {
        NWildcard::CCensorNode node;
        const QString prefix = QString("D/").repeated(4096);
        add(node, true, prefix + "END.BIN", false, true, false);
        check(node, prefix + "END.BIN", true, true, true);
        check(node, prefix + "OTHER.BIN", true, false, false);
        check(node, prefix + "END.BIN", false, false, false);
    }
};

QTEST_GUILESS_MAIN(WildcardPathTests)
#include "wildcard_path_tests.moc"
