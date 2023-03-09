#include "logging_file.hpp"

namespace moonlight
{

LoggingFile::LoggingFile()
{}

LoggingFile::LoggingFile(const char* filename, OpenMode open_mode)
    : m_filename(filename)
{
    switch (open_mode)
    {
    case Append:
        m_ofile.open(filename, std::ios::app);
        break;
    case SeekEnd:
        m_ofile.open(filename, std::ios::ate);
        break;
    case Truncate:
        m_ofile.open(filename, std::ios::trunc);
        break;
    default:
        m_ofile.open(filename, std::ios::app);
        break;
    }
}

LoggingFile::~LoggingFile()
{
    m_ofile.close();
}

void LoggingFile::close()
{
    m_ofile.close();
}

void LoggingFile::open(const char* filename, OpenMode open_mode)
{
    m_ofile.open(filename, open_mode);
}

void LoggingFile::open(OpenMode open_mode)
{
    m_ofile.open(m_filename, open_mode);
}

}