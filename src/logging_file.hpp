#pragma once
#include <fstream>
#include <string>

namespace moonlight
{

class LoggingFile
{
public:

    enum OpenMode
    {
        Append = 0, // Appends to the end of the stream before each write
        SeekEnd,    // Seeks to the end of the stream when the file is opened
        Truncate    // Erases the file contents, when the file is opened
    };

    LoggingFile();
    LoggingFile(const std::string& filename, OpenMode open_mode);
    ~LoggingFile();

    void close();
    void open(OpenMode open_mode);
    void open(const std::string& filename, OpenMode open_mode);

    template<typename T>
    LoggingFile& operator<<(T input)
    {
        m_ofile << input;
        return *this;
    }

private:

    std::string m_filename;
    std::ofstream m_ofile;
};

}