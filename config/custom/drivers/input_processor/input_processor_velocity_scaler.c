/*
 * Copyright (c) 2024 roBa Project
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_velocity_scaler

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <math.h>

#include <drivers/input_processor.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// 速度計算用の履歴バッファサイズ
#define VELOCITY_HISTORY_SIZE 4

struct velocity_scaler_config {
    uint8_t type;
    size_t codes_len;
    uint16_t *codes;
    
    // 加速度パラメータ
    uint16_t min_scale_factor;     // 最小倍率 (100分率)
    uint16_t max_scale_factor;     // 最大倍率 (100分率)
    uint16_t velocity_threshold;   // 速度閾値 (pixels/second)
    uint16_t acceleration_power;   // 加速度指数 (100分率)
    bool track_remainders;
};

struct velocity_scaler_data {
    // 時間追跡
    int64_t last_event_time;
    
    // 移動量履歴
    struct {
        int16_t x, y;
        int64_t timestamp;
    } movement_history[VELOCITY_HISTORY_SIZE];
    uint8_t history_index;
    
    // 現在の速度
    float current_velocity;
    
    // 残余値追跡用
    float remainder_x;
    float remainder_y;
};

// 移動量の大きさを計算
static float calculate_magnitude(int16_t dx, int16_t dy) {
    return sqrtf((float)(dx * dx + dy * dy));
}

// 速度を計算 (pixels per second)
static float calculate_velocity(struct velocity_scaler_data *data, int16_t dx, int16_t dy, int64_t current_time) {
    // 現在の移動量をヒストリに追加
    data->movement_history[data->history_index].x = dx;
    data->movement_history[data->history_index].y = dy;
    data->movement_history[data->history_index].timestamp = current_time;
    
    // 次のインデックスに進む
    data->history_index = (data->history_index + 1) % VELOCITY_HISTORY_SIZE;
    
    // 履歴から平均速度を計算
    float total_magnitude = 0;
    int64_t time_span = 0;
    uint8_t valid_samples = 0;
    
    for (int i = 0; i < VELOCITY_HISTORY_SIZE; i++) {
        if (data->movement_history[i].timestamp > 0) {
            total_magnitude += calculate_magnitude(
                data->movement_history[i].x, 
                data->movement_history[i].y
            );
            valid_samples++;
        }
    }
    
    if (valid_samples < 2) {
        return 0.0f;
    }
    
    // 最新と最古のタイムスタンプで時間差を計算
    int64_t newest_time = 0, oldest_time = INT64_MAX;
    for (int i = 0; i < VELOCITY_HISTORY_SIZE; i++) {
        if (data->movement_history[i].timestamp > 0) {
            if (data->movement_history[i].timestamp > newest_time) {
                newest_time = data->movement_history[i].timestamp;
            }
            if (data->movement_history[i].timestamp < oldest_time) {
                oldest_time = data->movement_history[i].timestamp;
            }
        }
    }
    
    time_span = newest_time - oldest_time;
    
    if (time_span <= 0) {
        return 0.0f;
    }
    
    // pixels per second で速度を計算
    return (total_magnitude * 1000.0f) / (float)time_span;
}

// 加速度カーブを適用してスケール係数を計算
static float calculate_scale_factor(const struct velocity_scaler_config *config, float velocity) {
    float min_scale = (float)config->min_scale_factor / 100.0f;
    float max_scale = (float)config->max_scale_factor / 100.0f;
    float threshold = (float)config->velocity_threshold;
    float power = (float)config->acceleration_power / 100.0f;
    
    if (velocity <= 0) {
        return min_scale;
    }
    
    // 正規化された速度 (0.0 - 1.0+)
    float normalized_velocity = velocity / threshold;
    
    // 加速度カーブ適用 (指数関数)
    float acceleration_factor = powf(normalized_velocity, power);
    
    // 最小倍率から最大倍率の間でスケール
    float scale_factor = min_scale + (max_scale - min_scale) * acceleration_factor;
    
    // 最大倍率でクリップ
    if (scale_factor > max_scale) {
        scale_factor = max_scale;
    }
    
    return scale_factor;
}

// 値をスケールして適用
static int apply_scaling(struct input_event *event, float scale_factor, float *remainder) {
    if (scale_factor <= 0) {
        event->value = 0;
        return 0;
    }
    
    // 残余を含めた値を計算
    float scaled_value = (float)event->value * scale_factor + *remainder;
    
    // 整数部分を抽出
    int16_t result = (int16_t)scaled_value;
    
    // 残余を更新
    *remainder = scaled_value - (float)result;
    
    event->value = result;
    return 0;
}

static int velocity_scaler_handle_event(const struct device *dev, struct input_event *event,
                                       uint32_t param1, uint32_t param2,
                                       struct zmk_input_processor_state *state) {
    struct velocity_scaler_data *data = dev->data;
    const struct velocity_scaler_config *config = dev->config;
    
    // イベントタイプをチェック
    if (event->type != config->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }
    
    // イベントコードをチェック
    bool code_matches = false;
    for (size_t i = 0; i < config->codes_len; i++) {
        if (config->codes[i] == event->code) {
            code_matches = true;
            break;
        }
    }
    
    if (!code_matches) {
        return ZMK_INPUT_PROC_CONTINUE;
    }
    
    // 現在の時刻を取得
    int64_t current_time = k_uptime_get();
    
    // X/Y軸の値を取得
    int16_t dx = 0, dy = 0;
    if (event->code == INPUT_REL_X) {
        dx = event->value;
    } else if (event->code == INPUT_REL_Y) {
        dy = event->value;
    }
    
    // 速度を計算
    float velocity = calculate_velocity(data, dx, dy, current_time);
    data->current_velocity = velocity;
    
    // スケール係数を計算
    float scale_factor = calculate_scale_factor(config, velocity);
    
    // スケーリングを適用
    float *remainder = (event->code == INPUT_REL_X) ? &data->remainder_x : &data->remainder_y;
    
    if (config->track_remainders) {
        apply_scaling(event, scale_factor, remainder);
    } else {
        event->value = (int16_t)((float)event->value * scale_factor);
    }
    
    // デバッグ出力
    LOG_DBG("Velocity: %.2f px/s, Scale: %.2f, Original: %d, Scaled: %d", 
            velocity, scale_factor, dx + dy, event->value);
    
    data->last_event_time = current_time;
    
    return ZMK_INPUT_PROC_CONTINUE;
}

static const struct zmk_input_processor_driver_api velocity_scaler_driver_api = {
    .handle_event = velocity_scaler_handle_event,
};

#define VELOCITY_SCALER_INST(index)                                                               \
    static uint16_t velocity_scaler_codes_##index[] = DT_INST_PROP(index, codes);               \
    static const struct velocity_scaler_config velocity_scaler_config_##index = {               \
        .type = DT_INST_PROP(index, type),                                                      \
        .codes = velocity_scaler_codes_##index,                                                 \
        .codes_len = DT_INST_PROP_LEN(index, codes),                                           \
        .min_scale_factor = DT_INST_PROP(index, min_scale_factor),                             \
        .max_scale_factor = DT_INST_PROP(index, max_scale_factor),                             \
        .velocity_threshold = DT_INST_PROP(index, velocity_threshold),                         \
        .acceleration_power = DT_INST_PROP(index, acceleration_power),                         \
        .track_remainders = DT_INST_PROP(index, track_remainders),                             \
    };                                                                                           \
    static struct velocity_scaler_data velocity_scaler_data_##index = {};                       \
    DEVICE_DT_INST_DEFINE(index, NULL, NULL, &velocity_scaler_data_##index,                    \
                          &velocity_scaler_config_##index, POST_KERNEL,                         \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &velocity_scaler_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VELOCITY_SCALER_INST)