#include "logging_file.hpp"

namespace moonlight
{

LoggingFile::LoggingFile()
{}

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
        ofile.open(filename, std::ios::app);
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

void LoggingFile::open(const char* filename, OpenMode open_mode)
{
    ofile.open(filename, open_mode);
}

void LoggingFile::open(OpenMode open_mode)
{
    ofile.open(filename, open_mode);
}

}