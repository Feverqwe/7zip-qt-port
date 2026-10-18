#pragma once
#include "layout_inspection.h"
#include <QtTest>

// Preserve QtTest's application setup and source location, then apply the
// opt-in review appearance before constructing any test fixtures or widgets.
#define SEVENZIP_UI_TEST_MAIN(TestObject)                                                          \
    QTEST_MAIN_WRAPPER(TestObject, QTEST_MAIN_SETUP() UiInspection::configureReviewAppearance();)
