#pragma once
#include <QtGlobal>

// One-shot, local, same-architecture descriptor handoff. No paths to output
// files, archive metadata, or executable commands cross this protocol.
struct MacDeviceReply {
    qint32 error = 0;
};
