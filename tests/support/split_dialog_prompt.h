#pragma once
#include <QString>

class CSplitDialog;
struct SplitPromptObservation {
    bool shown = false;
    bool sourcePolicy = false;
    bool geometry = false;
    QString text;
};
SplitPromptObservation SubmitInvalidSplit(CSplitDialog &dialog, const QString &report = {},
                                          bool escape = false);
