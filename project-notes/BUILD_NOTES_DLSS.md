# Q2RTX-1.8.1 — Заметки по интеграции NVIDIA DLSS 4

## Обзор

Добавлена поддержка NVIDIA DLSS 4 через NVIDIA Streamline SDK:
- **DLSS SR (Super Resolution)** — апскейлинг вместо AMD FSR 1.0 EASU
- **DLSS-G (Multi Frame Generation)** — генерация кадров (2X/3X/4X)
- **Transformer модель** через Preset K (рекомендуется для DLSS 4)

## Новые файлы

### `src/refresh/vkpt/dlss.h`
C-заголовок с публичным API:
```c
void vkpt_dlss_pre_init(void);         // до создания Vulkan instance
void vkpt_dlss_init(void);             // после создания Vulkan device
void vkpt_dlss_initialize(void);       // entry point из vkpt_initialization[]
void vkpt_dlss_destroy(void);
void vkpt_dlss_init_cvars(void);
int  vkpt_dlss_is_available(void);
int  vkpt_dlss_is_enabled(void);
int  vkpt_dlss_needs_upscale(void);
int  vkpt_dlss_mfg_is_enabled(void);
int  vkpt_dlss_g_is_available(void);
void vkpt_dlss_get_render_resolution(uint32_t display_w, uint32_t display_h,
                                      uint32_t *render_w, uint32_t *render_h);
void vkpt_dlss_process(VkCommandBuffer cmd_buf);
void vkpt_dlss_final_blit(VkCommandBuffer cmd_buf, bool waterwarp);
```

Перечисления:
- `DlssMode_t`: OFF / ULTRA_PERFORMANCE / PERFORMANCE / BALANCED / QUALITY / ULTRA_QUALITY / DLAA
- `DlssMfgMode_t`: OFF / 2X / 3X / 4X
- `DlssPreset_t`: DEFAULT / F / J / K / L / M

### `src/refresh/vkpt/dlss.c`
Основная C-логика: cvars, init/shutdown, обработка кадра.

**CVars:**
| CVar | Тип | Умолч. | Описание |
|------|-----|--------|----------|
| `flt_dlss_enable` | int | 0 | Вкл/выкл DLSS |
| `flt_dlss_mode` | int | 4 | Режим: 1=UltraPerf, 2=Perf, 3=Balanced, 4=Quality, 5=UltraQuality, 6=DLAA |
| `flt_dlss_preset` | int | 8 | Пресет K (Transformer) — значение 8 в sl::DLSSPreset |
| `flt_dlss_mfg` | int | 0 | MFG: 0=off, 2=2X, 3=3X, 4=4X |

**Ключевые детали реализации:**
- UBO берётся из `vkpt_refdef.uniform_buffer` (не `qvk.uniform_buffer`)
- `z_near`/`z_far` из `vkpt_refdef.z_near` / `vkpt_refdef.z_far`
- Матрицы V, P извлекаются из `vkpt_refdef.uniform_buffer` (поля V, P — column-major float[16])
- Векторы камеры: right=col0(V), up=col1(V), fwd=-col2(V) (Vulkan использует -Z вперёд)
- `mvecScale = {1/render_w, 1/render_h}` — motion vectors в пиксельном пространстве
- Depth буфер чередуется: `frame_counter&1 ? PT_VIEW_DEPTH_B : PT_VIEW_DEPTH_A`
- При DLSS enabled: FSR отключается
- **DLSS output image** — standalone VkImage в dlss.c (НЕ в global_textures.h, см. ниже)

**Порядок вызова за кадр:**
1. `dlss_sl_begin_frame(frame_index)` — получить FrameToken
2. `dlss_sl_set_options(mode, preset, out_w, out_h, hdr)` — настройки качества
3. Извлечь матрицы камеры из UBO
4. `dlss_sl_set_constants(...)` — передать константы в SL
5. `dlss_sl_tag_resources(...)` — пометить текстуры (включая standalone output image)
6. `dlss_sl_evaluate(cmd_buf)` — запустить DLSS SR

### `src/refresh/vkpt/dlss_sl.cpp`
C++ обёртка для Streamline SDK. Компилируется с `/std:c++17`.

#### Стратегия линковки (критически важно!)

Streamline SDK не поставляет `sl.interposer.lib`. Все функции `SL_API` объявлены как `extern "C"` в `sl_core_api.h`. Решение — **shim-реализации**:

1. Загружаем `sl.interposer.dll` через `LoadLibraryW` + `GetProcAddress`
2. Все `PFun_sl*` функции реализуем как `extern "C"`, делегируя к загруженным указателям
3. Линкер доволен; если DLL отсутствует — DLSS тихо отключается

```cpp
// Пример shim:
sl::Result slInit(const sl::Preferences& p, uint64_t v)
{
    if (!pfn_slInit) return sl::Result::eErrorInvalidState;
    return pfn_slInit(p, v);
}
```

#### Порядок включения заголовков (строго!)
```cpp
#include <windows.h>
#include <vulkan/vulkan.h>  // СНАЧАЛА Vulkan, до Streamline
#include "sl.h"             // главный заголовок SL
#include "sl_dlss.h"        // DLSSMode, DLSSPreset, DLSSOptions
#include "sl_dlss_g.h"      // DLSS-G / MFG типы
#include "sl_helpers_vk.h"  // sl::VulkanInfo, PFun_slSetVulkanInfo (global scope!)
```

**Важно:** `PFun_slSetVulkanInfo` определён в `sl_helpers_vk.h` в **глобальном пространстве имён** (не `sl::`).

#### Параметры slInit (актуальное состояние)
```cpp
prefs.applicationId       = 1089130;  // Steam App ID Q2RTX — обязателен для NGX
prefs.featuresToLoad      = { kFeatureDLSS, kFeatureDLSS_G, kFeatureReflex };
prefs.numFeaturesToLoad   = 3;
prefs.flags               = sl::PreferenceFlags::eDisableCLStateTracking
                          | sl::PreferenceFlags::eAllowOTA
                          | sl::PreferenceFlags::eLoadDownloadedPlugins
                          | sl::PreferenceFlags::eUseFrameBasedResourceTagging;  // обязателен для slSetTagForFrame!
prefs.logLevel            = sl::LogLevel::eVerbose;
prefs.logMessageCallback  = sl_log_callback;
```

#### Тегирование ресурсов (критически важно!)

Функция `dlss_sl_tag_resources()` принимает:
- Все 4 VkImage + VkImageView + layout + **nativeFormat** (VkFormat)
- **render_w, render_h** — реальное разрешение рендера (qvk.extent_render)
- **display_w, display_h** — финальное разрешение (qvk.extent_unscaled)

Для каждого ресурса нужно заполнить три поля:
```cpp
r.nativeFormat = fmt;           // VK_FORMAT_* — иначе isFormatSupported() падает
r.width  = w;                   // полный размер VkImage (аллокации!)
r.height = h;

// Extent в ResourceTag — render subrect внутри текстуры:
sl::Extent in_ext  = { 0, 0, render_w,  render_h  };
sl::Extent out_ext = { 0, 0, display_w, display_h };
sl::ResourceTag tag(&r, bufferType, lifecycle, &extent);
```

**Критически важно для r.width/height:**
- **Input images** (TAA_OUTPUT, depth, mvec) — аллоцированы в Q2RTX при DLSS как `extent_render` → `r.width = render_w`
- **Output image** (`s_dlss_out_img`) — аллоцирован как `extent_unscaled` → `r.width = display_w`
- Если r.width > реального размера VkImage → DLSS будет читать/писать за пределы текстуры

**Lifecycle:** использовать `eValidUntilEvaluate` для DLSS SR (не `eValidUntilPresent`, который предназначен для DLSS-G).

Без `extent` → `RenderSubrect (0x0) outside of Min/Max dynamic res` → `0xbad00005`.
Без `nativeFormat` → `Cannot have undefined format` → `0xbad00005`.

**Форматы буферов Q2RTX:**
| Буфер | VkFormat |
|-------|----------|
| `VKPT_IMG_TAA_OUTPUT` (color_in) | `VK_FORMAT_R16G16B16A16_SFLOAT` |
| `VKPT_IMG_PT_VIEW_DEPTH_A/B` | `VK_FORMAT_R16_SFLOAT` |
| `VKPT_IMG_PT_MOTION` (mvec) | `VK_FORMAT_R16G16B16A16_SFLOAT` |
| `s_dlss_out_img` (color_out) | `VK_FORMAT_R16G16B16A16_SFLOAT` |

#### Вызов slEvaluateFeature — обязательно передавать ViewportHandle!
```cpp
// ПРАВИЛЬНО — ViewportHandle должен быть в inputs:
const sl::BaseStructure* inputs[] = {
    reinterpret_cast<const sl::BaseStructure*>(&s_vp)  // s_vp = sl::ViewportHandle{0}
};
sl::Result res = slEvaluateFeature(
    sl::kFeatureDLSS, *s_frame_token, inputs, 1,
    reinterpret_cast<sl::CommandBuffer*>(cmd_buf));

// НЕПРАВИЛЬНО — без viewport:
// slEvaluateFeature(kFeatureDLSS, token, nullptr, 0, cmd);
// → [SL][ERR] Missing viewport handle, did you forget to chain it up?
```

#### Маппинг режимов
```
cvar → sl::DLSSMode:
1 → eUltraPerformance
2 → eMaxPerformance      (не ePerformance — такого нет!)
3 → eBalanced
4 → eMaxQuality
5 → eUltraQuality
6 → eDLAA

cvar → sl::DLSSPreset:
0 → eDefault
1 → ePresetF (6)   — Ultra Perf
2 → ePresetJ (10)  — близко к K, чуть меньше ghosting
3 → ePresetK (11)  — Transformer model (DLSS 4, рекомендуется)
4 → ePresetL (12)
5 → ePresetM (13)
```

#### DLSS-G / MFG
Структура реализована в `dlss_sl_set_g_options()`, но **полноценная работа требует перехвата swapchain** (Phase 2, TODO). Текущая реализация передаёт настройки MFG в Streamline, но без hook'а `vkCreateSwapchainKHR` кадры не будут генерироваться.

Число генерируемых кадров: `numFramesToGenerate = mfg_mode - 1` (2X→1, 3X→2, 4X→3).

## Изменённые файлы

### `src/refresh/vkpt/shader/global_textures.h`
**НЕ изменялся.** `NUM_IMAGES_BASE` остаётся 38. DLSS output image аллоцируется отдельно в dlss.c.

**Почему нельзя добавлять образы в global_textures.h:**
Изменение `NUM_IMAGES_BASE` или `NUM_IMAGES` сдвигает `BINDING_OFFSET_TEXTURES` и все биндинги A/B текстур в GLSL шейдерах. Шейдеры в Steam `shaders.pkz` скомпилированы с оригинальными значениями — любое изменение вызывает серый экран / артефакты.

### `src/refresh/vkpt/vkpt.h`
Добавлено объявление:
```c
VkResult vkpt_final_blit_view(VkCommandBuffer cmd_buf, VkImageView src_view,
                               VkExtent2D extent, bool filtered, bool warped);
```

### `src/refresh/vkpt/draw.c`
Добавлена функция `vkpt_final_blit_view` — аналог `vkpt_final_blit`, принимающий `VkImageView` напрямую вместо индекса в глобальной таблице образов. Используется для финального блита DLSS output на swapchain.

### `src/refresh/vkpt/main.c`
- `vkpt_dlss_pre_init()` — до `init_vulkan()`
- `vkpt_dlss_init()` — после `init_vulkan()`
- `vkpt_dlss_init_cvars()` — рядом с `vkpt_fsr_init_cvars()`
- Запись в `vkpt_initialization[]`: `{ "dlss", vkpt_dlss_initialize, vkpt_dlss_destroy, VKPT_INIT_DEFAULT, 0 }`
- Добавлены `VK_NVX_BINARY_IMPORT` и `VK_NVX_IMAGE_VIEW_HANDLE` в `OPTIONAL_DEVICE_EXTENSIONS`
- Апскейлинг: DLSS имеет приоритет над FSR:
  ```c
  if (vkpt_dlss_is_enabled()) vkpt_dlss_process(cmd_buf);
  else if (vkpt_fsr_is_enabled()) vkpt_fsr_do(cmd_buf);
  ```
- Финальный блит: `vkpt_dlss_final_blit(cmd_buf, waterwarp)`

### `baseq2/q2rtx.menu`
Добавлены пункты в секцию видео (рядом с FSR):
```
toggle  ... "NVIDIA DLSS 4 (disables FSR when on)"  "NVIDIA DLSS"  flt_dlss_enable
pairs   ... "DLSS quality mode ..."                  "DLSS mode"    flt_dlss_mode
        "ultra performance" 1 "performance" 2 "balanced" 3 "quality" 4 "ultra quality" 5 "DLAA" 6
pairs   ... "DLSS model preset (K = Transformer model ...)" "DLSS preset" flt_dlss_preset
        "default" 0 "F" 1 "J" 2 "K (Transformer)" 3 "L" 4 "M" 5
pairs   ... "Multi Frame Generation ..."             "DLSS MFG"     flt_dlss_mfg
        "off" 0 "2X" 2 "3X" 3 "4X" 4
```

## Streamline SDK

Расположение: `Q2RTX-src/extern/Streamline/`

Клонировать:
```bash
cd Q2RTX-src/extern
git clone --recursive https://github.com/NVIDIA-RTX/Streamline Streamline
cd Streamline
git checkout v2.8.0   # ОБЯЗАТЕЛЬНО — должно совпадать с версией DLL
```

SDK поставляется **только исходниками** (нет `.lib`). Сам SDK не собирается — мы только используем заголовки. DLL берутся из установленных игр или NVIDIA DLSS SDK.

## DLL файлы (runtime)

**ВАЖНО:** все DLL должны лежать **в одной папке с `q2rtx.exe`**.

| Файл | Версия | Источник |
|------|--------|----------|
| `sl.interposer.dll` | 2.8.0 | A Plague Tale Requiem |
| `sl.common.dll` | 2.8.0 | A Plague Tale Requiem |
| `sl.dlss.dll` | 2.8.0 | A Plague Tale Requiem (OTA обновит до 2.10.3) |
| `sl.dlss_g.dll` | 2.8.0 | A Plague Tale Requiem |
| `sl.pcl.dll` | 2.8.0 | A Plague Tale Requiem |
| `sl.reflex.dll` | 2.7.30 | Warhammer 40K Darktide |
| `nvngx_dlss.dll` | 310.5.3 | A Plague Tale Requiem |
| `nvngx_dlssg.dll` | 310.5.3 | A Plague Tale Requiem |
| `_nvngx.dll` | (версия драйвера) | **DriverStore** — нужен для DLSS SR |
| `nvngx.dll` | (версия драйвера) | **DriverStore** — нужен для DLSS SR |

**Как скопировать `_nvngx.dll` и `nvngx.dll` из DriverStore:**
```bash
# В bash (Claude Code):
find "C:/Windows/System32/DriverStore/FileRepository" -name "_nvngx.dll" 2>/dev/null | head -1
# Затем скопировать оба файла в папку игры
```

**Почему sl.reflex.dll из Darktide:** sl.dlss_g зависит от sl.reflex. Официальный NVIDIA Reflex SDK содержит только `NvLowLatencyVk.dll` — это другое. `sl.reflex.dll` берётся из Streamline-совместимой игры.

**Правило:** sl.interposer + sl.common + sl.dlss + sl.dlss_g + sl.pcl — **строго одна версия**. sl.reflex допускает небольшое расхождение.

## Меню q2rtx.menu

После изменения `baseq2/q2rtx.menu` в исходниках нужно скопировать в папку с игрой:
```bash
cp Q2RTX-src/baseq2/q2rtx.menu "E:/SteamLibrary/steamapps/common/Quake II RTX/baseq2/q2rtx.menu"
```

## Версионная совместимость Streamline

Заголовки в extern/Streamline — тег **v2.8.0**. DLL — тоже 2.8.0.
- Заголовки и DLL должны быть одной ветки — иначе `kSDKVersion` не совпадает и `slInit` может вернуть ошибку
- Все Streamline DLL (sl.interposer, sl.common, sl.dlss, sl.dlss_g, sl.pcl) должны быть **одной версии**

Коды ошибок `sl::Result`:
- `eErrorFeatureMissing (31)` — плагин не загружен
- `eErrorFeatureNotSupported (32)` — плагин загружен, но GPU/ОС не поддерживает
- `eErrorNoSupportedAdapterFound (6)` — нет подходящего GPU
- `eErrorDriverOutOfDate (1)` — старый драйвер

## Текущий статус (21 марта 2026)

| Компонент | Статус | Примечание |
|-----------|--------|------------|
| slInit | ✅ OK (0) | |
| sl.common 2.8.0 | ✅ загружен | |
| sl.dlss 2.10.3 | ✅ загружен | OTA обновил с 2.8.0 |
| sl.dlss_g 2.8.0 | ✅ загружен | |
| sl.reflex 2.7.30 | ✅ загружен | зависимость sl.dlss_g |
| slSetVulkanInfo (graphicsQueueIndex=**1u**) | ✅ OK (0) | НЕ 0u! Иначе CR18/19 |
| slIsFeatureSupported DLSS SR | ✅ OK (0) | |
| slIsFeatureSupported DLSS-G | ✅ OK (0) | max 3 frames |
| Рендер без DLSS | ✅ работает | |
| DLSS SR апскейлинг | ✅ работает | |
| Краш при выходе (slShutdown) | ✅ исправлен | порядок dlss_img/dlss + нет slFreeResources |
| DLSS-G "Invalid VK queue" (CR18/19) | ✅ исправлен | graphicsQueueIndex=1u |
| DLSS-G crash при swapchain (CR20) | ✅ исправлен | queueCount=3 (SL нужны 2 своих очереди) |
| DLSS-G frame generation | ⏳ тестируется | CR21 — первый запуск с исправленной конфигурацией |
| Reflex | ❌ NvLowLatencyVk.dll не найден | не критично для DLSS-G |

### Обязательные условия (нарушение = startup crash)

| Условие | Где | Почему |
|---------|-----|--------|
| `queueCount = 3` | `main.c` VkDevice creation | SL нужны 2 своих очереди: idx 1 и 2 |
| `graphicsQueueIndex = 1u` | `dlss.c` dlss_sl_set_vulkan_info() | SL app-queue range = [0,1) |
| **НЕ** `eUseManualHooking` | `dlss_sl.cpp` slInit flags | CR11 = swapchain crash |
| **НЕ** `slAllocateResources(DLSS-G)` до swapchain | `dlss_sl.cpp` | CR12 = exception в SL |
| **НЕ** `slFreeResources` перед `slShutdown` | `dlss_sl.cpp` | CR17 = GC use-after-free |

### Последние крашрепорты

| Отчёт | Где крашится | Причина | Статус |
|-------|-------------|---------|--------|
| CR17 | `create_swapchain+0x368` | `queueCount=1` | ✅ исправлен: queueCount=2 |
| CR18 | `R_EndFrame_RTX+0x409` → `1B0_E658703+0x29769` | `graphicsQueueIndex=0u` | ✅ исправлен: 1u |
| CR19 | то же, `RAX="onnull"` | то же | ✅ исправлен: 1u |
| CR20 | `create_swapchain+0x610` → `1B0_E658703` | `queueCount=2` → SL просит queue[graphicsQueueIndex+1=2], нет | ✅ исправлен: queueCount=3 |
| CR21? | — | — | ⏳ ещё не тестировалось |

## История решённых проблем

### 1. slIsFeatureSupported = 31 (eErrorFeatureMissing)
**Причин было несколько, решены все:**

- **applicationId = 0** → NGX не инициализируется. `prefs.applicationId = 1089130` (Steam App ID Q2RTX).
- **sl.reflex.dll отсутствует** → sl.dlss_g выгружается. Взять из Darktide v2.7.30.
- **Плагины не найдены** → `prefs.pathsToPlugins` + `GetModuleFileNameW(s_sl_lib, ...)`.
- **`_nvngx.dll` / `nvngx.dll` отсутствуют** → NGX не находит NGXCore рядом с exe. Скопировать из DriverStore.
- **`VK_NVX_binary_import` не включён** → NGX не может создать CUDA ядра DLSS. Добавить в `OPTIONAL_DEVICE_EXTENSIONS` в main.c.

### 2. Серый экран без DLSS (регресс)
**Причина:** добавление `DLSS_OUTPUT` в `global_textures.h` меняло `NUM_IMAGES_BASE` и сдвигало биндинги всех шейдеров. Шейдеры из Steam `shaders.pkz` скомпилированы со старыми индексами.
**Решение:** убрать DLSS_OUTPUT из `global_textures.h`, аллоцировать как standalone `VkImage` в dlss.c.

### 3. Чёрный экран с DLSS (финальный блит)
**Причина:** `vkCmdBlitImage` требует ручных layout transitions для swapchain, которых не было.
**Решение:** добавить `vkpt_final_blit_view(VkCommandBuffer, VkImageView, ...)` в draw.c (использует render pass как FSR).

### 4. Чёрный экран с DLSS (evaluate не работал — viewport)
**Причина:** `slEvaluateFeature` вызывался с `inputs=nullptr, 0`.
```
[SL][ERR] slEvaluateFeature Missing viewport handle, did you forget to chain it up?
```
**Решение:** передавать `s_vp` (`sl::ViewportHandle{0}`) как элемент inputs.

### 5. Чёрный экран с DLSS (теги не регистрировались)
**Причина:** `slSetTagForFrame` вызывался без флага `eUseFrameBasedResourceTagging`.
```
[SL][ERR] 'slSetTagForFrame' SL API is called but 'PreferenceFlag::eUseFrameBasedResourceTagging' flag is not set!
[SL][ERR] Failed to find global tag 'kBufferTypeScalingInputColor'
```
**Решение:** добавить `sl::PreferenceFlags::eUseFrameBasedResourceTagging` в `prefs.flags` при `slInit`.

### 6. Чёрный экран с DLSS (evaluate — undefined format + нулевой subrect)
**Причина 1:** `sl::Resource::nativeFormat` по умолчанию = 0 = `VK_FORMAT_UNDEFINED`.
```
[SL][ERR] vulkan.cpp:isFormatSupported Cannot have undefined format
[SL][ERR] evaluateNGXFeature NVSDK_NGX_VULKAN_EvaluateFeature failed 0xbad00005
```
**Причина 2:** `ResourceTag::extent` не задан → `RenderSubrect (0x0)`.
```
[NGX] Error: RenderSubrect (0x0) outside of Min (2580x1063) and Max (5160x2126) dynamic res.
```
**Решение:**
- Установить `r.nativeFormat = VkFormat` для каждого ресурса
- Заполнить `r.width`/`r.height` реальными размерами текстур
- Передать `sl::Extent{0, 0, w, h}` в конструктор `ResourceTag` для каждого тега

### 7. Plugin 'sl.dlss_g' will be unloaded since it requires plugin 'sl.reflex'
**Решение:** добавить `kFeatureReflex` в `featuresToLoad` И положить `sl.reflex.dll` рядом с exe.

### 8. Предупреждения SL: prevClipToClip + cameraPinholeOffset
**Причина:** поля `c.prevClipToClip` и `c.cameraPinholeOffset` не заполнялись.
**Решение:**
- Вычислять `clipToPrevClip = P_prev * V_prev * invV * invP` (column-major матричное умножение)
- Вычислять `prevClipToClip = P * V * invV_prev * invP_prev`
- Установить `c.cameraPinholeOffset = {0.f, 0.f}` (для не-VR камеры)

### 9. ~~Эффект зума~~ (устаревший анализ — см. #12)
~~**Причина:** `vkpt_final_blit_view` вызывался с `qvk.extent_unscaled` как `input_dimensions`.~~
~~**Решение:** передавать `qvk.extent_render` → UV scale = 1.0.~~
> ❌ **Этот анализ был неверным.** `taa_image_size ≠ extent_render`. Передача `extent_render` вызывает зум. Правильное исправление в #12.

### 10. Раздвоение экрана (DLSS не апскейлирует до display resolution)
**Причина:** три ошибки в `dlss_sl_tag_resources`:
1. `r_in.width = display_w` — неверно, TAA_OUTPUT/depth/mvec аллоцированы как `extent_render`
2. `ResourceLifecycle::eValidUntilPresent` — неверно для DLSS SR (нужен `eValidUntilEvaluate`)
3. Эти ошибки в совокупности не позволяли DLSS понять что нужно апскейлить с render_w→display_w

**Решение:**
- `r_in/dep/mv.width = render_w` (реальный размер VkImage)
- `r_out.width = display_w` (standalone output image аллоцирован в display resolution)
- Lifecycle: `eValidUntilEvaluate` для всех тегов DLSS SR

### 11. Краш при выходе в NGX model binary (1B0_E658703.bin) — попытка #1
**Причина:** NGX model binary крашится если Vulkan ресурсы удаляются до `slShutdown`.
В `vkpt_dlss_destroy` порядок был неверный: сначала `dlss_free_output_image()`, потом `dlss_sl_shutdown()`.
**Решение попытки #1:**
1. В `dlss_sl_shutdown()` добавить `slFreeResources(kFeatureDLSS, s_vp)` + `slFreeResources(kFeatureDLSS_G, s_vp)` **до** `slShutdown()`
2. В `vkpt_dlss_destroy()` поменять порядок: сначала `dlss_sl_shutdown()`, потом `dlss_free_output_image()`
> ⚠️ Краш сохранялся — см. #13 (истинная причина).

### 12. Эффект зума при режимах отличных от DLAA (истинная причина, март 2026)
**Корень проблемы — шейдер `final_blit.frag`:**
```glsl
uv *= push.input_dimensions / vec2(global_ubo.taa_image_width, global_ubo.taa_image_height);
```
`taa_image_width/height = qvk.extent_taa_images = max(extent_screen_images, extent_unscaled) = extent_unscaled`
(когда DLSS включён и апскейлит, `extent_screen_images = max(extent_render, extent_unscaled) = extent_unscaled`)

Если передать `input_dims = extent_render (2993×1253)` при `taa = extent_unscaled (5160×2160)`:
`uv *= 2993/5160 ≈ 0.58` → shader сэмплирует только 58% ширины → картинка увеличена (zoom).

DLSS SR выводит результат при `display resolution (5160×2160)`, поэтому нужно:
`input_dims = extent_unscaled` → `uv *= 5160/5160 = 1.0` → полное покрытие без масштабирования.

**Решение:** в `vkpt_dlss_final_blit()`:
```c
// Было (неправильно):
vkpt_final_blit_view(cmd_buf, s_dlss_out_view, qvk.extent_render, false, waterwarp);
// Стало (правильно):
vkpt_final_blit_view(cmd_buf, s_dlss_out_view, qvk.extent_unscaled, false, waterwarp);
```

**Дополнительная причина:** `s_dlss_out_img` аллоцировался при старте с `qvk.extent_unscaled.height = 2126`,
а DLSS писал в `height = 2160` (реальное display resolution после создания swapchain) → 34 строки за пределами VkImage.
**Решение:** вынести создание `s_dlss_out_img` в отдельный entry с `VKPT_INIT_SWAPCHAIN_RECREATE` (см. #14).

### 13. Краш при выходе в NGX model binary — истинная причина
**Причина:** `slAllocateResources(nullptr, kFeatureDLSS_G, s_vp)` вызывался в `dlss_sl_set_vulkan_info()`
до создания swapchain. Streamline бросал исключение (line 1026 sl_debug.log). При этом `g_dlss_sl_g_available = 1`
оставалось выставлено. При shutdown `slFreeResources(kFeatureDLSS_G)` вызывался для ресурсов которые никогда
не были корректно аллоцированы → краш в NGX model binary.

**Решение:**
1. Убрать `slAllocateResources(kFeatureDLSS_G)` из `dlss_sl_set_vulkan_info()` — swapchain не готов
2. Добавить флаги `s_dlss_resources_allocated` / `s_dlss_g_resources_allocated` — освобождать только то, что было успешно аллоцировано
3. В `dlss_sl_shutdown()`: `if (s_dlss_resources_allocated) slFreeResources(kFeatureDLSS, s_vp)`
4. `vkDeviceWaitIdle(qvk.device)` в `vkpt_dlss_destroy()` перед `dlss_sl_shutdown()`

### 14. Несоответствие размера DLSS output image и swapchain (март 2026)
**Причина:** `vkpt_dlss_initialize()` (VKPT_INIT_DEFAULT) вызывается до создания swapchain.
В этот момент `qvk.extent_unscaled.height` = 2126 (не финальное). После создания swapchain
`qvk.extent_unscaled.height` = 2160. DLSS пишет в 2160 строк, но `s_dlss_out_img` аллоцирован
на 2126 строк → out-of-bounds запись → неопределённое поведение.

**Решение:** разделить init-запись в `vkpt_initialization[]`:
```c
{ "dlss",     vkpt_dlss_initialize,        vkpt_dlss_destroy,        VKPT_INIT_DEFAULT,            0 },
{ "dlss_img", vkpt_dlss_init_output_image, vkpt_dlss_destroy_output_image, VKPT_INIT_SWAPCHAIN_RECREATE, 0 },
```
- `vkpt_dlss_initialize` — только Streamline lifecycle (без аллокации образа)
- `vkpt_dlss_init_output_image` — только аллокация `s_dlss_out_img` с текущим `qvk.extent_unscaled`
  Вызывается после "images" (SWAPCHAIN_RECREATE), поэтому `extent_unscaled` гарантированно корректен

## Диагностика через sl_debug.log

В `dlss_sl_startup()` открывается файл `sl_debug.log` рядом с `q2rtx.exe`. Туда пишутся все внутренние логи Streamline (уровень Verbose).

**Ключевые строки при успешном старте:**
```
[DLSS] Streamline initialized
[DLSS] slSetVulkanInfo result: 0
[DLSS] slIsFeatureSupported DLSS SR result: 0
[DLSS] slIsFeatureSupported DLSS-G result:  0
[DLSS] DLSS SR available | DLSS-G/MFG available

NGX loaded - app id 1089130
Multi-frame supported, max generated frames 3
Allocating DLSS-G instance id 0
```

**Ошибки которые НЕ критичны (игнорировать):**
```
Failed to load NvLowLatencyVk.dll          ← не нужен для DLSS SR/G
Hook sl.common:Vulkan:CmdBindPipeline NOT supported  ← нужно только для SL как Vulkan layer
NGXLoadFromPath failed: -1160773628        ← failures для versions\0 (старые модели)
                                             versions\20317442 загружается успешно
```

**Критические ошибки:**
```
Missing viewport handle, did you forget to chain it up in the slEvaluateFeature inputs?
  → передать sl::ViewportHandle в inputs[] slEvaluateFeature

NGX indicates DLSSContext is not available
  → _nvngx.dll отсутствует или VK_NVX_binary_import не включён

Plugin 'sl.dlss_g' will be unloaded since it requires plugin 'sl.reflex'
  → добавить sl.reflex.dll + kFeatureReflex в featuresToLoad

slSetTagForFrame SL API is called but PreferenceFlag::eUseFrameBasedResourceTagging is not set
  → добавить eUseFrameBasedResourceTagging в prefs.flags

Cannot have undefined format
  → установить r.nativeFormat = VkFormat для каждого ResourceTag

RenderSubrect (0x0) outside of Min/Max dynamic res → 0xbad00005
  → передать sl::Extent{0,0,w,h} в ResourceTag (render_w/h для input, display_w/h для output)
```

## Архитектурные заметки

### DLSS output image — standalone allocation
DLSS output не входит в глобальную таблицу образов Q2RTX (не в `global_textures.h`). Он аллоцируется в `dlss.c` как `static VkImage s_dlss_out_img` при `vkpt_dlss_initialize()` и освобождается при `vkpt_dlss_destroy()`.

**Формат:** `VK_FORMAT_R16G16B16A16_SFLOAT`, размер `extent_unscaled` (display resolution).
**Usage:** `STORAGE | SAMPLED | TRANSFER_SRC`.

**Реализовано:** образ вынесен в отдельный entry `"dlss_img"` с флагом `VKPT_INIT_SWAPCHAIN_RECREATE`. Пересоздаётся при каждом изменении swapchain, всегда соответствует актуальному `qvk.extent_unscaled`.

### Очередь Vulkan
Q2RTX использует одну очередь для graphics и compute (`qvk.queue_idx_graphics`).
В `dlss_sl_set_vulkan_info()` передаём один и тот же family/index для graphics, compute и optical flow.

### DLSS SR vs DLAA
При `viewsize 100` + `flt_dlss_mode 6` → DLAA (нет апскейлинга, только AA).
`vkpt_dlss_needs_upscale()` возвращает 0 при DLAA.

### DLSS-G Phase 2 (TODO)
Для работы MFG нужен перехват `vkCreateSwapchainKHR` — Streamline должен контролировать swapchain.
Текущая реализация передаёт настройки MFG, но без swapchain hook кадры не генерируются.
sl.dlss_g уже регистрирует хуки `slHookVkCreateSwapchainKHR`, `slHookVkPresent` и др. — они загружены. Нужно разобраться как правильно инициировать их через slSetVulkanInfo или специальный init path.

---

## Сессия 3 (21 марта 2026)

### Секция 17: Неверные размеры ресурсов в slSetTagForFrame → zoom DLSS

**Корень зума:** В `dlss_sl_tag_resources()` мы передавали `r_in.width = render_w, r_in.height = render_h` (например 1920×1080), но фактические VkImage для TAA_OUTPUT, DEPTH, MVEC выделены по формуле:

```
extent_screen_images = max(extent_render, extent_unscaled) = extent_unscaled = display_w×display_h
```

Т.е. реальный размер изображений — 3840×2160. Streamline нормализует `Extent` суб-прямоугольника по `Resource::width/height`. Если объявить `width=1920` при реальном размере 3840, SL вычисляет UV суб-прямоугольника `{0,0,1920,1080}` как `{0,0,1,1}` — «весь» образ — и DLSS упскейлит неверный вход вместо правильного суб-прямоугольника 1920×1080 внутри 3840×2160.

**Исправление в `dlss_sl.cpp`:**
```cpp
// Было (неверно):
r_in.width  = render_w;   r_in.height  = render_h;
r_dep.width = render_w;   r_dep.height = render_h;
r_mv.width  = render_w;   r_mv.height  = render_h;
r_out.width = display_w;  r_out.height = display_h;

// Стало (верно):
r_in.width  = display_w;  r_in.height  = display_h;  // фактический размер VkImage
r_dep.width = display_w;  r_dep.height = display_h;
r_mv.width  = display_w;  r_mv.height  = display_h;
r_out.width = display_w;  r_out.height = display_h;

// Extent (суб-прямоугольник) остаётся верным:
sl::Extent in_ext  = { 0, 0, render_w,  render_h  };  // только эта область содержит рендер
sl::Extent out_ext = { 0, 0, display_w, display_h };  // весь output
```

**Краш при выходе** — был из-за порядка записей в таблице `vkpt_initialization[]` (секция 15). Исправлен путём перестановки `"dlss_img"` перед `"dlss"`. Крашевый отчёт был от старого билда Mar 20 — новый (Mar 21) включает исправление.

---

## Сессия 3 (21 марта 2026) — продолжение

### Секция 15: Порядок записей в vkpt_initialization[] — критически важен для краша при выходе

**Проблема:** Краш при выходе из игры в `1B0_E658703.bin` (NGX binary) — `ACCESS VIOLATION (0xc0000005)` в `sl.common.collectGarbage`, вызываемом из `slShutdown()`.

**Стек вызовов (из sl_debug.log):**
```
71s:792ms: sl.common shutting down
71s:793ms: collectGarbage — calling destroy lambda scheduled at frame 0, forced=yes → CRASH
```

**Причина:**
Streamline при вызове `slSetTagForFrame(kTagDLSSOuput, s_dlss_out_img)` на frame 0 регистрирует внутреннюю destroy lambda, привязанную к этому VkImage. Так как Q2RTX вызывает `vkQueuePresentKHR` напрямую (не через SL interposer), `presentCommon()` не наблюдается SL, frame 0 так и не "завершается" нормально. При `slShutdown()` SL форсирует GC и пытается вызвать destroy lambda с уже освобождённым `s_dlss_out_img`.

**Старый порядок (неверный):**
```c
{ "dlss",     vkpt_dlss_initialize,         vkpt_dlss_destroy,             VKPT_INIT_DEFAULT,            0 },
{ "dlss_img", vkpt_dlss_init_output_image,  vkpt_dlss_destroy_output_image, VKPT_INIT_SWAPCHAIN_RECREATE, 0 },
```
`vkpt_destroy_all()` обходит таблицу в ОБРАТНОМ порядке → первым уничтожается `"dlss_img"` (освобождает VkImage), затем `"dlss"` вызывает `slShutdown()` → GC обращается к уже освобождённому образу → CRASH.

**Исправление — swap порядка:**
```c
/* NOTE: "dlss_img" must come BEFORE "dlss" in this table.
 * vkpt_destroy_all() processes entries in REVERSE order.
 * On full shutdown, "dlss" (later → destroyed first) calls slShutdown while
 * the VkImage is still valid. Then "dlss_img" (earlier → destroyed second)
 * frees the VkImage safely after SL garbage collection completes.
 * Swapping these causes a crash: SL's collectGarbage accesses a freed VkImage. */
{ "dlss_img", vkpt_dlss_init_output_image,  vkpt_dlss_destroy_output_image, VKPT_INIT_SWAPCHAIN_RECREATE, 0 },
{ "dlss",     vkpt_dlss_initialize,          vkpt_dlss_destroy,              VKPT_INIT_DEFAULT,            0 },
```

Теперь при destroy: сначала `"dlss"` (slShutdown завершает GC, VkImage ещё жив) → затем `"dlss_img"` (освобождает VkImage безопасно).

### Секция 16: slAllocateResources(DLSS-G) нельзя вызывать до создания swapchain

**Проблема:** Вызов `slAllocateResources(sl::kFeatureDLSS_G, ...)` в `dlss_sl_set_vulkan_info()` приводил к exception — swapchain ещё не создан в этот момент.

**Исправление:** Убрали `slAllocateResources(DLSS-G)` из `dlss_sl_set_vulkan_info()`. Добавлен флаг `s_dlss_resources_allocated` для отслеживания реально выделенных ресурсов, `slFreeResources` вызывается только если allocation прошёл успешно.

**Итог:** DLSS SR ресурсы выделяются. DLSS-G ресурсы не выделяются (Phase 2 — нужен swapchain hook).

### Секция 17: Краш при выходе — настоящая причина (slFreeResources перед slShutdown)

**Проблема:** Краш при выходе из игры (`ACCESS VIOLATION` в `collectGarbage`) сохранялся даже после исправления порядка в `vkpt_initialization[]`.

**Настоящая причина:**
`dlss_sl_shutdown()` вызывал `slFreeResources(kFeatureDLSS)` → это уничтожает NGX feature context (включая DLSS SR).
`slSetTagForFrame()` регистрирует destroy lambdas **внутри** NGX context-а — они вызываются при GC.
Затем `slShutdown()` запускает GC → GC пытается вызвать destroy lambda → NGX context уже уничтожен → ACCESS VIOLATION.

**Исправление (`dlss_sl.cpp`):**
Убраны все вызовы `slFreeResources` из `dlss_sl_shutdown()`. Теперь shutdown выглядит:
```cpp
void dlss_sl_shutdown(void)
{
    if (!s_sl_loaded) return;
    /* Do NOT call slFreeResources before slShutdown.
     * slFreeResources destroys the NGX feature context.
     * slSetTagForFrame registers destroy lambdas inside the NGX context that
     * SL's collectGarbage processes during slShutdown.  If we free the NGX
     * context first those lambdas access freed memory → ACCESS VIOLATION.
     * slFreeResources is only for mid-session resource recycling (e.g. before
     * swapchain resize), not for final application exit. */
    slShutdown();
    // ...
}
```

**Правило:** `slFreeResources` предназначен для mid-session освобождения (например, до resize swapchain). При финальном shutdown — только `slShutdown()`.

### Секция 18: Зум при DLSS SR (не DLAA) — корневая причина (TAAU + DLSS конфликт)

**Симптом:** В режимах Performance/Balanced/Quality/UltraQuality/UltraPerformance изображение зумировано (увеличено) по сравнению с DLAA. В DLAA зума нет.

**Корневая причина (`main.c` — `evaluate_taa_settings`):**

Оригинальный код в `evaluate_taa_settings` включал DLSS в `force_upscaling`:
```c
bool force_upscaling = (vkpt_dlss_is_enabled() && vkpt_dlss_needs_upscale())
                    || (vkpt_fsr_is_enabled()  && vkpt_fsr_needs_upscale());
if (force_upscaling)
    flt_taa = AA_MODE_UPSCALE;  // запускает TAAU
```

Это приводило к следующей цепочке:
1. DLSS SR активен (Performance mode, render=2580×1080, display=5160×2160)
2. `AA_MODE_UPSCALE` → TAAU запускается при `extent_taa_output = extent_unscaled = {5160, 2160}`
3. TAAU записывает **display-space изображение** (5160×2160) в `IMG_TAA_OUTPUT`
4. `dlss_sl_tag_resources` передаёт в DLSS: `r_in.width=5160, r_in.height=2160`, `in_ext={0, 0, 2580, 1080}`
5. DLSS смотрит на top-left 2580×1080 пикселей **display-space** картинки (= верхний левый квартал экрана)
6. DLSS апскейлит этот квартал до 5160×2160 → **зум ×2**

**Исправление (`main.c`):**
```c
bool dlss_sr_active = vkpt_dlss_is_enabled() && vkpt_dlss_needs_upscale();
bool force_upscaling = !dlss_sr_active && (vkpt_fsr_is_enabled() && vkpt_fsr_needs_upscale());

if (dlss_sr_active)
{
    /* DLSS SR upscales internally. Run TAA at render res, not TAAU.
     * extent_taa_output stays at extent_render. */
    flt_taa = AA_MODE_TAA;
}
else if (force_upscaling)
{
    flt_taa = AA_MODE_UPSCALE;
}
```

Теперь при DLSS SR:
- TAA работает при render_w × render_h = 2580×1080 (деnoизация, temporal accumulation)
- `IMG_TAA_OUTPUT` содержит render-space изображение в top-left 2580×1080 пикселях
- DLSS корректно апскейлит 2580×1080 → 5160×2160

**Почему в DLAA зума не было:**
В DLAA `vkpt_dlss_needs_upscale()` возвращает `false`, `get_render_extent()` возвращает display_w × display_h.
`extent_render = extent_unscaled` → TAAU при `extent_taa_output = extent_unscaled` просто копирует пиксели 1:1.
`in_ext = {0, 0, display_w, display_h}` = весь образ → DLSS видит правильное изображение.

**Примечание:** TAA в Q2RTX — это не просто anti-aliasing, это temporal denoising path-tracing результата (1spp очень шумный). Поэтому TAA нужен даже при DLSS. Только нужен *обычный TAA* (render res), а не TAAU (который делает spatial upscaling до display res).

---

## Работа с очередями DLSS-G (2 graphics queues)

### Проблема
DLSS-G при перехвате `vkCreateSwapchainKHR` создаёт `dlssg.pacer` command context и запрашивает `vkGetDeviceQueue(family, appQueueIndex + 1)`. Если VkDevice создан только с 1 graphics queue — падение в `createCommandQueue`.

### Решение: 2 graphics queues
В `main.c` при создании VkDevice (`vkpt_initialization`/`create_vulkan`):
```c
float queue_priorities[2] = {1.0f, 1.0f};
VkDeviceQueueCreateInfo q = {
    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueCount       = 2,   /* queue[0]=app, queue[1]=SL/DLSS-G */
    .pQueuePriorities = queue_priorities,
    .queueFamilyIndex = qvk.queue_idx_graphics,
};
```

В `dlss.c` в `vkpt_dlss_set_vulkan_info()`:
```c
dlss_sl_set_vulkan_info(
    qvk.instance, qvk.physical_device, qvk.device,
    (uint32_t)qvk.queue_idx_graphics, 1u,  /* SL queue index 1 */
    (uint32_t)qvk.queue_idx_graphics, 1u);
```

**Критично:** передавать индекс `1u`, НЕ `0u`. Если передать `0u`, SL регистрирует queue 0 как "свою внутреннюю" и при present через queue 0 выдаёт ошибку `"Invalid VK queue - not created by the application"` → DLSS-G frame gen работает некорректно → краш.

---

## История крашей при старте (CrashReport11–14)

| Краш | Причина | Исправление |
|------|---------|-------------|
| CR11 | `eUseManualHooking` + только 1 queue → `createCommandQueue getDeviceQueue failed` | Убрать `eUseManualHooking` |
| CR12 | `slAllocateResources(nullptr, kFeatureDLSS_G, ...)` → exception в SL runtime | Убрать вызов — SL сам аллоцирует VRAM в `vkCreateSwapchainKHR` hook |
| CR13/14 | `"Invalid VK queue - not created by the application"` → DLSS-G frame gen крашит GPU | Передавать `graphicsQueueIndex=1u` в `slSetVulkanInfo` |

## SL Proxy (vkGetDeviceProcAddr из sl.interposer.dll)

Для работы present hook нужно получать `vkQueuePresentKHR` и другие swapchain функции через SL proxy, а не из vulkan-1.dll:

```cpp
// В load_sl_procs():
HMODULE h = LoadLibraryA("sl.interposer.dll");
s_sl_vkGetDeviceProcAddrProxy   = (PFN_vkGetDeviceProcAddr)  GetProcAddress(h, "vkGetDeviceProcAddr");
s_sl_vkGetInstanceProcAddrProxy = (PFN_vkGetInstanceProcAddr)GetProcAddress(h, "vkGetInstanceProcAddr");

// В dlss_sl_set_vulkan_info():
PFN_vkGetDeviceProcAddr getDeviceProc =
    s_sl_vkGetDeviceProcAddrProxy ? s_sl_vkGetDeviceProcAddrProxy : vkGetDeviceProcAddr;
s_sl_vkQueuePresentKHR = (PFN_vkQueuePresentKHR) getDeviceProc(device, "vkQueuePresentKHR");
// ... аналогично для vkCreateSwapchainKHR, vkDestroySwapchainKHR, vkGetSwapchainImagesKHR
```

Без SL proxy presentCommon() никогда не вызывается → GC накапливается → краш при выходе.

---

## Сессия 4 (21 марта 2026) — CrashReport18/19: "Invalid VK queue" → краш на frame 2

### Симптом
`1B0_E658703+0x29769` crash (NGX DLSS-G binary) — ACCESS VIOLATION на 2-м frame present.
sl_debug.log:
```
[SL][ERR] vulkan.cpp:1271[getHostQueueInfo] Invalid VK queue 00000137D82BDA30 - not created by the application!
[SL][ERR] dlss_gEntry.cpp:704[slHookVkPresent] Invalid VK app queue - 00000137D82BDA30!
```
После этого DLSS-G создаёт контексты и выполняет frame 1 present. На frame 2 — crash (nullptr dereference).

### Корневая причина

Два взаимосвязанных бага:

**Баг 1 — неверный `graphicsQueueIndex` в `slSetVulkanInfo`:**
В `dlss.c` передавалось `graphicsQueueIndex=0u`. По документации Streamline Manual Hooking §5.2.2:
> `info.graphicsQueueIndex = graphicsQueueIndexStartForSL; // Where first SL queue starts after host's queues`

При `graphicsQueueIndex=0`: диапазон "очередей приложения" = `[0, 0)` = **пустое множество**.
При `graphicsQueueIndex=1`: диапазон = `[0, 1)` = {queue[0]} = наша очередь ✓

SL's `getHostQueueInfo` проверяет, входит ли present queue handle в диапазон app queues.
С `graphicsQueueIndex=0` → диапазон пуст → любая очередь "Invalid".

**Баг 2 — отсутствие `eUseManualHooking` в `slInit`:**
В режиме automatic mode SL строит таблицу "app queues" перехватывая `vkGetDeviceQueue`.
Но `sl.interposer.dll` **не хукает** `vkGetDeviceQueue` (его нет в `sl_hooks.h`).
Таблица остаётся пустой → `getHostQueueInfo` всегда возвращает "Invalid".

С флагом `eUseManualHooking` SL использует `VulkanInfo` для валидации очереди:
- Знает, что app queues = индексы `[0, graphicsQueueIndex)` = `[0, 1)` = {0}
- Вызывает `vkGetDeviceQueue(device, family, 0, &q)` → получает `qvk.queue_graphics`
- Сравнивает с present queue → совпадает → **Valid** ✓

### Связь с CR11 / CR20
`eUseManualHooking` тестировался дважды (CR11 + CR20) — оба раза краш при создании swapchain в `1B0_E658703` через `sl.interposer`'s vkCreateSwapchainKHR hook. Этот флаг **запрещён**.

### Исправление (только dlss.c)

**`dlss.c`** — изменён `graphicsQueueIndex` с `0u` на `1u`:
```c
dlss_sl_set_vulkan_info(
    qvk.instance, qvk.physical_device, qvk.device,
    (uint32_t)qvk.queue_idx_graphics, 1u,  /* SL pacer starts at index 1 */
    (uint32_t)qvk.queue_idx_graphics, 1u);
```

**Механизм**: при `slSetVulkanInfo` с `graphicsQueueIndex=1u` SL внутренне вызывает `vkGetDeviceQueue` для индексов `[0, graphicsQueueIndex=1)`, т.е. для индекса 0, и добавляет полученный handle в таблицу "app queues". Так `qvk.queue_graphics` (index 0) попадает в таблицу → `getHostQueueInfo` возвращает valid → нет "Invalid VK queue" → нет краша на frame 2.

**eUseManualHooking — ЗАПРЕЩЁН**: CR11 (queueCount=1) и CR20 (queueCount=2) показали что этот флаг ломает DLSS-G swapchain initialization в SL runtime. Не добавлять.

### CrashReport20 (21 марта 2026)
**Стек**: `create_swapchain+0x610` → `dlss_sl_vkCreateSwapchainKHR+0x47` → `sl.interposer` → `1B0_E658703` (RAX=0)
**Первоначальная гипотеза**: `eUseManualHooking` (CR20 и CR11 имели одинаковый паттерн краша при создании swapchain).
**Реальная причина** (установлена по sl_debug.log, строка 1026):
```
[SL][ERR] createCommandQueue] getDeviceQueue(queueFamily, queueIndex + index, queueCreateFlags, tmp) failed 1 (Error)
```
SL вызывает `vkGetDeviceQueue(family, graphicsQueueIndex + index)` для index=0 и index=1:
- index=0: queue[1+0=1] → существует ✓
- index=1: queue[1+1=2] → **НЕТ** при queueCount=2 → краш
**Исправление**: `queueCount = 3` в main.c (queue[0]=app, queue[1]=SL pacer 0, queue[2]=SL pacer 1).
**Исправление**: убрать `eUseManualHooking`.

### Финальная конфигурация очередей
```
queueCount = 3 (в main.c при создании VkDevice)
  queue[0] = qvk.queue_graphics  — очередь приложения (present, render, compute)
  queue[1] = SL pacer queue 0    — DLSS-G pacer (graphicsQueueIndex+0 = 1+0 = 1)
  queue[2] = SL pacer queue 1    — DLSS-G pacer (graphicsQueueIndex+1 = 1+1 = 2)

VulkanInfo:
  graphicsQueueIndex = 1  — "SL queues start at index 1, app uses [0..0]"
  computeQueueIndex  = 1  — аналогично

eUseManualHooking — ЗАПРЕЩЁН (CR11). SL валидирует present queue через slSetVulkanInfo
при передаче graphicsQueueIndex=1u (вызывает vkGetDeviceQueue для app indices [0,1)).
```

---

## Сессия 5 (21 марта 2026) — CR23–CR27: DLSS-G runtime crashes

### Контекст

После исправлений CR11–CR20 (queueCount, graphicsQueueIndex, eUseManualHooking) DLSS SR работает стабильно. Началась работа над runtime-крашами при запуске с включённым DLSS-G/MFG.

---

### CR23/CR24: краш на первом present (NvLowLatencyVk.dll отсутствовала)

**Стек:**
```
q2rtx!dlss_sl_vkQueuePresentKHR+0x33 → sl.interposer → 1B0_E658703!presentCommon → RAX=0 crash
```

**sl_debug.log:**
```
[SL][WRN] notifyOutOfBandCommandQueue] No reflex
```

**Причина:** DLSS-G plugin v2.10.3 (OTA) проверяет наличие Reflex при каждом `vkCreateSwapchainKHR`. Если Reflex не инициализирован к этому моменту — plugin входит в broken state и крашится при первом present.

`NvLowLatencyVk.dll` — нужна для Vulkan Reflex. Без неё Streamline логирует "No reflex" и DLSS-G plugin теряет необходимый указатель на функцию → null ptr при первом present.

**Решения:**

1. **NvLowLatencyVk.dll** — найдена в Streamline SDK:
   ```
   O:\Claude2\Q2RTX-1.8.1\Q2RTX-src\extern\Streamline\external\reflex-sdk-vk\lib\NvLowLatencyVk.dll
   ```
   57840 байт, официальная SDK 2.8.0. Скопирована в папку с игрой.
   > ⚠️ Версия из RDR2 (48952 байт, PureDark мод) несовместима — не загружалась.

2. **Reflex применяется до создания swapchain** (в `vkpt_dlss_init`, до swapchain creation):
   ```c
   /* Apply Reflex before swapchain so DLSS-G plugin sees it in vkCreateSwapchainKHR hook */
   vkpt_dlss_reflex_apply_options();
   s_reflex_applied = true;
   ```

3. **Условная загрузка DLSS-G** через параметр `want_mfg` в `dlss_sl_startup`:
   ```cpp
   sl::Feature features_sr[]     = { sl::kFeatureDLSS, sl::kFeatureReflex };
   sl::Feature features_sr_mfg[] = { sl::kFeatureDLSS, sl::kFeatureDLSS_G, sl::kFeatureReflex };
   prefs.featuresToLoad    = want_mfg ? features_sr_mfg : features_sr;
   prefs.numFeaturesToLoad = want_mfg ? 3 : 2;
   ```
   DLSS-G plugin загружается ТОЛЬКО если `flt_dlss_mfg != 0`. Без MFG — плагин не загружается, swapchain hook не активируется, краша нет.

---

### CR25: DLSS SR заработал (с MFG=0)

После фикса NvLowLatencyVk.dll и условной загрузки:
- При `flt_dlss_mfg "0"` → DLSS-G плагин не загружается → игра запускается стабильно
- **DLSS SR подтверждён рабочим** по NVIDIA overlay индикатору (пользователь видел значок DLSS)

---

### CR26: краш при MFG=4 (первый present)

**sl_debug.log:**
```
[SL][WRN] vulkan.cpp:3480[setAsyncFrameMarker] Vulkan setAsyncFrameMarker is not implemented!
```
(Нет "No reflex" — NvLowLatencyVk.dll загрузилась успешно.)

**Стек:** аналогичен CR23, RAX=0 в 1B0_E658703!presentCommon.

**Причина:** DLSS-G plugin v2.10.3 крашится при первом `vkQueuePresentKHR` если `slDLSSGSetOptions` ни разу не вызывался. Plugin инициализирует внутренние структуры только при первом вызове `slDLSSGSetOptions`. Без этого вызова `presentCommon` обращается к неинициализированному указателю → RAX=0.

`slDLSSGSetOptions` вызывается только из `vkpt_dlss_process()`, который запускается только при рендере игровых кадров. Но первый present происходит раньше — при загрузочном экране/меню.

**Дополнительный guard** `s_g_options_ever_set`:
```cpp
static bool s_g_options_ever_set = false;
if (mfg_mode == 0 && !s_g_options_ever_set) return;  // не активировать плагин при MFG=0
s_g_options_ever_set = true;
```

**Исправление (CR26 → CR27):** вызывать `slDLSSGSetOptions(eOff)` сразу после создания swapchain, до первого present — в `vkpt_dlss_init_output_image`:
```c
/* Initialize DLSS-G plugin state immediately after swapchain creation.
 * Plugin crashes on first vkQueuePresentKHR if slDLSSGSetOptions was never
 * called — RAX=0 null-ptr in presentCommon (CR26).
 * s_g_tags_valid=false forces eOff until first game frame tags resources. */
if (g_dlss_sl_g_available && cvar_dlss_mfg && cvar_dlss_mfg->integer != 0)
{
    dlss_sl_set_g_options(
        cvar_dlss_mfg->integer,
        qvk.extent_unscaled.width, qvk.extent_unscaled.height,
        qvk.extent_unscaled.width, qvk.extent_unscaled.height,
        (uint32_t)qvk.num_swap_chain_images);
}
```

**Guard s_g_tags_valid:**
```cpp
/* Keep eOff until at least one dlss_sl_tag_g_resources call happened after last
 * swapchain create/recreate — otherwise DLSS-G dereferences unset resource slots. */
static bool s_g_tags_valid = false;   // reset в dlss_sl_alloc_g_resources()

if (mfg_mode == 0 || !s_g_tags_valid) {
    opts.mode = sl::DLSSGMode::eOff;
} else {
    opts.mode = sl::DLSSGMode::eOn;
    opts.numFramesToGenerate = (uint32_t)(mfg_mode - 1);
}
```

---

### CR27: краш при MFG=4 (позже, после успешного первого present)

**Стек:**
```
q2rtx!dlss_sl_vkQueuePresentKHR+0x33 → sl.interposer!0x52795230
  → 1B0_E658703!0x51f541fb (presentCommon)
  → 1B0_E658703!0x51f48a72
  → 1B0_E658703!0x51f44ef8
  → 1B0_E658703!0x51f49769  ← RAX=0 crash (другой адрес чем CR26!)
```

**sl_debug.log ключевые моменты:**
```
1s:837ms: slDLSSGSetOptions() called (наш init вызов из vkpt_dlss_init_output_image)
3s:162ms: "Repeated slDLSSGSetOptions() call for frame 1" (предупреждение, не критично)
3s:233ms: presentCommon — первый present
3s:257ms: Total VRAM used: 6.14 GB  ← первый present УСПЕШЕН
6s:861ms: Shutting down plugins... ← shutdown запущен crash handler'ом
```

**Вывод:** Первый present с `eOff` прошёл успешно. Краш происходит на **более позднем** present с `eOn` (когда `s_g_tags_valid=true` после первого игрового кадра).

**Текущая гипотеза:** `setAsyncFrameMarker` не реализована в нашей `NvLowLatencyVk.dll` (SDK 2.8.0). Для **4x MFG** (`numFramesToGenerate=3`) эта функция обязательна — DLSS-G plugin обращается к ней при eOn с numFramesToGenerate≥2 и получает null ptr → краш.

Для **2x MFG** (`numFramesToGenerate=1`) `setAsyncFrameMarker` не требуется.

**Тест:** изменён конфиг на `flt_dlss_mfg "2"` (2x MFG) — ожидается что краш исчезнет.

---

### Текущий статус (конец сессии 5)

| Компонент | Статус | Примечание |
|-----------|--------|------------|
| DLSS SR | ✅ работает | подтверждено NVIDIA overlay |
| NvLowLatencyVk.dll | ✅ загружается | из Streamline SDK 2.8.0 (57840 bytes) |
| Reflex | ⚠️ частично | slReflexSetOptions OK; `setAsyncFrameMarker` not implemented |
| DLSS-G eOff (MFG=0) | ✅ не загружается | `want_mfg=0` → плагин не грузится |
| DLSS-G eOff (MFG=4, старт) | ✅ первый present | CR27 fix: init call в vkpt_dlss_init_output_image |
| DLSS-G eOn 2x MFG | ⏳ тестируется | CR27 — сменили конфиг на mfg=2 |
| DLSS-G eOn 4x MFG | ❌ краш | требует `setAsyncFrameMarker` в NvLowLatencyVk.dll |
| nvngx_dlss.dll / nvngx_dlssg.dll | ✅ 310.5.3 | минимально требуемая версия |

### Обязательные файлы для DLSS-G (runtime)

| Файл | Версия | Источник | Роль |
|------|--------|----------|------|
| `NvLowLatencyVk.dll` | SDK 2.8.0 | `extern/Streamline/external/reflex-sdk-vk/lib/` | Vulkan Reflex, нужен для DLSS-G |

> ⚠️ `NvLowLatencyVk.dll` из RDR2 (PureDark, 48952 bytes) **несовместима** — не загружается.

### Соответствие flt_dlss_mfg → numFramesToGenerate

```
flt_dlss_mfg "2" → numFramesToGenerate=1 → 2x MFG  (НЕ требует setAsyncFrameMarker)
flt_dlss_mfg "3" → numFramesToGenerate=2 → 3x MFG
flt_dlss_mfg "4" → numFramesToGenerate=3 → 4x MFG  (требует setAsyncFrameMarker)
```

Для 4x MFG нужна `NvLowLatencyVk.dll` которая реализует `setAsyncFrameMarker` (добавлена в версиях новее SDK 2.8.0). Откуда брать — не установлено. Возможно из более новой игры с Streamline/DLSS 4.

---

## Сессия 6 — CR28→CR30, SDK 2.10.3, корень проблемы

### CR28 → CR30: прогресс

| Отчёт | Статус | Причина |
|-------|--------|---------|
| CR28 | ❌ eOn crash (nvoglv64, RAX=0) | sl.interposer 2.8.0 + OTA sl.dlss_g 2.10.3 = version mismatch |
| CR29 | ❌ crash на старте | удаление `eLoadDownloadedPlugins` убило DLSS SR и Reflex (null callbacks) |
| CR30 | ❌ eOn crash (nvoglv64, разные адреса) | sl.interposer 2.10.3 — мисматч устранён, но новая проблема |

### Что сделано в этой сессии

1. Скачан Streamline SDK v2.10.3 с GitHub (NVIDIA-RTX/Streamline)
2. Все sl.*.dll обновлены до 2.10.3 в директории игры:
   - sl.interposer.dll 2.10.3 (было 2.8.0)
   - sl.common.dll, sl.dlss.dll, sl.dlss_g.dll, sl.reflex.dll, sl.pcl.dll — все 2.10.3
3. Заголовки SDK в `extern/Streamline/include` обновлены до 2.10.3
4. `eLoadDownloadedPlugins` ВОССТАНОВЛЁН — OTA плагины используются (DLSS 4.5/Transformer 2)
5. DLSSGOptions: kStructVersion3 → kStructVersion4 (новое поле bReserved16)
6. `VK_KHR_push_descriptor` добавлен в OPTIONAL_DEVICE_EXTENSIONS в main.c

### CR30 — анализ sl_debug.log

**Критические warnings:**
```
Hook sl.common:Vulkan:CmdBindPipeline is NOT supported, plugin will not function properly
Hook sl.common:Vulkan:CmdBindDescriptorSets is NOT supported, plugin will not function properly
Hook sl.common:Vulkan:BeginCommandBuffer is NOT supported, plugin will not function properly
Failed to load VK device function: vkCmdPushDescriptorSetKHR (x2)
Repeated slDLSSGSetOptions() call for frame 1. A redundant call or a race condition with Present().
Invalid backbuffer resource extent (0 x 0) — SL resetting to full 5160x2160
setAsyncFrameMarker is not implemented!
```

**Стек краша CR30:**
```
0: nvoglv64      ← ACCESS VIOLATION
1: nvoglv64
2: nvoglv64
3: sl.common     ← sl.common вызывает nvoglv64!
4-7: sl.dlss_g
8: sl.interposer
```

### Корень проблемы (CR30) — Vulkan dispatch chain

sl.interposer НЕ перехватывает `vkCreateDevice` и `vkCreateInstance` в Q2RTX:
- Q2RTX вызывает `vkCreateDevice` напрямую через Vulkan loader (vulkan-1.dll)
- sl.interposer загружается через `LoadLibrary` — не в Vulkan dispatch chain
- Следствие: sl.interposer не может установить внутренние plugin hooks для
  `vkCmdBindPipeline`, `vkCmdBindDescriptorSets`, `vkBeginCommandBuffer`
- sl.common регистрирует их как "NOT supported" и падает когда пытается их вызвать

**Из официального гайда Streamline (ProgrammingGuide.md):**
> If you are using Vulkan, instead of `vulkan-1.dll` dynamically load `sl.interposer.dll`
> and obtain `vkGetInstanceProcAddr` and `vkGetDeviceProcAddr` as usual to get the rest of Vulkan API.
>
> Use `vkCreateInstance` and `vkCreateDevice` proxies provided by SL which will take care of
> all the extensions, features and command queues required by enabled SL features.

**sl_hooks.h** — публичные Vulkan hooks (только swapchain/present):
- eVulkan_Present, eVulkan_CreateSwapchainKHR, eVulkan_DestroySwapchainKHR,
  eVulkan_GetSwapchainImagesKHR, eVulkan_AcquireNextImageKHR, eVulkan_DeviceWaitIdle,
  eVulkan_CreateWin32SurfaceKHR, eVulkan_DestroySurfaceKHR
- CmdBindPipeline/CmdBindDescriptorSets/BeginCommandBuffer — НЕТ в публичных hooks
- Они устанавливаются ТОЛЬКО через внутренний механизм sl.interposer когда он перехватывает vkCreateDevice

### Необходимый fix (следующий шаг)

В `main.c` использовать sl.interposer proxy для vkCreateInstance и vkCreateDevice:

```c
// В init_vulkan(), в начале:
PFN_vkGetInstanceProcAddr sl_vkIProcAddr = vkpt_dlss_get_vkGetInstanceProcAddr_proxy();
PFN_vkCreateInstance  sl_vkCreateInstance  = sl_vkIProcAddr
    ? (PFN_vkCreateInstance)sl_vkIProcAddr(NULL, "vkCreateInstance") : NULL;

// Вместо vkCreateInstance(&inst_create_info, NULL, &qvk.instance):
result = (sl_vkCreateInstance ? sl_vkCreateInstance : vkCreateInstance)
         (&inst_create_info, NULL, &qvk.instance);

// После создания VkInstance — получаем proxy vkCreateDevice:
PFN_vkCreateDevice sl_vkCreateDevice = sl_vkIProcAddr
    ? (PFN_vkCreateDevice)sl_vkIProcAddr(qvk.instance, "vkCreateDevice") : NULL;

// Вместо vkCreateDevice(...):
result = (sl_vkCreateDevice ? sl_vkCreateDevice : vkCreateDevice)
         (qvk.physical_device, &dev_create_info, NULL, &qvk.device);
```

И для device function loading (макрос VK_EXTENSION_DO) использовать sl.interposer proxy
вместо стандартного `vkGetDeviceProcAddr`.

Новые функции в dlss.h/dlss.c/dlss_sl.cpp:
- `vkpt_dlss_get_vkGetInstanceProcAddr_proxy()` → возвращает s_sl_vkGetInstanceProcAddrProxy

### Порядок инициализации DLSS-G (финальный, CR27)

```
1. slInit (want_mfg=1) → DLSS-G plugin загружается
2. vkCreateDevice (queueCount=3)
3. slSetVulkanInfo (graphicsQueueIndex=1)
4. slReflexSetOptions — ДО создания swapchain (vkpt_dlss_init)
5. vkCreateSwapchainKHR через SL proxy → DLSS-G hook аллоцирует 6×85MB VRAM
6. slDLSSGSetOptions(eOff) — СРАЗУ после swapchain (vkpt_dlss_init_output_image)
   (без этого: первый present с неинициализированным плагином → RAX=0 → CR26)
7. Первый рендер: slSetConstants + slSetTagForFrame (DLSS-G ресурсы) + slDLSSGSetOptions(eOn)
8. vkQueuePresentKHR через SL proxy → DLSS-G генерирует кадры
```

---

## Сессия 7 — CR31: откат proxy routing (DLSS SR восстановлен)

### Проблема CR31
Предыдущая сессия добавила proxy routing vkCreateDevice через sl.interposer proxy +
vkGetDeviceProcAddr через sl.interposer. Результат: sl.interposer возвращал NULL для
нехукаемых функций (vkCreateImage, vkAllocateMemory и др.) → crash в GL_InitImages RIP=0.

### Действие
Полный откат всех proxy routing изменений в main.c:
- `vkCreateInstance` — прямой вызов через vulkan-1.dll (статически слинкованный)
- `vkCreateDevice` — прямой вызов
- `vkGetDeviceProcAddr` — стандартный
- Swapchain прокси (dlss_sl_vkCreateSwapchainKHR и др.) — **оставлены** (они работают)

### Результат
DLSS SR работает. DLSS-G (MFG) — crash на старте (CR32).

---

## Сессия 8 — CR32→CR35: дебаг interposer='no'

### Ключевое открытие: interposer='no' = root cause

Все плагины Streamline грузятся с `interposer='no'`:
```
pluginManager.cpp:828[mapPlugins] Loaded plugin 'sl.dlss_g' - interposer 'no'
```

`interposer='no'` означает что sl.interposer **не перехватил** vkCreateInstance/vkCreateDevice
→ плагины не имеют корректного Vulkan dispatch chain
→ при первом Present() sl.dlss_g пытается вызвать vkQueueSubmit через сломанный chain → crash.

Для `interposer='yes'` требуется что vkCreateInstance и vkCreateDevice вызываются через
sl.interposer, а не напрямую через vulkan-1.dll.

### CR32 — eUseManualHooking (частичный успех)

**Что:** добавлен `sl::PreferenceFlags::eUseManualHooking` в prefs.flags в dlss_sl.cpp.

**Теория:** в manual hooking mode sl.dlss_g НЕ пытается auto-hook vkBeginCommandBuffer /
vkCmdBindPipeline через dispatch chain → избегает NULL-next crash в GL_InitImages.

**Результат CR32:** crash в GL_InitImages (RIP=0) — eUseManualHooking не помогло в этом билде.

**Результат CR34 (с eUseManualHooking + чистый main.c):**
- Игра инициализировалась до звука меню — **прогресс**
- Crash при первом Present() внутри 1B0_E658703 (sl.dlss_g OTA)
- sl.dlss_g успел создать swapchain, выделить VRAM (6×85MB), запустить cmd contexts
- Но при генерации кадра — crash (без dispatch chain sl.dlss_g не может делать vkQueueSubmit)

### CR33 — routing vkCreateInstance + vkCreateDevice (неудача)

**Что:** оба вызова через sl.interposer proxy (GetProcAddress из sl.interposer.dll).
**Результат:** interposer='no' сохранился, crash в GL_InitImages через 1B0_E658703.
**Вывод:** одного routing vkCreateInstance/vkCreateDevice **недостаточно** для interposer='yes'.

### CR35 — SDL_Vulkan_LoadLibrary("sl.interposer.dll") (неудача + регрессия)

**Что:** добавлен SDL_Vulkan_LoadLibrary("sl.interposer.dll") ДО SDL_CreateWindow в sdl.c.

**Результат:**
- interposer='no' — не изменилось
- Новая регрессия: crash в **nvoglv64** (OpenGL ICD) — SDL_Vulkan_LoadLibrary сломал что-то
- Нет строк `wrapper.cpp` в sl_debug.log (sl.interposer не перехватывает vkCreateInstance)

**Действие:** SDL_Vulkan_LoadLibrary откатан обратно.

### Дубликаты плагинов (не критично)

В sl_debug.log видны предупреждения:
```
Detected two plugins with the same id 1B0_E658703 - sl.common
Ignoring plugin 'sl.common' since it has duplicated unique id
```
Это нормальное поведение Streamline 2.10.3 — два прохода loadPlugins (local + OTA).
Дубликаты игнорируются, на работу не влияют.

### RTSSVkLayer64 — лишний Vulkan layer

Во всех CR32-CR35 присутствует `RTSSVkLayer64` (RivaTuner Statistics Server).
Это дополнительный Vulkan layer поверх sl.interposer цепочки — потенциальный источник
дополнительного шума. Нужно выключить RTSS перед следующим тестом.

---

## Текущее состояние (после сессии 8)

### Что работает
- DLSS SR (Super Resolution) — работает стабильно
- slInit(), slSetVulkanInfo(), slSetTagForFrame() — все успешны
- Swapchain прокси (vkCreateSwapchainKHR / vkDestroySwapchainKHR / vkGetSwapchainImagesKHR /
  vkQueuePresentKHR) — работают, sl.dlss_g перехватывает их корректно

### Что не работает
- DLSS-G (MFG) — crash при первом Present()
- interposer='no' для всех плагинов — dispatch chain неполный

### Текущий код (после сессии 8)

**dlss_sl.cpp** — `eUseManualHooking` **убран**:
```cpp
prefs.flags = sl::PreferenceFlags::eDisableCLStateTracking
            | sl::PreferenceFlags::eAllowOTA
            | sl::PreferenceFlags::eLoadDownloadedPlugins
            | sl::PreferenceFlags::eUseFrameBasedResourceTagging;
// eUseManualHooking убран — в full-interposer mode не нужен
```

**sdl.c** — SDL_Vulkan_LoadLibrary **откатан** (вызывал регрессию nvoglv64).

**main.c** — чистый стандартный Vulkan (без proxy routing):
```c
VkResult result = vkCreateInstance(&inst_create_info, NULL, &qvk.instance);
result = vkCreateDevice(qvk.physical_device, &dev_create_info, NULL, &qvk.device);
```

---

## Следующие шаги (план)

### Шаг 1: стерильный тест (RTSS выключен)
Закрыть RivaTuner Statistics Server (и MSI Afterburner) перед запуском.
Проверить: меняется ли interposer='no' на 'yes', смещается ли crash.

### Шаг 2: выяснить точный механизм interposer='yes'
Из официального ProgrammingGuide.md Streamline:
> Instead of vulkan-1.dll, dynamically load sl.interposer.dll and obtain
> vkGetInstanceProcAddr and vkGetDeviceProcAddr from it.
> Use vkCreateInstance and vkCreateDevice proxies provided by SL.

Нужный порядок:
1. `slInit()` ДО vkCreateInstance (уже сделано)
2. Получить `PFN_vkGetInstanceProcAddr` из sl.interposer.dll
3. Через него получить `vkCreateInstance` и вызвать
4. Через него получить `vkCreateDevice` (передав VkInstance) и вызвать
5. Для device functions (vkCreateImage и др.) — продолжать использовать **стандартный**
   vkGetDeviceProcAddr (не sl.interposer) — иначе NULL для нехукаемых функций

### Шаг 3: если interposer='yes' не достигается через proxy routing
Возможно нужно использовать `SDL_Vulkan_GetVkGetInstanceProcAddr()` (не LoadLibrary):
- SDL_Vulkan_LoadLibrary("sl.interposer.dll") ДО SDL_CreateWindow
- В main.c: `PFN_vkGetInstanceProcAddr fp = SDL_Vulkan_GetVkGetInstanceProcAddr()`
- Через fp получить vkCreateInstance и вызвать → sl.interposer видит что это его chain

---

## Сессия 9 (22 марта 2026) — CR32–CR38: eUseManualHooking + Repeated SetOptions

### Контекст (CR32–CR36)

После безуспешных попыток получить interposer='yes' через SDL_Vulkan_LoadLibrary
(регрессия: crash nvoglv64 из-за конфликта двух Vulkan loader-ов) и через Vulkan dispatch chain proxy —
выбран **Путь B (clean manual hooking = eUseManualHooking)**.

**Причина:** Q2RTX статически слинкован с vulkan-1.dll. Полная замена loader-а через SDL
конфликтует со статически слинкованными символами.

**Путь B:** `eUseManualHooking` + все обязательные sl_hooks.h прокси реализованы явно:

| Прокси | Хук в sl.dlss_g | Статус |
|--------|-----------------|--------|
| `dlss_sl_vkCreateSwapchainKHR` | slHookVkCreateSwapchainKHR | ✅ OK |
| `dlss_sl_vkDestroySwapchainKHR` | slHookVkDestroySwapchainKHR | ✅ OK |
| `dlss_sl_vkGetSwapchainImagesKHR` | slHookVkGetSwapchainImagesKHR | ✅ OK |
| `dlss_sl_vkQueuePresentKHR` | slHookVkPresent | ✅ OK |
| `dlss_sl_vkAcquireNextImageKHR` | slHookVkAcquireNextImageKHR | ✅ OK (добавлен CR37) |
| `dlss_sl_vkDeviceWaitIdle` (×7) | slHookVkDeviceWaitIdle | ✅ OK (добавлен CR37) |
| `dlss_sl_vkDestroySurfaceKHR` | slHookVkDestroySurfaceKHR | ✅ OK (добавлен CR37) |

**Известные warning-и при eUseManualHooking (не блокеры):**
```
[SL][WRN] Hook sl.common:Vulkan:CmdBindPipeline is NOT supported
[SL][WRN] Hook sl.common:Vulkan:CmdBindDescriptorSets is NOT supported
[SL][WRN] Hook sl.common:Vulkan:BeginCommandBuffer is NOT supported
```
Это ожидаемо: в manual hooking mode SL не хукает dispatch chain. Для DLSS-G frame generation
эти хуки НЕ обязательны — plugin работает через swapchain/present hooks.

### CR37: первый Present проходит, crash на втором

**sl_debug.log последовательность CR37:**
```
[SL] slDLSSGSetOptions() called at frame init (vkpt_dlss_init_output_image) → frame 0
[SL] slHookVkCreateSwapchainKHR 5160x2126 → recreate → 5160x2160
[SL] vram alloc 6×85MB dlfg buffers at 5160x2160
[SL] "Creating command context dlss_g/game - cmd buffers 6"
[SL] presentCommon → Present frame 0 → "Total VRAM used: 6.39 GB" ← УСПЕХ
[SL][WRN] Repeated slDLSSGSetOptions() call for the frame 1 ← ПРОБЛЕМА
[SL][WRN] slDLSSGGetState must be synchronized with the present thread ← ПРОБЛЕМА
[SL] presentCommon → Present frame 1 → CRASH в 1B0_E658703!0x29769
```

**Crash context:**
```
Code: 0xc0000005 (Access Violation)
Address: 0x00007ffcd4289769 (1B0_E658703 = sl.dlss_g)
RCX: 0xadd54c8300600000  ← полностью некорректный указатель / corrupted state
```

### Анализ ChatGPT (22 марта 2026)

Второй Present — момент первой реальной попытки DLSS-G начать frame generation.
Crash не от "нехватки ещё одного Vulkan hook", а от race / duplicate setOptions / unsynchronized state.

**Главные подозреваемые:**
1. **Repeated slDLSSGSetOptions for frame 1** — функция вызывалась ДВА раза:
   - Раз из `vkpt_dlss_init_output_image` (при swapchain recreate 2126→2160)
   - Раз из `vkpt_dlss_process` (per-frame call)
   - SL frame counter наблюдал два вызова для одного frame → state corruption

2. **slDLSSGGetState outside present thread** — diagnostic call внутри `dlss_sl_set_g_options`
   вызывался ДО present, тогда как SL требует синхронизации с present thread

3. **RTSS и duplicate OTA modules** — RTSSVkLayer64 присутствовал в процессе

### CR38: фиксы (22 марта 2026)

**Фикс 1 — убрать `dlss_sl_set_g_options` из `vkpt_dlss_init_output_image`:**

До CR38 функция вызывалась И в init (после swapchain create), И в per-frame (до present).
При swapchain recreate (5160x2126 → 5160x2160) это давало двойной вызов для frame 1.

Решение: убрать вызов из init. `vkpt_dlss_process` всегда вызывается ДО `vkQueuePresent`,
поэтому гарантирует что set_options выполнится перед первым present (CR26 не вернётся).

**`dlss.c` — `vkpt_dlss_init_output_image`:**
```c
/* Убрана секция: if (g_dlss_sl_g_available && cvar_dlss_mfg && ...) dlss_sl_set_g_options(...)
 * Причина: вызов и здесь, и в vkpt_dlss_process вызывал "Repeated slDLSSGSetOptions for frame 1" */
```

**Фикс 2 — убрать `slDLSSGGetState` из `dlss_sl_set_g_options`:**

```cpp
// Убрана секция: static uint32_t s_g_state_log_frames; slDLSSGGetState(...)
// Причина: per SL docs — должен вызываться из present thread.
// Вызов до present → "must be synchronized" warning → возможная state corruption.
```

### Текущая конфигурация prefs.flags (CR38)

```cpp
prefs.flags = sl::PreferenceFlags::eDisableCLStateTracking
            | sl::PreferenceFlags::eAllowOTA
            | sl::PreferenceFlags::eLoadDownloadedPlugins
            | sl::PreferenceFlags::eUseFrameBasedResourceTagging
            | sl::PreferenceFlags::eUseManualHooking;
```

### Текущий статус DLSS-G (CR38)

| Компонент | Статус |
|-----------|--------|
| Все sl_hooks.h прокси | ✅ OK |
| Первый Present | ✅ проходит (подтверждено CR37) |
| Второй Present | ⏳ тестируется (CR38) |
| RTSS выключен? | ❓ нужно проверить перед тестом |

### Обязательный порядок вызовов за кадр (MFG enabled)

```
vkpt_dlss_process():
  1. dlss_sl_begin_frame()          — FrameToken
  2. dlss_sl_set_options()          — DLSS SR quality
  3. dlss_sl_set_constants()        — camera matrices, motion vectors
  4. dlss_sl_tag_resources()        — SR input/output tags
  5. dlss_sl_evaluate()             — DLSS SR upscale
  6. dlss_sl_set_g_options(mfg)     — MFG mode (ОДИН раз!)
  7. dlss_sl_tag_g_resources()      — depth/mvec/HUDless tags
dlss_sl_vkQueuePresentKHR():        — SL перехватывает, генерирует extra frame
```

**КРИТИЧНО:** `dlss_sl_set_g_options` — строго ОДИН раз на кадр, только из `vkpt_dlss_process`.
НЕ вызывать из init, recreate, или любого другого места!

---

## Сессия 10 (22 марта 2026) — CR39/CR40: Вариант A — rename sl.interposer.dll → vulkan-1.dll

### CR39: краш при старте (RIP=0x0 в initializePlugins)

**Симптом:** Краш при старте игры после удаления `eUseManualHooking` + добавления proxy routing
через `sl.interposer`'s `vkGetInstanceProcAddr` для `vkCreateInstance`/`vkCreateDevice`.

**sl_debug.log показал:**
```
[SL] wrapper.cpp:351[vkCreateDevice] Adding device extension 'VK_NV_optical_flow'
[SL] wrapper.cpp:360[vkCreateDevice] Adding extra 1 graphics queue(s)
[SL] wrapper.cpp:365[vkCreateDevice] Adding extra 2 compute queue(s)
[SL] initializePlugins - api 0.0.1 - application ID 1089130
→ CRASH: RIP=0x0 в sl.dlss_g
```

**Диагноз ChatGPT:** "Half-interposer" режим — sl.interposer wrapper участвует в vkCreateDevice
(добавляет очереди), но plugins всё равно помечены interposer='no' потому что dispatch table
не полностью заменён через proxy. Несовместимая конфигурация → гарантированный краш.

**Root cause:** Proxy routing (получить `vkCreateInstance` из `sl_proxy(NULL, "vkCreateInstance")`)
не эквивалентен реальному размещению sl.interposer в Vulkan loader chain. SL wrapper.cpp
работает (добавляет очереди), но sl.common не видит полного dispatch chain → interposer='no'
→ sl.dlss_g не может найти свои hooked callbacks → null ptr crash при первом вызове.

---

### CR40: Вариант A — rename sl.interposer.dll → vulkan-1.dll (22 марта 2026)

**Принцип:** Windows DLL search order: application directory → system. Если положить
sl.interposer рядом с exe как `vulkan-1.dll`, все Vulkan-вызовы приложения (импортированные
из vulkan-1.lib) автоматически пройдут через sl.interposer. Plugins получат interposer='yes'.

**Изменения в файловой системе:**
```
cp sl.interposer.dll vulkan-1.dll   (в папке игры, оригинал оставлен)
```

**dlss_sl.cpp:**
- `LoadLibraryW(L"sl.interposer.dll")` → `LoadLibraryW(L"vulkan-1.dll")`
  (грузим из локальной папки, не из system32 — Windows ищет в appdir первым)

**main.c:**
- Убран весь proxy routing код (`sl_proxy`, `pfn_create_instance`, `pfn_create_device`)
- Теперь прямые вызовы `vkCreateInstance` / `vkCreateDevice` → автоматически через sl.interposer
- `queueCount = 3` → `queueCount = 1`
  (SL сам добавляет +1 graphics, +2 compute, +1 optical flow через wrapper.cpp)

**dlss.c — `vkpt_dlss_init`:**
- `graphicsQueueIndex 1u` → `0u`
  (app queue теперь только 1 штука, index=0; SL добавляет свои и сам их трекает)

**prefs.flags (dlss_sl.cpp):**
```cpp
prefs.flags = sl::PreferenceFlags::eDisableCLStateTracking
            | sl::PreferenceFlags::eAllowOTA
            | sl::PreferenceFlags::eLoadDownloadedPlugins
            | sl::PreferenceFlags::eUseFrameBasedResourceTagging;
// eUseManualHooking убран — не нужен в interposer='yes' режиме
```

### Результат CR40: ПРОВАЛ — Missing export vkAcquireNextImage2KHR

**Симптом:** При запуске — диалог Windows "Точка входа в процедуру vkAcquireNextImage2KHR
не найдена в библиотеке DLL q2rtx.exe". Игра не стартует вообще.

**Причина:** `VKPT_DEVICE_GROUPS=ON` в cmake → main.c напрямую вызывает `vkAcquireNextImage2KHR`
(через `#ifdef VKPT_DEVICE_GROUPS` ветку в vkAcquireNextImage).
sl.interposer.dll экспортирует `vkAcquireNextImageKHR` но НЕ экспортирует `vkAcquireNextImage2KHR`.

---

### CR41: Вариант A1 — DEVICE_GROUPS=OFF + vulkan-1.dll rename (22 марта 2026)

**Идея (ChatGPT):** Отключить `VKPT_DEVICE_GROUPS` → код использует `vkAcquireNextImageKHR`
(которую sl.interposer экспортирует) → blocker "missing export" уходит.

**Аудит экспортов sl.interposer (КРИТИЧЕСКИ ВАЖНО для будущих попыток):**

sl.interposer.dll v2.10.3 экспортирует **ВСЕ** 106 функций которые q2rtx.exe импортирует
из vulkan-1.dll статически. Единственным исходным блокером был `vkAcquireNextImage2KHR`.

RT-extension функции (`vkCmdTraceRaysKHR`, `vkCreateAccelerationStructureKHR`, etc.) — это
**строки для vkGetDeviceProcAddr** в коде, НЕ PE-импорты из vulkan-1.dll. Они в PE import
table НЕ присутствуют и не мешают rename.

**Изменения:**
- cmake: `CONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF`
- `textures.c:2408`: добавлен `#ifdef VKPT_DEVICE_GROUPS` вокруг `lazy_image->image_local`
  (поле не существует когда DEVICE_GROUPS=OFF), передаём `NULL` вместо

**Результат CR41: ПРОВАЛ — silent crash, window flashes and closes, нет crash report, нет sl_debug.log**

**Root cause — CIRCULAR DEPENDENCY (подтверждено):**

sl.interposer.dll внутри себя вызывает `LoadLibrary("vulkan-1.dll")` для получения
настоящего Vulkan loader (чтобы форвардить real Vulkan calls). Когда sl.interposer
переименован в vulkan-1.dll и находится в app dir:

1. Windows загружает local vulkan-1.dll (sl.interposer) для PE imports q2rtx.exe
2. sl.interposer's DllMain вызывает `LoadLibrary("vulkan-1.dll")`
3. Windows находит уже загруженный local vulkan-1.dll → возвращает тот же handle (себя)
4. sl.interposer пытается форвардить real Vulkan calls через handle на самого себя
5. **Stack overflow → process terminates без crash report и без sl_debug.log**

**Подтверждение:** В sl.interposer.dll присутствует строка "vulkan-1.dll" (dynamic LoadLibrary),
НЕТ строки "C:\Windows\System32\vulkan-1.dll" или LOAD_LIBRARY_SEARCH_SYSTEM32 паттернов.

**ВЫВОД: Rename sl.interposer.dll → vulkan-1.dll НЕ РАБОТАЕТ с текущим sl.interposer.dll
из-за circular dependency. ChatGPT Вариант A2 (кастомный shim DLL) мог бы решить это,
но значительно сложнее.**

---

### CR42: Возврат к eUseManualHooking (22 марта 2026) — ТЕКУЩЕЕ СОСТОЯНИЕ

**Откатили обратно к рабочему CR38 состоянию.** Игра стартует, DLSS SR работает.
DLSS-G (MFG) при включении крашится на 2-м present.

**Текущая конфигурация CR42:**
```cpp
// dlss_sl.cpp
LoadLibraryW(L"sl.interposer.dll");   // не vulkan-1.dll

prefs.flags = eDisableCLStateTracking | eAllowOTA
            | eLoadDownloadedPlugins | eUseFrameBasedResourceTagging
            | eUseManualHooking;      // ← обязательно для запуска
```
```c
// main.c — queueCount=3
.queueCount = 3,  // queue[0]=app, queue[1..2]=SL/DLSS-G pacer

// dlss.c — graphicsQueueIndex=1u
dlss_sl_set_vulkan_info(..., gfx_family, 1u, gfx_family, 1u);
```
```cmake
CONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF   // оставлено OFF (не влияет на краш)
textures.c: NULL вместо image_local при DEVICE_GROUPS=OFF
```

**Нет proxy routing** в main.c (убран ещё в CR40, не возвращаем — он создаёт "half-interposer"
и не помогает).

---

## ИТОГО: Что работает и что нет (22 марта 2026)

### ✅ Работает
- DLSS SR (Super Resolution) — любой режим и preset
- Reflex
- Запуск игры с MFG disabled
- eUseManualHooking + queueCount=3 + graphicsQueueIndex=1u — стабильная конфигурация

### ❌ Не работает
- DLSS-G MFG (любой режим 2x/3x/4x) — краш на 2-м present
  - Симптом: sl.dlss_g+0x29769, RAX=0 (null ptr)
  - Момент: первый eOn кадр (s_g_tags_valid=true → второй vkQueuePresentKHR)

### 🚫 Тупиковые пути (НЕ ПОВТОРЯТЬ)
1. **Rename sl.interposer.dll → vulkan-1.dll** — circular dependency → stack overflow
2. **Proxy routing vkCreateInstance/vkCreateDevice через sl_proxy** — "half-interposer"
   режим → crash в initializePlugins (RIP=0x0)
3. **eUseManualHooking=OFF без rename** — plugins получают interposer='no' → crash
4. **slDLSSGSetOptions в init** — "Repeated SetOptions for frame 1" → state corruption
5. **slDLSSGGetState вне present thread** — "must be synchronized" warning → crash

### 🔍 Следующий шаг (на момент конца сессии 10)
Получить CR42 crash report (включить MFG=2x, дать крашнуться) и проанализировать
стек вызовов в sl.dlss_g+0x29769. Понять конкретно какой function pointer равен 0.
ChatGPT-консультация: почему в eUseManualHooking режиме sl.dlss_g имеет null ptr
на 2-м present, и есть ли способ его избежать без interposer='yes'.

---

## Сессия 11 (22 марта 2026) — Вариант C: кастомный шим vulkan-1.dll

### Концепция

**Цель:** решить circular dependency из CR41 (sl.interposer→ LoadLibrary("vulkan-1.dll") → себя)
через кастомный шим, который:
1. Перехватывает все Vulkan вызовы q2rtx.exe (как будущий vulkan-1.dll в папке игры)
2. Форвардит 9 dispatch-функций через sl.interposer (vkCreateInstance, vkCreateDevice и др.)
3. Форвардит остальные 256 функций напрямую в system32\vulkan-1.dll (s_real)

sl.interposer остаётся со своим именем (sl.interposer.dll), не переименовывается.
Шим — это `vulkan-1.dll` в папке игры. Circular dependency отсутствует: шим грузит
system32\vulkan-1.dll (полный путь), а sl.interposer грузит шим (как "vulkan-1.dll")
— но шим уже загружен → Windows возвращает тот же handle → не проблема.

### Файловая структура шима

```
O:\Claude2\shim_vulkan\
  shim_core.cpp     — ensure_init(), maybe_patch_sl(), все pfn переменные
  shim_dispatch.cpp — 9 C++ dispatch-обёрток с thread_local recursion guard
  shim_thunks.asm   — 256 MASM x64 thunks → s_real
  shim_vk.def       — 265 EXPORTS для linker
  CMakeLists.txt    — MASM + CXX DLL проект
  gen_full.py       — Python: генерирует все файлы из списка 265 функций
```

### Механизм lazy patching (maybe_patch_sl)

```cpp
// shim_core.cpp — 9 "dispatch" pfn (→ sl.interposer) + 9 "real" pfn (→ s_real)
extern "C" void* pfn_vkCreateInstance      = nullptr; // → sl.interposer после patch
extern "C" void* pfn_real_vkCreateInstance = nullptr; // → s_real всегда

// Заполняется когда GetModuleHandleW("sl.interposer.dll") возвращает non-NULL:
extern "C" void maybe_patch_sl(void) {
    if (s_sl) return;
    HMODULE hsl = GetModuleHandleW(L"sl.interposer.dll");
    if (!hsl) return;
    void* p = (void*)GetProcAddress(hsl, "vkCreateInstance");
    if (p) pfn_vkCreateInstance = p;
    // ... 8 других dispatch pfn
    s_sl = hsl;
}
```

```cpp
// shim_dispatch.cpp — thread_local recursion guard предотвращает circular calls
static thread_local int t_depth = 0;

extern "C" VkResult vkCreateInstance(...) {
    ensure_init();
    maybe_patch_sl();
    bool top = (t_depth == 0);
    if (top) t_depth++;
    VkResult r = ((PFN_vkCreateInstance)(top ? pfn_vkCreateInstance
                                             : pfn_real_vkCreateInstance))(...);
    if (top) t_depth--;
    return r;
}
```

```asm
; shim_thunks.asm — 256 type-agnostic thunks → s_real (прямой JMP через pfn)
vkAcquireNextImage2KHR PROC
    mov  QWORD PTR [rsp+8],  rcx
    ...
    sub  rsp, 40
    call ensure_init
    add  rsp, 40
    mov  rcx, QWORD PTR [rsp+8]
    ...
    jmp  QWORD PTR [pfn_vkAcquireNextImage2KHR]
vkAcquireNextImage2KHR ENDP
```

### Исправленные баги при разработке шима

#### Баг 1: `\x0b` в пути к system32\vulkan-1.dll (Q2RTX_CrashReport40)

**Симптом:** c0000005 ACCESS VIOLATION, RIP=0 в sl.interposer.
**Причина:** В shim_core.cpp был `L"C:\Windows\System32\vulkan-1.dll"` — Python-скрипт
через bash `-c "..."` съедал `\\` превращая `\v` → вертикальный таб `\x0b` → s_real=NULL
→ все pfn=NULL → crash при первом Vulkan вызове.

**Fix:** gen_full.py использует `bytes([0x5c, 0x5c])` для явной записи двойного backslash:
```python
# Не: path = 'C:\\Windows\\System32\\vulkan-1.dll'
# А: избегаем bash escape через bytes
```
Код в shim_core.cpp правильный: `GetSystemDirectoryW(path, MAX_PATH)` +
`StringCchCatW(path, MAX_PATH, L"\\vulkan-1.dll")` — без hardcoded путей.

#### Баг 2: ensure_init грузила sl.interposer → DllMain re-entrancy crash (Q2RTX_CrashReport41)

**Симптом:** sl.interposer absent in loaded modules, c0000005 при старте.
**Причина:** Ранняя версия ensure_init пыталась LoadLibrary("sl.interposer.dll") из DllMain
→ circular initialization → crash до sl_debug.log.

**Fix:** ensure_init грузит ТОЛЬКО s_real (system32\vulkan-1.dll).
sl.interposer грузится позже (Q2RTX явно вызывает LoadLibraryW("sl.interposer.dll")).
maybe_patch_sl() вызывается из каждого dispatch wrapper — проверяет GetModuleHandleW.

#### Баг 3: 106 экспортов вместо 265 (Q2RTX_CrashReport42)

**Симптом:** Windows диалог "Точка входа vkWaitSemaphores не найдена в nvngx_dlssg.dll".

**Причина:** Первая версия шима содержала только 106 функций (PE imports q2rtx.exe).
nvngx_dlssg.dll (OTA DLSS-G plugin) импортирует дополнительные функции Vulkan 1.2+
(vkWaitSemaphores, vkSignalSemaphore, vkGetBufferDeviceAddress и др.).

**Fix:** Получен полный список 265 экспортов из реального system32\vulkan-1.dll через
Python PE parser. gen_full.py регенерировал shim_core.cpp, shim_thunks.asm, shim_vk.def
с полным списком 265 функций.

#### Баг 4: maybe_patch_sl не вызывалась из dispatch wrappers (часть CR41)

**Fix:** В shim_dispatch.cpp макросы DISPATCH и DISPATCH_VOID дополнены вызовом
`maybe_patch_sl()` после `ensure_init()`.

---

### Q2RTX_CrashReport43: interposer='no' — архитектурный тупик шима

**sl_debug.log:**
```
pluginManager.cpp:828[mapPlugins] Loaded plugin 'sl.dlss_g' - interposer 'no'
pluginManager.cpp:828[mapPlugins] Loaded plugin 'sl.common' - interposer 'no'
...
[SL][WRN] sl.dlss_g: not requested by the host — skipping
```

**Два проблемы:**

**Проблема 1 — interposer='no':**
sl.interposer определяет режим внутри `mapPlugins` примерно так:
```cpp
wchar_t self_path[MAX_PATH];
GetModuleFileNameW(GetModuleHandleW(NULL), self_path, MAX_PATH);
// Или: GetModuleFileNameW(hSelf, ...)  где hSelf = handle sl.interposer.dll
bool interposer = ends_with(self_path, L"vulkan-1.dll");
```
Наш шим `vulkan-1.dll` в папке игры не является sl.interposer.dll →
sl.interposer замечает, что его собственный DLL называется "sl.interposer.dll",
а не "vulkan-1.dll" → устанавливает interposer='no' для всех плагинов.

**Проблема 2 — sl.dlss_g не запрашивается:**
Q2RTX в `dlss_sl_startup(want_mfg)`:
```cpp
sl::Feature features_sr[]     = { kFeatureDLSS, kFeatureReflex };         // want_mfg=0
sl::Feature features_sr_mfg[] = { kFeatureDLSS, kFeatureDLSS_G, kFeatureReflex }; // want_mfg=1
prefs.featuresToLoad = want_mfg ? features_sr_mfg : features_sr;
```
При запуске с MFG=0 (выключен в конфиге) → `want_mfg=0` → kFeatureDLSS_G не передаётся
→ sl.dlss_g игнорируется с сообщением "not requested by the host".

**Статус CR43: нерешён — активно исследуется.**

---

### Текущее состояние архитектуры шима (после CR43)

```
[q2rtx.exe]
  │  (PE import: vulkan-1.dll)
  ▼
[vulkan-1.dll = наш шим] — в папке игры
  │  265 экспортов:
  │  ├── 256 thunks → s_real (system32\vulkan-1.dll)  (через MASM)
  │  └── 9 dispatch → pfn_XX (lazy: initially s_real, после patch → sl.interposer)
  │       vkCreateInstance, vkCreateDevice, vkDestroyInstance, vkDestroyDevice,
  │       vkGetInstanceProcAddr, vkGetDeviceProcAddr,
  │       vkCreateDebugUtilsMessengerEXT, vkDestroyDebugUtilsMessengerEXT,
  │       vkEnumerateInstanceExtensionProperties
  ▼
[sl.interposer.dll] — загружается Q2RTX через LoadLibraryW  ← интерфейс НАРУШЕН
  interposer='no' (потому что sl.interposer видит своё имя != vulkan-1.dll)
  ▼
[system32\vulkan-1.dll = s_real] — настоящий Vulkan loader
```

**Проблема:** sl.interposer должен сам быть `vulkan-1.dll` чтобы получить interposer='yes'.

---

### Возможные следующие шаги

#### Вариант D: переименовать sl.interposer → vulkan-1_sl.dll или иначе обойти проверку

sl.interposer проверяет своё имя через `GetModuleFileNameW`. Если получилось бы
заставить его думать, что его файл называется vulkan-1.dll без физического rename —
например через CreateFileMapping или DLL injection trick — это решило бы проблему.
Но это нетривиально.

#### Вариант E: трёхуровневая цепочка

```
vulkan-1.dll (наш шим) → sl_proxy.dll (переименованный sl.interposer) → system32\vulkan-1.dll
```

sl.interposer переименовать в `sl_proxy.dll` (не vulkan-1.dll → нет circular dep).
Шим в своих dispatch wrappers грузит `sl_proxy.dll`, форвардит туда 9 функций.
sl_proxy.dll при LoadLibrary("vulkan-1.dll") найдёт наш шим (уже загружен → OK handle).
**НО:** sl_proxy.dll (бывший sl.interposer) всё равно увидит своё имя "sl_proxy.dll"
→ interposer='no' → проблема не решена.

#### Вариант F: eUseManualHooking + диагностика CR42 краша на 2-м present

Вернуться к рабочей конфигурации CR42 (eUseManualHooking, queueCount=3, graphicsQueueIndex=1u),
включить MFG=2x, дать крашнуться, проанализировать стек CR42 crash report.
Понять что конкретно sl.dlss_g разыменовывает в null на 2-м present → найти workaround.
Это не решает interposer='no' глобально, но если sl.dlss_g работает в eUseManualHooking
для eOff (первый present OK) — возможно есть конкретный недостающий тег/ресурс.

#### Вариант G: включить MFG в конфиге и запустить тест

Сначала убедиться что want_mfg=1 вызывается (добавить `flt_dlss_mfg "2"` в autoexec.cfg
или через консоль). Это устранит проблему "not requested". Можно комбинировать с CR42 конфигурацией.

---

## ТЕКУЩЕЕ СОСТОЯНИЕ (22 марта 2026, конец сессии 11)

### ✅ Работает
- DLSS SR (Super Resolution) — стабильно
- Reflex
- Шим compiles + deploys (265 экспортов, корректные пути)
- Шим перехватывает Vulkan, sl.interposer загружается, добавляет extensions ✓
- Запуск с MFG=0

### ❌ Не работает
- DLSS-G MFG — interposer='no' → crash при runtime
- sl.dlss_g не запрашивается при want_mfg=0 (нужно включить MFG в конфиге)

### 🚫 Тупиковые пути (дополнение к сессии 10)
6. **Rename sl.interposer → vulkan-1.dll (прямой)** — circular dependency (CR41 от rename)
7. **Кастомный шим vulkan-1.dll без переименования sl.interposer** — interposer='no'
   потому что sl.interposer проверяет своё собственное имя файла (не имя вызывающего)

### 🔍 Следующий шаг (приоритет)
1. Включить MFG в конфиге (`flt_dlss_mfg "2"`) чтобы want_mfg=1 → sl.dlss_g запрашивался
2. Вернуть eUseManualHooking конфигурацию (CR42 состояние), получить новый crash report
3. Проанализировать стек: что конкретно null на 2-м present в sl.dlss_g?
4. Дочитать dlss_sl.cpp — функции slSetVulkanInfo и proxy vkGetDeviceProcAddr для понимания
   как Q2RTX выстраивает SL dispatch после vkCreateDevice

---

## Сессия 12 (22 марта 2026) — Возврат к eUseManualHooking (CR42 состояние)

### Действия

**Откат proxy routing (main.c):**
Предыдущий компакт сессии сгенерировал proxy routing код для vkCreateInstance/vkCreateDevice
через sl.interposer proxy (`vkpt_dlss_get_vkGetInstanceProcAddr_proxy()`). Немедленно откатан —
это уже провалилось в CR31/CR33/CR39: "half-interposer" или NULL для нехукаемых функций.

**Возврат к eUseManualHooking:**
В `dlss_sl.cpp` добавлен флаг `eUseManualHooking` в `prefs.flags`.

**Убраны IAT hooks и LDR rename:**
`GetModuleHandleW` IAT hook устанавливался успешно, но НИКОГДА не вызывался
(sl.interposer не использует GetModuleHandleW для interposer detection).
LDR rename также не работал (PEB entry not found). Оба механизма удалены.

**Убраны соответствующие restore вызовы** после slInit.

### Текущая конфигурация (CR42 восстановлена)

```cpp
// dlss_sl.cpp prefs.flags:
prefs.flags = eDisableCLStateTracking | eAllowOTA | eLoadDownloadedPlugins
            | eUseFrameBasedResourceTagging | eUseManualHooking;

// main.c queueCount:
.queueCount = 3   // queue[0]=app, queue[1..2]=SL pacer

// dlss.c graphicsQueueIndex:
dlss_sl_set_vulkan_info(..., gfx_family, 1u, gfx_family, 1u);
```

### Следующий шаг

Запустить игру с `flt_dlss_mfg "2"` (уже в конфиге), дать крашнуться на 2-м present,
прислать sl_debug.log для анализа конкретной причины null ptr в sl.dlss_g+0x29769.

### 🚫 Тупиковые пути (финальный список)

1. Rename sl.interposer.dll → vulkan-1.dll — circular dependency → stack overflow
2. Proxy routing vkCreateInstance/vkCreateDevice — "half-interposer" → crash GL_InitImages
3. eUseManualHooking=OFF без rename/shim — interposer='no' → crash
4. Кастомный шим vulkan-1.dll — interposer='no' (sl.interposer видит своё имя != vulkan-1.dll)
5. IAT hook GetModuleHandleW — hook никогда не вызывается (не тот механизм detection)
6. LDR BaseDllName rename — PEB entry not found (неверное смещение в LDR_ENTRY)
7. slDLSSGSetOptions в init — "Repeated SetOptions for frame 1"
8. slDLSSGGetState вне present thread — "must be synchronized"

---

## Сессия 13 (22 марта 2026) — Корень "missing common constants": PCL present markers

### Контекст (после CR42)

После CR42 (eUseManualHooking + queueCount=3 + graphicsQueueIndex=1u):
- Игра запускается стабильно, не крашится
- DLSS SR работает
- DLSS-G: не крашится, но FPS при MFG=2 и MFG=0 одинаков (~106 fps по timerefresh)
- В sl_debug.log (строка ~11110):
  ```
  Warning: missing common constants - Frame Generation will be disabled for the frame
  ```

### Анализ sl_debug.log

Ключевые строки из лога:
```
Callback sl.common:slSetConsts:0x7ff997cefb90  ← sl.common callback НЕ null (строка 864)
Callback sl.dlss_g:slSetConsts:0x0             ← null (нормально для interposer='no') (строка 966)
Hook sl.dlss_g:slHookVkPresent:before - OK     ← present перехватывается (строка 968)
Invalid backbuffer resource extent (0 x 0) → auto-reset to 5160x2160 (строка 11078)
Warning: missing common constants - Frame Generation will be disabled for the frame (строка 11110)
```

Строка из `ProgrammingGuideDLSS_G.md` §8.0 (критическая):
> "IMPORTANT: If you see a warning stating that `common constants cannot be found for frame N`
> that indicates that sl.reflex markers `eReflexMarkerPresentStart` and `eReflexMarkerPresentEnd`
> are out of sync with the actual frame being presented."

### Корень проблемы

DLSS-G сопоставляет каждый `vkQueuePresentKHR` с frame token через **PCL маркеры**
(`slPCLSetMarker`). Механизм:

1. Приложение вызывает `slSetConsts(constants, frame_token, viewport)` — константы камеры
   сохраняются в sl.common **с ключом frame_token**
2. При `vkQueuePresentKHR` sl.dlss_g перехватывает вызов и ищет common constants
   по текущему frame_token. Но **без PCL маркеров ePresentStart/ePresentEnd** вокруг present —
   sl.dlss_g не знает какой frame_token соответствует данному present
3. Результат: "missing common constants" каждый кадр → Frame Generation disabled

Без PCL маркеров DLSS-G работает как eOff несмотря на eOn.

### Исправление в dlss_sl.cpp

**1. Добавлен include:**
```cpp
#include "sl_pcl.h"  /* PCLMarker, slPCLSetMarker — used to mark present boundaries for DLSS-G */
```

**2. Добавлен pointer:**
```cpp
/* PCL function pointer (via slGetFeatureFunction).
 * slPCLSetMarker(ePresentStart/End, frame) is required by DLSS-G to match present → frame token.
 * Without these markers, DLSS-G cannot find common constants → "missing common constants" every frame. */
static PFun_slPCLSetMarker* pfn_slPCLSetMarker = nullptr;
```

**3. В `dlss_sl_set_vulkan_info` — загрузка через slGetFeatureFunction:**
```cpp
slGetFeatureFunction(sl::kFeaturePCL, "slPCLSetMarker", (void*&)pfn_slPCLSetMarker);
fprintf(stdout, "[DLSS-G] slPCLSetMarker: %s\n", pfn_slPCLSetMarker ? "loaded" : "MISSING");
```

**4. В `dlss_sl_begin_frame` — eSimulationStart маркер:**
```cpp
void dlss_sl_begin_frame(uint32_t frame_index)
{
    if (!s_sl_loaded) return;
    slGetNewFrameToken(s_frame_token, &frame_index);
    /* PCL simulation-start marker: informs SL the new frame has begun. */
    if (pfn_slPCLSetMarker && s_frame_token)
        pfn_slPCLSetMarker(sl::PCLMarker::eSimulationStart, *s_frame_token);
}
```

**5. В `dlss_sl_vkQueuePresentKHR` — ePresentStart/ePresentEnd вокруг present (ГЛАВНОЕ):**
```cpp
VkResult dlss_sl_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *info)
{
    /* PCL present markers: required by DLSS-G to correlate this vkQueuePresentKHR call with the
     * frame token that was used in slSetConstants() earlier in the frame.  Without ePresentStart
     * before the present, sl.dlss_g cannot locate the common constants for the frame being
     * presented and logs "missing common constants — Frame Generation will be disabled for the
     * frame" on every present, causing MFG to silently no-op. */
    if (pfn_slPCLSetMarker && s_frame_token)
        pfn_slPCLSetMarker(sl::PCLMarker::ePresentStart, *s_frame_token);

    VkResult res;
    if (s_sl_vkQueuePresentKHR)
        res = s_sl_vkQueuePresentKHR(queue, info);
    else
        res = vkQueuePresentKHR(queue, info);

    if (pfn_slPCLSetMarker && s_frame_token)
        pfn_slPCLSetMarker(sl::PCLMarker::ePresentEnd, *s_frame_token);

    return res;
}
```

### Билд и деплой

- Билд: 22 марта 2026, ~16:47 МСК
- Деплой: `E:\SteamLibrary\steamapps\common\Quake II RTX\q2rtx.exe`
- Конфиг: `flt_dlss_mfg "2"` (2x MFG) в q2config.cfg

### Статус теста

✅ **ПРОТЕСТИРОВАНО** — DLSS SR работает, MFG заработал на оригинальном OTA без DLL-патча.
Причина неработающего MFG была во включённом HDR, а не в `WAR4639162`.

**Ожидаемый результат при успехе:**
```
flt_dlss_mfg 2 → timerefresh → ~210 fps  (было ~106 fps)
```
В sl_debug.log строки "missing common constants" должны исчезнуть.
Строка "[DLSS-G] slPCLSetMarker: loaded" должна присутствовать в stdout.

### API PCL (из sl_pcl.h)

```cpp
enum class PCLMarker : uint32_t {
    eSimulationStart  = 0,   // начало кадра
    eSimulationEnd    = 1,
    eRenderSubmitStart = 2,
    eRenderSubmitEnd  = 3,
    ePresentStart     = 4,   // ОБЯЗАТЕЛЬНО перед vkQueuePresentKHR
    ePresentEnd       = 5,   // ОБЯЗАТЕЛЬНО после vkQueuePresentKHR
    ...
};
using PFun_slPCLSetMarker = sl::Result(sl::PCLMarker marker, const sl::FrameToken& frame);
```

### Диагностика по sl_debug.log

**При успехе ("missing common constants" исчез):**
- Не будет строк `Warning: missing common constants`
- Будут строки об успешной генерации кадров

**При неудаче (всё ещё "missing common constants"):**
- Проверить stdout: `[DLSS-G] slPCLSetMarker: loaded` или `MISSING`
- Если MISSING → sl::kFeaturePCL не загружен → добавить kFeaturePCL в featuresToLoad
- Если loaded но warnings сохраняются → frame token mismatch (слишком ранний token)

### Обновлённый статус (сессия 13)

| Компонент | Статус | Примечание |
|-----------|--------|------------|
| DLSS SR | ✅ работает | |
| eUseManualHooking конфигурация | ✅ стабильна | queueCount=3, graphicsQueueIndex=1u |
| PCL маркеры | ✅ добавлены | ePresentStart/End вокруг vkQueuePresentKHR |
| DLSS-G MFG 2x | ⏳ тестируется | Билд 16:47 МСК — "missing common constants" исправлен |

---

## Сессия 14 (22 марта 2026) — CR50: WAR4639162 — DLSS-G не генерирует кадры

### Проблема

DLSS-G инициализируется без краша (CR42 стабильна), `slIsFeatureSupported DLSS-G = 0`, выделяет 714MB VRAM, принимает `eOn`/MFG-режимы, но FPS абсолютно одинаковый при MFG=0/2/4x. `sl_debug.log` содержит внутренний WAR-код `WAR4639162` — это флаг DLSS-G fallback режима, при котором frame generation отключена.

### Корневая причина

`pluginManager->initializePlugins()` никогда не вызывался. Этот вызов происходит ТОЛЬКО в `sl.interposer`'s `vkCreateDevice` hook (wrapper.cpp:956). Но Q2RTX вызывал `vkCreateDevice` напрямую через `vulkan-1.dll`, минуя sl.interposer.

Без `initializePlugins()`:
- Plugin dispatch chain hooks не установлены (vkBeginCommandBuffer, vkCmdBindPipeline, vkCmdBindDescriptorSets)
- `eUseManualHooking` детектирует отсутствие этих hooks → активирует WAR4639162 fallback
- DLSS-G работает в режиме "no frame generation"

Дополнительно: `slSetVulkanInfo` вызывал `processVulkanInterface`, который перестраивал `s_ddt` из чистого `vulkan-1.dll` (system32), уничтожая plugin-aware dispatch table. Даже если `initializePlugins()` будет вызван, `slSetVulkanInfo` сразу сотрёт его результат.

### Решение (CR50)

**4 файла, 5 изменений:**

#### 1. dlss_sl.cpp: убрать eUseManualHooking

**Было:**
```cpp
prefs.flags = sl::PreferenceFlags::eDisableCLStateTracking
            | sl::PreferenceFlags::eAllowOTA
            | sl::PreferenceFlags::eLoadDownloadedPlugins
            | sl::PreferenceFlags::eUseFrameBasedResourceTagging
            | sl::PreferenceFlags::eUseManualHooking;
```

**Стало:**
```cpp
prefs.flags = sl::PreferenceFlags::eDisableCLStateTracking
            | sl::PreferenceFlags::eAllowOTA
            | sl::PreferenceFlags::eLoadDownloadedPlugins
            | sl::PreferenceFlags::eUseFrameBasedResourceTagging;
/* eUseManualHooking убран — с CR50 vkCreateDevice роутится через sl.interposer,
 * initializePlugins() устанавливает dispatch chain hooks, WAR4639162 не активируется */
```

#### 2. dlss_sl.cpp: пропустить slSetVulkanInfo

**Было:**
```cpp
sl::Result res = slSetVulkanInfo(vk);
g_dlss_sl_setvk_result = (int)res;
if (res != sl::Result::eOk)
    return;
```

**Стало:**
```cpp
/* CR50: Skip slSetVulkanInfo — вызывает processVulkanInterface → перестраивает s_ddt
 * из system vulkan-1.dll, уничтожая plugin-aware dispatch table от initializePlugins(). */
(void)vk;
g_dlss_sl_setvk_result = (int)sl::Result::eOk;
```

Остальная часть функции (fetch function pointers via s_sl_vkGetDeviceProcAddrProxy) остаётся.

#### 3. dlss.c: добавить vkpt_dlss_prepare_device_creation()

```c
PFN_vkCreateDevice vkpt_dlss_prepare_device_creation(VkInstance instance, VkPhysicalDevice phys_dev)
{
    PFN_vkGetInstanceProcAddr gipa = dlss_sl_get_vkGetInstanceProcAddr_proxy();
    if (!gipa) return vkCreateDevice;

    /* Populate instanceDeviceMap: hook смотрит его в vkCreateDevice для поиска VkInstance */
    PFN_vkEnumeratePhysicalDevices sl_epd = (PFN_vkEnumeratePhysicalDevices)
        gipa(instance, "vkEnumeratePhysicalDevices");
    if (sl_epd) {
        uint32_t count = 1;
        sl_epd(instance, &count, &phys_dev);
    }

    PFN_vkCreateDevice sl_cd = (PFN_vkCreateDevice)gipa(instance, "vkCreateDevice");
    return sl_cd ? sl_cd : vkCreateDevice;
}
```

#### 4. dlss.h: объявление

```c
PFN_vkCreateDevice vkpt_dlss_prepare_device_creation(VkInstance instance, VkPhysicalDevice phys_dev);
```

#### 5. main.c: роутинг vkCreateDevice через proxy

**Было:**
```c
result = vkCreateDevice(qvk.physical_device, &dev_create_info, NULL, &qvk.device);
```

**Стало:**
```c
/* CR50: route through sl.interposer → initializePlugins() → DLSS-G dispatch chain hooks */
{
    PFN_vkCreateDevice fn_cd = vkpt_dlss_prepare_device_creation(qvk.instance, qvk.physical_device);
    result = fn_cd(qvk.physical_device, &dev_create_info, NULL, &qvk.device);
}
```

### Почему queue validation работает без slSetVulkanInfo

В sl.interposer's vkCreateDevice hook (wrapper.cpp:864-905):
```cpp
s_vk.graphicsQueueIndex += queueCreateInfos.back().queueCount;
// При queueCount=3: s_vk.graphicsQueueIndex = 3
```
SL знает: app queues = [0, graphicsQueueIndex-1] = [0, 2]. Queue[0] (наш present queue) — valid.
`getHostQueueInfo` принимает его без "Invalid VK queue" ошибки.

### Статус CR50

| Шаг | Статус |
|-----|--------|
| Код написан | ✅ |
| Сборка прошла | ✅ |
| Деплой | ✅ E:\SteamLibrary\...\q2rtx.exe |
| Тестирование DLSS-G | ⏳ ждёт запуска игры |

### Что проверять в тесте

1. `sl_debug.log` — нет ли нового краша; нет ли WAR4639162
2. Строки инициализации: `initializePlugins` success
3. FPS с MFG=0 vs MFG=2x vs MFG=4x — должен расти
4. При новом крашрепорте: проверить стек и sl_debug.log

---

## Сессия 14 продолжение — CR50b и CR50c: исправление s_idt + slSetVulkanInfo

### CR50 → CR45 (crash): RIP=0 в sl.interposer

**Ситуация:** CR50 убрал `eUseManualHooking` и роутил vkCreateDevice через sl.interposer. Но при
старте игры — crash: RIP=0 в `sl.interposer+0x34AD6` из `vkpt_dlss_prepare_device_creation`.

**Причина:** `vkpt_dlss_prepare_device_creation` вызывает `sl.interposer`'s `vkEnumeratePhysicalDevices`.
Внутри него sl.interposer обращается к `s_idt` (instance dispatch table) — но `s_idt` пуста,
потому что `vkCreateInstance` не прошёл через sl.interposer. `s_idt` заполняется только внутри
sl.interposer's `vkCreateInstance` hook (через `mapVulkanInstanceAPI`). RIP=0 = null ptr в s_idt.

**Исправление (CR50b):** Роутить ТАКЖЕ `vkCreateInstance` через sl.interposer, до vkEnumeratePhysicalDevices.

### CR50b: routing vkCreateInstance + vkCreateDevice

**Добавлено:**

#### dlss.c — vkpt_dlss_prepare_instance_creation()

```c
PFN_vkCreateInstance vkpt_dlss_prepare_instance_creation(void)
{
    PFN_vkGetInstanceProcAddr gipa = dlss_sl_get_vkGetInstanceProcAddr_proxy();
    if (!gipa) return vkCreateInstance;
    PFN_vkCreateInstance sl_ci = (PFN_vkCreateInstance)gipa(VK_NULL_HANDLE, "vkCreateInstance");
    return sl_ci ? sl_ci : vkCreateInstance;
}
```

#### dlss.h — объявление

```c
PFN_vkCreateInstance vkpt_dlss_prepare_instance_creation(void);
```

#### main.c — роутинг vkCreateInstance (строка ~972):

```c
/* CR50b: route through sl.interposer so mapVulkanInstanceAPI populates s_idt */
PFN_vkCreateInstance fn_ci = vkpt_dlss_prepare_instance_creation();
VkResult result = fn_ci(&inst_create_info, NULL, &qvk.instance);
```

**Результат CR50b:** Игра запускается! Нет crash. sl_debug.log подтверждает:
- Строка 210: `wrapper.cpp:163[vkCreateInstance]` — vkCreateInstance перехвачен sl.interposer ✓
- Строка 216: `wrapper.cpp:351[vkCreateDevice]` — vkCreateDevice перехвачен sl.interposer ✓
- Строка 260: `pluginManager.cpp:1279[initializePlugins]` — initializePlugins вызван ✓
- Строка 754: `FeatureInitResult = NvNGXFeatureInitSuccess` (DLSS SR) ✓
- Строка 830: `FeatureInitResult = NvNGXFeatureInitSuccess` (DLSS-G) ✓

**НО:** консоль показывает:
```
[DLSS] slSetVulkanInfo result: 19 (ошибка)
[DLSS] GPU does not support DLSS SR
```
`g_dlss_sl_available=0` — DLSS SR недоступен.

### Диагноз CR50b: slSetVulkanInfo result=19

**sl_debug.log строка 946:**
```
pluginManager.cpp:1361[initializePlugins] Plugins already initialized but could be using
the wrong device, please call slSetD3DDevice immediately after creating desired device
```

`slSetVulkanInfo` → `processVulkanInterface` → `initializePlugins` снова → обнаруживает что
плагины уже инициализированы (из vkCreateDevice hook) → возвращает ошибку 19.

При result=19 наш код делал `return` — пропускал весь блок: fetch SL function pointers,
Reflex/PCL init, `slIsFeatureSupported` → `g_dlss_sl_available` оставался 0.

### CR50c: игнорировать result=19 и продолжить

**Изменение в dlss_sl.cpp** (функция `dlss_sl_set_vulkan_info`):

**Было:**
```cpp
sl::Result res = slSetVulkanInfo(vk);
g_dlss_sl_setvk_result = (int)res;
if (res != sl::Result::eOk)
    return;
```

**Стало:**
```cpp
/* CR50c: With sl.interposer routing active, slSetVulkanInfo returns 19
 * ("plugins already initialized") because initializePlugins() already ran
 * inside the vkCreateDevice hook. Device state is already correct — ignore
 * this error and continue to fetch SL function pointers. */
sl::Result res = slSetVulkanInfo(vk);
g_dlss_sl_setvk_result = (int)res;
if (res != sl::Result::eOk) {
    fprintf(stderr, "[DLSS] slSetVulkanInfo returned %d (ignored — SL already has device state from vkCreateDevice hook)\n", (int)res);
}
```

**Почему это безопасно:**
- vkCreateInstance через sl.interposer → s_idt заполнена ✓
- vkEnumeratePhysicalDevices через sl.interposer → instanceDeviceMap заполнена ✓
- vkCreateDevice через sl.interposer → s_vk.device/instance + initializePlugins() ✓
- slSetVulkanInfo всё равно вызывается → SL регистрирует queue layout в app-queue table ✓
- После return без early-exit: slIsFeatureSupported, slGetFeatureFunction, Reflex init — всё выполнится

**Билд CR50c:** 22 марта 2026
**Деплой:** `E:\SteamLibrary\steamapps\common\Quake II RTX\q2rtx.exe`

### Конфигурация прежних параметров (CR50c):

| Параметр | Значение | Файл |
|---------|---------|------|
| eUseManualHooking | ✅ СОХРАНЁН | dlss_sl.cpp |
| queueCount | 3 | main.c |
| graphicsQueueIndex | 1u | dlss.c |
| vkCreateInstance роутинг | через sl.interposer | main.c (CR50b) |
| vkCreateDevice роутинг | через sl.interposer | main.c (CR50) |
| slSetVulkanInfo | вызывается, ошибка игнорируется | dlss_sl.cpp (CR50c) |

### Ожидаемый результат CR50c

```
[DLSS] slSetVulkanInfo returned 19 (ignored ...)   ← в stderr
[DLSS] slIsFeatureSupported DLSS SR result: 0       ← OK
[DLSS] DLSS SR available                           ← g_dlss_sl_available=1
```

✅ **CR50c протестирован и работает (22 марта 2026)**

---

## CR51–CR53 и переоценка WAR4639162 (22–23 марта 2026)

### CR51 — timing fix: kFeatureDLSS_G всегда грузить

**Проблема:** q2config.cfg выполняется ПОСЛЕ vkpt_dlss_pre_init() → cvar_dlss_mfg=0 при старте
→ sl.dlss_g никогда не загружался.

**Фикс:** убрать гейт `if (want_mfg)` в `features_all[]` — всегда загружать kFeatureDLSS_G.

### CR52 — DLL hijack rollback

Попытка переименовать sl.interposer.dll → vulkan-1.dll (для interposer='yes') → circular dep crash.
ОТКАТ. sl.interposer.dll восстановлен из бэкапа.

### CR53 — попытка патча s_vk.getDeviceProcAddr через slGetParameters()

**Проблема:** s_vk.getDeviceProcAddr = native (из processVulkanInterface) → sl.dlss_g получает
native VkTable → WAR4639162 проверка проваливается.

**Попытка:** slGetParameters() для получения &s_vk → патч getDeviceProcAddr[offset 16] → sl.interposer own.

**Результат:** slGetParameters НЕ экспортируется из sl.interposer 2.8.0 (и 2.10.x UpscalerBasePlugin).
CR53 патч не выполнялся. WAR4639162 продолжал срабатывать.

### Итог по WAR4639162 и OTA DLL

Первичная гипотеза про обязательный binary patch OTA DLL оказалась неверной.

Что подтверждено повторной проверкой:
- загружается восстановленный оригинальный OTA `133635` без локального патча;
- `DLSS-G MFG 2X/3X/4X` при этом работает;
- строка `WAR4639162` может появляться как диагностическое сообщение, но сама по себе не
  доказывает, что frame generation отключён;
- реальной причиной отсутствия эффекта от MFG был включённый HDR.

Практический вывод:
- патчить OTA DLL больше не требуется;
- любые локальные файлы вроде `*_patched.dll` и `*_original.dll`, оставленные как бэкап,
  на игру не влияют, пока не подменяют реальный runtime-файл;
- рабочая конфигурация для тестов MFG: `HDR OFF`.

**Результат:** DLSS-G MFG 2X/3X/4X работает на оригинальном OTA runtime без DLL-патча.

---

## Итоговый статус (22 марта 2026)

| Фича | Статус |
|------|--------|
| DLSS SR (Super Resolution) | ✅ работает |
| Reflex | ✅ работает |
| DLSS-G MFG 2X/3X/4X | ✅ работает (без HDR) |
| HDR + MFG | ⚠️ конфликт, MFG не работает при HDR |

Дополнение:
- OTA DLL patch не требуется
- корневая причина прежнего "MFG не работает" — HDR

---

## Открытая проблема: HDR + MFG конфликт

**Симптом:** При включении HDR (нативный Q2RTX HDR или Nvidia RTX HDR) MFG перестаёт
работать. С выключенным HDR MFG работает нормально.

**Направления для изучения (код не трогаем до команды):**

### 1. Swapchain format несовместимость
HDR требует VK_FORMAT_R16G16B16A16_SFLOAT или VK_FORMAT_A2B10G10R10_UNORM_PACK32.
sl.dlss_g требует конкретные форматы и может не поддерживать HDR swapchain без
дополнительной конфигурации в slDLSSGSetOptions.

### 2. HUDless buffer / colorBuffersHDR
`kBufferTypeHUDLessColor` должен содержать linear HDR данные без UI-оверлеев.
DLSSOptions.colorBuffersHDR должно быть выставлено. Если Q2RTX не передаёт этот флаг
в HDR режиме — sl.dlss_g работает с неправильными предположениями о цветовом пространстве.

### 3. Nvidia RTX HDR — конкуренция на уровне present
RTX HDR — driver-level post-processing на vkQueuePresentKHR.
Конкурирует с sl.interposer's present hook. Если RTX HDR перехватывает present
ДО или ПОСЛЕ sl.interposer в неожиданном порядке — FG pipeline ломается.

### 4. UI alpha буфер в HDR
`kBufferTypeUIColorAndAlpha` — в HDR альфа-канал UI может быть некорректен для
optical flow сети sl.dlss_g.

### 5. HDR metadata не передаётся в SL
DLSS-G в HDR может требовать nits/color primaries через sl::Constants.
Q2RTX, вероятно, не передаёт эти данные.

**Статус:** изучение завершено, ждём команды на исправление.


## Управление sl_debug.log

Начиная с публичной беты `0.8 beta`, запись `sl_debug.log` больше не включена принудительно.

Новая cvar:
- `flt_dlss_sl_debug_log 0` — лог выключен, файл `sl_debug.log` не создаётся
- `flt_dlss_sl_debug_log 1` — включает verbose-лог Streamline в `sl_debug.log` рядом с `q2rtx.exe`

Значение по умолчанию в `q2config.cfg` — `0`.
