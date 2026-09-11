Q2RTX DLSS Edition by ly - Public Beta 0.95
Streamline 2.14.1 / DLSS 310.9.1
Based on Q2RTX 1.8.1

Обновление 0.95 beta (2026-09-11)
Подробные изменения: CHANGELOG_0.95_RU.md.
Исправлено дрожание поверхностей/освещения при RR; добавлен и выбран по умолчанию RR preset F.
Остался известный баг плавности MFG 4X–6X. Лимит Reflex 300 его не исправляет.
Dynamic MFG в Vulkan использует Auto fallback: режим не даёт нативного динамического выбора множителя.
DeepDVC может показывать служебную надпись NVIDIA и выключен в стартовом конфиге пакета.
Стартовый конфиг пакета: RR on / F, MFG 3X Fixed, DeepDVC off, экспериментальная буферизация MFG off.
Лимит Reflex учитывает итоговый FPS вместе со сгенерированными кадрами; 0 отключает лимит.

Назначение
Эта публичная бета-сборка предназначена для тестирования интеграции NVIDIA Streamline 2.14.1, DLSS Super Resolution, DLSS Ray Reconstruction, DLSS Multi Frame Generation, NVIDIA Reflex и RTX Dynamic Vibrance / DeepDVC в Q2RTX.

Сборка ориентирована на владельцев совместимых NVIDIA RTX GPU и предназначена для проверки качества изображения, производительности, совместимости и поведения новых режимов DLSS в реальной игре.

Важно
- Оригинальные игровые .pak-файлы в этот пакет не входят по лицензионным причинам.
- Для запуска нужна уже установленная легальная копия Quake II RTX / Quake II RTX Remaster с базовыми данными игры.
- Пакет ставится поверх существующей папки Q2RTX.
- В пакет намеренно включён рабочий baseq2\q2config.cfg. Перед установкой желательно сделать резервную копию своего baseq2\q2config.cfg.
- Это beta/release-candidate сборка. Некоторые режимы зависят от GPU, драйвера и текущего NVIDIA runtime.

Установка
1. Сделайте резервную копию своей текущей папки Q2RTX или хотя бы файла baseq2\q2config.cfg.
2. Распакуйте архив в каталог Q2RTX вашей установленной Quake II RTX.
3. Разрешите замену файлов.
4. Убедитесь, что ваши легальные .pak-файлы остаются на месте в baseq2.
5. Запустите q2rtx.exe.

Что входит в Public Beta 0.95
- NVIDIA Streamline 2.14.1.
- DLSS 310.9.1 runtime.
- NVIDIA DLSS Super Resolution.
- DLAA.
- DLSS Custom Scale.
- DLSS Ray Reconstruction.
- DLSS Multi Frame Generation 2X / 3X / 4X / 5X / 6X.
- Fixed / Auto / Dynamic-Variable MFG policy controls.
- Отдельный лимит Variable MFG max.
- Dynamic/Variable MFG target FPS.
- NVIDIA Reflex.
- NVIDIA Reflex FPS cap.
- RTX Dynamic Vibrance / DeepDVC.
- Live-применение всех параметров подменю NVIDIA DLSS.
- Отдельное подменю Video -> NVIDIA DLSS.
- Улучшенный цветной DLSS/FPS performance overlay.
- Отображение overlay поверх меню, но только когда загружена игровая карта.
- Настройка прозрачности меню.
- Удобный пункт r_maxfps в DLSS-меню.

Видео-меню
Основные параметры DLSS теперь находятся в:

Video -> NVIDIA DLSS

Параметры применяются live, без закрытия меню. Это сделано специально для тестирования с прозрачным меню и включённым overlay.

Основные пункты меню

1. Max FPS
- CVar: r_maxfps
- Диапазон: 0..1000
- 0 = лимит выключен.
- Это стандартная переменная Q2RTX, вынесенная в меню для удобства тестов DLSS/MFG/Reflex.

2. NVIDIA DLSS
- CVar: flt_dlss_enable
- Включает или выключает DLSS Super Resolution.

3. DLSS mode
- CVar: flt_dlss_mode
- Значения:
  - Ultra Performance
  - Performance
  - Balanced
  - Quality
  - Custom
  - DLAA

4. DLSS custom scale
- CVar: flt_dlss_custom_ratio
- Диапазон: 33..99
- Используется в режиме Custom.

5. DLSS preset
- CVar: flt_dlss_preset
- Значения:
  - recommended
  - F
  - J
  - K
  - L
  - M
- Recommended использует текущую рекомендуемую логику для разных DLSS-режимов.

6. DLSS sharpness
- CVar: flt_dlss_sharpness
- Диапазон: 0..1.

7. DLSS auto-exposure
- CVar: flt_dlss_auto_exposure

8. RTX Dynamic Vibrance
- CVar: flt_deepdvc
- Streamline DeepDVC / RTX Dynamic Vibrance.
- Работает в SDR. HDR для DeepDVC не является целевым режимом.

9. DeepDVC intensity
- CVar: flt_deepdvc_intensity
- Диапазон: 0..1.
- Применяется live на загруженной карте.

10. DeepDVC saturation
- CVar: flt_deepdvc_saturation_boost
- Диапазон: 0..1.
- Применяется live на загруженной карте.

11. DLSS Ray Reconstruction
- CVar: flt_dlss_rr
- При активном RR legacy denoiser считается заменённым.

12. DLSS RR preset
- CVar: flt_dlss_rr_preset
- Значения:
  - default
  - D
  - E
  - F

13. DLSS MFG
- CVar: flt_dlss_mfg
- Значения:
  - off
  - 2X
  - 3X
  - 4X
  - 5X
  - 6X

14. DLSS MFG policy
- CVar: flt_dlss_mfg_policy
- Значения:
  - fixed
  - auto
  - dynamic

15. DLSS MFG variable max
- CVar: flt_dlss_mfg_dynamic_max
- Значения:
  - auto
  - 2X
  - 3X
  - 4X
  - 5X
  - 6X
- Используется как верхняя граница для Dynamic/Variable MFG.

16. DLSS MFG dynamic target
- CVar: flt_dlss_mfg_dynamic_target_fps
- Диапазон: 0..480
- 0 = auto по частоте дисплея.

17. DLSS MFG queue mode
- CVar: flt_dlss_mfg_queue_parallelism
- Значения:
  - default
  - parallel

18. NVIDIA Reflex
- CVar: flt_dlss_reflex
- Значения:
  - off
  - on
  - on + boost

19. NVIDIA Reflex FPS cap
- CVar: flt_dlss_reflex_fps_cap
- Диапазон: 0..480
- 0 = выключено.

20. DLSS debug overlay
- CVar: r_debug_dlss_overlay
- Показывает DLSS/RR/MFG/Reflex/DeepDVC состояние, пресеты, DLL-версии и параметры рендера.

Дополнительные пункты Video

Menu opacity
- CVar: cl_menu_alpha
- Диапазон: 0..1
- Позволяет менять прозрачность фона меню во время игры.

FPS counter
- CVar: scr_fps
- Значения:
  - off
  - show FPS
  - show FPS and resolution scale

Overlay
При r_debug_dlss_overlay 1 и/или scr_fps overlay отображается:
- во время игры на загруженной карте;
- поверх любого открытого меню, если карта уже загружена;
- не отображается поверх меню до загрузки игровой карты.

Ограничения и важные замечания
- MFG 5X/6X и Variable MFG зависят от GPU, драйвера и возможностей текущего Streamline/NVIDIA runtime.
- 2X рассчитан прежде всего на RTX 40/50.
- 3X/4X/5X/6X рассчитаны прежде всего на RTX 50 и только когда runtime сообщает поддержку.
- Для серьёзного тестирования MFG рекомендуется выключать HDR.
- DeepDVC рассчитан на SDR.
- RR стабилен для обычной игры, но в сложных сценах с зеркалами, стеклом и отражениями остаточные артефакты возможны.
- Ultra Quality не вынесен отдельным пунктом меню; для ручного аналога используйте Custom scale.

Основные консольные переменные
- r_maxfps
- cl_menu_alpha
- scr_fps
- flt_dlss_enable
- flt_dlss_mode
- flt_dlss_custom_ratio
- flt_dlss_preset
- flt_dlss_sharpness
- flt_dlss_auto_exposure
- flt_deepdvc
- flt_deepdvc_intensity
- flt_deepdvc_saturation_boost
- flt_dlss_rr
- flt_dlss_rr_preset
- flt_dlss_mfg
- flt_dlss_mfg_policy
- flt_dlss_mfg_dynamic_max
- flt_dlss_mfg_dynamic_target_fps
- flt_dlss_mfg_queue_parallelism
- flt_dlss_reflex
- flt_dlss_reflex_fps_cap
- r_debug_dlss_overlay

Диагностика Streamline
- flt_dlss_sl_debug_log 0 - по умолчанию выключено, sl_debug.log не создаётся.
- flt_dlss_sl_debug_log 1 - включает подробный лог Streamline в sl_debug.log рядом с q2rtx.exe.
- Используйте только для диагностики.

Credits / Third-party Content
Часть моделей в этой сборке использует данные мода Cinematic Mod for Quake II RTX:
https://www.moddb.com/mods/cinematic-mod-for-quake-ii-rtx/downloads

Этот пакет публикуется как публичная beta/release-candidate сборка для тестирования. Все права на Quake II RTX, NVIDIA Streamline / NGX и сторонние компоненты принадлежат их правообладателям.

Каналы проекта
- Telegram: https://t.me/Q2RTX
- YouTube: https://www.youtube.com/@QuakeJourney
