# Q2RTX-1.8.1-GPT — заметки по интеграции NVIDIA Ray Reconstruction

Этот файл является текущим рабочим журналом по интеграции `DLSS Ray Reconstruction` в `Q2RTX`.

Документ надо обновлять по мере продвижения. Он фиксирует:
- как `RR` встроен в проект;
- какие файлы за это отвечают;
- какие баги уже закрыты;
- какой статус сейчас;
- что проверять дальше.

## Статус на 23 марта 2026

Подтверждено:
- `RR` больше не падает и не даёт серое `milk`-изображение;
- `RR` реально запускается через `Streamline / sl.dlss_d`;
- `DLSS SR` вместе с `RR` работает;
- `MFG` для чистых тестов `RR` надо держать выключенным;
- самый тяжёлый ранний ghosting уже сильно уменьшен.

Текущая незакрытая проблема:
- при `RR on` остаётся заметный shimmer / ghosting на поверхностях;
- визуально это похоже на “горячий воздух” над полом и стенами;
- артефакт виден даже без `MFG`;
- артефакт слабее в `DLAA`, сильнее в `Quality`, ещё сильнее в `Ultra Performance`.
- отдельные scene-алиасы (`reflect/refract`, `bounce rays`, `caustics`) не убрали артефакт;
- значит проблема не локализуется в одном gameplay cvar и сидит глубже в семантике `RR` inputs / temporal path.

Вывод:
- интеграция уже частично рабочая;
- проблема сейчас не в запуске `RR`, а в качестве входных данных / семантике guides для `RR`.

## Последний подтверждённый статус

На текущем этапе:
- серое `milk`-изображение уже устранено;
- самый тяжёлый ghosting после фикса `mvecScale={1,1}` устранён;
- `RR` снова работает в обычном runtime и больше не фризит мир из-за старого debug-blit пути;
- включение/выключение legacy `denoiser` больше не должно ломать сам RR-пайплайн;
- в меню прямо указано, что legacy `denoiser` игнорируется при активном `RR`;
- остаётся residual shimmer / static ghosting, который:
  - не зависит от `MFG` в чистом тесте (`flt_dlss_mfg 0`);
  - присутствует даже в статике;
  - усиливается при снижении input resolution (`DLAA -> Quality -> Ultra Performance`).

Отдельный активный open issue:
- в зеркалах / сильных отражениях при `RR on` может появляться заметный шум;
- это уже не тот же баг, что ранний `milk` / freeze;
- текущая рабочая гипотеза для него: нехватка или некорректная семантика specular guides в RR input path.

## 2026-03-27 RR Residual Reflection Noise Follow-up

### Update: alias-based scene toggles did not move the artifact

Пользователь прогнал подготовленный `q2rtx_rr_test.cfg` и проверил:
- `rr_refl1`
- `rr_refl2`
- `rr_refl4`
- `rr_refl8`
- `rr_glass0`
- `rr_glass2`
- `rr_caustics0`
- `rr_caustics1`
- `rr_bounce05`
- `rr_bounce1`
- `rr_bounce2`
- `rr_mis0`
- `rr_mis1`

Результат:
- мерцающие точки и residual shimmer в зеркальных/стеклянных отражениях не изменились;
- значит root cause почти точно не в gameplay-level PT cvars (`reflect/refract`, `glass`, `caustics`, `MIS`, `bounce rays`);
- дальнейшая работа должна идти в семантику RR inputs: motion/depth/history/guides.

### Update: RR depth tag corrected to LinearDepth

Новая рабочая гипотеза после alias-тестов:
- оставшиеся speckles и ghosting в движении очень похожи на mismatch между motion и depth semantics;
- в `dlss_rr_prep.comp` RR depth уже готовится как linear camera-space depth;
- но в `slSetTagForFrame` этот буфер раньше всё ещё мог тегаться как обычный `Depth`;
- для `DLSS-RR` это плохая семантика, потому что буфер не является hardware depth.

Исправление:
- в [dlss_sl.cpp](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\dlss_sl.cpp)
  RR depth tag переведён на `sl::kBufferTypeLinearDepth`.

Ожидаемый эффект теста:
- если проблема сидит в depth semantics, RR должен лучше стабилизировать отражения в движении;
- speckles/ghost trails вокруг отражённого игрока должны уменьшиться без грубых новых артефактов.

### Update: GI low clue points to indirect-specular stabilization, not generic RR failure

Новая пользовательская зацепка:
- при `global illumination = low` (`pt_num_bounce_rays = 0.5`) количество мерцающих точек в отражениях падает в несколько раз;
- при этом старые alias-тесты по `reflect/refract`, `caustics`, `glass`, `MIS` почти ничего не меняли.

Практический вывод:
- residual RR artifact сидит не в generic scene toggles, а в indirect/specular contribution path;
- в `Q2RTX` настройка `GI low` не просто “делает меньше лучей”, а фактически убирает problematic full-res indirect specular branch.

Дополнительная найденная проблема в коде:
- при активном `RR` движок раньше принудительно делал:
  - `pt_specular_anti_flicker = 0`
  - `pt_sun_bounce_range = 10000`
- это отключало штатную q2rtx-стабилизацию specular / distant sun bounce как раз в том режиме,
  где пользователь и наблюдает residual sparkles в отражениях.

Новый тестовый шаг:
- добавлен cvar `flt_dlss_rr_specular_stabilizers`
  - `0` = старое “raw RR” поведение
  - `1` = вернуть `pt_specular_anti_flicker` и `pt_sun_bounce_range` под RR
  - `2` = дополнительно вернуть `pt_fake_roughness_threshold` и `pt_ndf_trim` под RR
- в `q2rtx_rr_test.cfg` добавлены алиасы:
  - `rr_rrstab0`
  - `rr_rrstab1`
  - `rr_rrstab2`

### Result: `rr_rrstab2` is the new working RR baseline

Подтверждено пользовательским тестом:
- `rr_rrstab1` уже заметно снижал количество мерцающих точек;
- `rr_rrstab2` убрал residual bright speckles в отражениях практически полностью;
- это подтвердило, что открытая проблема сидела не в generic RR runtime, а в том,
  что при `RR` мы слишком агрессивно выключали штатные q2rtx stabilizers для
  indirect/specular path.

Текущий практический вывод:
- для `RR` новым базовым режимом должен быть `flt_dlss_rr_specular_stabilizers 2`;
- этот режим не переводит GI в `low`, не режет bounce rays и не трогает scene-level cvars;
- он только перестаёт искусственно “разжимать” rough/specular indirect path под RR.

После подтверждения:
- default `flt_dlss_rr_specular_stabilizers` переведён на `2`;
- `rr_base` в `q2rtx_rr_test.cfg` теперь тоже выставляет `flt_dlss_rr_specular_stabilizers 2`.

Симптом после фикса неверной геометрии отражений:
- при `RR on` исчезли явные неправильные отражения / чёрные дыры;
- но на зеркалах и стеклянных отражениях остались мелкие яркие точки / residual shimmer;
- по пользовательскому видео это уже выглядит как mismatch guides, а не как поломка самого RR.

Новая рабочая гипотеза:
- для checkerboard reflect/refract surfaces `RR` получает `resolved` noisy color (`FLAT_COLOR`),
  но auxiliary guides всё ещё были mostly single-branch;
- это даёт уже не крупные артефакты, а остаточный sparkly noise на отражениях.

Что изменено:
- в [dlss_rr_prep.comp](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\shader\dlss_rr_prep.comp)
  для `is_checkerboarded_surface` auxiliary guides теперь частично приводятся к той же логике
  resolve, что и noisy color:
  - `PT_TRANSPARENT`
  - `PT_BASE_COLOR_A`
  - `PT_METALLIC_A`
  - `PT_GODRAYS_THROUGHPUT_DIST`
  - `PT_NORMAL_A`
  - `PT_VIEW_DIRECTION`
- blend делается по тому же cross-neighborhood pattern, что и в
  [checkerboard_interleave.comp](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\shader\checkerboard_interleave.comp),
  но без возврата к старой brightness-эвристике.

Практически:
- это shader-only тест;
- `exe` может иметь тот же hash, что и предыдущий `RR2`, если C/C++ код не менялся;
- проверять надо по свежему runtime shader:
  [dlss_rr_prep.comp.spv](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\shader_vkpt\dlss_rr_prep.comp.spv)

Текущий тестовый запуск:
- exe: [q2rtx_rr3.exe](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx_rr3.exe)
- exe SHA256: `A50B557543A9888AD3D7D331239796391963372928A9717F568F3D0670AC647D`
- shader SHA256: `01CD1AE7F9F0400DE5D04B04058076601A0488010F0F01547181574CA89D2665`

Backup runtime shader before this pass:
- [dlss_rr_prep.before_resolved_guides_20260327_0029.spv](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\shader_vkpt\dlss_rr_prep.before_resolved_guides_20260327_0029.spv)

## Эталонная сборка

Рабочая схема сборки для этой папки:
- исходники: `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src`
- build: `O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off`
- runtime: `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX`
- Vulkan SDK: `O:\Claude2\Vulkan\SDK`
- критичный флаг: `CONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF`

См. также:
- [BUILD_FROM_SCRATCH.md](O:\Claude2\Q2RTX-1.8.1-GPT\BUILD_FROM_SCRATCH.md)
- [BUILD_NOTES.md](O:\Claude2\Q2RTX-1.8.1-GPT\BUILD_NOTES.md)

## Что нужно для RR в runtime

Минимально:
- `q2rtx.exe`
- `sl.interposer.dll`
- `sl.dlss_d.dll`
- `nvngx_dlssd.dll`

Важно:
- для `RR` не нужен никакой DLL patch;
- использовать оригинальный runtime NVIDIA;
- если меняются `shader` или `global_textures.h`, надо синхронизировать весь каталог:
  `Q2RTX-src\baseq2\shader_vkpt -> Q2RTX\baseq2\shader_vkpt`

## Основные файлы интеграции RR

### Логика и orchestration
- [dlss.c](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\dlss.c)
- [main.c](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\main.c)

### Streamline / NGX binding
- [dlss_sl.cpp](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\dlss_sl.cpp)

### Подготовка ресурсов для RR
- [dlss_rr.c](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\dlss_rr.c)
- [dlss_rr_prep.comp](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\shader\dlss_rr_prep.comp)
- [global_textures.h](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\shader\global_textures.h)

### UI / menu / cvars
- [q2rtx.menu](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\baseq2\q2rtx.menu)

## Как RR сейчас встроен в пайплайн

Текущая схема при `RR on`:
1. трассировка primary / indirect / reflect_refract;
2. `ASVGF` denoiser path пропускается;
3. вызывается `vkpt_compositing()` для noisy composited PT color;
4. вызывается `vkpt_interleave()`;
5. вызывается `vkpt_dlss_rr_process()`;
6. далее bloom / tone mapping / final output.

Ключевая идея:
- `RR` должен заменять финальный `TAA/TAAU` resolve;
- внутренний temporal history и jitter в path tracing при этом всё равно остаются нужны;
- вручную выключать `anti-aliasing` в меню для `RR` не требуется.

## Текущие cvars RR / DLSS

### Основные
- `flt_dlss_enable`
- `flt_dlss_rr`
- `flt_dlss_rr_preset`
- `flt_dlss_mode`
- `flt_dlss_preset`
- `flt_dlss_mfg`

### Для чистых тестов RR
- `flt_dlss_mfg 0`
- `pt_roughness_override -1`

Важно:
- в пользовательском [q2config.cfg](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2config.cfg) был найден `pt_roughness_override "0.5"`;
- это искажает поведение отражающих поверхностей;
- для всех RR-тестов нужно сбрасывать его в `-1`.

Готовый тестовый конфиг:
- [q2rtx_test.cfg](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\q2rtx_test.cfg)

## Что уже было исправлено

### 1. RR вообще запускался с ошибками

Ранее были:
- `milk`-экран;
- `slEvaluateFeature failed: 20`;
- missing tags / invalid parameter.

Что помогло:
- корректный `slSetTagForFrame`;
- корректные `DLSS-RR` presets `D/E`;
- lazy create вместо раннего preallocation;
- обязательные `ScalingInputColor` / `MotionVectors` / `Depth` guides;
- правильный `backbuffer` / `swapchain` / `DLSS` state.

### 2. Motion vectors были масштабированы неверно

Критичный фикс:
- `PT_MOTION` в Q2RTX уже в normalized screen space;
- для Streamline надо `mvecScale = {1,1}`;
- старая схема с `1 / renderWidth`, `1 / renderHeight` почти зануляла motion vectors;
- именно это дало самый большой реальный прогресс по ghosting.

Файл:
- [dlss.c](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\dlss.c)

### 3. RR depth был неправильной семантики

Критичный нюанс Q2RTX:
- `PT_VIEW_DEPTH_A` хранит не классический camera-space `Z`, а радиальную дистанцию;
- для reflect/refract path там ещё используется sign hack для внутренних фильтров.

Исправление:
- `RR` depth теперь считается из `PT_SHADING_POSITION` через матрицу камеры `V`;
- то есть в `RR` уходит настоящий camera-space depth.

Файлы:
- [dlss_rr_prep.comp](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\shader\dlss_rr_prep.comp)
- [dlss_sl.cpp](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\dlss_sl.cpp)

### 4. Optional specular guides сначала были отключены, затем возвращены для тестов зеркал

Промежуточное состояние было таким:
- `sl.dlss_d` пробрасывает optional guides в NGX почти без meaningful validation;
- если guide семантически неверен, он может ухудшать картинку вместо помощи;
- поэтому на раннем этапе specular guides были временно отключены и RR гонялся только на mandatory inputs.

Что изменилось позже:
- пользовательский тест с зеркалами показал, что при `RR on` шум в отражении становится заметно сильнее;
- это очень похоже именно на отсутствие корректных specular guides для отражающих поверхностей;
- в `Q2RTX` нужные specular guide textures уже генерируются, но в одном из проходов они не тегались в `Streamline`.

Текущее состояние:
- specular guides снова включены в отдельной тестовой ветке;
- в `slSetTagForFrame` для RR возвращены:
  - `kBufferTypeSpecularHitDistance`
  - `kBufferTypeSpecularRayDirectionHitDistance`
- это нужно проверять именно на зеркалах / сильных отражениях.

Файл:
- [dlss_sl.cpp](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\dlss_sl.cpp)

### 5. RR больше не должен зависеть от legacy denoiser toggle

Проблема:
- пользовательские сравнения показали, что `RR ON + Denoiser OFF` и `RR ON + Denoiser ON` всё ещё давали materially different output;
- это означало, что legacy denoiser продолжает косвенно влиять на RR path, хотя пользовательски так быть не должно.

Что было не так:
- `evaluate_taa_settings()` раньше мог рано выйти, если legacy denoiser выключен, даже когда `RR` активен;
- `prepare_ubo()` воспринимал `flt_enable` только как legacy denoiser switch;
- из-за этого RR prep / checkerboard helper path вёл себя по-разному при `denoiser on/off`.

Исправление:
- введён helper `is_dlss_rr_active_for_frame()`;
- RR теперь удерживает нужный temporal / reconstruction path сам;
- `ubo->flt_enable` теперь эффективно считается как `legacy_denoiser || RR_active_for_frame`;
- в overlay при активном RR legacy denoiser показывается как `replaced`;
- в меню у `denoiser` явно указано, что он игнорируется при включённом `RR`.

Файлы:
- [main.c](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\main.c)
- [q2rtx.menu](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\baseq2\q2rtx.menu)

### 6. RR freeze после quality-экспериментов был связан с debug-post-DLSS blit

Симптом:
- мир визуально “замирал”, но HUD продолжал анимироваться;
- особенно это проявлялось при `RR on` и в некоторых режимах SR.

Реальная причина:
- в пользовательском конфиге оставался включён `r_debug_post_dlss_color 1`;
- финальный blit path продолжал брать standalone DLSS output view, даже когда активен `RR`;
- в результате пользователь видел старый / stale кадр мира и живой HUD поверх него.

Исправление:
- `debug_post_dlss` больше не должен переопределять final blit, когда `RR` активен.

Файл:
- [main.c](O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src\src\refresh\vkpt\main.c)

## Что уже проверено и исключено

Подтверждено тестами:
- проблема не зависит от `MFG`, если тестировать `RR` с `flt_dlss_mfg 0`;
- проблема не зависит от оружия от первого лица;
- проблема почти не меняется от простых cvar-тестов:
  - `pt_reflect_refract`
  - `pt_num_bounce_rays`
  - `pt_caustics`
  - `pt_roughness_override`
- значит это не просто “слишком много specular/refraction в сцене”.

Вывод:
- текущий баг сидит в самой интеграции `RR`, а не в обычных сценовых настройках path tracer.

## Самые важные наблюдения по симптомам

Наблюдения пользователя:
- без `RR` ghosting нет;
- при `RR on` остаётся shimmer даже в статике;
- в движении он усиливается;
- особенно заметен на полу, стенах и дальних поверхностях;
- визуально похож на тепловое марево / дрожание воздуха;
- чем ниже input resolution DLSS, тем сильнее артефакт.

Практический вывод:
- проблема очень похожа на несогласованность mandatory RR guides:
  - `color`
  - `motion`
  - `depth`
  - `normals`
  - `roughness`
- а не на отдельный эффект типа caustics или reflect depth.

## Что делать правильно при дальнейшей доработке

### Если меняется только shader
- не ждать нового `q2rtx.exe`;
- синхронизировать `shader_vkpt`;
- полностью перезапускать игру перед тестом.

### Если меняется C/C++
- пересобрать `client`;
- скопировать свежий `build-claude-off\Bin\q2rtx.exe` в `Q2RTX\q2rtx.exe`;
- убедиться, что timestamp runtime-файла обновился.

### Для чистого теста RR
1. запустить карту;
2. использовать `flt_dlss_enable 1`;
3. использовать `flt_dlss_rr 1`;
4. держать `flt_dlss_mfg 0`;
5. держать `pt_roughness_override -1`;
6. смотреть один и тот же участок пола / стен;
7. сравнивать с `RR off`.

## Текущий открытый вопрос

Главные незакрытые баги:
1. residual shimmer / “heat haze” на поверхностях при `RR on`;
2. шум в зеркалах / сильных отражениях при `RR on`.

Наиболее вероятные дальнейшие направления:
1. проверить семантику `Normals` для RR;
2. проверить, не подаётся ли в RR уже частично legacy-resolved noisy color вместо truly correct raw input;
3. перепроверить флаги и семантику motion/depth для split-path reflect/refract surfaces;
4. отдельно проверить влияние возвращённых specular guides именно на зеркальный шум;
5. только после стабилизации зеркал решать, нужны ли дополнительные guide / prep-правки.

## Короткий changelog

### 2026-03-23
- добавлен `DLSS RR` как отдельная опция;
- добавлены RR presets `D/E`;
- `RR` перестал давать `milk`-экран;
- `RR` перестал падать на `slEvaluateFeature`;
- исправлен `mvecScale` для normalized motion vectors;
- `RR` depth переведён на настоящий camera-space depth;
- optional specular guides временно отключены;
- текущий остаточный баг: shimmer / “heat haze” на поверхностях.

### 2026-03-26
- `RR` возвращён в рабочее состояние после quality-экспериментов;
- устранён freeze мира, вызванный `r_debug_post_dlss_color 1`;
- legacy `denoiser` перестал ломать RR-путь и теперь считается `replaced / ignored` при активном RR;
- в меню текст `denoiser` обновлён, чтобы это было видно пользователю;
- по зеркальным артефактам начата отдельная ветка проверки specular guides;
- для этой ветки собран отдельный кандидат:
  - `Q2RTX\\q2rtx_rr_specfix.exe`
  - SHA256 `4041C774CE8E2036C4E8817314297A0891C851B23E090950B6BEF20B319A7866`
## 2026-03-26 RR Transparency Layer Follow-up
- User-reported mirror/glass noise remained even after returning explicit specular guide tags.
- New hypothesis: for glass / transparent reflective surfaces, RR needs an explicit transparency guide more than it needs only specular-hit guidance.
- Q2RTX already has a separate premultiplied transparent layer in `IMG_PT_TRANSPARENT` (`R16G16B16A16_SFLOAT`) at input resolution.
- New test branch tags this image for RR as:
  - `kBufferTypeTransparencyLayer`
- Files changed in this pass:
  - `Q2RTX-src\\src\\refresh\\vkpt\\dlss.c`
  - `Q2RTX-src\\src\\refresh\\vkpt\\dlss_sl.cpp`
- Fresh short-name test executable:
  - `Q2RTX\\q2rtx_rr_trans.exe`
- Goal of this branch:
  - check whether glass / mirror noise is caused by RR lacking an explicit transparency overlay guide.
## 2026-03-26 RR Transparency Layer Regression And Corrected Follow-up
- The first transparency-layer test was invalid for mirrors/glass quality because RR was receiving a transparency overlay on top of noisy color that already had transparency composited in.
- User test result for that branch: image became much worse, with duplicated / layered artifacts.
- That branch is now considered a failed experiment and should not be used as reference.
- Corrected follow-up implemented:
  - RR noisy color is rebuilt as color-before-transparency inside `dlss_rr_prep.comp`
  - resolved transparency is written separately and tagged as `kBufferTypeTransparencyLayer`
  - this keeps RR inputs semantically aligned with NVIDIA's guide instead of double-counting transparent contribution
- Files changed in corrected branch:
  - `Q2RTX-src\\src\\refresh\\vkpt\\shader\\dlss_rr_prep.comp`
  - `Q2RTX-src\\src\\refresh\\vkpt\\dlss.c`
  - `Q2RTX-src\\src\\refresh\\vkpt\\dlss_sl.cpp`
- Fresh short-name test executable:
  - `Q2RTX\\q2rtx_rr_trans2.exe`
- Runtime shader updated for this branch:
  - `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.comp.spv`
## 2026-03-26 RR Specular Motion Vector Follow-up
- User confirmed that the corrected transparency experiment (`q2rtx_rr_trans2`) still produced duplicated / layered imagery.
- That means the transparency-guided RR branch is still not a good fit in the current Q2RTX pipeline and should not be treated as a quality fix.
- Current next-step hypothesis:
  - for mirrors / strong specular reflections, RR is likely more sensitive to explicit `SpecularMotionVectors` than to the specular-hit-distance fallback path.
- New test branch:
  - keep the regular motion guide
  - additionally tag the resolved RR motion buffer as `kBufferTypeSpecularMotionVectors`
  - drop the explicit specular hit-distance fallback tags from the RR tag list for this branch
- Fresh short-name test executable:
  - `Q2RTX\\q2rtx_rr_smvec.exe`
## 2026-03-26 RR Specular Motion Vector Regression
- User screenshot confirmed that `q2rtx_rr_smvec.exe` is not a viable RR branch.
- Symptom: RR output collapses into a heavily smeared / incorrect image instead of only showing noisy mirrors.
- Practical conclusion:
  - regular resolved motion vectors from Q2RTX cannot simply be re-tagged as `kBufferTypeSpecularMotionVectors`.
  - real specular motion vectors would need dedicated generation with correct reflected-geometry semantics.
- Action taken:
  - rolled back the `SpecularMotionVectors` branch
  - restored RR tagging to the previous explicit specular-hit-guide path
- Restored short-name build:
  - `Q2RTX\\q2rtx_rr_restore.exe`

## 2026-03-26 RR Mirror / Checkerboard Guide Mismatch Follow-up
- User provided fresh comparison screenshots:
  - `Q2RTX\\test\\1_RR-ON.jpg`
  - `Q2RTX\\test\\1_RR-OFF.jpg`
  - `Q2RTX\\test\\2_RR-ON.jpg`
  - `Q2RTX\\test\\2_RR-OFF.jpg`
- These screenshots narrowed the mirror problem much better than the previous videos:
  - RR-on reflections do not merely look noisy;
  - they show broken reflective regions, dark / missing areas, and branch-mismatched imagery;
  - RR-off keeps the same mirrors materially more plausible.
- Strong current root-cause hypothesis:
  - in `dlss_rr_prep.comp`, checkerboard-resolved reflection / refraction surfaces were feeding RR a resolved `FLAT_COLOR` / `FLAT_MOTION` pair,
  - but auxiliary RR guides (`base_color`, `metallic_roughness`, `throughput_dist`, `normal`, `view_dir`, `shading_position_ws`) could still be taken from the *opposite* split branch through the old brightness-based `guide_pos` heuristic.
- That old heuristic came from the legacy TAA path and is a poor fit for RR:
  - RR then sees color / motion from one branch,
  - but normals / roughness / hit-distance / depth context from the other branch,
  - which matches the observed mirror failures much better than a simple “RR is too noisy” explanation.
- Fix applied in `Q2RTX-src\\src\\refresh\\vkpt\\shader\\dlss_rr_prep.comp`:
  - for checkerboard-resolved surfaces, RR still uses resolved `IMG_FLAT_COLOR` and `IMG_FLAT_MOTION`,
  - but all RR guide textures are now sampled strictly from the same `src_pos`,
  - the old “pick brighter side” guide switching was removed for RR.
- Important practical note:
  - this pass is shader-only;
  - no new C/C++ build was required;
  - the runtime shader in `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.comp.spv` was explicitly updated from the rebuilt source shader.
- Runtime shader backup created before deployment:
  - `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.runtime_before_checker_guides_BC9FE2E6.spv`
- Current deployed runtime shader:
  - `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.comp.spv`
  - SHA256 `327B8755E0F3BDF9C2A8082A1B1358323F8E8BF17EB4AC9E67C70F9CD9D968A9`
- Recommended executable for validating this shader pass:
  - `Q2RTX\\q2rtx_rr_restore.exe`
- Validation goal of this branch:
  - keep the previously restored non-broken RR tagging path,
  - but remove the mirror/glass corruption caused by branch-mismatched RR guides on checkerboard reflection surfaces.

## 2026-03-26 RR ColorBeforeTransparency Guide Pass
- After the checkerboard guide-branch fix, user feedback changed in an important way:
  - incorrect / broken reflections were fixed,
  - but mirror / glass reflections with `RR on` still remained noisier than with `RR off`.
- This is a cleaner RR quality problem than the earlier mirror corruption bug.
- New current hypothesis:
  - transparent / glass-heavy reflective surfaces still need an explicit `ColorBeforeTransparency` guide,
  - but **without** reintroducing the previous failed transparency-layer branch that caused duplicated imagery.
- This follows the Streamline RR guide recommendation more closely:
  - start with `ColorBeforeTransparency` if transparent objects are problematic,
  - do not immediately force a separate transparency overlay if it is not strictly needed.
- Implementation in this pass:
  - added a dedicated runtime image `DLSS_RR_COLOR_BEFORE_TRANSPARENCY`
  - `dlss_rr_prep.comp` now reconstructs a pre-transparency guide from the same noisy HDR RR input and the premultiplied `PT_TRANSPARENT` layer
  - tagged the new guide as:
    - `kBufferTypeColorBeforeTransparency`
  - no separate `TransparencyLayer` is tagged in this branch
- Files changed:
  - `Q2RTX-src\\src\\refresh\\vkpt\\shader\\global_textures.h`
  - `Q2RTX-src\\src\\refresh\\vkpt\\shader\\dlss_rr_prep.comp`
  - `Q2RTX-src\\src\\refresh\\vkpt\\dlss_rr.c`
  - `Q2RTX-src\\src\\refresh\\vkpt\\dlss.c`
  - `Q2RTX-src\\src\\refresh\\vkpt\\dlss_sl.cpp`
- Fresh short-name test executable:
  - `Q2RTX\\q2rtx_rr2.exe`
  - SHA256 `A50B557543A9888AD3D7D331239796391963372928A9717F568F3D0670AC647D`
- Fresh runtime shader deployed:
  - `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.comp.spv`
  - SHA256 `FC35C4C944111AE0EF99F7A43E2FD223640595B01C1DEEFADDD3BEE358772EB4`
- Runtime shader backup created:
  - `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.before_cbt_327B8755.spv`
- Validation goal:
  - keep the now-correct mirror geometry / branch alignment,
  - but reduce the residual RR noise on glass / reflective surfaces without reintroducing duplication artifacts.

## 2026-03-27 RR Deployment Fix For ColorBeforeTransparency Branch
- First user test of `q2rtx_rr2.exe` showed a gray / broken RR frame instead of a valid image.
- Root cause was deployment-related, not the RR logic itself:
  - this branch added a new image to `global_textures.h`
  - which changed descriptor/image bindings for many shaders
  - but only `dlss_rr_prep.comp.spv` had initially been copied into the runtime folder
  - so the runtime ended up with a new exe + one new shader + many stale SPIR-V files
- Fix applied:
  - full `Q2RTX-src\\baseq2\\shader_vkpt` runtime sync performed into `Q2RTX\\baseq2\\shader_vkpt`
  - runtime shader folder backed up before the full sync
- Runtime shader folder backup:
  - `Q2RTX\\baseq2\\shader_vkpt_backup_before_fullsync_retry_2026-03-27_00-18-15`
- Practical rule going forward:
  - if a change touches `global_textures.h` or any other shared shader include that affects bindings/layouts,
    the entire runtime `shader_vkpt` folder must be refreshed, not only the directly edited `.spv`.

## 2026-03-27 RR Motion Resolve Follow-up
- User then provided a more precise behavioral clue:
  - residual bright speckles in mirrors / reflective glass appear mainly while the camera moves;
  - in static view the artifacts largely disappear;
  - after stopping, they fade out over time instead of disappearing instantly.
- That pattern strongly suggests a temporal reprojection / motion mismatch rather than a static material-guide mismatch.

- New hypothesis:
  - for checkerboard reflect/refract surfaces, RR already consumes a resolved noisy color (`FLAT_COLOR`);
  - but motion still effectively came from the legacy TAA-oriented resolve path, where motion for `FLAT_MOTION`
    is selected abruptly based on luminance of the opposite branch;
  - this can be acceptable for the old TAA path but can destabilize RR during camera motion and show up as
    sparkly reflection noise that slowly settles when motion stops.

- New shader-only test:
  - in `Q2RTX-src\\src\\refresh\\vkpt\\shader\\dlss_rr_prep.comp`
  - for checkerboarded surfaces RR motion is now resolved more smoothly:
    - center motion comes from `PT_MOTION` at `src_pos`;
    - opposite-side neighbor motion is gathered with the same cross-neighborhood pattern used by checkerboard resolve;
    - final motion is blended using the luminance balance between center and neighbor noisy color contributions.

- This branch intentionally does **not** add new SL buffer types and does **not** change the RR tagging contract.
  It only changes how the existing RR motion input is prepared for checkerboard reflections.

- Current test launch:
  - exe: `Q2RTX\\q2rtx_rr4.exe`
  - exe SHA256: `A50B557543A9888AD3D7D331239796391963372928A9717F568F3D0670AC647D`
  - runtime shader:
    - `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.comp.spv`
    - SHA256 `7180E6316AFC44DCC9F55B988A440210166FAA45247F4E0C43482872141D95CB`

- Runtime shader backup before this pass:
  - `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.before_motion_resolve_20260327_0040.spv`

## 2026-03-27 RR Explicit Specular Motion Vectors
- User then narrowed the remaining RR problem even further:
  - residual color speckles in reflections still remain;
  - ghosting trails are visible around the player reflection in glass;
  - artifacts are strongly motion-dependent and fade out after camera stop.
- This is a much stronger match for reflected-geometry motion semantics than for static RR guides.

- Important engine-side observation:
  - Q2RTX already computes apparent reflected/refracted motion in `reflect_refract.rgen` and stores it into `PT_MOTION`;
  - previous RR experiments only adjusted the generic motion input path used by RR;
  - RR still did **not** receive an explicit `kBufferTypeSpecularMotionVectors` guide.

- New hypothesis for this branch:
  - keep the current RR motion / hit-distance guides intact;
  - add a separate specular-motion guide using the raw reflected motion already written into `PT_MOTION`;
  - this should help RR track moving reflected geometry (especially the player reflection) without replacing the existing fallback specular-hit-distance path.

- Implementation:
  - added runtime image:
    - `DLSS_RR_SPEC_MOTION`
  - `dlss_rr_prep.comp` now writes:
    - regular RR motion to `IMG_DLSS_RR_MOTION`
    - raw reflected motion to `IMG_DLSS_RR_SPEC_MOTION`
  - `dlss_rr.c` adds the required barrier for `VKPT_IMG_DLSS_RR_SPEC_MOTION`
  - `dlss.c` / `dlss_sl.cpp` now pass and tag the new buffer as:
    - `kBufferTypeSpecularMotionVectors`
  - the older specular-hit-distance guides remain enabled in parallel

- Files changed:
  - `Q2RTX-src\\src\\refresh\\vkpt\\shader\\global_textures.h`
  - `Q2RTX-src\\src\\refresh\\vkpt\\shader\\dlss_rr_prep.comp`
  - `Q2RTX-src\\src\\refresh\\vkpt\\dlss_rr.c`
  - `Q2RTX-src\\src\\refresh\\vkpt\\dlss.c`
  - `Q2RTX-src\\src\\refresh\\vkpt\\dlss_sl.cpp`

- Because `global_textures.h` changed again, the full runtime shader folder was refreshed:
  - backup:
    - `Q2RTX\\baseq2\\shader_vkpt_backup_before_specmv_2026-03-27_01-13-00`
  - current runtime RR prep shader:
    - `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.comp.spv`
    - SHA256 `3C7C9BFF6C85B95423C58470B989DB14191D18081E1604621EF49816622D3453`

- Fresh test executable for this branch:
  - `Q2RTX\\q2rtx_rr_specmv.exe`
  - SHA256 `2E40FF57FFC2F6D9E13B35D85061483C9BAA9EE6885FDAF11F422558E2DDF8F8`

- Validation goal:
  - reduce motion-dependent color speckles in mirrors / glass;
  - reduce ghosting trails around reflected player geometry;
  - do so without reintroducing the earlier broken-image / duplicate-image regressions.

## 2026-03-27 RR Specular Motion Without Hit-Distance Fallback
- User feedback on the explicit specular-motion branch:
  - mirror/glass motion artifacts clearly improved;
  - bright speckles became smaller and fewer;
  - but residual motion-dependent noise and ghost trails around reflected player geometry still remained.

- This suggests the explicit `SpecularMotionVectors` path is helping, but RR may still be receiving conflicting specular guidance.
- According to the Streamline RR guide, applications can provide:
  - explicit `kBufferTypeSpecularMotionVectors`
  - **or alternatively**
  - `kBufferTypeSpecularHitDistance` with the required matrices.

- New hypothesis for this branch:
  - now that Q2RTX provides an explicit specular-motion buffer,
    the old specular-hit-distance fallback may no longer be helping and could instead be partially fighting the motion-based history for reflected geometry.

- Test change:
  - keep all existing RR inputs the same;
  - keep the new explicit `SpecularMotionVectors`;
  - stop tagging:
    - `kBufferTypeSpecularHitDistance`
    - `kBufferTypeSpecularRayDirectionHitDistance`
  - this is an executable-only change; runtime shaders remain unchanged from the previous `specmv` branch.

- Fresh test executable:
  - `Q2RTX\\q2rtx_rr_specmv_only.exe`
  - SHA256 `24411DF55907CA5F2795FDEB6F98D93351861328B79A1905597C63E4274A0AA4`

- Runtime shader remains:
  - `Q2RTX\\baseq2\\shader_vkpt\\dlss_rr_prep.comp.spv`
  - SHA256 `3C7C9BFF6C85B95423C58470B989DB14191D18081E1604621EF49816622D3453`

- Validation goal:
  - determine whether the remaining reflection speckles / ghost trails come from a conflict between
    explicit specular motion and the legacy specular-hit-distance fallback, rather than from the motion buffer itself.
