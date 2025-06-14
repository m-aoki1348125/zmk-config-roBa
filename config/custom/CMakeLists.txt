# Copyright (c) 2024 roBa Project
# SPDX-License-Identifier: MIT

# カスタム入力プロセッサーをビルドに追加
target_sources(app PRIVATE drivers/input_processor/input_processor_velocity_scaler.c)