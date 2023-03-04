#include "logging_file.hpp"

namespace moonlight
{

LoggingFile::LoggingFile(const char* filename, OpenMode open_mode)
    : filename(filename)
{
    switch (open_mode)
    {
    case Append:
        ofile.open(filename, std::ios::app);
        break;
    case SeekEnd:
        ofile.open(filename, std::ios::ate);
        break;
    case Truncate:
        ofile.open(filename, std::ios::trunc);
        break;
    default:
        break;
    }
}

LoggingFile::~LoggingFile()
{
    ofile.close();
}

void LoggingFile::close()
{
    ofile.close();
}

void LoggingFile::open()
{
    ofile.open(filename, std::ios::in);
}

}