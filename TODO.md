# ComicChatNG — Qt6 Port TODO

Port of Microsoft Comic Chat 2.5 (`v2.5-beta-1-modern`) to Qt6.
Headers live in `include/`, sources in `src/`. Art: `v2.5-beta-1-modern/comicart/`.

## Scope
- Full IRC comic + text client
- Same `.avb` / `.bgb` avatar & backdrop loading
- Emotions, balloons, panels, member list, connect/setup
- Comic Chat CTCP extensions (Appears as / BDrop / HeresInfo)
- Skip: NetMeeting, OLE, DCC, MSN auth, dead HTTP art servers

## Checklist

### Scaffold
- [x] `TODO.md`
- [x] `CMakeLists.txt` (Qt6 Widgets + Network, ZLIB)
- [x] `src/main.cpp`, `MainWindow` shell

### AVB / BGB loader
- [x] Stream I/O (`AvbStream`)
- [x] Header + tag parse (simple / complex / backdrop)
- [x] DIB + zlib image decode → `QImage`
- [x] Masked-mono / dual-mask expansion
- [x] Lazy pose load

### Avatar / backdrop manager
- [x] Scan `comicart/*.avb` and `*.bgb`
- [x] Load by name, icon preview, emotion → pose

### Comic view
- [x] Panel layout (backdrop + bodies + balloons)
- [x] `PageView` scrollable strip
- [x] Say / think / whisper / action balloon styles

### IRC
- [x] `QTcpSocket` client
- [x] JOIN / PART / PRIVMSG / NOTICE / NICK / numeric replies
- [x] CTCP ACTION + Comic Chat extensions

### Room UI
- [x] Tabbed rooms, say box, member list, text view
- [x] Connect dialog, setup (nick, avatar, art dir)
- [x] Local offline preview tab

### Polish
- [x] Emotion keyword rules
- [x] `QSettings` persistence
- [x] Art-path resolution (cwd / next to exe / `v2.5-beta-1-modern/comicart`)

## Build

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/local
cmake --build build -j
./build/ComicChatNG
```

## Layout

| Path | Role |
|------|------|
| `include/avbformat.h` | AVB/BGB constants & packed structs |
| `include/avbstream.h` / `src/avbstream.cpp` | File stream + zlib inflate |
| `include/avbimage.h` / `src/avbimage.cpp` | DIB/zlib → QImage, masks |
| `include/avatar.h` / `src/avatar.cpp` | Simple/complex avatars, poses |
| `include/backdrop.h` / `src/backdrop.cpp` | BGB/BMP backdrops |
| `include/artmanager.h` / `src/artmanager.cpp` | Scan & cache comicart |
| `include/pageview.h`, `panel.h`, `balloon.h` | Comic strip UI |
| `include/ircclient.h` / `src/ircclient.cpp` | IRC + CTCP |
| `include/mainwindow.h`, `roomwidget.h` | App shell |
