#pragma once
#include <fstream>

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
    LoggingFile(const char* filename, OpenMode open_mode);
    ~LoggingFile();

    void close();
    void open(OpenMode open_mode);
    void open(const char* filename, OpenMode open_mode);

    template<typename T>
    LoggingFile& operator<<(T input)
    {
        ofile << input;
        return *this;
    }

private:

    const char* filename;
    std::ofstream ofile;
};

}