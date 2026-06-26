#ifndef SINGLETON_H
#define SINGLETON_H

namespace MyLib
{
	/// singletonパターン
	template <class T>
	class Singleton
	{
	protected:
		Singleton() {};
		virtual ~Singleton() {};

	private:
		// Singletonのためインスタンスをわかりやすい位置に変更​
		/// インスタンス
		static T* instance;

	public:
		/**
		* @brief インスタンスの生成
		* @return インスタンスのポインタ
		*/
		static T* CreateInstance()
		{
			if (instance == nullptr)
			{
				instance = new T();
			}

			return instance;
		};

		/**
		* @brief インスタンスの破棄
		*/
		static void DeleteInstance()
		{
			delete instance;
			instance = nullptr;
		};

		/**
		* @brief インスタンスの取得
		* @return インスタンスのポインタ
		*/
		static T* GetInstance() { return instance; }

	};

	template< class T >
	T* Singleton<T>::instance = nullptr;
}

#endif // !SINGLETON_H


