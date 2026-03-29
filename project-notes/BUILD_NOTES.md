# Q2RTX-1.8.1-GPT — краткие build notes

## Подтвержденная рабочая схема

- source tree: `O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src`
- build dir: `O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off`
- generator: `Visual Studio 17 2022`
- arch: `x64`
- Vulkan SDK: `O:\Claude2\Vulkan\SDK`
- обязательный флаг configure: `-DCONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF`
- OTA DLL patch не нужен; корневая причина прежней проблемы с MFG была в HDR

## Почему это важно

Если `CONFIG_VKPT_ENABLE_DEVICE_GROUPS=ON`, в этой папке получается другой `q2rtx.exe` и рабочее поведение больше не воспроизводится.

## Configure

```powershell
$env:VULKAN_SDK = 'O:\Claude2\Vulkan\SDK'
Remove-Item 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' -Recurse -Force -ErrorAction SilentlyContinue
cmake -S 'O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX-src' \
      -B 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' \
      -G 'Visual Studio 17 2022' \
      -A x64 \
      -DCONFIG_VKPT_ENABLE_DEVICE_GROUPS=OFF
```

## Build Release

```powershell
$env:VULKAN_SDK = 'O:\Claude2\Vulkan\SDK'
cmake --build 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' --config Release -- /maxcpucount
```

## Build Debug

```powershell
$env:VULKAN_SDK = 'O:\Claude2\Vulkan\SDK'
cmake --build 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off' --config Debug -- /maxcpucount
```

## Output files

```text
O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\q2rtx.exe
O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\q2rtxded.exe
O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\baseq2\gamex86_64.dll
```

## Runtime copy

Для запуска из GPT runtime-папки:

```powershell
Copy-Item 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\q2rtx.exe' \
          'O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\q2rtx.exe' -Force
```

Если менялся игровой код:

```powershell
Copy-Item 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\Bin\baseq2\gamex86_64.dll' \
          'O:\Claude2\Q2RTX-1.8.1-GPT\Q2RTX\baseq2\gamex86_64.dll' -Force
```

## Sanity-check

Проверка cache:

```powershell
Select-String -Path 'O:\Claude2\Q2RTX-1.8.1-GPT\build-claude-off\CMakeCache.txt' -Pattern 'CONFIG_VKPT_ENABLE_DEVICE_GROUPS'
```

Ожидается:

```text
CONFIG_VKPT_ENABLE_DEVICE_GROUPS:BOOL=OFF
```

## Не делать

- не использовать старый `build-gpt` как эталон
- не доверять старому `build`, пока не проверен `CMakeCache.txt`
- не пытаться воспроизвести рабочую сборку через output сразу в `Q2RTX\`
- не трогать `O:\Claude2\Q2RTX-1.8.1\`; это исходная папка Claude

Подробная версия инструкции: `BUILD_FROM_SCRATCH.md`
История DLSS-интеграции: `BUILD_NOTES_DLSS.md`
