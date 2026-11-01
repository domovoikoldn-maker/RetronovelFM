// RetroCharacters.hpp - original visual-novel heroines, one per instrument family
#pragma once
#include "RetroData.hpp"

namespace retrofm {

enum HairStyle { hsLong, hsTwintails, hsHime, hsWavy, hsPonytail, hsBob, hsBraids, hsBun, hsSideTail, hsBoyShort, hsBoySpiky };
enum Accessory { acRibbon, acBell, acNone, acSnow, acScrunchie, acHeadphones, acGlasses, acKanzashi, acHeaddress, acHeadset };
enum Outfit    { ofSailor, ofBlazer, ofSweater, ofHoodie, ofVest, ofKimono, ofGothic, ofIdolDress, ofIdolJacket };
enum Celestial { ceSun, ceSunset, ceMoon, ceCloudy, ceRedMoon };
enum Particle  { ptSakura, ptSnow, ptMaple, ptLeaf, ptSparkle };

struct CharacterDef {
    const char* name;      // UTF-8, shown on the name plate
    const char* role;      // what she plays
    uint32_t hair, hairDark, hairLight;
    uint32_t eye, eyeDark;
    uint32_t outfit, collar, ribbon;
    HairStyle hs; Accessory acc; Outfit of;
    uint32_t skyTop, skyBottom, horizon;
    Celestial ce; Particle pt;
    uint32_t particle, accent;
    int male;   // 0 = girl, 1 = boy
};

static constexpr int kNumChars = 9;
static const CharacterDef kChars[kNumChars] = {
    // Hikari - keys
    {"Хикари", "клавишные",
     0x7a4a34, 0x3e2218, 0xc08868, 0xb0603a, 0x4a1c08,
     0xf6f4f0, 0x24335e, 0xe23a5a, hsLong, acRibbon, ofSailor,
     0x6cbcff, 0xdff2ff, 0xbfe4ff, ceSun, ptSakura, 0xffb7cf, 0xff6f9f},
    // Suzu - bells & mallets
    {"Сузу", "колокольчики",
     0xcfc6ee, 0x8a7fbc, 0xfaf8ff, 0x8a5cd6, 0x35186e,
     0xf4f7ff, 0x5a7fd8, 0xffb43c, hsTwintails, acBell, ofSailor,
     0xff9f7a, 0xffe6c4, 0xffcfa8, ceSunset, ptSakura, 0xffc6dc, 0xa979ff},
    // Shizuka - strings & harp
    {"Сидзука", "струнные",
     0x262433, 0x0c0b12, 0x5a5678, 0x3d5bd6, 0x111a55,
     0x2d3a34, 0xf2efe6, 0xc9a64b, hsHime, acNone, ofBlazer,
     0x10163a, 0x3b3f7e, 0x2a2e62, ceMoon, ptSakura, 0xf3d2e6, 0x8fa6ff},
    // Yuki - pads & choir
    {"Юки", "пэды и хор",
     0xf2f6fc, 0xaebfd8, 0xffffff, 0x58b6e8, 0x16557e,
     0xdbe7f6, 0xa8c4e6, 0x6f9fd8, hsWavy, acSnow, ofSweater,
     0x8ea6cf, 0xeef2fa, 0xd4deef, ceCloudy, ptSnow, 0xffffff, 0x6fb7ff},
    // Akane - brass & leads
    {"Аканэ", "лиды и медь",
     0xdc4430, 0x861a12, 0xff9a78, 0xe7a321, 0x7a4200,
     0xf7f5f0, 0x1f2a4a, 0xffc93a, hsPonytail, acScrunchie, ofSailor,
     0x4fb2ff, 0xd8f0ff, 0xaedcff, ceSun, ptSakura, 0xffb0c8, 0xff7a3a},
    // Rin - bass
    {"Рин", "бас",
     0x2e4277, 0x141d3e, 0x6684c6, 0x2fd0d8, 0x0a5664,
     0x3a3a4a, 0x26262f, 0xff4f8b, hsBob, acHeadphones, ofHoodie,
     0x3b2366, 0xff7d6b, 0xd65a7a, ceSunset, ptSparkle, 0xfff0a8, 0xff4f8b},
    // Midori - winds
    {"Мидори", "духовые",
     0x4f9a5a, 0x22552c, 0x93d69e, 0x2aa868, 0x0c4628,
     0xf3f0e2, 0x2f6b4a, 0x2f6b4a, hsBraids, acGlasses, ofVest,
     0x9ad8c0, 0xf2fff6, 0xc6efdc, ceSun, ptLeaf, 0x9fd88a, 0x3fae6a},
    // Kaede - koto, sitar, guitar
    {"Каэдэ", "восточные струны",
     0xc25a26, 0x6a2a0e, 0xf2a466, 0xd27a1e, 0x642c00,
     0x8c1d3c, 0xf2c14e, 0x2d1530, hsBun, acKanzashi, ofKimono,
     0xff8a52, 0xffe2a6, 0xffc480, ceSunset, ptMaple, 0xe8572a, 0xff9a3a},
    // Sumire - dark ambient (horror-mystery scenes)
    {"Сумирэ", "тёмный эмбиент",
     0x3a2448, 0x140a1c, 0x8a6aa8, 0xd8203a, 0x560010,
     0x1c1620, 0xf2eef4, 0xb0102a, hsLong, acHeaddress, ofGothic,
     0x0b0610, 0x3a1020, 0x24101c, ceRedMoon, ptSakura, 0x8a1028, 0xd83a5a},
};

// ------------------------------------------------------------------ vocal idols (original characters)
static const CharacterDef kIdols[6] = {
    {"Сора", "яркий поп",
     0xf2dc9a, 0xa07a30, 0xfff6d0, 0x3a9cff, 0x0c3a8a,
     0xf4f8ff, 0x3a7ce0, 0x46a8ff, hsSideTail, acHeadset, ofIdolDress,
     0x0a0820, 0x241040, 0x1a1030, ceMoon, ptSparkle, 0xbfe4ff, 0x46a8ff, 0},
    {"Мэй", "милый голос с придыханием",
     0xff9cc8, 0xb44a80, 0xffd8ea, 0xd23a8a, 0x5a0a3a,
     0xffeef6, 0xff6fae, 0xff5fa8, hsBob, acHeadset, ofIdolDress,
     0x0a0820, 0x241040, 0x1a1030, ceMoon, ptSparkle, 0xffc0dc, 0xff5fa8, 0},
    {"Рэйна", "холодный альт",
     0x5a3a8a, 0x221238, 0x9a7ad0, 0xe0a828, 0x6a4200,
     0x1e1828, 0x7a4ad0, 0xb07cff, hsLong, acHeadset, ofIdolDress,
     0x0a0820, 0x241040, 0x1a1030, ceMoon, ptSparkle, 0xd8c0ff, 0xb07cff, 0},
    {"Хару", "тёплая баллада",
     0xe08a4a, 0x8a4418, 0xffc890, 0x3aa860, 0x0c4a24,
     0xfff4d0, 0xffb347, 0xff8a3a, hsBraids, acHeadset, ofIdolDress,
     0x0a0820, 0x241040, 0x1a1030, ceMoon, ptSparkle, 0xffe0a0, 0xffb347, 0},
    {"Хаято", "рок-тенор",
     0xc8323a, 0x5a0c14, 0xff7a70, 0xe0a028, 0x6a3c00,
     0x1c1c24, 0xff4a4a, 0xff4a4a, hsBoySpiky, acHeadset, ofIdolJacket,
     0x0a0820, 0x241040, 0x1a1030, ceMoon, ptSparkle, 0xffb0a0, 0xff4a4a, 1},
    {"Ю", "мягкий голос",
     0xd0cce6, 0x7a76a0, 0xffffff, 0x4ab0d0, 0x0a4a62,
     0xf6f8ff, 0x4a8ae0, 0x6ad0ff, hsBoyShort, acHeadset, ofIdolJacket,
     0x0a0820, 0x241040, 0x1a1030, ceMoon, ptSparkle, 0xc0ecff, 0x6ad0ff, 1},
};

// instrument -> heroine + her line
struct InstrumentFlavor { int ch; const char* line; };

static const InstrumentFlavor kFlavor[kNumInstruments] = {
    {0, "Электропиано — мой любимый звук! Тёплый, как утро перед уроками."},
    {1, "Шкатулка играет только для тебя... Слышишь, как звенит?"},
    {2, "Струнные... Пусть эта мелодия станет нашим общим воспоминанием."},
    {4, "Медные! Громко и смело — вперёд, к финальной битве!"},
    {5, "Слэп-бас. Держи грув, а я подхвачу."},
    {5, "Синт-бас качает. Только не говори никому, что мне нравится."},
    {4, "Квадратная волна, как в старых играх на PC-98! Поехали!"},
    {6, "Флейта... Я читала, что ей больше сорока тысяч лет."},
    {6, "Кларнет звучит мягко. Почти как шелест страниц."},
    {0, "Орган! Как в той маленькой церкви у моря, помнишь?"},
    {2, "Арфа. Тише... Будто звёзды перебирают струны."},
    {1, "Маримба! Тук-тук-тук — правда весело?"},
    {1, "Вибрафон дрожит, совсем как моё сердце~"},
    {3, "Хор... Снег падает, и все голоса сливаются в один."},
    {1, "Челеста — звук волшебства. Загадай желание!"},
    {7, "Кото. Звук моего дома и осенних храмов."},
    {0, "Клавесин! Немного старомодно, но так элегантно."},
    {6, "Гобой. Грустный, но честный. Как хорошая книга."},
    {7, "Ситар... Далёкие страны, далёкие дороги."},
    {7, "Гитара у костра — лучший конец любого дня."},
    {3, "Тёплый пэд. Укутайся в него, как в плед зимним вечером."},
    {4, "Чистый синус — простой и прямой, как моё «не сдамся!»"},
    {1, "Колокола! Динь-дон — начинается новая глава."},
    {1, "Калимба. Маленькая, но звонкая — совсем как я!"},
    // ---- Windows XP / 2000s
    {0, "Настоящий рояль... Помнишь мелодию из клубной комнаты после уроков?"},
    {0, "Тихо-тихо, как воспоминание, которое боишься спугнуть."},
    {1, "Шкатулка с чердака. Её мелодия всегда звучит в конце главы~"},
    {2, "Струнный ансамбль. Самое время для признания под цветущей сакурой."},
    {2, "Медленные струнные... Сцена прощания на пустой станции."},
    {1, "Пиццикато! Тик-так, тик-так — смешная сцена в классе!"},
    {8, "Тремоло... Кажется, за дверью кто-то стоит."},
    {2, "Виолончель плачет вместо меня."},
    {2, "Арфа — звук сна, в котором мы снова встретились."},
    {6, "Настоящая флейта. Весенний ветер в школьном дворе."},
    {6, "Кларнет для тёплой повседневной сцены. Чай и печенье."},
    {6, "Гобой — тихая грусть летнего вечера."},
    {6, "Блокфлейта! Как на уроке музыки в младшей школе."},
    {4, "Окарина! Звучит как начало большого приключения."},
    {1, "Глокеншпиль звенит, как падающие снежинки."},
    {1, "Мягкий вибрафон для вечернего кафе."},
    {1, "Ручные колокольчики... Скоро Рождество?"},
    {1, "Калимба. Тихий летний полдень у реки."},
    {7, "Цитра. Почти как кото, только из далёкой страны."},
    {0, "Клавесин — для старого особняка с тайнами."},
    {3, "Хор... Будто ангелы поют над заснеженным городом."},
    {7, "Нейлоновая гитара. Пляж, закат и разговор по душам."},
    {3, "Стеклянный пэд. Звук воспоминаний, которые тают."},
    {3, "Пэд мечты. Сейчас начнётся флешбэк..."},
    {8, "Тёмное пианино. Не оборачивайся."},
    {8, "Гул из подвала. Мир вокруг уже не тот, что раньше."},
    {8, "Голоса... Ты тоже их слышишь?"},
    {8, "Сломанная шкатулка. Она всё ещё играет нашу песню."},
    {8, "Время идёт назад. Хорошо, что ты здесь."},
    {8, "Призрачные колокола звонят ровно в полночь."},
    // ---- user samples
    {5, "Твой собственный сэмпл! Загрузи WAV или OGG — и я его сыграю."},
    {5, "Слот номер два. Нарежь любимый звук и неси сюда."},
    {5, "Третий слот. Можно хоть звон школьного звонка."},
    {5, "Четвёртый слот. Корневую ноту настрой стрелками."},
};

} // namespace retrofm
