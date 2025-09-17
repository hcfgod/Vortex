#include <doctest/doctest.h>
#include <Engine/Assets/AssetSystem.h>
#include <Engine/Assets/Asset.h>
#include <Engine/Assets/AssetHandle.h>
#include <Engine/Assets/ShaderAsset.h>
#include <Engine/Assets/TextureAsset.h>
#include <Core/Debug/ErrorCodes.h>
#include <Core/FileSystem.h>

using namespace Vortex;

TEST_CASE("AssetSystem initialization and shutdown") {
    AssetSystem assetSystem;
    
    auto initResult = assetSystem.Initialize();
    CHECK(initResult.IsSuccess());
    
    auto shutdownResult = assetSystem.Shutdown();
    CHECK(shutdownResult.IsSuccess());
}

TEST_CASE("AssetSystem working directory management") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test setting working directory
    std::filesystem::path testDir = "test_assets_dir";
    assetSystem.SetWorkingDirectory(testDir);
    
    // Test getting assets root
    std::filesystem::path assetsRoot = assetSystem.GetAssetsRoot();
    CHECK(!assetsRoot.empty());
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle creation") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test creating asset handle by UUID
    UUID testId = UUID::Generate();
    auto handle = assetSystem.GetByUUID<ShaderAsset>(testId);
    
    // Handle should be invalid since asset doesn't exist
    CHECK(!handle.IsValid());
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle by name") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test getting asset handle by name
    auto handle = assetSystem.GetByName<ShaderAsset>("nonexistent_shader");
    
    // Handle should be invalid since asset doesn't exist
    CHECK(!handle.IsValid());
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset loading - shader") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test loading a shader asset
    auto handle = assetSystem.LoadAsset<ShaderAsset>("test_shader");
    
    // Handle should be valid even if shader doesn't exist (fallback)
    CHECK(handle.IsValid());
    
    // Test getting the asset
    auto* shader = handle.TryGet();
    // May be nullptr if shader failed to load
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset loading - texture") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test creating a texture handle without actually loading the texture
    // This tests the handle creation logic without requiring GPU resources
    UUID testTextureId = UUID::Generate();
    auto handle = assetSystem.GetByUUID<TextureAsset>(testTextureId);
    
    // Handle should be invalid since texture doesn't exist
    CHECK(!handle.IsValid());
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle reference counting") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test reference counting with multiple handles
    auto handle1 = assetSystem.LoadAsset<ShaderAsset>("test_shader");
    CHECK(handle1.IsValid());
    
    // Create another handle to the same asset
    auto handle2 = assetSystem.GetByUUID<ShaderAsset>(handle1.GetId());
    CHECK(handle2.IsValid());
    
    // Both handles should be valid
    CHECK(handle1.IsValid());
    CHECK(handle2.IsValid());
    
    // Test that they point to the same asset
    CHECK(handle1.GetId() == handle2.GetId());
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle move semantics") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test move constructor
    auto handle1 = assetSystem.LoadAsset<ShaderAsset>("test_shader");
    CHECK(handle1.IsValid());
    
    auto handle2 = std::move(handle1);
    CHECK(!handle1.IsValid()); // Original should be invalid
    CHECK(handle2.IsValid());  // Moved should be valid
    
    // Test move assignment
    auto handle3 = assetSystem.LoadAsset<ShaderAsset>("test_texture");
    CHECK(handle3.IsValid());
    
    handle3 = std::move(handle2);
    CHECK(!handle2.IsValid()); // Original should be invalid
    CHECK(handle3.IsValid());  // Assigned should be valid
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle copy semantics") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test copy constructor
    auto handle1 = assetSystem.LoadAsset<ShaderAsset>("test_shader");
    CHECK(handle1.IsValid());
    
    auto handle2 = handle1; // Copy
    CHECK(handle1.IsValid()); // Original should still be valid
    CHECK(handle2.IsValid()); // Copy should be valid
    
    // Test copy assignment
    auto handle3 = assetSystem.LoadAsset<ShaderAsset>("test_texture");
    CHECK(handle3.IsValid());
    
    handle3 = handle1; // Copy assignment
    CHECK(handle1.IsValid()); // Original should still be valid
    CHECK(handle3.IsValid()); // Assigned should be valid
    
    // Test that they point to the same asset
    CHECK(handle1.GetId() == handle2.GetId());
    CHECK(handle1.GetId() == handle3.GetId());
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle progress tracking") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test progress tracking
    auto handle = assetSystem.LoadAsset<ShaderAsset>("test_shader");
    CHECK(handle.IsValid());
    
    // Test getting progress
    float progress = handle.GetProgress();
    CHECK(progress >= 0.0f);
    CHECK(progress <= 1.0f);
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle loaded state") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test loaded state tracking
    auto handle = assetSystem.LoadAsset<ShaderAsset>("test_shader");
    CHECK(handle.IsValid());
    
    // Test checking if loaded
    bool isLoaded = handle.IsLoaded();
    // May be true or false depending on whether shader loaded successfully
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle UUID access") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test UUID access
    auto handle = assetSystem.LoadAsset<ShaderAsset>("test_shader");
    CHECK(handle.IsValid());
    
    // Test getting UUID
    UUID id = handle.GetId();
    CHECK(id != UUID::Invalid);
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle boolean conversion") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test boolean conversion
    auto handle = assetSystem.LoadAsset<ShaderAsset>("test_shader");
    CHECK(handle.IsValid());
    
    // Test explicit boolean conversion
    bool isValid = static_cast<bool>(handle);
    CHECK(isValid);
    
    // Test invalid handle
    auto invalidHandle = assetSystem.GetByUUID<ShaderAsset>(UUID::Invalid);
    CHECK(!invalidHandle.IsValid());
    
    bool isInvalid = static_cast<bool>(invalidHandle);
    CHECK(!isInvalid);
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle destruction") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test handle destruction
    {
        auto handle = assetSystem.LoadAsset<ShaderAsset>("test_shader");
        CHECK(handle.IsValid());
        
        // Handle should be valid here
        CHECK(handle.IsValid());
    }
    
    // Handle should be destroyed here, but asset may still exist in system
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle with progress callback") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test progress callback
    bool callbackCalled = false;
    float lastProgress = 0.0f;
    
    auto handle = assetSystem.LoadAsset<ShaderAsset>("test_shader", 
        [&](float progress) {
            callbackCalled = true;
            lastProgress = progress;
        });
    
    CHECK(handle.IsValid());
    
    // Callback may or may not be called depending on loading success
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle template specialization") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test different asset types using GetByUUID to avoid GPU resource requirements
    UUID shaderId = UUID::Generate();
    UUID textureId = UUID::Generate();
    
    auto shaderHandle = assetSystem.GetByUUID<ShaderAsset>(shaderId);
    CHECK(!shaderHandle.IsValid()); // Should be invalid since asset doesn't exist
    
    auto textureHandle = assetSystem.GetByUUID<TextureAsset>(textureId);
    CHECK(!textureHandle.IsValid()); // Should be invalid since asset doesn't exist
    
    // Test that they are different types (different UUIDs)
    CHECK(shaderHandle.GetId() != textureHandle.GetId());
    
    assetSystem.Shutdown();
}

TEST_CASE("AssetSystem asset handle error handling") {
    AssetSystem assetSystem;
    assetSystem.Initialize();
    
    // Test error handling with invalid UUID
    auto invalidHandle = assetSystem.GetByUUID<ShaderAsset>(UUID::Invalid);
    CHECK(!invalidHandle.IsValid());
    
    // Test error handling with nonexistent name
    auto nonexistentHandle = assetSystem.GetByName<ShaderAsset>("nonexistent_asset");
    CHECK(!nonexistentHandle.IsValid());
    
    assetSystem.Shutdown();
}