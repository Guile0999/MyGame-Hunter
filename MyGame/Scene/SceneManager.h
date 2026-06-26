//--------------------------------------------------------------------------------------
// File: SceneManager.h
//
// シーンを管理するクラス
//
// Date: 2025.2.28
//--------------------------------------------------------------------------------------
#pragma once
#include <thread>   
#include <mutex>    
#include <chrono>
// ESCキーで終了したい場合有効にしてください
#define ESC_QUIT_ENABLE

#ifdef ESC_QUIT_ENABLE
#include "Keyboard.h"
#endif

namespace MyLib
{
	template <typename T>
	class SceneManager;

	// シーンの基底クラス
	template <typename T>
	class Scene
	{
	private:

		// シーンマネージャーへのポインタ
		SceneManager<T>* m_sceneManager;

	public:

		// コンストラクタ
		Scene() : m_sceneManager(nullptr) {}

		// デストラクタ
		virtual ~Scene() = default;

		// 初期化
		virtual void Initialize() = 0;

		// 更新
		virtual void Update(float elapsedTime) = 0;

		// 描画
		virtual void Render() = 0;

		// 終了処理
		virtual void Finalize() = 0;

		// デバイスに依存するリソースを作成する関数
		virtual void CreateDeviceDependentResources() {}

		// ウインドウサイズに依存するリソースを作成する関数
		virtual void CreateWindowSizeDependentResources() {}

		// デバイスロストした時に呼び出される関数
		virtual void OnDeviceLost() {}

		virtual bool IsReadyToFinish() const { return true; }
		  
		virtual void OnLoadingComplete() {}
	public:

		// シーンマネージャー設定関数
		void SetSceneManager(SceneManager<T>* sceneManager) { m_sceneManager = sceneManager; }

		// シーンの切り替え関数
		template <typename U>
		void ChangeScene();

		template <typename U, typename V>
		void ChangeScene();

		// ユーザーが設定したリソース取得関数
		T* GetUserResources();

	};



	// ロード画面の基底クラス
	template <typename T>
	using LoadingScreen = Scene<T>;



	// シーンマネージャークラス
	template <typename T>
	class SceneManager
	{
	private:

		// 共通でアクセスしたいオブジェクトへのポインタ
		T* m_userResources;

		// 実行中のシーンへのポインタ
		std::unique_ptr<Scene<T>> m_scene;

		// 次のシーンへのポインタ
		std::unique_ptr<Scene<T>> m_nextScene;

		// ロード画面 
		std::unique_ptr<LoadingScreen<T>> m_loadingScreen;

		//// Thread used for loading data or resources in the background
		std::thread m_loadingThread;
		// Mutex to protect shared data access during the loading process
		std::mutex  m_loadingMutex;
		bool        m_isLoading = false;


		// シーン削除関数
		void DeleteScene();

	public:

		// コンストラクタ
		SceneManager(T* userResources = nullptr)
			: m_userResources(userResources)
			, m_scene()
			, m_nextScene()
			, m_loadingScreen()
		{
		};

		// デストラクタ
		virtual ~SceneManager()
		{
			DeleteScene();
			//123
			if (m_loadingThread.joinable())
			{
				m_loadingThread.join();
			}
		};

		// 更新
		void Update(float elapsedTime);

		// 描画
		void Render();

		// デバイスに依存するリソースを作成する関数
		void CreateDeviceDependentResources();

		// ウインドウサイズに依存するリソースを作成する関数
		void CreateWindowSizeDependentResources();

		// デバイスロストした時に呼び出される関数
		virtual void OnDeviceLost();

		// シーンの設定関数
		template <typename U>
		void SetScene();

		// 次のシーンのリクエスト関数
		template <typename U>
		bool RequestSceneChange();

		template <typename U, typename V>
		bool RequestSceneChange();

		// ユーザーリソース設定関数
		void SetUserResources(T* userResources) { m_userResources = userResources; }

		// ユーザーリソース取得関数
		T* GetUserResources() { return m_userResources; }

		void PrepareNextScene(std::function<std::unique_ptr<Scene<T>>()> sceneFactory);

		bool IsLoading();

	};

	// シーンの切り替え関数
	template <typename T>
	template <typename U>
	void Scene<T>::ChangeScene()
	{
		m_sceneManager->RequestSceneChange<U>();
	}

	template <typename T>
	template <typename U, typename V>
	void Scene<T>::ChangeScene()
	{
		m_sceneManager->RequestSceneChange<U, V>();
	}


	// ユーザーが設定したリソース取得関数
	template <typename T>
	T* Scene<T>::GetUserResources()
	{
		assert(m_sceneManager);

		return m_sceneManager->GetUserResources();
	}

	// シーンの設定関数
	template <typename T>
	template <typename U>
	void SceneManager<T>::SetScene()
	{
		assert(m_scene == nullptr);

		RequestSceneChange<U>();
	}

	// シーンの設定関数
	template <typename T>
	template <typename U>
	bool SceneManager<T>::RequestSceneChange()
	{
		if (!m_nextScene)
		{
			// シーンを生成
			PrepareNextScene([]() {return std::make_unique<U>(); });
			return true;
		}

		return false;
	}

	template <typename T>
	template <typename U, typename V>
	bool SceneManager<T>::RequestSceneChange()
	{
		if (RequestSceneChange<U>())
		{
			// ロード画面を作成
			m_loadingScreen = std::make_unique<V>();
			m_loadingScreen->SetSceneManager(this);
			m_loadingScreen->Initialize();
			return true;
		}

		return false;
	}

	// 更新関数
	template <typename T>
	void SceneManager<T>::Update(float elapsedTime)
	{
#ifdef ESC_QUIT_ENABLE
		// ESCキーで終了
		auto kb = DirectX::Keyboard::Get().GetState();
		if (kb.Escape) PostQuitMessage(0);
#endif
		


		// シーンの切り替え処理
		//if (!IsLoading() && m_nextScene)
		if (!IsLoading() && m_nextScene && 
			(!m_loadingScreen || m_loadingScreen->IsReadyToFinish()))
		{
			DeleteScene();

			assert(m_scene == nullptr);

			// シーンを切り替え
			m_scene = std::move(m_nextScene);

			if (m_loadingScreen)
			{
				m_loadingScreen->Finalize();
				m_loadingScreen.reset();
			}
		}

		if (m_loadingScreen)
		{
			m_loadingScreen->Update(elapsedTime);
			return;
		}

		// シーンの更新
		if (m_scene) m_scene->Update(elapsedTime);

	}

	// 描画関数
	template <typename T>
	void SceneManager<T>::Render()
	{
		if (m_loadingScreen)
		{
			m_loadingScreen->Render();
			return;
		}

		// シーンの描画
		if (m_scene) m_scene->Render();
	}

	// デバイスに依存するリソースを作成する関数
	template <typename T>
	void SceneManager<T>::CreateDeviceDependentResources()
	{
		if (m_scene) m_scene->CreateDeviceDependentResources();
	}

	// ウインドウサイズに依存するリソースを作成する関数
	template <typename T>
	void SceneManager<T>::CreateWindowSizeDependentResources()
	{
		if (m_scene) m_scene->CreateWindowSizeDependentResources();

		if (m_loadingScreen)
			m_loadingScreen->CreateWindowSizeDependentResources();
	}

	// デバイスロストした時に呼び出される関数
	template <typename T>
	void SceneManager<T>::OnDeviceLost()
	{
		if (m_scene) m_scene->OnDeviceLost();
	}

	// シーンの削除関数
	template <typename T>
	void SceneManager<T>::DeleteScene()
	{
		if (m_scene)
		{
			m_scene->Finalize();

			m_scene.reset();
		}
	}


	/*template<typename T>
	inline void SceneManager<T>::PrepareNextScene(std::function<std::unique_ptr<Scene<T>>()> sceneFactory)
	{
		if (m_loadingThread.joinable())
		{
			m_loadingThread.join();
		}
		m_isLoading = true;
		m_loadingThread = std::thread{
			[=]() {
		m_nextScene = sceneFactory();
		m_nextScene->SetSceneManager(this);
		m_nextScene->Initialize();

		m_loadingMutex.lock();
		m_isLoading = false;
		m_loadingMutex.unlock();
		}

		};
	}*/
	template<typename T>
	inline void SceneManager<T>::PrepareNextScene(
		std::function<std::unique_ptr<Scene<T>>()> sceneFactory)
	{
		if (m_loadingThread.joinable())
		{
			m_loadingThread.join();
		}

		{
			std::lock_guard<std::mutex> lock(m_loadingMutex);
			m_isLoading = true;
		}

		m_loadingThread = std::thread{
			[this, sceneFactory]()
			{
				auto start = std::chrono::high_resolution_clock::now();
				auto scene = sceneFactory();
				scene->SetSceneManager(this);
				scene->Initialize();

				auto end = std::chrono::high_resolution_clock::now();
				float ms = std::chrono::duration<float, std::milli>(end - start).count();
				OutputDebugStringA(("Scene init took: " + std::to_string(ms) + "ms\n").c_str());
				{
					std::lock_guard<std::mutex> lock(m_loadingMutex);
					m_nextScene = std::move(scene);
					m_isLoading = false;
				}
				if (m_loadingScreen)
				{
					m_loadingScreen->OnLoadingComplete();
				}
				
			}
		};
	}

	template<typename T>
	inline bool SceneManager<T>::IsLoading()
	{
		m_loadingMutex.lock();
		bool isLoading = m_isLoading;
		m_loadingMutex.unlock();
		return isLoading;
	}

}

