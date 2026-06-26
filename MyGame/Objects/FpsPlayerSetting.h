//--------------------------------------------------------------------------------------
// File: PlayerSettings.h
//
// 一人称プレイヤーの調整用パラメータ
//--------------------------------------------------------------------------------------
#pragma once
struct PlayerSettings
{
    // 移動
    float moveSpeed = 4.0f;            // 通常の歩行速度
    float crouchSpeed = 2.0f;          // しゃがみ時の移動速度
    float gravity = 25.0f;             // 重力（落下用、必要なら）

    // カメラ / 視点
    float mouseSensitivity = 0.0025f;  // マウス感度
    float fieldOfView = 65.0f;         // 視野角（狙いやすいよう少し狭め）

    // ステータス
    float maxHealth = 100.0f;          // 最大体力
    float maxStamina = 100.0f;         // 最大スタミナ（歩行や息止め用）
    float staminaRegenRate = 10.0f;    // スタミナ回復速度
    float staminaDrainRate = 5.0f;     // スタミナ消費速度

    // エイム / 息止め
    float breathHoldMaxTime = 3.0f;    // 息を止めて狙いを安定させられる最大秒数
    float breathRecoveryRate = 1.5f;   // 息止めをやめた時の回復速度
};