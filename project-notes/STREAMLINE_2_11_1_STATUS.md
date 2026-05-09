# Q2RTX 0.9 / Streamline 2.11.1 Status

Дата фиксации: 2026-05-09

## Итоговый статус

Обновление Q2RTX + DLSS до Streamline 2.11.1 / DLSS 4.5 считается завершённым и проверенным пользователем.

Пользователь подтвердил:
- runtime-сборка работает нормально;
- меню DLSS работает;
- overlay работает;
- MFG / Variable MFG после исправлений работает;
- DeepDVC параметры применяются live на загруженной карте;
- готов ModDB NoPAK пакет для выкладки.

## Текущая runtime-сборка

Полная рабочая папка с `.pak` файлами для локального тестирования:

- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\`

Главный executable:

- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- `SHA256`: `9E93070B76497563E238558C4D41AB926224396B82E4D20A6A373D4B66B78079`

Актуальное меню:

- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
- `SHA256`: `87AA5DBE4971C7F6E335031190C3723A15E157D34378D0BFDB665DBC952C8C65`

Актуальный game DLL:

- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
- `SHA256`: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`

## Streamline / DLSS бинарники

Для релизного пакета используются DLL из текущей проверенной runtime-сборки:

- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.common.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.deepdvc.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.directsr.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.dlss.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.dlss_d.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.dlss_g.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.interposer.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.nis.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.nvperf.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.pcl.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.reflex.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\nvngx_dlss.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\nvngx_dlssd.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\nvngx_dlssg.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\nvngx_deepdvc.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\NvLowLatencyVk.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\nvngx.dll`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\_nvngx.dll`

В релизный NoPAK пакет все top-level `*.dll` и `*.exe` были скопированы именно из `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\`.

## Что внедрено

Основные возможности:

- NVIDIA Streamline 2.11.1.
- DLSS 4.5 runtime.
- DLSS Super Resolution.
- DLAA.
- DLSS Custom Scale.
- DLSS Ray Reconstruction.
- DLSS RR presets: `default`, `D`, `E`.
- DLSS MFG 2X / 3X / 4X / 5X / 6X.
- MFG policy: `fixed`, `auto`, `dynamic`.
- Variable/Dynamic MFG max: `auto`, `2X`, `3X`, `4X`, `5X`, `6X`.
- Dynamic/Variable MFG target FPS.
- Vulkan DLSS-G queue parallelism mode.
- NVIDIA Reflex.
- NVIDIA Reflex FPS cap.
- RTX Dynamic Vibrance / Streamline DeepDVC.
- DeepDVC intensity.
- DeepDVC saturation boost.
- Modern colored DLSS/FPS performance overlay.
- Live-применение параметров подменю `NVIDIA DLSS`.
- Overlay поверх меню, но только когда загружена игровая карта.
- `r_maxfps` добавлен в DLSS-меню как удобный пункт для тестов.
- `cl_menu_alpha` / `menu opacity` добавлен в Video menu.
- Версия публичной сборки поднята до `0.9`.

## Меню

Главные DLSS-настройки перенесены в отдельное подменю:

- `Video -> NVIDIA DLSS`

Файл меню:

- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\baseq2\q2rtx.menu`
- runtime copy: `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`

Все основные параметры внутри `NVIDIA DLSS` помечены `--live`, поэтому применяются сразу при изменении значения или движении слайдера.

Live-параметры:

- `r_maxfps`
- `flt_dlss_enable`
- `flt_dlss_mode`
- `flt_dlss_custom_ratio`
- `flt_dlss_preset`
- `flt_dlss_sharpness`
- `flt_dlss_auto_exposure`
- `flt_deepdvc`
- `flt_deepdvc_intensity`
- `flt_deepdvc_saturation_boost`
- `flt_dlss_rr`
- `flt_dlss_rr_preset`
- `flt_dlss_mfg`
- `flt_dlss_mfg_policy`
- `flt_dlss_mfg_dynamic_max`
- `flt_dlss_mfg_dynamic_target_fps`
- `flt_dlss_mfg_queue_parallelism`
- `flt_dlss_reflex`
- `flt_dlss_reflex_fps_cap`
- `r_debug_dlss_overlay`

## Overlay

DLSS/FPS performance overlay:

- рисуется во время игры на загруженной карте;
- рисуется поверх любого открытого меню, если карта уже загружена;
- не рисуется поверх меню до загрузки игровой карты;
- скрывается/показывается через `r_debug_dlss_overlay` и/или `scr_fps`.

Overlay содержит информацию о:

- render FPS / display FPS;
- DLSS mode;
- SR/RR presets;
- DLL versions;
- MFG state;
- MFG cap / max;
- Variable/Dynamic MFG policy;
- Reflex;
- render/output resolution;
- scale;
- mip bias;
- TAA tuning;
- DeepDVC state/intensity/saturation.

## Исправленные проблемы по ходу внедрения

- Variable MFG больше не зависит от обычного фиксированного MFG multiplier как от целевого множителя.
- Для Variable/Dynamic MFG добавлена отдельная настройка max multiplier.
- Overlay больше не показывает бессмысленную Vulkan-строку про Dynamic Native unavailable.
- Reflex строка в overlay переставлена ниже MFG.
- Исправлено восстановление позиции меню после выхода из подменю DLSS через Esc.
- `r_maxfps` добавлен именно как существующая стандартная Q2RTX cvar, без создания новой команды.
- Исправлена просадка render FPS к 10 FPS, связанная с ошибочным изменением логики вокруг `r_maxfps`.
- Queue parallelism возвращён и подтверждён пользователем как рабочий.
- `cl_menu_alpha` реализован по аналогии с q2pro и реально меняет прозрачность фона меню.
- DeepDVC параметры теперь применяются live при открытом меню на загруженной карте.
- Overlay поверх меню ограничен состоянием загруженной карты.

## Ограничения

- DeepDVC рассчитан на SDR. HDR не является целевым режимом для DeepDVC.
- Для серьёзного тестирования MFG рекомендуется выключать HDR.
- MFG 5X/6X и Variable MFG зависят от GPU, драйвера и runtime support.
- 2X в первую очередь рассчитан на RTX 40/50.
- 3X/4X/5X/6X в первую очередь рассчитаны на RTX 50 и только когда runtime сообщает поддержку.
- RR стабилен для обычной игры, но сложные сцены с зеркалами, стеклом и отражениями могут иметь остаточные артефакты.
- DX12-only возможности Streamline не внедрялись, так как Q2RTX использует Vulkan.

## ModDB NoPAK пакет

Пакет для выкладки:

- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK.zip`
- `SHA256`: `17F17FFFEF04BB6239ADDFCDE79AAA39DA5474CC0E275D71A745DDD61C20D644`
- размер: `2479166898` bytes

Папка, из которой собран архив:

- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\`

Проверка состава:

- `.pak` не включены;
- `.pdb` не включены;
- `baseq2\logs\` не включён;
- `baseq2\save\` не включён;
- `baseq2\condumps\` не включён;
- backup-папки shader_vkpt не включены;
- top-level runtime binaries совпадают с текущей папкой `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\`;
- `baseq2\q2rtx.menu` совпадает с текущей runtime-сборкой.

## Публичная документация

Русская документация:

- `O:\Claude2\Q2RTX-1.8.1-GPT\README_PUBLIC_BETA_RU.txt`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\README_PUBLIC_BETA_RU.txt`
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\README_PUBLIC_BETA_RU.txt`

Английская документация:

- `O:\Claude2\Q2RTX-1.8.1-GPT\README_PUBLIC_BETA_EN.txt`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\README_PUBLIC_BETA_EN.txt`
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\README_PUBLIC_BETA_EN.txt`

Package readme:

- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\README_PACKAGE.md`

## GitHub source release 0.9

Публичный репозиторий:

- `https://github.com/Quake-Journey/Q2RTX_DLSS`

Локальный checkout для source-выкладки:

- `O:\Claude2\Q2RTX-1.8.1-GPT\github\Q2RTX-1.8.1-GPT-source\`

Батник публикации:

- `O:\Claude2\Q2RTX-1.8.1-GPT\github\publish_q2rtx_dlss_0_9_to_github.bat`

Что делает батник:

- проверяет `origin` на `https://github.com/Quake-Journey/Q2RTX_DLSS.git`;
- проверяет ветку `main`;
- зеркалит source-каталоги из `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\`;
- исключает `.git`, `.gitmodules`, runtime/binary artifacts, `.pak/.pkz`, `.dll/.exe/.pdb`;
- копирует актуальные публичные docs в `O:\Claude2\Q2RTX-1.8.1-GPT\github\Q2RTX-1.8.1-GPT-source\project-notes\`;
- может выполнить dry-run, sync-only, локальный commit без push или commit+push.

Команды:

- preview без изменений: `O:\Claude2\Q2RTX-1.8.1-GPT\github\publish_q2rtx_dlss_0_9_to_github.bat --dry-run`
- только локальная подготовка checkout: `O:\Claude2\Q2RTX-1.8.1-GPT\github\publish_q2rtx_dlss_0_9_to_github.bat --sync-only`
- локальный commit без push: `O:\Claude2\Q2RTX-1.8.1-GPT\github\publish_q2rtx_dlss_0_9_to_github.bat --no-push`
- commit и push в `origin/main`: `O:\Claude2\Q2RTX-1.8.1-GPT\github\publish_q2rtx_dlss_0_9_to_github.bat --push`

Текущий статус на 2026-05-09:

- `--dry-run` выполнен успешно;
- `--sync-only --yes` выполнен успешно;
- локальный checkout `O:\Claude2\Q2RTX-1.8.1-GPT\github\Q2RTX-1.8.1-GPT-source\` подготовлен под 0.9;
- commit и push не выполнялись.

## Промо-картинка

Исходная картинка:

- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX_DLSS_FG_RR.png`

Новая картинка:

- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX_Streamline_2_11_1_DLSS45_MFG6X_DeepDVC.png`
- `SHA256`: `E537EAAE624EA521D01AF6641CECF4A66CA5A4116FCD725DE760A24E900A68EA`

Правило для следующих вариантов промо:

- не убирать уже заявленные крупные фишки `DLSS 4.5`, `Transformer 2`, `MFG`, `Ray Reconstruction`;
- новые фишки Streamline 2.11.1 добавлять рядом или дополнительными акцентами.

## Рабочие правила на будущее

- Для тестов пользователь использует полную runtime-папку `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\`.
- Не делать отдельные тестовые архивы без команды пользователя.
- NoPAK архивы нужны только для ModDB/публичной выкладки и делаются только по команде пользователя.
- Если `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe` заблокирован, значит пользователь, скорее всего, запустил игру; нужно попросить закрыть игру и повторить копирование.
- В отчётах пользователю всегда указывать полные локальные пути.
