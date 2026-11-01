# Retro Novel FM — VST3 / VST2 / CLAP инструмент

4-операторный FM-синтезатор с 24 инструментами в духе PC-98, визуальных новелл 90-х
и RPG Maker. Работает в любой DAW: FL Studio, Ableton, Cubase, Bitwig, REAPER и т.д.

## Как получить готовый .vst3 для Windows (бесплатно, ~5 минут)

1. Зарегистрируйся на github.com (если ещё нет аккаунта).
2. Справа вверху **+ → New repository**, назови, например, `retro-novel-fm`, нажми **Create**.
3. На странице репозитория: **uploading an existing file** → перетащи ВСЁ содержимое этой
   папки (включая скрытую папку `.github`) → **Commit changes**.
   - Если папка `.github` не перетаскивается (скрыта): создай файл вручную через
     **Add file → Create new file**, имя `.github/workflows/build.yml`, вставь текст из
     этого файла.
4. Открой вкладку **Actions** — сборка запустится сама (если нет — выбери
   "Build plugin" → **Run workflow**). Ждать 3–5 минут.
5. Когда появится зелёная галочка, открой запуск и внизу, в разделе **Artifacts**,
   скачай `RetroNovelFM-Windows-x64` (или macOS / Linux).

## Установка (Windows)
- `RetroNovelFM.vst3` → скопируй в `C:\Program Files\Common Files\VST3\`
- `RetroNovelFM-vst.dll` (VST2, для старых DAW) → в свою папку VST-плагинов
- `RetroNovelFM.clap` → `C:\Program Files\Common Files\CLAP\`

Затем в DAW: пересканируй плагины, найди **Retro Novel FM** среди инструментов.

## macOS
Сборка не подписана. После копирования в `~/Library/Audio/Plug-Ins/VST3/` выполни в Терминале:
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/RetroNovelFM.vst3`

## Параметры
Instrument (24 пресета), Volume, Brightness (глубина FM), Release x, Vibrato Depth/Rate
(+ колесо модуляции), Lo-Fi, Chorus, Reverb, Pitch Bend Range, Transpose, Velocity Sens.
Педаль сустейна (CC64) поддерживается. Окно плагина — стандартные ползунки DAW.

## Сборка вручную (для разработчиков)
```
git clone --recursive https://github.com/DISTRHO/DPF dpf
make            # Linux / macOS
make WINDOWS=true CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++   # кросс-сборка под Windows
```
Готовые файлы появятся в папке `bin/`. Звуковой движок — `plugins/RetroNovelFM/RetroFMEngine.hpp`,
пресеты задаются в таблице `kPatches`.
