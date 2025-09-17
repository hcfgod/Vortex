#include <doctest/doctest.h>
#include <Core/Debug/ErrorCodes.h>
#include <vector>

using namespace Vortex;

TEST_CASE("Result<T> success construction and access") {
    Result<int> success(42);
    
    CHECK(success.IsSuccess());
    CHECK(!success.IsError());
    CHECK(success.GetValue() == 42);
}

TEST_CASE("Result<T> failure construction and access") {
    Result<int> failure(ErrorCode::SystemInitializationFailed, "Test error");
    
    CHECK(failure.IsError());
    CHECK(!failure.IsSuccess());
    CHECK(failure.GetErrorCode() == ErrorCode::SystemInitializationFailed);
    CHECK(failure.GetErrorMessage() == "Test error");
}

TEST_CASE("Result<void> success and failure") {
    Result<void> success;
    CHECK(success.IsSuccess());
    
    Result<void> failure(ErrorCode::EngineAlreadyInitialized, "Already initialized");
    CHECK(failure.IsError());
    CHECK(failure.GetErrorCode() == ErrorCode::EngineAlreadyInitialized);
}

TEST_CASE("ErrorCode enum values are distinct") {
    CHECK(static_cast<int>(ErrorCode::Success) != static_cast<int>(ErrorCode::SystemInitializationFailed));
    CHECK(static_cast<int>(ErrorCode::SystemInitializationFailed) != static_cast<int>(ErrorCode::EngineAlreadyInitialized));
    CHECK(static_cast<int>(ErrorCode::EngineAlreadyInitialized) != static_cast<int>(ErrorCode::EngineNotInitialized));
}

TEST_CASE("ErrorCodeToString converts codes to strings") {
    CHECK(ErrorCodeToString(ErrorCode::Success) == "Success");
    CHECK(ErrorCodeToString(ErrorCode::SystemInitializationFailed) == "System Initialization Failed");
    CHECK(ErrorCodeToString(ErrorCode::EngineAlreadyInitialized) == "Engine Already Initialized");
    CHECK(ErrorCodeToString(ErrorCode::EngineNotInitialized) == "Engine Not Initialized");
}

TEST_CASE("Result operator bool") {
    Result<int> success(42);
    Result<int> failure(ErrorCode::SystemInitializationFailed, "Test error");
    
    CHECK(static_cast<bool>(success));
    CHECK(!static_cast<bool>(failure));
}

TEST_CASE("Result operator* and operator->") {
    Result<int> success(42);
    
    CHECK(*success == 42);
    CHECK(success.GetValue() == 42);
}

TEST_CASE("Error handling patterns") {
    // Test common error handling patterns
    
    auto successFunction = []() -> Result<int> {
        return Result<int>(42);
    };
    
    auto failureFunction = []() -> Result<int> {
        return Result<int>(ErrorCode::SystemInitializationFailed, "Failed to initialize");
    };
    
    // Test success case
    auto result1 = successFunction();
    if (result1.IsSuccess()) {
        CHECK(result1.GetValue() == 42);
    } else {
        CHECK(false); // Should not reach here
    }
    
    // Test failure case
    auto result2 = failureFunction();
    if (result2.IsError()) {
        CHECK(result2.GetErrorCode() == ErrorCode::SystemInitializationFailed);
        CHECK(result2.GetErrorMessage() == "Failed to initialize");
    } else {
        CHECK(false); // Should not reach here
    }
}

TEST_CASE("Result chaining and error propagation") {
    auto step1 = []() -> Result<int> {
        return Result<int>(10);
    };
    
    auto step2 = [](int value) -> Result<int> {
        if (value < 0) {
            return Result<int>(ErrorCode::SystemInitializationFailed, "Negative value");
        }
        return Result<int>(value * 2);
    };
    
    auto step3 = [](int value) -> Result<void> {
        if (value > 50) {
            return Result<void>(ErrorCode::SystemInitializationFailed, "Value too large");
        }
        return Result<void>();
    };
    
    // Test successful chain
    auto result1 = step1();
    if (result1.IsSuccess()) {
        auto result2 = step2(result1.GetValue());
        if (result2.IsSuccess()) {
            auto result3 = step3(result2.GetValue());
            CHECK(result3.IsSuccess());
        }
    }
    
    // Test failure propagation
    auto failureResult = step2(-5);
    CHECK(failureResult.IsError());
    CHECK(failureResult.GetErrorCode() == ErrorCode::SystemInitializationFailed);
}

TEST_CASE("Error code categories") {
    // Test that error codes are properly categorized
    
    // System errors
    CHECK(ErrorCodeToString(ErrorCode::SystemInitializationFailed) == "System Initialization Failed");
    CHECK(ErrorCodeToString(ErrorCode::EngineAlreadyInitialized) == "Engine Already Initialized");
    
    // Engine errors
    CHECK(ErrorCodeToString(ErrorCode::EngineNotInitialized) == "Engine Not Initialized");
    CHECK(ErrorCodeToString(ErrorCode::EngineShutdownFailed) == "Engine Shutdown Failed");
    
    // File system errors
    CHECK(ErrorCodeToString(ErrorCode::FileNotFound) == "File Not Found");
    CHECK(ErrorCodeToString(ErrorCode::FileAccessDenied) == "File Access Denied");
    
    // Renderer errors
    CHECK(ErrorCodeToString(ErrorCode::RendererInitFailed) == "Renderer Initialization Failed");
    CHECK(ErrorCodeToString(ErrorCode::ShaderCompilationFailed) == "Shader Compilation Failed");
}

TEST_CASE("Result move semantics") {
    // Test move construction
    Result<int> original(42);
    Result<int> moved(std::move(original));
    
    CHECK(moved.IsSuccess());
    CHECK(moved.GetValue() == 42);
    
    // Test move assignment
    Result<int> assigned = std::move(moved);
    CHECK(assigned.IsSuccess());
    CHECK(assigned.GetValue() == 42);
}

TEST_CASE("Result with different types") {
    // Test Result with string
    Result<std::string> stringResult("hello world");
    CHECK(stringResult.IsSuccess());
    CHECK(stringResult.GetValue() == "hello world");
    
    // Test Result with vector
    Result<std::vector<int>> vectorResult(std::vector<int>{1, 2, 3});
    CHECK(vectorResult.IsSuccess());
    CHECK(vectorResult.GetValue().size() == 3);
    
    // Test Result with custom type
    struct TestStruct {
        int value;
        std::string name;
    };
    
    Result<TestStruct> structResult(TestStruct{42, "test"});
    CHECK(structResult.IsSuccess());
    CHECK(structResult.GetValue().value == 42);
    CHECK(structResult.GetValue().name == "test");
}

TEST_CASE("Error code range validation") {
    // Test that error codes are within expected ranges
    
    // General errors (1-99)
    CHECK(static_cast<int>(ErrorCode::Unknown) >= 1);
    CHECK(static_cast<int>(ErrorCode::Unknown) <= 99);
    
    // Engine errors (100-199)
    CHECK(static_cast<int>(ErrorCode::EngineNotInitialized) >= 100);
    CHECK(static_cast<int>(ErrorCode::EngineNotInitialized) <= 199);
    
    // Renderer errors (200-299)
    CHECK(static_cast<int>(ErrorCode::RendererInitFailed) >= 200);
    CHECK(static_cast<int>(ErrorCode::RendererInitFailed) <= 299);
    
    // File I/O errors (500-599)
    CHECK(static_cast<int>(ErrorCode::FileNotFound) >= 500);
    CHECK(static_cast<int>(ErrorCode::FileNotFound) <= 599);
}

TEST_CASE("Result error message handling") {
    // Test error message handling
    Result<int> errorWithMessage(ErrorCode::SystemInitializationFailed, "Detailed error message");
    CHECK(errorWithMessage.GetErrorMessage() == "Detailed error message");
    
    Result<int> errorWithoutMessage(ErrorCode::SystemInitializationFailed);
    CHECK(errorWithoutMessage.GetErrorMessage().empty());
    
    // Test that error messages are preserved
    std::string originalMessage = "Original message";
    Result<int> errorWithOriginalMessage(ErrorCode::SystemInitializationFailed, originalMessage);
    CHECK(errorWithOriginalMessage.GetErrorMessage() == originalMessage);
}
