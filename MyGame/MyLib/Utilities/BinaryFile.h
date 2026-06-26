/// <summary>
/// 	BinaryFile.h
///
///	CSOをはじめとした、バイナリ形式で保存されたファイルを読み込み、管理するクラス。
///	ファイルデータをchar配列としてm_dataとして確保し続けるため、何らかの形で重複を避ける管理の仕組みが必要。
/// </summary>
#pragma once

#include <memory>

namespace MyLib
{
	class BinaryFile
	{
	protected:

		//	データ
		std::unique_ptr<char[]> m_data;

		//	サイズ
		unsigned int m_size;

	public:

		//	ファイル名を指定してロード
		static std::unique_ptr<MyLib::BinaryFile> LoadFile(const wchar_t* fileName);

		BinaryFile();

		//	ムーブコンストラクタ
		BinaryFile(BinaryFile* in);

		//	アクセサ
		char* GetData() { return m_data.get(); }
		unsigned int GetSize() { return m_size; }
	};
}