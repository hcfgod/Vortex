#pragma once

#include "Core/Debug/ErrorCodes.h"
#include "Core/Layer/LayerStack.h"
#include "Systems/SystemManager.h"

namespace Vortex 
{
	class Engine
	{
	public:
		Engine();
		~Engine();

		Result<void> Initialize();
		Result<void> Shutdown();
		Result<void> Update();
		Result<void> Render();
		
		bool IsInitialized() const { return m_Initialized; }
		bool IsRunning() const { return m_Running; }
		void Stop() { m_Running = false; }

		/// <summary>
		/// Get the system manager for registering and accessing systems
		/// </summary>
		SystemManager& GetSystemManager() { return m_SystemManager; }
		const SystemManager& GetSystemManager() const { return m_SystemManager; }

		// Convenience typed system accessors
		template<typename T>
		T* GetSystem() { return m_SystemManager.GetSystem<T>(); }
		template<typename T>
		const T* GetSystem() const { return m_SystemManager.GetSystem<T>(); }
	
		// ===== Layer API (Engine manages LayerStack) =====
		LayerStack& GetLayerStack() { return m_LayerStack; }
		const LayerStack& GetLayerStack() const { return m_LayerStack; }
	
		Layer* PushLayer(std::unique_ptr<Layer> layer) { return m_LayerStack.PushLayer(std::move(layer)); }
		template<typename T, typename... Args>
		T* PushLayer(Args&&... args) { return m_LayerStack.PushLayer<T>(std::forward<Args>(args)...); }
		bool PopLayer(Layer* layer) { return m_LayerStack.PopLayer(layer); }
		bool PopLayer(const std::string& name) { return m_LayerStack.PopLayer(name); }
		size_t PopLayersByType(LayerType type) { return m_LayerStack.PopLayersByType(type); }

		private:
		bool m_Initialized = false;
		bool m_Running = false;
	
		SystemManager m_SystemManager;
		LayerStack m_LayerStack;
	
		// Cached system pointers for frequently accessed systems
		class RenderSystem* m_RenderSystem = nullptr;

		/// <summary>
		/// Register all core engine systems
		/// </summary>
		Result<void> RegisterCoreSystems();
	};
}