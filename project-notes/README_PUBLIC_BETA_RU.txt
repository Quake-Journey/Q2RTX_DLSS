Q2RTX DLSS Edition by ly — Public Beta 0.8
Based on Q2RTX 1.8.1

Назначение
Эта публичная бета-сборка предназначена для тестирования интеграции NVIDIA DLSS Super Resolution, DLSS Ray Reconstruction, DLSS Multi Frame Generation и NVIDIA Reflex в Q2RTX.

Сборка ориентирована на владельцев совместимых NVIDIA RTX GPU и предназначена именно для бета-теста: сравнения качества изображения, поиска артефактов и проверки совместимости на разных конфигурациях.

Важно
- В пакет намеренно включён текущий рабочий q2config.cfg.
- Перед установкой желательно сделать резервную копию своего q2config.cfg.
- Это бета-версия. Некоторые режимы всё ещё могут вести себя нестабильно или давать артефакты в отдельных сценах.
- Оригинальные игровые pak-файлы в этот пакет не входят по лицензионным причинам. Для запуска нужна уже установленная копия Quake II RTX / Quake II RTX Remaster с легально полученными базовыми данными игры.

Установка
1. Сделайте резервную копию своей текущей папки Q2RTX или хотя бы файла baseq2\q2config.cfg.
2. Скопируйте содержимое папки/архива Q2RTX-Beta в каталог Q2RTX вашей установки Quake II RTX.
3. Разрешите замену файлов.
4. Запустите q2rtx.exe.

Что входит в эту бету
- NVIDIA DLSS Super Resolution
- DLAA
- DLSS Custom Scale
- DLSS Ray Reconstruction
- DLSS Multi Frame Generation 2X / 3X / 4X
- NVIDIA Reflex
- DLSS debug overlay с выводом пресетов, DLL-версий и активных параметров
- Улучшенный baseline для агрессивных DLSS-режимов через r_dlss_taa_input_profile 2
- Улучшенный baseline для RR-отражений через flt_dlss_rr_specular_stabilizers 2

Основные ограничения
- MFG уже работает, но при агрессивных режимах DLSS SR и особенно в Ultra Performance может наблюдаться существенная просадка render FPS.
- Для MFG рекомендуется тестировать игру с выключенным HDR.
- RR уже работает стабильно, но в отдельных сценах с зеркалами, стеклом и сложными отражениями остаточные артефакты всё ещё возможны.
- Формальный режим Ultra Quality скрыт из меню, вместо него используется режим Custom с ручной настройкой масштаба.
- Поддержка MFG зависит от GPU и драйвера. В рамках этой беты:
  - 2X рассчитан прежде всего на RTX 40/50;
  - 3X и 4X — прежде всего на RTX 50.
  Неподдерживаемые комбинации могут отображаться в настройках, но не гарантируются.

Видео-меню: добавленные и изменённые пункты
Все основные параметры находятся в Video -> RTX.

1. NVIDIA DLSS
- CVar: flt_dlss_enable
- Назначение: включает/выключает DLSS SR.

2. DLSS Ray Reconstruction
- CVar: flt_dlss_rr
- Назначение: включает DLSS RR.
- При активном RR legacy denoiser считается заменённым.

3. DLSS RR preset
- CVar: flt_dlss_rr_preset
- Значения:
  - 0 = default
  - 4 = D
  - 5 = E

4. DLSS mode
- CVar: flt_dlss_mode
- Значения:
  - 1 = Ultra Performance
  - 2 = Performance
  - 3 = Balanced
  - 4 = Quality
  - 6 = DLAA
  - 7 = Custom
- В меню Ultra Quality скрыт. Для близкого аналога используйте Custom.

5. DLSS custom scale
- CVar: flt_dlss_custom_ratio
- Диапазон: 33..99
- Назначение: ручной выбор render scale для режима Custom.

6. DLSS preset
- CVar: flt_dlss_preset
- Значения:
  - 0 = default
  - 6 = F
  - 7 = J
  - 8 = K (Transformer)
  - 9 = L (Transformer 2)
  - 10 = M (Transformer 2)

7. DLSS sharpness
- CVar: flt_dlss_sharpness
- Диапазон: 0..1
- Назначение: резкость DLSS.

8. DLSS auto-exposure
- CVar: flt_dlss_auto_exposure
- Назначение: включает автоэкспозицию DLSS.

9. DLSS debug overlay
- CVar: r_debug_dlss_overlay
- Назначение: выводит в верхний overlay информацию о DLSS/RR/FG, пресетах, DLL-версиях, Reflex, scale и диагностических параметрах.

10. DLSS MFG
- CVar: flt_dlss_mfg
- Значения:
  - 0 = off
  - 2 = 2X
  - 3 = 3X
  - 4 = 4X

11. NVIDIA Reflex
- CVar: flt_dlss_reflex
- Значения:
  - 0 = off
  - 1 = on
  - 2 = on + boost

12. denoiser
- CVar: flt_enable
- При включённом RR legacy denoiser не является основным реконструктором кадра и в UI отображается как replaced.

Основные консольные переменные
Пользовательские / рабочие
- flt_dlss_enable
- flt_dlss_rr
- flt_dlss_rr_preset
- flt_dlss_mode
- flt_dlss_custom_ratio
- flt_dlss_preset
- flt_dlss_sharpness
- flt_dlss_auto_exposure
- flt_dlss_mfg
- flt_dlss_reflex
- r_debug_dlss_overlay

Продвинутые / бета
- r_dlss_taa_input_profile
  - 0 = legacy
  - 1 = mild tuning
  - 2 = current recommended baseline for aggressive DLSS SR modes
- flt_dlss_rr_specular_stabilizers
  - 0 = raw RR behavior
  - 1 = partial RR reflection stabilization
  - 2 = current recommended RR baseline
- r_debug_pre_dlss_color
- r_debug_post_dlss_color
- r_debug_post_tonemap_color

Рекомендуемая базовая конфигурация для теста
- DLSS SR: под задачу пользователя
- RR: по необходимости
- MFG: тестировать отдельно от качества изображения
- Reflex: on или on + boost при использовании MFG
- r_dlss_taa_input_profile 2
- flt_dlss_rr_specular_stabilizers 2

Оверлей DLSS
При включении r_debug_dlss_overlay 1 overlay показывает:
- режим DLSS / RR / MFG
- пресеты SR и RR
- версии загруженных DLL
- состояние Reflex
- render resolution -> output resolution
- scale
- mip bias
- параметры TAA / anti-sparkle / variance

Credits / Third-party Content
Часть моделей в этой сборке использует данные мода Cinematic Mod for Quake II RTX:
https://www.moddb.com/mods/cinematic-mod-for-quake-ii-rtx/downloads

Этот пакет публикуется как публичная бета для тестирования. Все права на Quake II RTX, NVIDIA Streamline / NGX и исходные сторонние компоненты принадлежат их правообладателям.


DLSS / Streamline диагностика
- `flt_dlss_sl_debug_log 0` — по умолчанию выключено, файл `sl_debug.log` не создаётся.
- `flt_dlss_sl_debug_log 1` — включает подробный лог Streamline в `sl_debug.log` рядом с `q2rtx.exe`. Использовать только для диагностики.
- Переменная доступна только через консоль / `q2config.cfg` и не вынесена в меню игры.


Каналы проекта
- Telegram: https://t.me/Q2RTX
- YouTube: https://www.youtube.com/@QuakeJourney
