#include <doctest/doctest.h>
#include <Core/FileSystem.h>
#include <Core/Debug/ErrorCodes.h>
#include <fstream>
#include <filesystem>
#include <algorithm>

using namespace Vortex;

TEST_CASE("FileSystem executable path operations") {
    // Test getting executable directory
    auto execDir = FileSystem::GetExecutableDirectory();
    CHECK(execDir.has_value());
    CHECK(std::filesystem::exists(execDir.value()));
    
    // Test getting executable path
    auto execPath = FileSystem::GetExecutablePath();
    CHECK(execPath.has_value());
    CHECK(std::filesystem::exists(execPath.value()));
}

TEST_CASE("FileSystem config directory finding") {
    // Test finding config directory
    std::string configDir = FileSystem::FindConfigDirectory("Config/Engine");
    CHECK(!configDir.empty());
    
    // Test with custom subpath
    std::string customConfigDir = FileSystem::FindConfigDirectory("Custom/Config");
    CHECK(!customConfigDir.empty());
}

TEST_CASE("FileSystem directory existence checks") {
    // Test with non-existent directory
    std::filesystem::path nonExistentDir = "non_existent_directory_12345";
    CHECK(!FileSystem::DirectoryExists(nonExistentDir));
    
    // Test with existing directory (create a temporary one)
    std::filesystem::path tempDir = "temp_test_directory";
    std::filesystem::create_directories(tempDir);
    
    CHECK(FileSystem::DirectoryExists(tempDir));
    
    // Clean up
    std::filesystem::remove(tempDir);
}

TEST_CASE("FileSystem file existence checks") {
    // Test with non-existent file
    std::filesystem::path nonExistentFile = "non_existent_file_12345.txt";
    CHECK(!FileSystem::FileExists(nonExistentFile));
    
    // Test with existing file (create a temporary one)
    std::filesystem::path tempFile = "temp_test_file.txt";
    std::ofstream file(tempFile);
    file << "test content";
    file.close();
    
    CHECK(FileSystem::FileExists(tempFile));
    
    // Clean up
    std::filesystem::remove(tempFile);
}

TEST_CASE("FileSystem directory creation") {
    // Test directory creation
    std::filesystem::path testDir = "test_directory_creation";
    
    // Create directory
    bool createResult = FileSystem::CreateDirectories(testDir);
    CHECK(createResult);
    CHECK(FileSystem::DirectoryExists(testDir));
    
    // Try to create again (should succeed - idempotent)
    bool createAgainResult = FileSystem::CreateDirectories(testDir);
    CHECK(createAgainResult);
    
    // Clean up
    std::filesystem::remove(testDir);
}

TEST_CASE("FileSystem nested directory creation") {
    // Test nested directory creation
    std::filesystem::path nestedDir = "parent/child/grandchild";
    
    // Create nested directories
    bool createResult = FileSystem::CreateDirectories(nestedDir);
    CHECK(createResult);
    CHECK(FileSystem::DirectoryExists(nestedDir));
    CHECK(FileSystem::DirectoryExists("parent"));
    CHECK(FileSystem::DirectoryExists("parent/child"));
    
    // Clean up
    std::filesystem::remove_all("parent");
}

TEST_CASE("FileSystem path operations with std::filesystem") {
    // Test path operations using std::filesystem
    std::filesystem::path testPath = "test/path/file.txt";
    
    // Test path joining
    std::filesystem::path joined = std::filesystem::path("test") / "path" / "file.txt";
    CHECK(joined == testPath);
    
    // Test file name extraction
    std::string fileName = testPath.filename().string();
    CHECK(fileName == "file.txt");
    
    // Test parent directory
    std::string parentDir = testPath.parent_path().string();
    CHECK(parentDir == "test/path");
    
    // Test extension
    std::string extension = testPath.extension().string();
    CHECK(extension == ".txt");
    
    // Test stem (filename without extension)
    std::string stem = testPath.stem().string();
    CHECK(stem == "file");
}

TEST_CASE("FileSystem working directory operations") {
    // Test getting current working directory using std::filesystem
    std::filesystem::path currentDir = std::filesystem::current_path();
    CHECK(!currentDir.empty());
    
    // Test changing working directory
    std::filesystem::path originalDir = currentDir;
    
    // Create a test directory
    std::filesystem::path testDir = "test_cwd_dir";
    std::filesystem::create_directories(testDir);
    
    // Change to test directory
    std::filesystem::current_path(testDir);
    std::filesystem::path newDir = std::filesystem::current_path();
    CHECK(newDir.filename() == testDir.filename());
    
    // Change back
    std::filesystem::current_path(originalDir);
    
    // Clean up
    std::filesystem::remove(testDir);
}

TEST_CASE("FileSystem file operations with std::filesystem") {
    std::filesystem::path testFile = "test_file_operations.txt";
    std::string testContent = "Hello, World!\nThis is a test file.";
    
    // Test file writing
    std::ofstream file(testFile);
    file << testContent;
    file.close();
    
    CHECK(FileSystem::FileExists(testFile));
    
    // Test file reading
    std::ifstream readFile(testFile);
    std::string readContent((std::istreambuf_iterator<char>(readFile)),
                           std::istreambuf_iterator<char>());
    readFile.close();
    
    CHECK(readContent == testContent);
    
    // Test file size - account for platform-specific line ending differences
    auto fileSize = std::filesystem::file_size(testFile);
    auto expectedSize = testContent.size();
    
    // On Windows, text files may have CRLF line endings instead of LF
    // Count the number of \n characters and adjust expected size accordingly
    size_t newlineCount = std::count(testContent.begin(), testContent.end(), '\n');
    #ifdef _WIN32
        expectedSize += newlineCount; // Each \n becomes \r\n on Windows
    #endif
    
    CHECK(fileSize == expectedSize);
    
    // Clean up
    std::filesystem::remove(testFile);
}

TEST_CASE("FileSystem directory listing with std::filesystem") {
    std::filesystem::path testDir = "test_listing_dir";
    
    // Create test directory
    std::filesystem::create_directories(testDir);
    
    // Create some test files
    std::ofstream file1(testDir / "file1.txt");
    file1 << "content1";
    file1.close();
    
    std::ofstream file2(testDir / "file2.txt");
    file2 << "content2";
    file2.close();
    
    std::filesystem::create_directories(testDir / "subdir");
    
    // Test directory listing
    std::vector<std::filesystem::path> entries;
    for (const auto& entry : std::filesystem::directory_iterator(testDir)) {
        entries.push_back(entry.path());
    }
    
    CHECK(entries.size() >= 3); // At least our test files and subdirectory
    
    // Clean up
    std::filesystem::remove_all(testDir);
}

TEST_CASE("FileSystem error handling") {
    // Test operations on non-existent paths
    std::filesystem::path nonExistentFile = "non_existent_file_12345.txt";
    std::filesystem::path nonExistentDir = "non_existent_directory_12345";
    
    // These should return false for non-existent paths
    CHECK(!FileSystem::FileExists(nonExistentFile));
    CHECK(!FileSystem::DirectoryExists(nonExistentDir));
    
    // Test file size on non-existent file
    try {
        std::filesystem::file_size(nonExistentFile);
        CHECK(false); // Should not reach here
    } catch (const std::filesystem::filesystem_error&) {
        // Expected exception
        CHECK(true);
    }
}
