#pragma once
#include <string>
#include <vector> 
#include <SimpleMath.h>  

namespace MyLib
{
    class Levels
    {
    public:
        Levels() = default;

        // Configure all data for the given stage
        void SetStage(int stage);

        int  GetStage() const { return m_stage; }

        // Spawn counts
        int  GetDeerCount()   const { return m_deerCount; }
        int  GetRabbitCount() const { return m_rabbitCount; }

        // Boss: appears after this many kills (0 = no boss this stage)
        int  GetBossTriggerKills() const { return m_bossTriggerKills; }
        bool HasBoss() const { return m_bossTriggerKills > 0; }

        // Completion
        int   GetKillsNeeded() const { return m_killsNeeded; }
        float GetTimeLimit()   const { return m_timeLimit; }

        // UI text
        const std::wstring& GetMissionText() const { return m_missionText; }


        // 巡回経路（外側=ルート、内側=ウェイポイント）
        const std::vector<std::vector<DirectX::SimpleMath::Vector3>>& GetDeerPaths()   const { return m_deerPaths; }
        const std::vector<std::vector<DirectX::SimpleMath::Vector3>>& GetRabbitPaths() const { return m_rabbitPaths; }


    private:
        int          m_stage = 1;
        int          m_deerCount = 1;
        int          m_rabbitCount = 0;
        int          m_bossTriggerKills = 0;
        int          m_killsNeeded = 1;
        float        m_timeLimit = 60.0f;
        std::wstring m_missionText = L"Shoot a deer";

        std::vector<std::vector<DirectX::SimpleMath::Vector3>> m_deerPaths;
        std::vector<std::vector<DirectX::SimpleMath::Vector3>> m_rabbitPaths;
    };
}
