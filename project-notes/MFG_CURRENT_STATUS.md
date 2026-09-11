# MFG_CURRENT_STATUS

## Актуальный статус — 2026-09-11, 0.95 beta

Текущий релиз: **0.95 beta / Streamline 2.14.1 / DLSS 310.9.1**. Главная сводка: [RELEASE_0.95_STATUS.md](RELEASE_0.95_STATUS.md). Более старые записи ниже сохраняются как история.

Исправление дрожания поверхностей/освещения с RR принято пользователем. RR preset F доступен в меню и выбран по умолчанию. Сохранены исправления запуска и live-применения Reflex.

Плавность MFG 4X–6X **не исправлена**. Исследование остановлено по ограничению времени; не возобновлять автоматически. Постоянная история и два итоговых CSV: [history/MFG_2026-09-11/README.md](history/MFG_2026-09-11/README.md). Лимит Reflex 300 не считать решением. Не сводить симптом к задержке мыши, RR или монитору; корневая причина пока не доказана.

Пакет: `releases/Q2RTX-DLSS-Public-Beta-0.95-NoPAK.zip`. Changelog: `CHANGELOG_0.95_RU.md` и `CHANGELOG_0.95_EN.md`. В GitHub используется отдельный checkout `github/Q2RTX-1.8.1-GPT-source`, remote `https://github.com/Quake-Journey/Q2RTX_DLSS.git`.


Дата фиксации: 2026-03-25

## Обновление 2026-05-09, итоговая сводка Streamline 2.11.1

Создан канонический итоговый статус по всему обновлению Streamline 2.11.1 / Q2RTX 0.9:

- `O:\Claude2\Q2RTX-1.8.1-GPT\STREAMLINE_2_11_1_STATUS.md`

Считать этот файл главным ориентиром перед продолжением работ по ветке Streamline/DLSS 0.9.
В нём зафиксированы:

- текущая проверенная runtime-сборка;
- актуальные DLL Streamline / DLSS / DeepDVC / Reflex;
- все внедрённые фишки;
- статус меню и live-применения;
- статус overlay;
- исправленные баги;
- ограничения;
- ModDB NoPAK пакет;
- публичная RU/EN документация;
- промо-картинка;
- рабочие правила на будущее.

## Обновление 2026-05-09, промо-картинка Streamline 2.11.1

Создана обновлённая промо-картинка рядом с исходником:
- исходная картинка: `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX_DLSS_FG_RR.png`
- новая картинка: `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX_Streamline_2_11_1_DLSS45_MFG6X_DeepDVC.png`
- `SHA256`: `E537EAAE624EA521D01AF6641CECF4A66CA5A4116FCD725DE760A24E900A68EA`

Смысл картинки:
- сохранить старые крупные акценты `DLSS 4.5`, `Transformer 2`, `MFG`, `Ray Reconstruction`;
- добавить новые фишки после внедрения Streamline 2.11.1: `MFG 5X/6X`, `Variable MFG`, `DeepDVC`, `Reflex FPS Cap`, live DLSS menu, modern overlay, variable max.

## Обновление 2026-05-09, ModDB NoPAK пакет

Подготовлен пакет для выкладки на ModDB без оригинальных `.pak` файлов.

Обновлена публичная документация:
- `O:\Claude2\Q2RTX-1.8.1-GPT\README_PUBLIC_BETA_RU.txt`
- `O:\Claude2\Q2RTX-1.8.1-GPT\README_PUBLIC_BETA_EN.txt`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\README_PUBLIC_BETA_RU.txt`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\README_PUBLIC_BETA_EN.txt`
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\README_PUBLIC_BETA_RU.txt`
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\README_PUBLIC_BETA_EN.txt`
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\README_PACKAGE.md`

Пакетная папка:
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\`

Архив для выкладки:
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK.zip`
- `SHA256`: `17F17FFFEF04BB6239ADDFCDE79AAA39DA5474CC0E275D71A745DDD61C20D644`
- размер: `2479166898` bytes

Контроль состава:
- внутри архива нет `.pak`;
- внутри архива нет `.pdb`;
- не включены `baseq2\logs\`, `baseq2\save\`, `baseq2\condumps\`;
- не включены backup-папки shader_vkpt;
- включены свежие top-level `*.dll` и `*.exe` из текущей runtime-сборки `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\`;
- включён свежий `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`.

Ключевые хэши в пакете:
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\q2rtx.exe`
  - `SHA256`: `9E93070B76497563E238558C4D41AB926224396B82E4D20A6A373D4B66B78079`
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\baseq2\q2rtx.menu`
  - `SHA256`: `87AA5DBE4971C7F6E335031190C3723A15E157D34378D0BFDB665DBC952C8C65`
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\nvngx_dlss.dll`
  - `SHA256`: `4E86DAD07A052A0672F231F98A11A463DA99239CA73E154D8E27951B260B99A8`
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\nvngx_dlssg.dll`
  - `SHA256`: `83230804A087272C6D7EB4C805AC5925CE1188C70F150CC8DBD5CD843F53946A`
- `O:\Claude2\Q2RTX-1.8.1-GPT\releases\Q2RTX-DLSS-Public-Beta-0.9-Streamline-2.11.1-NoPAK\nvngx_deepdvc.dll`
  - `SHA256`: `814BD287102FA0DBABA612FF94BE9023EA940210B5FBD2BAC2FABF7CB248B38A`

## Обновление 2026-05-09, DeepDVC live в меню

Исправлены две проблемы тестового меню:
- `DeepDVC` больше не отключается из-за `qvk.frame_menu_mode`, поэтому параметры `flt_deepdvc`, `flt_deepdvc_intensity`, `flt_deepdvc_saturation_boost` из подменю `NVIDIA DLSS` доходят до Streamline live на загруженной карте с открытым меню;
- в menu-mode DLSS SR-проход разрешён для обновления DeepDVC, но MFG при этом не тегается и принудительно уводится в off через `vkpt_dlss_force_mfg_off_for_menu()`;
- overlay поверх меню теперь рисуется только при `cls.state == ca_active`, то есть только когда загружена игровая карта.

Изменённые исходники:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\dlss.c`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\main.c`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\client\screen.c`

Runtime для проверки:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
  - `SHA256`: `9E93070B76497563E238558C4D41AB926224396B82E4D20A6A373D4B66B78079`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
  - `SHA256`: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
  - `SHA256`: `87AA5DBE4971C7F6E335031190C3723A15E157D34378D0BFDB665DBC952C8C65`

Проверка:
- `git -C O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src diff --check -- src/client/screen.c src/refresh/vkpt/dlss.c src/refresh/vkpt/main.c`
- `$env:VULKAN_SDK='O:\Claude2\Vulkan\SDK'; cmake --build 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' --config Release -- /maxcpucount`

## Обновление 2026-05-09, уточнение overlay

Уточнено поведение DLSS/FPS performance overlay:
- overlay больше не привязан к открытому подменю `NVIDIA DLSS`;
- при запущенной игровой карте без меню overlay рисуется штатно через игровой HUD;
- при любом открытом меню overlay дополнительно рисуется поверх меню, если соответствующее отображение включено настройками меню (`r_debug_dlss_overlay` для DLSS debug overlay и/или `scr_fps` для FPS).

Runtime для проверки:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
  - `SHA256`: `31BF0048539D43474CCE76928988E386A0F10C76FA18A4B6DC7167DAA8D9B869`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
  - `SHA256`: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
  - `SHA256`: `87AA5DBE4971C7F6E335031190C3723A15E157D34378D0BFDB665DBC952C8C65`

## Обновление 2026-05-09

Сделана косметическая/тестовая доработка DLSS-меню:
- в парсер меню добавлен флаг `--live` для cvar-backed пунктов `toggle`, `pairs`, `values/strings` и `range`;
- все параметры подменю `NVIDIA DLSS` в `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\baseq2\q2rtx.menu` помечены `--live`, поэтому изменения применяются сразу при переключении или движении слайдера, без закрытия меню;
- DLSS/FPS performance overlay теперь дополнительно рисуется поверх активного подменю `NVIDIA DLSS`, чтобы его можно было смотреть вместе с прозрачным меню.

Основные исходники:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\client\ui\script.c`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\client\ui\menu.c`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\client\ui\ui.c`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\client\screen.c`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\inc\client\ui.h`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\client\ui\ui.h`

Runtime для проверки разложен в полную папку с `.pak`:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
  - `SHA256`: `6BE937AB9BE2509EC54AB99C194161552DB19FC9C3C98248D379DE0C4A702DF3`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
  - `SHA256`: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
  - `SHA256`: `87AA5DBE4971C7F6E335031190C3723A15E157D34378D0BFDB665DBC952C8C65`

Проверка:
- `git -C O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src diff --check -- src/client/ui/menu.c src/client/ui/script.c src/client/ui/ui.h inc/client/ui.h src/client/ui/ui.c src/client/screen.c baseq2/q2rtx.menu`
- `$env:VULKAN_SDK='O:\Claude2\Vulkan\SDK'; cmake --build 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' --config Release -- /maxcpucount`

## Главное
Сейчас активная проблема не в том, что DLSS-G/MFG не включается.
MFG работает, но масштабируется плохо:
- render FPS деградирует слишком сильно при 2X/3X/4X;
- в неудачных ветках 4X даже оказывался хуже 3X;
- timerefresh сейчас не использовать для принятия решений по MFG;
- ориентироваться на `scr_fps 2` и внешний FPS overlay (Afterburner).

## Обновление 2026-03-26 01:14
По решению пользователя ветка активной доработки DLSS FG / MFG временно поставлена на паузу.

Что сделано перед паузой:
1. Откатаны два последних диагностических эксперимента:
   - `flt_dlss_mfg_swapchain_images`
   - `flt_dlss_mfg_compact_depth_mvec`
2. Эти временные cvar убраны из исходников.
3. Runtime возвращён на последнюю чистую стабильную точку до этих двух экспериментов:
   - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
   - время: `2026-03-26 00:25:15`
   - `SHA256`: `C2C8D53E1DC1C090E310998BF7C01C9CD2EDE5CBC1A45A7AA96A7F8494BD6319`
4. Для истории отдельно сохранена свежая пересборка уже очищённых исходников:
   - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx_reverted_source_clean_595F.exe`
   - `SHA256`: `595F72D20C9EFDA8825F0D2B05438B24964A1305D82AAE52ED15EF1D21871AF9`

Текущий practical status:
- DLSS FG / MFG в целом работает;
- отдельная остающаяся проблема: крупная просадка render FPS, особенно заметная в `Ultra Performance`;
- эта проблема отложена для отдельного возвращения позже, без продолжения текущей экспериментальной ветки.

## Обновление 2026-03-26 00:37
Проверен новый диагностический baseline:
- `Q2RTX\\baseq2\\condumps\\20.txt`

Что он наконец подтвердил без догадок:
1. При `MFG OFF` (`mode=0`) DLSS-G runtime остаётся валидным, а базовые внутренние времена такие:
   - `driver ~ 3.5 ms`
   - `osq ~ 5.8-5.9 ms`
   - `gpu ~ 5.2-5.3 ms`
   - `gpu_frame ~ 3.6-3.7 ms`
2. При `MFG ON` те же величины растут вместе с множителем:
   - `2X`: `driver ~ 5.5 ms`, `osq ~ 10.8 ms`, `gpu_frame ~ 5.6 ms`
   - `3X`: `driver ~ 6.7 ms`, `osq ~ 12.0 ms`, `gpu_frame ~ 6.8 ms`
   - `4X`: `driver ~ 8.1 ms`, `osq ~ 13.9 ms`, `gpu_frame ~ 8.2 ms`
3. Самое важное структурное отличие:
   - при `MFG OFF` в debug-печати видно `swap_images=2`
   - при `MFG ON` видно `swap_images=4`

Практический вывод:
- app-side CPU markers (`sim`, `submit`) сами по себе уже не выглядят корнем проблемы;
- сильный новый кандидат на remaining bottleneck: глубина swapchain / queueing path,
  который меняется именно при включении MFG.

Сделанная правка:
1. В `src/refresh/vkpt/main.c` добавлен новый cvar:
   - `flt_dlss_mfg_swapchain_images`
2. Этот cvar управляет количеством swapchain images только при активном MFG.
3. Значение по умолчанию оставлено `4`, чтобы не ломать текущую стабильность.
4. Допустимый диапазон для диагностики:
   - `2 .. 8`

Новый runtime-кандидат:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-26 00:37:19`
- `SHA256`: `318EEF2884B239D1685A4107103039DF1BAB2C023C97F72F64F85ED330A633E8`

Как проверять этот билд:
1. В игре:
   - `flt_dlss_mfg_swapchain_images 2`
   - `vid_restart`
   - проверить `0X -> 2X -> 3X -> 4X`
2. Затем:
   - `flt_dlss_mfg_swapchain_images 3`
   - `vid_restart`
   - повторить те же замеры
3. Потом можно вернуть:
   - `flt_dlss_mfg_swapchain_images 4`
   - `vid_restart`

Ключевая цель теста:
- проверить, действительно ли навязанные `4` swapchain images являются частью
  причины деградации render FPS при MFG.

## Обновление 2026-03-26 01:02
Проверен `Q2RTX\\baseq2\\condumps\\21.txt`.

Что подтвердилось:
1. Диагностика с `flt_dlss_mfg_swapchain_images` не дала заметного улучшения.
2. Практически значимой разницы между `2`, `3` и `4` swapchain images пользователь не увидел.
3. Значит ветку с swapchain depth / `numBackBuffers` можно считать вторичной, а не корневой.

Следующий сильный кандидат:
- PT depth и motion vectors в Q2RTX физически аллоцированы как display-sized ресурсы,
  а render-sized область задаётся только через extents/subrect.
- Для DLSS-G это может означать более дорогой внутренний path именно при сильном upscaling,
  даже если render extents переданы корректно.

Сделанная новая диагностическая правка:
1. Добавлен cvar:
   - `flt_dlss_mfg_compact_depth_mvec`
2. При `1`:
   - depth и motion vectors копируются в render-sized ring images;
   - HUDless остаётся на исходном display-sized ресурсе;
   - если размеры ring images не совпадают с текущим render size, код безопасно
     откатывается на обычный старый путь.
3. Это уже не возврат к старой неудачной схеме "копировать depth + mvec + hudless",
   а более узкий тест только для `depth+mvec`.

Новый runtime-кандидат:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-26 01:02:33`
- `SHA256`: `EFBBF6C714DD5504822CAA16BEC4B6769AB3FC76D0CA23DFA81E62C50A7889EF`

Как проверять:
1. Базово:
   - `flt_dlss_mfg_compact_depth_mvec 0`
   - `vid_restart`
   - замерить `0X -> 2X -> 3X -> 4X`
2. Затем:
   - `flt_dlss_mfg_compact_depth_mvec 1`
   - `vid_restart`
   - повторить те же замеры

Ключевая цель теста:
- проверить, не в display-sized `depth/mvec` path сидит основная remaining деградация
  render FPS при MFG, особенно в режимах сильного апскейлинга.

## Обновление 2026-03-25 22:55
Проверены новые `condump`:
- `Q2RTX\\baseq2\\condumps\\13.txt`
- `Q2RTX\\baseq2\\condumps\\14.txt`

Что они показали:
1. Перенос `RenderSubmitStart` ближе к первому graphics submit не дал полезного эффекта.
   По `13.txt` картина осталась прежней:
   - `2X`: `reflex ~ 4.6-4.8 ms`, `host ~ 5.5-5.7 ms`
   - `3X`: `reflex ~ 5.6-6.3 ms`, `host ~ 6.5-7.2 ms`
   - `4X`: `reflex ~ 7.2-7.8 ms`, `host ~ 8.3-8.5 ms`
   Поэтому этот перенос признан бесполезным и откатан.
2. Жёсткая диагностика с отключением `slReflexSleep()` при активном MFG полностью
   сломала DLSS-G pacing.
   По `14.txt`:
   - `reflex = 0.000`, но
   - `present` сразу вырос до десятков миллисекунд:
     - `2X`: `present ~ 13-33 ms`
     - `3X`: `present ~ 49 ms`
     - `4X`: `present ~ 74-104 ms`
   Практический вывод:
   - `slReflexSleep()` не является "лишней задержкой", которую можно просто убрать;
   - без неё MFG runtime начинает сам разваливать pacing через `present`;
   - значит основной remaining bug не в факте наличия `ReflexSleep`, а в том,
     как текущая интеграция Q2RTX взаимодействует с Reflex/Frame Generation pacing.
3. Пользователь справедливо указал на риск хождения по кругу. Этот вывод зафиксирован:
   - проблема последнего регресса не связана с файлами инструкции по сборке;
   - корректная схема сборки по-прежнему:
     - `build-claude-off`
     - `CONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF`
   - регресс был чисто логическим, в диагностическом патче по Reflex.

Сделанные действия:
1. Убран диагностический early-return из `vkpt_dlss_reflex_sleep()`.
2. Возвращён прежний placement `RenderSubmitStart` до transfer submit.
3. Собран свежий `Release` из правильного дерева `build-claude-off`.

Новый runtime после отката неудачной ветки:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-25 22:57:32`
- `SHA256`: `6FE0B0E625A737149C29251F23D8B262449519A34C78DC61D5E54F01E9B2EE7B`

## Обновление 2026-03-25 23:15
По `Q2RTX\\baseq2\\condumps\\15.txt` подтверждено:
- MFG снова работает в валидном runtime-состоянии:
  - `dg_status=0x0`
  - `dg_presented=2/3/4`
  - фиолетовая заливка и полный развал pacing отсутствуют
- но основной bottleneck не изменился:
  - `2X`: `reflex ~ 4.2-4.7 ms`, `host ~ 5.5-5.8 ms`
  - `3X`: `reflex ~ 5.1-5.9 ms`, `host ~ 6.3-6.8 ms`
  - `4X`: `reflex ~ 7.1-7.3 ms`, `host ~ 8.0-8.2 ms`

Новый сильный кандидат на корневую причину:
- до этого `SimulationStart` ставился прямо внутри `dlss_sl_begin_frame()`;
- `slReflexSleep()` вызывался уже после этого;
- значит PCL мог учитывать сам sleep как часть simulation-фазы кадра;
- это хорошо объясняет, почему pacer систематически завышал host frame time именно через `ReflexSleep`.

Сделанная правка:
1. `dlss_sl_begin_frame()` теперь только получает `FrameToken`.
2. Новый marker `SimulationStart` вызывается явно уже после `slReflexSleep()`.
3. `slReflexSleep()` и `SimulationStart` перенесены перед началом CPU build/work нового кадра в `R_BeginFrame_RTX`.

Текущий кандидат для проверки:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-25 23:15:20`
- `SHA256`: `C1FD2F09F1C2D8E53EF2EBCC20A60395ECABB091CB50C85E8894C4570F0B272A`

Примечание:
- первый relink после этой правки не дал нового бинарника из-за неочевидного поведения
  incremental build cache;
- затем был принудительно обновлён `refresh\\vkpt\\main.c.obj` и выполнен отдельный
  `--target client` relink;
- итоговый runtime above — это уже реальный новый кандидат с перенесённым `slReflexSleep()`
  в самый верх начала кадра.

## Обновление 2026-03-25 21:19
По `Q2RTX\\baseq2\\condumps\\11.txt` и `Q2RTX\\sl_debug.log` локализован отдельный регресс
диагностической ветки:
- в `[MFGDBG]` при включённом MFG появлялся `dg_status=0x2`;
- `reflex_eff` при этом оставался `0`;
- в `sl_debug.log` Streamline писал:
  - `eDLSSGStatusFailReflexNotDetectedAtRuntime - sl.reflex must be enabled and active`

Практический вывод:
- фиолетовая/малиновая заливка кадра не связана с самой проблемой деградации render FPS;
- это отдельный регресс от оставшегося в исходниках диагностического патча, который
  принудительно ломал runtime-активацию Reflex для DLSS-G.

Сделанная правка:
1. В `src/refresh/vkpt/dlss_sl.cpp` убран диагностический принудительный `Reflex Off`.
2. Собран новый `Release` из правильного дерева `build-claude-off`.

Новый бинарник:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-25 21:19:18`
- `SHA256`: `CDAEF0084795B0AFEEA5FD63BB3FFBC3E374464F0CD485CF8C35FB45DE4816AE`

Ожидаемое поведение этого билда:
- фиолетовая заливка при MFG должна исчезнуть;
- после подтверждения этого можно продолжать уже чистую диагностику основной проблемы:
  сильного падения render FPS при 2X/3X/4X.

## Обновление 2026-03-25 21:45
- Пользователь подтвердил: после отката сломанной диагностической ветки фиолетовая
  заливка при MFG исчезла.
- Дополнительно проверен новый кандидат, в котором `ReflexOptions.useMarkersToOptimize`
  возвращён в безопасное состояние `false`.
- По `Q2RTX\\baseq2\\condumps\\12.txt` существенных изменений по скорости нет.

Что важно из `12.txt`:
1. DLSS-G runtime теперь находится в валидном состоянии:
   - `dg_status=0x0`
   - `dg_presented=2/3/4` для соответствующих режимов
   - `reflex_eff=2`
2. Значит:
   - текущая проблема больше не связана с `Reflex not detected`;
   - MFG реально активен и генерирует нужное число кадров;
   - главный remaining bottleneck по-прежнему в pacing/render cadence, а не в том,
     что FG "не работает".
3. По временам из `12.txt`:
   - `2X`: `reflex ~ 4.7-4.9 ms`, `host ~ 5.6-5.8 ms`
   - `3X`: `reflex ~ 5.2-6.4 ms`, `host ~ 7.0-7.9 ms`
   - `4X`: `reflex ~ 6.8-7.6 ms`, `host ~ 8.0-8.3 ms`
   - `fence` почти нулевой;
   - `present` обычно низкий, но иногда всплескивает на переходах.

Практический вывод на текущий момент:
- после починки Reflex runtime фиолетовая заливка закрыта;
- `useMarkersToOptimize=true` не был корнем деградации render FPS;
- текущий основной диагноз:
  - MFG работает корректно,
  - DLSS-G state валиден,
  - render FPS режется главным образом в Reflex/pacing path.

Текущий runtime-кандидат:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-25 21:45:32`
- `SHA256`: `9D490077190D8E3E602B7993174D4CB9E2593216522E0D960BB13D7EE42326E2`

Что делать после переподключения:
1. продолжить разбор именно Reflex/pacing path, а не DLSS-G enable path;
2. проверить, не остаётся ли Reflex в "ранне применённом" состоянии до фактической
   активации игрового MFG-path;
3. отдельно проверить, требуется ли более корректная повторная подача Reflex options
   / marker state при переходе из menu/startup в активную игровую карту с включённым MFG;
4. при необходимости расширить `[MFGDBG]` ещё на момент/результат последнего
   `slReflexSetOptions` и `slReflexGetState`.

## Обновление 2026-03-25 11:39
Новые измерения пользователя (Ultra Performance) перед этим билдом:
- `flt_dlss_mfg_fps_cap = 0`: `280 -> 173/351 -> 129/375 -> 81/335`
- `flt_dlss_mfg_fps_cap = 200`: `275 -> 172/346 -> 129/388 -> 85/331`

Вывод:
- 4X по-прежнему проседает ниже 3X;
- `flt_dlss_mfg_fps_cap` в текущем виде не решает задачу пользователя.

Что изменено сейчас:
1. `flt_dlss_mfg_fps_cap` убран из игрового меню (сам cvar остаётся консольным).
2. Для MFG увеличено число swapchain back buffers с 4 до 6 (при включённом MFG), чтобы уменьшить throttling на 4X.

Новый бинарник для проверки:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-25 11:39:46`
- `SHA256`: `3FF724CE14575714AA24FD91783BEA698A2621176A76CC1F2102A09D3445BDC5`

## Обновление 2026-03-25 11:48
Найден и откатан регресс по старту/меню:
- в `sl_debug.log` после эксперимента было:
  - `Requesting swap-chain ... with 6 buffers, using 4 buffers`
  - затем `AcquireNextImageKHR ... warning 2` и фриз/зависание в меню.

Действие:
- откатили эксперимент `6` back buffers обратно на стабильные `4` при MFG.

Новый бинарник:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-25 11:48:45`
- `SHA256`: `7FCEBAD668A95342D080114150D7BA66B93715FC919EBEED18BCD2014686CEFA`

## Обновление 2026-03-25 11:58
Пользователь подтвердил: после отката на 4 буфера фриз в меню всё ещё есть.

Анализ нового `sl_debug.log`:
- swapchain уже корректно `with 4 buffers, using 4 buffers`;
- но через ~2 секунды после первого present всё равно появляется:
  - `AcquireNextImageKHR ... warning 2`
  - затем повторный `flush all current work` и зависание.

Сделанная правка:
- путь кадра теперь считает MFG-активным только когда:
  - `flt_dlss_mfg != 0`,
  - `cls.state == ca_active`,
  - `!qvk.frame_menu_mode`.
- То есть в меню/неактивном состоянии кадр всегда идёт по обычной (не-MFG) синхронизации.

Новый бинарник:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-25 11:58:41`
- `SHA256`: `EC9B3B4D1F50B0E67253CCCB64BA6C18561359D9529F0235AF97B3A87C2D1049`

## Обновление 2026-03-25 12:07
Пользователь подтвердил: фриз на старте/в меню оставался.

Дополнительная правка:
- `vkpt_dlss_mfg_is_enabled()` теперь возвращает `true` только при `cls.state == ca_active`.
- Это убирает активацию MFG-режима swapchain/SL в главном меню и на раннем старте.
- В активной игре MFG работает как раньше (меню внутри карты по-прежнему отключается через frame-path условия).

Новый бинарник:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-25 12:07:49`
- `SHA256`: `4DDE749D0C9991BD0B330B1CCDD25843B473801925F0A3C9B7C08C10B49E52E8`

## Обновление 2026-03-25 12:11 (rollback)
По запросу пользователя выполнен откат runtime-бинарника к последнему рабочему:
- источник: `O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\q2rtx.exe`
- установлен в: `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- `SHA256`: `72BB523E5C3BA309902018E3E62BD1508B766424B46F1165796D16FFFDCE2538`
- время файла: `2026-03-25 10:46:26`

## Обновление 2026-03-25 12:17 (buffer retry)
По запросу пользователя собран минимальный тест:
- база: рабочий rollback-бинарник (`72BB...`) по поведению;
- изменение только на MFG-буферы:
  - в swapchain при активном MFG `minImageCount` поднят до `6`.
- мои поздние ограничения `ca_active/menu` для MFG-path убраны, чтобы не смешивать факторы.

Новый бинарник:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- время: `2026-03-25 12:17:42`
- `SHA256`: `EA7DAF89617FD3DA5889ACC201EF9BA24F793CDCE37E39E60A3B88B4A9415439`

Результат теста пользователя:
- фриз при появлении меню повторился.

Вывод:
- увеличение host swapchain до 6 буферов в текущей интеграции непригодно (регресс старта/меню).

Действие:
- runtime откатан обратно на стабильный `72BB523E5C3BA309902018E3E62BD1508B766424B46F1165796D16FFFDCE2538`.

## Последний проверенный билд
Файл:
`O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`

Результат пользователя по последнему билду:
- `253 -> 163/319 -> 121/370 -> 88/360`
- интерпретация:
  - MFG 0X: render 253
  - MFG 2X: render 163, display 319
  - MFG 3X: render 121, display 370
  - MFG 4X: render 88, display 360

Вывод:
- перенос `ReflexSleep` в начало кадра был правильным шагом:
  - 2X и 3X заметно улучшились относительно плохих ранних состояний;
- но 4X всё ещё деградирует непропорционально сильно;
- display FPS всё ещё почти равен `render * multiplier`, что выглядит как throttling render cadence.

## Лучший недавний MFG-базис
Лучшее из недавних состояний пока такое:
- `253 -> 163/319 -> 121/370 -> 88/360`
- интерпретация:
  - MFG 0X: render 253
  - MFG 2X: render 163, display 319
  - MFG 3X: render 121, display 370
  - MFG 4X: render 88, display 360

Вывод:
- хотя render FPS всё ещё деградирует слишком сильно,
- это лучше ранних билдов, потому что:
  - 2X и 3X уже не режут render так катастрофически, как раньше;
  - 4X уже выше 3X по display FPS.

## Что уже установлено
1. `DLSS-G MFG 2X/3X/4X` работает без DLL-патча OTA runtime.
2. `WAR4639162` сам по себе не означает, что FG выключен.
3. Корневая причина старого кейса "MFG не работает" была в HDR, а не в OTA patch.
4. Основная открытая проблема сейчас: неэффективный MFG path и слишком большая просадка render FPS.
5. `timerefresh` временно не использовать как источник правды по MFG.
6. `scr_fps 2` и Afterburner совпадают по display-side поведению и считаются эталоном для текущих тестов.
7. Контрольный тест в другой игре (`Avowed`) на той же системе показывает нормальное scaling MFG:
   - `63 -> 109 -> 157 -> 196`
   - значит проблема локальна для интеграции `Q2RTX`, а не для драйвера, DLDSR или RTX 5090.
8. `MFG off in menu` уже исправлен и работает корректно.
9. Проблема зависит именно от связки `DLSS SR + MFG`:
   - `DLAA + MFG` почти линейный и близок к ожидаемому scaling;
   - `Quality + MFG` уже имеет заметный performance cost;
   - `Ultra Performance + MFG` ломается сильнее всего.
10. В свежем `sl_debug.log` по рабочим билдам остаются подозрительные сигналы:
   - `Vulkan setAsyncFrameMarker is not implemented!`
   - `Engaging WAR4639162`
   - `Invalid no warp resource extent ... (0 x 0)`
   - `slSetVulkanInfo: result=0 gfxFamily=0 gfxIndex=2 computeFamily=0 computeIndex=3 ofFamily=5 ofIndex=0 nativeOF=1 reqGfx=1 reqCompute=2 reqOF=1`

## Какие ветки уже проверены и признаны неудачными
1. Нативный optical flow / усложнённый queue layout.
   - Регрессия: 4X снова становился хуже 3X.
2. Возврат на `eBlockPresentingClientQueue` + снятие wait.
   - Регрессия: масштабирование 3X/4X стало ещё хуже.
3. Разные попытки править swapchain/backbuffer extent, fullscreen/borderless и DLDSR-ветку.
   - Существенного эффекта на основную проблему render FPS не дали.
4. Полное отключение `ReflexSleep`.
   - Это дало катастрофический регресс (`251 -> 26/51 -> 15/43 -> 10/37`) и признано ошибкой.
5. Разные queue/present эксперименты без изменения acquire cadence.
   - Часть из них улучшала 2X/3X, но не решала 4X.

## Текущая рабочая гипотеза для следующего захода
Сейчас наиболее вероятная корневая проблема такая:
1. при активном `MFG` host всё ещё слишком рано блокируется на present/backbuffer cadence;
2. конкретный сильный кандидат — ранний `vkAcquireNextImageKHR` и ожидание `image_available`, которое до сих пор происходило в начале кадра;
3. это особенно хорошо объясняет симптом:
   - `render FPS` падает почти пропорционально росту множителя;
   - `display FPS` остаётся близок к `render * multiplier`.

## Текущий билд, который ещё нужно проверить
Файл:
`O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`

Проверка файла:
- дата: `2026-03-25 10:46:26`
- `SHA256`: `72BB523E5C3BA309902018E3E62BD1508B766424B46F1165796D16FFFDCE2538`

Что изменено в этом билде:
- при активном `MFG` `vkAcquireNextImageKHR` перенесён из `R_BeginFrame_RTX` в `R_EndFrame_RTX`;
- ранний служебный submit в начале кадра больше не ждёт `image_available`;
- ожидание `image_available` теперь происходит только на финальном render submit в конце кадра.
- убран неудачный адаптивный governor для `flt_dlss_mfg_fps_cap` (он вызывал мерцание и скачки);
- `flt_dlss_mfg_fps_cap` снова работает как обычный стабильный render FPS cap;
- при активном `MFG` финальный render submit больше не ждёт `image_available` semaphore (ждём только без MFG), чтобы не блокировать render cadence.

Смысл теста:
- проверить, перестанет ли `render FPS` при активном `MFG` упираться в cadence backbuffer/present уже до начала рендера кадра.

## Что НЕ делать при следующем заходе
- не уходить в RR;
- не уходить в timerefresh;
- не переоценивать строку `WAR4639162` как главный блокер;
- не начинать снова с экспериментальной ветки native OFA, потому что она уже давала регресс;
- не отключать `ReflexSleep`, это уже ломало performance катастрофически.

## Что открыть первым делом, если нужен быстрый контекст
1. Этот файл.
2. `O:\Claude2\Q2RTX-1.8.1-GPT\BUILD_NOTES_DLSS.md`
3. свежий `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl_debug.log`

## Обновление 2026-03-25 12:38 (MSK)

Собран и выложен новый тестовый билд:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- `SHA256`: `7A9CECF47D5C73D5FFD3FA2F53E59CD75FB61638CAA81AB094F54C6EF198F9C9`
- `LastWriteTime`: `2026-03-25 12:38:11`

Что изменено в этом билде (точечно, без увеличения swapchain buffers):
1. Для `DLSS-G` при `MFG ON` включен
   `DLSSGOptions::queueParallelismMode = eBlockNoClientQueues`.
2. Добавлено корректное ожидание `DLSSGState::inputsProcessingCompletionFence`
   на GPU submit-пути кадра перед обновлением FG-входов (через timeline wait).
3. После успешного submit фиксируется состояние
   `vkpt_dlss_mark_frame_generation_inputs_waited(...)`, чтобы не ждать тот же fence повторно.
4. Количество backbuffers для MFG оставлено на стабильном уровне `4` (ветка с `6` снова вызывала фриз в меню).

Цель изменения:
- снизить блокировку presenting-очереди и убрать лишний pressure на render cadence,
  который может особенно сильно бить по `MFG 4X`.

## Обновление 2026-03-25 12:47 (MSK)

Последний заход с `queueParallelismMode = eBlockNoClientQueues` и GPU-wait на
`inputsProcessingCompletionFence` откатан.

Причина отката:
- при `MFG`, включённом в конфиге до старта, игра снова фризилась на появлении меню;
- при `MFG OFF` появился стартовый крэш в цепочке
  `sl.interposer -> 1B0_E658703 -> nvoglv64`;
- значит для текущей интеграции `Q2RTX + Streamline Vulkan` эта ветка сейчас нестабильна.

Собран новый откатный рабочий билд:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- `SHA256`: `74E49076D7FB223A4FCB998E0F4A66D8F14E7667B2853660D68AFF817F42D57A`
- `LastWriteTime`: `2026-03-25 12:46:58`

Практический вывод:
- ветку `queueParallelism + inputs fence wait` пока больше не трогать;
- возвращаемся к предыдущей рабочей линии и дальше ищем причину деградации `MFG 4X`
  без изменения startup/present stability.

## Обновление 2026-03-25 12:46 (MSK)

Сделан отдельный startup-fix после новых крэшей/фризов на старте:
- `vkpt_dlss_mfg_is_enabled()` снова ужесточён;
- теперь MFG считается реально активным только если:
  - `DLSS-G` доступен,
  - `flt_dlss_mfg != off`,
  - `cls.state == ca_active`,
  - `qvk.frame_menu_mode == false`.

Смысл:
- startup/menu/swapchain path больше не должен вести себя так, будто FG уже активен,
  просто потому что значение `flt_dlss_mfg` сохранено в конфиге;
- это должно вернуть стабильный старт и сохранить логику
  “MFG выключен в меню, включается только на игровой карте”.

Текущий тестовый билд:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- `SHA256`: `0433B5CB597972D0613C9143490A52AF0296C3B185DA6254A7B91D9E4A99B912`

## Обновление 2026-03-25 13:15 (forced rollback to known-good runtime)

После серии новых стартовых крэшей:
- `Q2RTX_CrashReport04.txt`
- `Q2RTX_CrashReport05.txt`

стало ясно, что текущая исходниковая ветка всё ещё содержит startup-regression в
раннем `sl.dlss_g / present` пути. Даже после частичного отката логика старта не
вернулась к реально рабочему состоянию.

Действие:
- текущий сломанный runtime сохранён как артефакт:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx_broken_2026-03-25_13-11-50.exe`
- рабочий runtime восстановлен из:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\q2rtx.exe`

Текущий рабочий runtime:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- `SHA256`: `72BB523E5C3BA309902018E3E62BD1508B766424B46F1165796D16FFFDCE2538`
- `LastWriteTime`: `2026-03-25 10:46:26`

Практический вывод:
- больше не пытаемся "докатывать" сломанный startup path поверх текущего exe;
- следующая работа по MFG должна идти от этого известного живого runtime-базиса.

## Обновление 2026-03-25 13:25 (user confirmation)

Пользователь подтвердил:
- текущий `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe` снова запускается нормально.

Дополнительно создан новый restore point именно для этого подтверждённого runtime:
- `O:\Claude2\Q2RTX-1.8.1-GPT\restore-points\2026-03-25_13-25-16_startup_recovered_72BB`
- runtime hash: `72BB523E5C3BA309902018E3E62BD1508B766424B46F1165796D16FFFDCE2538`

Следующий рабочий этап:
- больше не трогать startup/present stability;
- продолжать только MFG performance path, в первую очередь проблему
  непропорциональной деградации `MFG 4X`.

## Обновление 2026-03-25 13:43 (source/runtime desync confirmed)

После прямой перепроверки замечания пользователя подтверждено:
- текущий рабочий `Q2RTX\q2rtx.exe` с hash `72BB...` не является свежей сборкой
  из текущих исходников;
- это rollback-runtime, вручную восстановленный из `build-claude-off\Bin`;
- свежая сборка из текущего `Q2RTX-src` реально существует в
  `O:\Claude2\Q2RTX-1.8.1-GPT\build\Bin\q2rtx.exe`;
- hash этого fresh build: `0433B5CB597972D0613C9143490A52AF0296C3B185DA6254A7B91D9E4A99B912`;
- именно fresh build из текущих исходников и даёт startup crash/freeze.

Итог:
- факт "игра запускается" сейчас относится только к rollback-runtime `72BB...`;
- исходники и рабочий runtime сейчас рассинхронизированы;
- ближайшая задача: вернуть текущий `Q2RTX-src` в состояние, где новый build снова
  стартует так же стабильно, как rollback-runtime.

## Обновление 2026-03-25 14:08 (minimal startup rollback in source)

Сделан минимальный откат именно активной startup/menu логики в исходниках:
- `vkpt_dlss_mfg_is_enabled()` возвращён к простой схеме:
  `DLSS-G available && flt_dlss_mfg != off`;
- из `dlss_tag_mfg_common()` убрано принудительное выключение `MFG` при
  `cls.state != ca_active` / `frame_menu_mode`;
- из `R_EndFrame_RTX()` убран активный вызов `vkpt_dlss_force_mfg_off_for_menu()`.

Смысл этого отката:
- синхронизировать текущие исходники с более ранней, уже работавшей логикой
  startup/present path;
- не трогать при этом остальную DLSS/MFG интеграцию.

Собран новый fresh build из текущих исходников:
- `O:\Claude2\Q2RTX-1.8.1-GPT\build\Bin\q2rtx.exe`
- `SHA256`: `CCD57056C88DB02594504122C67CD282E29652D0D0E102A227E872903226423A`
- `LastWriteTime`: `2026-03-25 14:07:41`

Он уже переложен в runtime:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`

Предыдущий живой rollback-runtime сохранён отдельно:
- `O:\Claude2\Q2RTX-1.8.1-GPT\restore-points\2026-03-25_14-08-45_source_sync_candidate\Q2RTX\q2rtx.exe`

## Обновление 2026-03-25 14:15 (present path candidate)

Новый startup-candidate собран после отката `R_BeginFrame_RTX / R_EndFrame_RTX`
ближе к более старой рабочей схеме:
- `vkAcquireNextImageKHR` снова выполняется в `R_BeginFrame_RTX` всегда;
- убран специальный `MFG`-ветвящийся acquire в `R_EndFrame_RTX`;
- submit снова ждёт `image_available` обычным путём;
- перед `present` снова вызывается `vkpt_dlss_reflex_sleep()`;
- present снова идёт через `qvk.queue_graphics`, без отдельного `present_queue`.

Новый fresh build:
- `O:\Claude2\Q2RTX-1.8.1-GPT\build\Bin\q2rtx.exe`
- `SHA256`: `AAB9A6A49D835B50BFC816E7BD8BA3F303E337E00CB7D94C8C2FB10B81B8B8D1`
- `LastWriteTime`: `2026-03-25 14:14:27`

Он переложен в runtime:
- `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`

Предыдущий рабочий rollback-runtime сохранён отдельно:
- `O:\Claude2\Q2RTX-1.8.1-GPT\restore-points\2026-03-25_14-15-18_present_path_candidate\Q2RTX\q2rtx.exe`

## 2026-03-25 15:07:36
- Q2RTX_CrashReport09.txt confirmed startup crash is still the same first-present Streamline failure in 1B0_E658703.
- current main + old dlss_sl crashed.
- old main + current dlss_sl crashed.
- Built a true relink-only hybrid candidate from current build tree using the old stable objects from uild-claude-off for all three startup-sensitive units:
  - src/refresh/vkpt/main.c.obj
  - src/refresh/vkpt/dlss.c.obj
  - src/refresh/vkpt/dlss_sl.cpp.obj
- Candidate runtime copied to Q2RTX\\q2rtx.exe.
- Candidate SHA256: CD0251D5B7296D8FA2C73DBB704D5626A4F059E137AC43B727B15C7FE5D24A43
- If this candidate starts, the startup regression is isolated to those three translation units as a set.

## 2026-03-25 15:44:17
- User reported severe performance regression in fresh source build despite successful startup:
  - Ultra Performance no MFG dropped from about 275 to about 200 FPS
  - DLAA no MFG dropped from 60+ to about 35 FPS
- Built an additional baseline candidate from the partial source snapshot in
estore-points\2026-03-23_02-32-07_working_mfg_baseline\source-snapshot by overlaying its src/inc/baseq2 onto a temporary source root and configuring a dedicated uild-baseline-off with CONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF.
- Baseline candidate runtime now copied to Q2RTX\\q2rtx.exe for perf comparison.
- Baseline candidate SHA256: 553411AB0F20C968C876547E6DC056E768156A2620CABD022F273337A1C3B69B
- Previous fresh source build preserved as Q2RTX\\q2rtx_source_fresh_930A8AC9.exe.

## 2026-03-25 16:22:51
- Confirmed major perf regression between fast 72BB runtime and fresh source build even without MFG:
  - user measured roughly DLAA 60 FPS on 72BB
  - user measured roughly DLAA 33 FPS on fresh source build
- sl_debug comparison showed DLAA context creation itself is effectively identical between the two binaries (same 5160x2160 DLAA path, same DLUnified mode, same extents).
- Found a concrete code regression in current sources:
  - old fast path had a single
kpt_dlss_reflex_sleep() before present in main.c
  - current sources also called
kpt_dlss_reflex_sleep() inside
kpt_dlss_begin_frame() in dlss.c
  - this likely caused a double Reflex sleep per host frame.
- Applied minimal fix: removed Reflex sleep + simulation-end marker from
kpt_dlss_begin_frame(), leaving only dlss_sl_begin_frame(frame_idx) there.
- Rebuilt fresh Release from uild-claude-off and copied to runtime.
- New candidate SHA256: 73607E5845C9B54C870F246F09314B65B1619C082085EA90CD7DB1FDC39BC313
- Previous runtime saved as Q2RTX\\q2rtx_before_reflex_beginframe_fix.exe.
## 2026-03-25 16:34:24 (clean note)
- Confirmed closed issue: no-MFG performance regression in fresh source build.
- Root cause: a second
kpt_dlss_reflex_sleep() was being called from
kpt_dlss_begin_frame() in dlss.c, while main.c already called
kpt_dlss_reflex_sleep() before present.
- Fix applied:
kpt_dlss_begin_frame() now only calls dlss_sl_begin_frame(frame_idx).
- User confirmed the fixed fresh source build restored no-MFG performance to the level of q2rtx_restore_72BB_fast.exe.
- Working runtime SHA256: 73607E5845C9B54C870F246F09314B65B1619C082085EA90CD7DB1FDC39BC313.
- Restore point:
estore-points\2026-03-25_16-34-24_reflex_fix_perf_restored.
- Remaining open issue: severe render FPS drop when DLSS MFG 2X/3X/4X is enabled, especially with DLSS upscaling.

## 2026-03-25 17:00:00
- Tested a dedicated present-queue candidate by switching `dlss_sl_vkQueuePresentKHR(...)` from `qvk.queue_graphics` to `qvk.queue_present`.
- User test result in DLSS Ultra Quality:
  - `273 -> 164/325 -> 72/215 -> 60/242`
- Conclusion: this made MFG significantly worse, so the issue is not a simple "present must move off graphics queue" fix in the current synchronization model.
- Candidate was rejected and runtime was restored to the previous pre-test build.
## 2026-03-25 20:25:01
- Telemetry from condumps 6.txt and 7.txt still shows the main render-FPS bottleneck under MFG is ReflexSleep, not GPU fence or present.
- Marker-chain fix (SimulationEnd, RenderSubmitStart, RenderSubmitEnd) helped only marginally; MFG 2X/3X/4X still lose too much render FPS.
- New targeted candidate moves
kpt_dlss_reflex_sleep() from end-of-frame (R_EndFrame_RTX, immediately before
kQueuePresentKHR) to start-of-frame (R_BeginFrame_RTX, immediately after
kpt_dlss_begin_frame() / frame-token acquisition).
- Rationale: per Streamline guidance, sleep should happen where the app should idle before starting new-frame work. With sleep at end-of-frame, host render cadence is directly throttled after the frame is already built.
- Runtime candidate copied to Q2RTX\\q2rtx.exe.
- Candidate SHA256: 940CAD177F4C05BCA21F7B0014310F3FD1FA9EB3AF243FDDA2D334647D8C13D3.
- Previous runtime preserved as Q2RTX\\q2rtx_before_reflex_sleep_stage_move.exe.
## 2026-03-25 20:38:09
- condump 8.txt showed the sleep-stage move helped mainly 3X:
  - user result: 282 -> 182/364 -> 138/417 -> 79/331
  - ReflexSleep dropped for 3X from about 8.3 ms to about 6.0 ms
  - 2X stayed around 4.4-4.6 ms
  - 4X stayed around 9.5-11.0 ms
- This confirms the main bottleneck is still Reflex pacing, but marker-chain + sleep placement were only partial fixes.
- New diagnostic candidate forces driver Reflex low-latency mode OFF inside dlss_sl_reflex_set_options, while keeping the sl.reflex integration itself active:
  - slReflexSleep still called
  - PCL markers still sent
  - only ReflexOptions.mode applied to the driver is forced to Off
- Purpose: determine whether the catastrophic render-FPS drop is caused by Reflex low-latency mode itself rather than DLSS-G resource/present integration.
- Runtime candidate SHA256: 130F5AE483DFBA8FEAEB807B8BA1C1DAAA2D1DAE07569AE15EE84438FEAFA4D1.
- Previous runtime preserved as Q2RTX\\q2rtx_before_reflex_mode_off_diag.exe.
## 2026-03-25 20:54:06
- Reverted the invalid diagnostic build which forced Reflex mode OFF at the driver level; it caused a purple-tinted scene and was not a usable runtime.
- Analysis of condumps/9.txt (G-Sync on) vs condumps/9_gsync_off.txt showed:
  - the fundamental render-FPS problem remains in both cases;
  - however, with G-Sync ON the 4X path also accumulates much larger present cost;
  - with G-Sync OFF the 4X present cost is near-zero and total behavior is noticeably healthier.
- This points to an additional DLSS-G pacer / present-mode interaction layer on top of the existing Reflex pacing problem.
- New candidate changes Vulkan swapchain present-mode selection when MFG is active and
id_vsync 0:
  - old behavior preferred VK_PRESENT_MODE_IMMEDIATE_KHR
  - new behavior prefers VK_PRESENT_MODE_MAILBOX_KHR for the MFG path if available
- Rationale: IMMEDIATE is fine for plain rendering, but under DLSS-G it can interact poorly with VRR/G-Sync and inflate present-side pacing cost, especially at 4X.
- Runtime candidate SHA256: C6ADF9DB3A4B11243A52936E5B64A184EF9E078A1227F41D77F91F6E2603E4A2.
- Previous runtime preserved as Q2RTX\\q2rtx_before_mfg_mailbox_present.exe.
## 2026-03-25 21:09:23
- Reverted the bad MAILBOX-present experiment in the source tree; runtime is back on the normal non-purple visual path.
- Added extended [MFGDBG] telemetry so the next condump includes DLSS-G runtime state, not just timing:
  - dg_status (DLSSGState.status)
  - dg_presented (DLSSGState.numFramesActuallyPresented)
  - dg_vsync (DLSSGState.bIsVsyncSupportAvailable)
  -
eflex_eff (effective Reflex mode)
  - surf_vsync
  - present_mode
  - swap_images
- Purpose: determine whether the remaining render-FPS collapse is tied to an invalid DLSS-G runtime state, partial generated-frame drops, vsync/VRR capability handling, or purely to Reflex pacing with a valid FG state.
- Runtime candidate SHA256: 86CF1C0F3E23FD2249C231ED4B0C5D11AD356144B50094D6FDB8820534C41A3F.
- Previous runtime preserved as Q2RTX\\q2rtx_before_extended_mfg_dbg.exe.
## 2026-03-26 00:05:00
- condump 17.txt showed the previous "move Reflex sleep to very start of frame" candidate did not materially change the MFG render-FPS collapse.
- Current telemetry still points to Reflex/pacing as the dominant wall-clock cost, but our own coarse [MFGDBG] timers are no longer enough to tell whether the driver is spending that time in simulation, render-submit, driver queue, OS queue, or GPU frame pacing.
- Added a new C-compatible `DlssReflexDebugReport_t` bridge from `sl::ReflexState.frameReport`.
- New wrapper path:
  - `dlss_sl_get_latest_reflex_report(...)` in `dlss_sl.cpp`
  - `vkpt_dlss_get_latest_reflex_report(...)` in `dlss.c`
  - public declaration in `dlss.h`
- Extended `[MFGDBG]` in `main.c` to print internal Reflex report aggregates from Streamline itself:
  - `rpt(count,last, avg(sim, submit, present, driver, osq, gpu, gpu_frame), max(...))`
- This is diagnostic, not yet a fix. Goal: identify whether the real pacing loss is in app-side sim/submit markers, driver queue, OS render queue, or GPU timing.
- Fresh Release rebuilt from `build-claude-off` and copied to runtime.
- Runtime candidate path: `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
- Previous runtime preserved as `Q2RTX\q2rtx_before_reflex_report_diag.exe`.
## 2026-03-26 00:20:00
- condump 18.txt added the crucial missing detail: app-side `sim` and `submit` are tiny (~0.2 ms / ~0.5 ms), while the cost that grows with MFG multiplier is lower in the stack:
  - `driver` rises roughly 5.5 -> 6.8 -> 8.1 ms
  - `osq` rises roughly 10.7 -> 12.0 -> 13.9 ms
  - `gpu_frame` rises roughly 5.6 -> 6.8 -> 8.2 ms
- Conclusion: the current bottleneck is no longer explained by Q2RTX CPU marker placement alone; DLSS-G is likely choosing an expensive internal path based on our tagged inputs/formats.
- Strong new hypothesis from NVIDIA's official `ProgrammingGuideDLSS_G.md`:
  - the guide explicitly warns that DLSS-G does **not** support FP16/scRGB paths efficiently because they are too expensive in compute/bandwidth;
  - Q2RTX currently tags `HUDLessColor` using FP16 (`VK_FORMAT_R16G16B16A16_SFLOAT`) tone-mapped data.
- New diagnostic candidate disables the actual `HUDLessColor` resource input for DLSS-G while keeping the rest of MFG integration intact:
  - `dlss_tag_mfg_common(...)` now passes `hudLessBufferFormat = 0`
  - `dlss_sl_tag_g_resources(...)` now tags `kBufferTypeHUDLessColor` with a null resource when HUDless is intentionally omitted.
- Purpose of this build: verify whether FP16 HUD-less input is what pushes Streamline/DLSS-G into the expensive driver/GPU path.
- If performance improves materially, next step is not to leave HUDless disabled forever, but to build a proper SDR/LDR HUDless path matching backbuffer color space/format.
## 2026-03-26 00:28:00
- Rejected the "omit HUDLessColor" diagnostic branch:
  - user reported no FPS improvement;
  - HUD became visibly unstable / jerky;
  - occasional hitching appeared.
- Therefore FP16 HUD-less input is not the main cause of the render-FPS collapse, at least not in a way that is fixed by simply omitting the HUDLess tag.
- Reverted the HUDLess omission and returned to the normal MFG input set.
- New diagnostic build changes only logging behavior:
  - `[MFGDBG]` no longer resets when `flt_dlss_mfg 0` is selected;
  - it now logs baseline Reflex/driver/GPU telemetry for MFG OFF too;
  - `display_multiplier` is clamped to at least 1 in the debug print, and `display_fps` falls back to host-FPS estimate if Streamline returns 0.
- Goal of the next condump: first true apples-to-apples comparison of `0X` vs `2X/3X/4X` for `driver`, `osq`, `gpu`, and `gpu_frame`.
- Fresh Release rebuilt from `build-claude-off` and copied to runtime.
- Previous runtime preserved as `Q2RTX\\q2rtx_before_mode0_reflex_diag.exe`.

## 2026-05-08
- Current accepted test workflow:
  - runtime with pak files lives at `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX`;
  - source lives at `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src`;
  - build tree lives at `O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off`;
  - reports to the user must include full local paths.
- Streamline 2.11.1 is now the project runtime baseline for Q2RTX DLSS beta 0.9.
- Integrated MFG 5X/6X and Variable MFG controls:
  - `flt_dlss_mfg` remains the fixed multiplier or the on/off switch for Variable;
  - `flt_dlss_mfg_policy` selects fixed/auto/dynamic;
  - `flt_dlss_mfg_dynamic_max` is the separate Variable MFG max multiplier;
  - `flt_dlss_mfg_dynamic_target_fps` is the Variable target FPS, with 0 meaning display-refresh auto.
- Fixed Variable MFG multiplier coupling:
  - when policy is dynamic, Streamline no longer receives the ordinary fixed multiplier as the target multiplier;
  - the normal multiplier only enables/disables MFG, while `flt_dlss_mfg_dynamic_max` supplies the ceiling.
- Overlay now reports MFG state, runtime cap, requested/effective policy and Reflex on a separate line below MFG.
- Native Dynamic MFG availability was removed from the overlay because this Q2RTX integration is Vulkan-only and Streamline 2.11.1 native Dynamic MFG is not active on Vulkan.
- DLSS menu now exposes `r_maxfps` as `Max FPS` near the top of `Video -> NVIDIA DLSS`, range `0..1000`, with `0` meaning disabled.
- Streamline 2.11.1 follow-up implementation for the current build:
  - explicitly load PCL as a Streamline feature because Q2RTX depends on PCL markers for Reflex/DLSS-G pacing;
  - expose Reflex's driver-aware FPS limiter through `ReflexOptions::frameLimitUs`, cvar `flt_dlss_reflex_fps_cap`;
  - expose Vulkan DLSS-G queue parallelism (`DLSSGOptions::queueParallelismMode`) as opt-in cvar `flt_dlss_mfg_queue_parallelism`, default off.
- Deferred:
  - DeepDVC needs a careful post-tonemap, pre-HUD/UI evaluate point;
  - NIS is lower priority because Q2RTX already has DLSS and FSR upscale paths;
  - DirectSR is not applicable to this Vulkan renderer because Streamline DirectSR is D3D12-only.
- Fresh Release rebuilt from `O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off` and copied to runtime with pak files:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
    - SHA256: `45A3FF877B806FE846CEC3CCB5FFD2D56A2350482D676B7D876F73C007431CAF`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
    - SHA256: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
    - SHA256: `E8F3809A6383F793D28540F9BE6DC1F89B0FEB940866E95D56E92A138A8867EB`

## 2026-05-09
- Overlay cleanup:
  - removed `native dynamic unavailable/available` from the DLSS overlay because Q2RTX is currently Vulkan-only and Streamline 2.11.1 native Dynamic MFG is not active on Vulkan.
- Menu cleanup:
  - added `Max FPS` to the top of `Video -> NVIDIA DLSS`;
  - cvar: `r_maxfps`;
  - range: `0..1000`;
  - `0` means disabled/unlimited.
- Regression fix:
  - `flt_dlss_mfg_queue_parallelism` was found saved as `1` in `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2config.cfg`;
  - this experimental Streamline `eBlockNoClientQueues` path can collapse render pacing toward 10 FPS in the current Vulkan submit/present path;
  - menu entry `DLSS MFG queue mode` was removed;
  - code now keeps the cvar for config compatibility but forces the safe default value `0`.
- Fresh Release rebuilt from `O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off` and copied to runtime with pak files:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
    - SHA256: `D1AE078EA9F59359F6AF05F128749BF0CE072491541D3BE28D7CD363DD158931`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
    - SHA256: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
    - SHA256: `E6641173BB233D5FE4C123164A3F28E4F6A9D8723FEFF74A75FEF4F1CC2B5527`

## 2026-05-09 hotfix
- User reported render FPS trending to ~10 even with `r_maxfps 1000`.
- Confirmed `r_maxfps` is the stock Q2RTX cvar from `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\client\main.c`; only the menu entry was added.
- Runtime config at `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2config.cfg` had:
  - `r_maxfps "1000"`;
  - `flt_dlss_mfg_fps_cap "0"`;
  - `flt_dlss_reflex_fps_cap "0"`;
  - `flt_dlss_mfg_queue_parallelism "1"`.
- Temporary hotfix for testing:
  - removed `DLSS MFG queue mode` from the menu;
  - kept `flt_dlss_mfg_queue_parallelism` only for config compatibility;
  - forced `vkpt_dlss_get_mfg_queue_parallelism()` to return `0`, so Streamline `eBlockNoClientQueues` is not activated even if old config has `1`.
- Fresh Release copied to runtime with pak files:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
    - SHA256: `309634C3D2BBC3B4C854EDEE994588CC682A1ABBA8BA11383049AF18292E0C96`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
    - SHA256: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
    - SHA256: `1719A601C24C81A9067D3E4C8179A30FA3BEA62BFD4BA096FCE563B52EC970E3`

## 2026-05-09 queue parallelism restored
- User confirmed the 10 FPS regression did not reproduce after the hotfix, so `flt_dlss_mfg_queue_parallelism` is restored as a real option rather than a forced-off compatibility cvar.
- Restored:
  - `DLSS MFG queue mode` in `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\baseq2\q2rtx.menu`;
  - overlay token `queue default/parallel` in `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\main.c`;
  - `vkpt_dlss_get_mfg_queue_parallelism()` returning the cvar value in `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\dlss.c`.
- Runtime config prepared for immediate test:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2config.cfg`
  - `flt_dlss_mfg_queue_parallelism "1"`.
- Fresh Release copied to runtime with pak files:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
    - SHA256: `0B3C6333A62FA76220D38435263C107D2226255376001A6E155A0C28D1A60599`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
    - SHA256: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
    - SHA256: `E6641173BB233D5FE4C123164A3F28E4F6A9D8723FEFF74A75FEF4F1CC2B5527`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2config.cfg`
    - SHA256: `66657FC527A5789038D312CF98394775C481C00A246BD4565F18D8893C75BC6B`

## 2026-05-09 DeepDVC / RTX Dynamic Vibrance
- User approved implementing only DeepDVC from the remaining Streamline 2.11.1 features.
- Implemented Streamline `sl::kFeatureDeepDVC` loading and support query in:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\dlss_sl.cpp`
- Runtime DeepDVC files are present:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\sl.deepdvc.dll`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\nvngx_deepdvc.dll`
- Added cvars:
  - `flt_deepdvc`
  - `flt_deepdvc_intensity`
  - `flt_deepdvc_saturation_boost`
  - `flt_deepdvc_available`
- Added menu controls in:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\baseq2\q2rtx.menu`
  - menu path: `Video -> NVIDIA DLSS -> RTX Dynamic Vibrance`
- Render integration:
  - DeepDVC evaluates after tone mapping and before final blit/HUD;
  - on DLSS SR/RR paths it runs before the DLSS-G `HUDLessColor` resource tag, so MFG sees the processed color buffer;
  - supported sources: DLSS SR output, DLSS RR final TAA output, FSR output, final-resolution TAA output;
  - HDR/scRGB, `tm_enable 0`, menu frame, or non-final-resolution source disable the feature for that frame.
- Fresh Release copied to runtime with pak files:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
    - SHA256: `4637C8460AB5F0806A155E8068B52031248FCB2A5136800FFDB1CD4C088CDF27`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
    - SHA256: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
    - SHA256: `26937CFF78A9707F22E0817404E2E5F93B540527B08915829A03238904CB6508`

## 2026-05-09 Menu opacity control
- Ported q2pro-style runtime menu background opacity control as `cl_menu_alpha`.
- Source changes:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\client\ui\menu.c`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\client\ui\ui.c`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\baseq2\q2rtx.menu`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\doc\client.md`
- Behavior:
  - `cl_menu_alpha 1` keeps the original menu opacity;
  - lower values multiply only menu background/decorative surfaces;
  - menu text and selectable controls remain readable;
  - the Video menu now contains a `menu opacity` slider for `cl_menu_alpha`.
- Build/copy status:
  - Release build succeeded from `O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off`
  - copied to runtime with pak files:
    - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
      - SHA256: `1503DA8DFCA30A513F4208683D234D5F005323719AB59BBE7CD0440A33DE7B61`
    - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
      - SHA256: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`
    - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
      - SHA256: `05B4DB37D802325D4CA98730E9860B5FC6D7C10476CBA14A5146E2B6B801148A`

## 2026-05-09 Menu opacity follow-up
- User reported that `menu opacity` still showed Q2RTX's blurred menu backdrop, so the live opacity slider did not reveal the real game image clearly.
- Follow-up change:
  - when `cl_menu_alpha < 1`, Q2RTX no longer forces menu bloom/blur or HDR menu clamp;
  - `cl_menu_alpha 1` keeps the original Q2RTX menu backdrop behavior.
- Source changes:
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\bloom.c`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\main.c`
  - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\vkpt.h`
- Build/copy status:
  - Release build succeeded from `O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off`
  - copied to runtime with pak files:
    - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
      - SHA256: `24C286DD019C1A6B9A8ABED8D92857D04AEA6266C6281E37EB117615E49E04B8`
    - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll`
      - SHA256: `FFD8BEEE397C8AC9689343C98A0EF60374C465A5962ABF4F8CB428C2BC065335`
    - `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx.menu`
      - SHA256: `05B4DB37D802325D4CA98730E9860B5FC6D7C10476CBA14A5146E2B6B801148A`
