#pragma once

// Read-only integration readiness at the source folder/presentation boundary.
// Command state must not depend on a concrete engine-resource implementation.
class IQtFolderPropertyCapabilities {
  public:
    virtual ~IQtFolderPropertyCapabilities() = default;
    virtual bool CanComment() const = 0;
};
