#include "hohnor/file/FileUtils.h"
#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

using namespace Hohnor;
using namespace Hohnor::FileUtils;

class FileUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        mkdir("test_files", 0755);
        
        // Create a test file with known content
        testFileName_ = "test_files/test_file.txt";
        testContent_ = "Hello, World!\nThis is a test file.\nLine 3\n";
        
        std::ofstream ofs(testFileName_);
        ofs << testContent_;
        ofs.close();
        
        // Create an empty test file
        emptyFileName_ = "test_files/empty_file.txt";
        std::ofstream emptyOfs(emptyFileName_);
        emptyOfs.close();
        
        // Create a large test file (just under 64KB)
        largeFileName_ = "test_files/large_file.txt";
        std::ofstream largeOfs(largeFileName_);
        std::string largeContent;
        for (int i = 0; i < 1000; ++i) {
            largeOfs << "This is line " << i << " of the large test file.\n";
        }
        largeOfs.close();
        
        // File names for testing
        appendFileName_ = "test_files/append_test.txt";
        nonExistentFileName_ = "test_files/non_existent.txt";
        directoryName_ = "test_files";
    }
    
    void TearDown() override {
        // Clean up test files
        unlink(testFileName_.c_str());
        unlink(emptyFileName_.c_str());
        unlink(largeFileName_.c_str());
        unlink(appendFileName_.c_str());
        rmdir("test_files");
    }
    
    std::string testFileName_;
    std::string emptyFileName_;
    std::string largeFileName_;
    std::string appendFileName_;
    std::string nonExistentFileName_;
    std::string directoryName_;
    std::string testContent_;
};

// AppendFile Tests
TEST_F(FileUtilsTest, AppendFileConstructorAndDestructor) {
    {
        AppendFile appendFile(appendFileName_);
        EXPECT_EQ(appendFile.writtenBytes(), 0);
    }
    
    // File should exist after destructor
    struct stat statbuf;
    EXPECT_EQ(stat(appendFileName_.c_str(), &statbuf), 0);
}

TEST_F(FileUtilsTest, AppendFileBasicAppend) {
    AppendFile appendFile(appendFileName_);
    
    std::string content = "Hello, World!";
    appendFile.append(content.c_str(), content.length());
    
    EXPECT_EQ(appendFile.writtenBytes(), content.length());
    
    appendFile.flush();
    
    // Verify content was written
    std::ifstream ifs(appendFileName_);
    std::string readContent((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
    EXPECT_EQ(readContent, content);
}

TEST_F(FileUtilsTest, AppendFileMultipleAppends) {
    AppendFile appendFile(appendFileName_);
    
    std::string content1 = "First line\n";
    std::string content2 = "Second line\n";
    std::string content3 = "Third line\n";
    
    appendFile.append(content1.c_str(), content1.length());
    appendFile.append(content2.c_str(), content2.length());
    appendFile.append(content3.c_str(), content3.length());
    
    size_t expectedBytes = content1.length() + content2.length() + content3.length();
    EXPECT_EQ(appendFile.writtenBytes(), expectedBytes);
    
    appendFile.flush();
    
    // Verify all content was written
    std::ifstream ifs(appendFileName_);
    std::string readContent((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
    EXPECT_EQ(readContent, content1 + content2 + content3);
}

TEST_F(FileUtilsTest, AppendFileEmptyAppend) {
    AppendFile appendFile(appendFileName_);
    
    appendFile.append("", 0);
    EXPECT_EQ(appendFile.writtenBytes(), 0);
    
    appendFile.flush();
    
    // File should exist but be empty
    std::ifstream ifs(appendFileName_);
    std::string readContent((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
    EXPECT_TRUE(readContent.empty());
}

TEST_F(FileUtilsTest, AppendFileLargeContent) {
    AppendFile appendFile(appendFileName_);
    
    // Create content larger than buffer size (64KB)
    std::string largeContent;
    for (int i = 0; i < 2000; ++i) {
        largeContent += "This is a long line of text for testing large content appends.\n";
    }
    
    appendFile.append(largeContent.c_str(), largeContent.length());
    EXPECT_EQ(appendFile.writtenBytes(), largeContent.length());
    
    appendFile.flush();
    
    // Verify content was written correctly
    std::ifstream ifs(appendFileName_);
    std::string readContent((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
    EXPECT_EQ(readContent, largeContent);
}

// ReadSmallFile Tests
TEST_F(FileUtilsTest, ReadSmallFileConstructorAndDestructor) {
    {
        ReadSmallFile readFile(testFileName_);
        // Constructor should succeed for existing file
    }
    
    {
        ReadSmallFile readFile(nonExistentFileName_);
        // Constructor should handle non-existent file gracefully
    }
}

TEST_F(FileUtilsTest, ReadSmallFileReadToString) {
    ReadSmallFile readFile(testFileName_);
    
    std::string content;
    int64_t fileSize = 0;
    int64_t modifyTime = 0;
    int64_t createTime = 0;
    
    int err = readFile.readToString(1024, &content, &fileSize, &modifyTime, &createTime);
    
    EXPECT_EQ(err, 0);
    EXPECT_EQ(content, testContent_);
    EXPECT_EQ(fileSize, testContent_.length());
    EXPECT_GT(modifyTime, 0);
    EXPECT_GT(createTime, 0);
}

TEST_F(FileUtilsTest, ReadSmallFileReadToStringWithMaxSize) {
    ReadSmallFile readFile(testFileName_);
    
    std::string content;
    int maxSize = 10;
    
    int err = readFile.readToString(maxSize, &content, nullptr, nullptr, nullptr);
    
    EXPECT_EQ(err, 0);
    EXPECT_EQ(content.length(), maxSize);
    EXPECT_EQ(content, testContent_.substr(0, maxSize));
}

TEST_F(FileUtilsTest, ReadSmallFileReadEmptyFile) {
    ReadSmallFile readFile(emptyFileName_);
    
    std::string content;
    int64_t fileSize = 0;
    
    int err = readFile.readToString(1024, &content, &fileSize, nullptr, nullptr);
    
    EXPECT_EQ(err, 0);
    EXPECT_TRUE(content.empty());
    EXPECT_EQ(fileSize, 0);
}

TEST_F(FileUtilsTest, ReadSmallFileReadNonExistentFile) {
    ReadSmallFile readFile(nonExistentFileName_);
    
    std::string content;
    
    int err = readFile.readToString(1024, &content, nullptr, nullptr, nullptr);
    
    EXPECT_NE(err, 0);
    EXPECT_EQ(err, ENOENT);
}

TEST_F(FileUtilsTest, ReadSmallFileReadDirectory) {
    ReadSmallFile readFile(directoryName_);
    
    std::string content;
    
    int err = readFile.readToString(1024, &content, nullptr, nullptr, nullptr);
    
    EXPECT_EQ(err, EISDIR);
}

TEST_F(FileUtilsTest, ReadSmallFileReadToBuffer) {
    ReadSmallFile readFile(testFileName_);
    
    int size = 0;
    int err = readFile.readToBuffer(&size);
    
    EXPECT_EQ(err, 0);
    EXPECT_EQ(size, testContent_.length());
    EXPECT_STREQ(readFile.buffer(), testContent_.c_str());
}

TEST_F(FileUtilsTest, ReadSmallFileReadToBufferNonExistent) {
    ReadSmallFile readFile(nonExistentFileName_);
    
    int size = 0;
    int err = readFile.readToBuffer(&size);
    
    EXPECT_NE(err, 0);
    EXPECT_EQ(err, ENOENT);
}

TEST_F(FileUtilsTest, ReadSmallFileReadToBufferEmpty) {
    ReadSmallFile readFile(emptyFileName_);
    
    int size = 0;
    int err = readFile.readToBuffer(&size);
    
    EXPECT_EQ(err, 0);
    EXPECT_EQ(size, 0);
    EXPECT_EQ(readFile.buffer()[0], '\0');
}

TEST_F(FileUtilsTest, ReadSmallFileLargeFile) {
    ReadSmallFile readFile(largeFileName_);
    
    std::string content;
    int err = readFile.readToString(ReadSmallFile::kBufferSize, &content, nullptr, nullptr, nullptr);
    
    EXPECT_EQ(err, 0);
    EXPECT_LE(content.length(), ReadSmallFile::kBufferSize);
    
    // Read the actual file to compare
    std::ifstream ifs(largeFileName_);
    std::string expectedContent((std::istreambuf_iterator<char>(ifs)),
                               std::istreambuf_iterator<char>());
    
    // Content should match up to buffer size
    size_t compareSize = std::min(content.length(), expectedContent.length());
    EXPECT_EQ(content.substr(0, compareSize), expectedContent.substr(0, compareSize));
}

// readFile template function tests
TEST_F(FileUtilsTest, ReadFileFunction) {
    std::string content;
    int64_t fileSize = 0;
    int64_t modifyTime = 0;
    int64_t createTime = 0;
    
    int err = readFile(testFileName_, 1024, &content, &fileSize, &modifyTime, &createTime);
    
    EXPECT_EQ(err, 0);
    EXPECT_EQ(content, testContent_);
    EXPECT_EQ(fileSize, testContent_.length());
    EXPECT_GT(modifyTime, 0);
    EXPECT_GT(createTime, 0);
}

TEST_F(FileUtilsTest, ReadFileFunctionNonExistent) {
    std::string content;
    
    int err = readFile(nonExistentFileName_, 1024, &content);
    
    EXPECT_NE(err, 0);
    EXPECT_EQ(err, ENOENT);
}

TEST_F(FileUtilsTest, ReadFileFunctionWithMaxSize) {
    std::string content;
    int maxSize = 5;
    
    int err = readFile(testFileName_, maxSize, &content);
    
    EXPECT_EQ(err, 0);
    EXPECT_EQ(content.length(), maxSize);
    EXPECT_EQ(content, testContent_.substr(0, maxSize));
}

TEST_F(FileUtilsTest, ReadFileFunctionEmptyFile) {
    std::string content;
    
    int err = readFile(emptyFileName_, 1024, &content);
    
    EXPECT_EQ(err, 0);
    EXPECT_TRUE(content.empty());
}

// Edge cases and error handling
TEST_F(FileUtilsTest, ReadFileZeroMaxSize) {
    std::string content;
    
    int err = readFile(testFileName_, 0, &content);
    
    EXPECT_EQ(err, 0);
    EXPECT_TRUE(content.empty());
}

TEST_F(FileUtilsTest, AppendFileToExistingFile) {
    // First, create a file with some content
    {
        AppendFile appendFile(appendFileName_);
        std::string initialContent = "Initial content\n";
        appendFile.append(initialContent.c_str(), initialContent.length());
        appendFile.flush();
    }
    
    // Then append more content
    {
        AppendFile appendFile(appendFileName_);
        std::string additionalContent = "Additional content\n";
        appendFile.append(additionalContent.c_str(), additionalContent.length());
        appendFile.flush();
    }
    
    // Verify both contents are present
    std::ifstream ifs(appendFileName_);
    std::string readContent((std::istreambuf_iterator<char>(ifs)),
                           std::istreambuf_iterator<char>());
    EXPECT_EQ(readContent, "Initial content\nAdditional content\n");
}

TEST_F(FileUtilsTest, StringPieceCompatibility) {
    // Test that StringPiece works correctly with file operations
    const char* filename = testFileName_.c_str();
    StringPiece filenamePiece(filename);
    
    ReadSmallFile readFile(filenamePiece);
    std::string content;
    int err = readFile.readToString(1024, &content, nullptr, nullptr, nullptr);
    
    EXPECT_EQ(err, 0);
    EXPECT_EQ(content, testContent_);
}