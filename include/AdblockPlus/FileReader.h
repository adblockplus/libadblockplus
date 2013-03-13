#ifndef ADBLOCKPLUS_FILE_READER_H
#define ADBLOCKPLUS_FILE_READER_H

#include <istream>
#include <string>
#include <memory>

namespace AdblockPlus
{
  class FileReader
  {
  public:
    virtual ~FileReader();
    virtual std::auto_ptr<std::istream> Read(const std::string& path) const = 0;
  };
}

#endif
