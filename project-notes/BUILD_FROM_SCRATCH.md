# Q2RTX-1.8.1-GPT — рабочая инструкция по сборке

Этот файл описывает именно ту схему сборки, которая была подтверждена как рабочая в этой папке.

## Статус на 23 марта 2026

Подтверждено:
- `q2rtx.exe` запускается из `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\`
- `DLSS SR` работает
- `DLSS-G / MFG` работает
- `HDR + MFG` одновременно не использовать
- OTA DLL patch не нужен; использовать оригинальный runtime от NVIDIA

## Что считать эталоном

Эталонная схема для этой копии проекта:
- исходники: `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src`
- сборочный каталог: `O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off`
- generator: `Visual Studio 17 2022`
- architecture: `x64`
- Vulkan SDK: `O:\Claude2\Vulkan\SDK`
- критичный CMake-флаг: `CONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF`

Критично:
- build-каталог должен быть чистым
- старый `CMakeCache.txt` легко ломает воспроизводимость
- нельзя слепо переиспользовать старые `build`, `build-gpt`, `build-gpt-clean`

## Почему это важно

Именно `CONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF` отделил рабочую сборку от нерабочей.

При `ON` в этой рабочей папке получался другой `q2rtx.exe`, который не воспроизводил рабочее поведение.

## Структура директорий

```text
O:\Claude2\Q2RTX-1.8.1-GPT\
  Q2RTX-src\         исходники
  build-claude-off\  подтвержденный рабочий build-каталог
  Q2RTX\             runtime-папка для запуска

O:\Claude2\Vulkan\SDK\
  Bin\glslang.exe
  Bin\glslangValidator.exe
```

Важно:
- в подтвержденной схеме CMake пишет артефакты в `build-claude-off\Bin\`
- в `Q2RTX\` мы копируем готовый `q2rtx.exe` для удобного запуска

## Требования

- Visual Studio 2022 Community с workload `Desktop development with C++`
- CMake 4.x
- Git
- Vulkan SDK в `O:\Claude2\Vulkan\SDK`

## Переменная окружения

Перед configure и build задать:

```powershell
$env:VULKAN_SDK = 'O:\Claude2\Vulkan\SDK'
```

Не надо передавать `-DVULKAN_SDK=...` как CMake cache variable. Для этого проекта рабочий путь - обычная environment variable.

## Чистая конфигурация с нуля

```powershell
$env:VULKAN_SDK = 'O:\Claude2\Vulkan\SDK'

Remove-Item 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' | Out-Null

cmake -S 'O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src' \
      -B 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' \
      -G 'Visual Studio 17 2022' \
      -A x64 \
      -DCONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF
```

Что проверить после configure:

```powershell
Select-String -Path 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\CMakeCache.txt' -Pattern 'CONFIG_VKPT_ENABLE_DEVICE_GROUPS'
```

Ожидаемая строка:

```text
CONFIG_VKPT_ENABLE_DEVICE_GROUPS:BOOL=OFF
```

## Release-сборка

Это основной рабочий вариант.

```powershell
$env:VULKAN_SDK = 'O:\Claude2\Vulkan\SDK'
cmake --build 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' --config Release -- /maxcpucount
```

Эта команда безопаснее, чем сборка только target `client`, потому что синхронизирует и `gamex86_64.dll`, и сопутствующие артефакты.

## Debug-сборка

Для диагностики:

```powershell
$env:VULKAN_SDK = 'O:\Claude2\Vulkan\SDK'
cmake --build 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' --config Debug -- /maxcpucount
```

Если нужен только клиент:

```powershell
$env:VULKAN_SDK = 'O:\Claude2\Vulkan\SDK'
cmake --build 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' --config Debug --target client -- /maxcpucount
```

## Где лежат результаты

После рабочей Release-сборки:

```text
O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\q2rtx.exe
O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\q2rtxded.exe
O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\baseq2\gamex86_64.dll
```

## Копирование в runtime-папку

Если хочешь запускать из `Q2RTX\`, копируй туда собранный exe:

```powershell
Copy-Item 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\q2rtx.exe' \
          'O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe' -Force
```

Если менялся игровой код, синхронизируй и DLL:

```powershell
Copy-Item 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\baseq2\gamex86_64.dll' \
          'O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll' -Force
```

## Проверка результата

Минимальные sanity-checks:

1. В `build-claude-off\CMakeCache.txt` стоит `CONFIG_VKPT_ENABLE_DEVICE_GROUPS:BOOL=OFF`
2. Размер рабочего `Release q2rtx.exe` в этой конфигурации: `7251968` байт
3. После копирования обновляется файл:
   `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe`
4. Игра стартует, `DLSS SR` и `MFG` работают

Важно:
- hash бинарника не обязан полностью совпасть с другой машиной или другой локальной средой
- для этого проекта важнее правильная конфигурация build и фактический рабочий запуск

## Главные грабли

1. Не переиспользовать старый cache без проверки `CONFIG_VKPT_ENABLE_DEVICE_GROUPS`
2. Не переключать output обратно на прямую сборку в `Q2RTX\` через CMake, если цель - воспроизвести рабочий билд Claude один в один
3. Не считать `build`, `build-gpt`, `build-gpt-clean` эталоном только по имени каталога
4. Не включать `HDR`, если тестируешь `MFG`
5. Не редактировать оригинальную папку `O:\Claude2\Q2RTX-1.8.1\`; работать только в `-GPT`

## Быстрый рабочий шаблон

```powershell
$env:VULKAN_SDK = 'O:\Claude2\Vulkan\SDK'
Remove-Item 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' -Recurse -Force -ErrorAction SilentlyContinue
cmake -S 'O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src' -B 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' -G 'Visual Studio 17 2022' -A x64 -DCONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF
cmake --build 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' --config Release -- /maxcpucount
Copy-Item 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\q2rtx.exe' 'O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe' -Force
```
