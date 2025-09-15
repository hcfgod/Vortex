#include <doctest/doctest.h>
#include <Engine/Renderer/RenderCommandQueue.h>
#include <Engine/Renderer/GraphicsContext.h>

using namespace Vortex;

namespace {
    class DummyContext : public GraphicsContext {
    public:
        Result<void> Initialize(const GraphicsContextProps&) override { return Result<void>(); }
        Result<void> Shutdown() override { return Result<void>(); }
        Result<void> MakeCurrent() override { return Result<void>(); }
        Result<void> Present() override { return Result<void>(); }
        Result<void> Resize(uint32_t, uint32_t) override { return Result<void>(); }
        Result<void> SetVSync(bool) override { return Result<void>(); }
        Result<void> SetVSyncMode(VSyncMode) override { return Result<void>(); }
        bool IsVSyncEnabled() const override { return false; }
        VSyncMode GetVSyncMode() const override { return VSyncMode::Disabled; }
        std::vector<VSyncMode> GetSupportedVSyncModes() const override { return {}; }
        const GraphicsContextInfo& GetInfo() const override { return m_Info; }
        void GetFramebufferSize(uint32_t& w, uint32_t& h) const override { w = 0; h = 0; }
        bool IsValid() const override { return true; }
        std::string GetDebugInfo() const override { return "DummyContext"; }
    };

    class DummyCommand : public RenderCommand {
    public:
        DummyCommand(const char* name = "Dummy", float cost = 0.01f) : m_Name(name), m_Cost(cost) {}
        Result<void> Execute(GraphicsContext* /*ctx*/) override { ++s_Executed; return Result<void>(); }
        std::string GetDebugName() const override { return m_Name; }
        float GetEstimatedCost() const override { return m_Cost; }
        static void Reset() { s_Executed = 0; }
        static int Executed() { return s_Executed; }
    private:
        std::string m_Name;
        float m_Cost;
        inline static int s_Executed = 0;
    };
}

TEST_CASE("RenderCommandQueue submit/execute immediate and queued")
{
    RenderCommandQueue::Config cfg; cfg.MaxQueuedCommands = 16; cfg.MaxCommandsPerFrame = 0; cfg.EnableProfiling = true; cfg.WarnOnDrop = false;
    RenderCommandQueue q(cfg);

    DummyContext ctx;
    REQUIRE(q.Initialize(&ctx).IsSuccess());

    DummyCommand::Reset();
    // Immediate execution: should be processed and counted
    CHECK(q.Submit(std::make_unique<DummyCommand>("Immediate"), true) == true);
    CHECK(q.GetStatistics().ProcessedCommands >= 1);
    CHECK(DummyCommand::Executed() == 1);

    // On the render thread, even executeImmediate=false executes immediately by design
    CHECK(q.Submit(std::make_unique<DummyCommand>("Queued1"), false) == true);
    CHECK(q.GetQueueSize() == 0);

    REQUIRE(q.ProcessCommands().IsSuccess());
    CHECK(q.GetQueueSize() == 0);
    CHECK(q.GetStatistics().ProcessedCommands >= 2);
    CHECK(DummyCommand::Executed() == 2);

    // Batch submit (queued=false still executes immediately on render thread)
    std::vector<std::unique_ptr<RenderCommand>> cmds;
    cmds.push_back(std::make_unique<DummyCommand>("B1"));
    cmds.push_back(std::make_unique<DummyCommand>("B2"));
    size_t submitted = q.SubmitBatch(std::move(cmds), false);
    CHECK(submitted == 2);
    CHECK(q.GetQueueSize() == 0);
    REQUIRE(q.FlushAll().IsSuccess());
    CHECK(q.GetQueueSize() == 0);
    CHECK(DummyCommand::Executed() >= 4);

    REQUIRE(q.Shutdown().IsSuccess());
}

TEST_CASE("RenderCommandQueue immediate path on render thread and stats reset")
{
    RenderCommandQueue::Config cfg; cfg.MaxQueuedCommands = 2; cfg.MaxCommandsPerFrame = 0; cfg.EnableProfiling = false; cfg.WarnOnDrop = false;
    RenderCommandQueue q(cfg);
    DummyContext ctx; REQUIRE(q.Initialize(&ctx).IsSuccess());

    // Submissions with executeImmediate=false on render thread execute immediately; queue stays empty
    size_t beforeProcessed = q.GetStatistics().ProcessedCommands;
    CHECK(q.Submit(std::make_unique<DummyCommand>("Q1"), false) == true);
    CHECK(q.Submit(std::make_unique<DummyCommand>("Q2"), false) == true);
    CHECK(q.GetQueueSize() == 0);
    CHECK(q.IsQueueFull() == false);
    CHECK(q.GetStatistics().ProcessedCommands >= beforeProcessed + 2);

    // Drain
    REQUIRE(q.ProcessCommands().IsSuccess());
    CHECK(q.GetQueueSize() == 0);
    // Reset stats
    q.ResetStatistics();
    CHECK(q.GetStatistics().ProcessedCommands == 0);
    REQUIRE(q.Shutdown().IsSuccess());
}


