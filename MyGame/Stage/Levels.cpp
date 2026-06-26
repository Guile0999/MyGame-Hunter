#include "pch.h"
#include "Stage/Levels.h"
#include <MyLib/PicoJSON/Include/picojson.h>
#include <fstream>
#include <string>

namespace MyLib
{
    // ヘルパー関数（このファイル内だけで使う）

    // UTF-8のstd::stringをstd::wstringに変換する
    static std::wstring Utf8ToWide(const std::string& s)
    {
        if (s.empty()) return std::wstring();
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
        std::wstring w(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], len);
        return w;
    }

    // JSONの経路配列（[[x,y,z],...]の配列）をパスのリストに変換する
    static std::vector<std::vector<DirectX::SimpleMath::Vector3>>
        ParsePaths(picojson::object& e, const std::string& key)
    {
        using DirectX::SimpleMath::Vector3;
        std::vector<std::vector<Vector3>> result;

        if (!e.count(key) || !e[key].is<picojson::array>())
            return result;   // キーが無ければ空

        auto& paths = e[key].get<picojson::array>();
        for (auto& pathVal : paths)            // 各ルート
        {
            if (!pathVal.is<picojson::array>()) continue;
            std::vector<Vector3> path;

            auto& points = pathVal.get<picojson::array>();
            for (auto& ptVal : points)         // 各ウェイポイント [x,y,z]
            {
                if (!ptVal.is<picojson::array>()) continue;
                auto& xyz = ptVal.get<picojson::array>();
                if (xyz.size() < 3) continue;

                Vector3 p(
                    (float)xyz[0].get<double>(),
                    (float)xyz[1].get<double>(),
                    (float)xyz[2].get<double>());
                path.push_back(p);
            }
            result.push_back(std::move(path));
        }
        return result;
    }

    // 失敗時にステージ1の安全な初期値へ戻す
    static void SetDefaults(
        int& deerCount, int& rabbitCount, int& bossTriggerKills,
        int& killsNeeded, float& timeLimit, std::wstring& missionText)
    {
        deerCount = 1;
        rabbitCount = 0;
        bossTriggerKills = 0;
        killsNeeded = 1;
        timeLimit = 60.0f;
        missionText = L"Shoot a deer";
    }

    // ステージ設定をJSONファイルから読み込む
    void Levels::SetStage(int stage)
    {
        m_stage = stage;

        // JSONファイルを開く
        std::ifstream file("Resources/Data/levels.json");
        if (!file)
        {
            OutputDebugStringA("Levels: levels.json not found, using defaults\n");
            SetDefaults(m_deerCount, m_rabbitCount, m_bossTriggerKills,
                m_killsNeeded, m_timeLimit, m_missionText);
            return;
        }

        // JSONをパース
        picojson::value root;
        file >> root;
        std::string err = picojson::get_last_error();
        if (!err.empty() || !root.is<picojson::object>())
        {
            OutputDebugStringA(("Levels: JSON parse error: " + err + "\n").c_str());
            SetDefaults(m_deerCount, m_rabbitCount, m_bossTriggerKills,
                m_killsNeeded, m_timeLimit, m_missionText);
            return;
        }

        // "stages" 配列を取得
        auto& obj = root.get<picojson::object>();
        if (!obj.count("stages") || !obj["stages"].is<picojson::array>())
        {
            OutputDebugStringA("Levels: 'stages' array missing\n");
            SetDefaults(m_deerCount, m_rabbitCount, m_bossTriggerKills,
                m_killsNeeded, m_timeLimit, m_missionText);
            return;
        }
        auto& stages = obj["stages"].get<picojson::array>();

        // stage番号が一致するエントリを探す
        for (auto& entry : stages)
        {
            if (!entry.is<picojson::object>())
                continue;

            auto& e = entry.get<picojson::object>();

            int entryStage = (int)e["stage"].get<double>();
            if (entryStage != stage)
                continue;

            // 一致したので各値を読み込む（数値は全てdoubleで保持される）
            m_deerCount = (int)e["deerCount"].get<double>();
            m_rabbitCount = (int)e["rabbitCount"].get<double>();
            m_bossTriggerKills = (int)e["bossTriggerKills"].get<double>();
            m_killsNeeded = (int)e["killsNeeded"].get<double>();
            m_timeLimit = (float)e["timeLimit"].get<double>();
            m_missionText = Utf8ToWide(e["missionText"].get<std::string>());

            // 巡回経路を読み込む
            m_deerPaths = ParsePaths(e, "deerPaths");
            m_rabbitPaths = ParsePaths(e, "rabbitPaths");
            return;
        }

        // 一致するステージが無かった場合はデフォルトへ
        OutputDebugStringA("Levels: stage not found in JSON, using defaults\n");
        SetDefaults(m_deerCount, m_rabbitCount, m_bossTriggerKills,
            m_killsNeeded, m_timeLimit, m_missionText);
    }
}