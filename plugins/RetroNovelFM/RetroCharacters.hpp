// RetroCharacters.hpp - original visual-novel heroines, one per instrument family
#pragma once
#include "RetroData.hpp"

namespace retrofm {

enum HairStyle { hsLong, hsTwintails, hsHime, hsWavy, hsPonytail, hsBob, hsBraids, hsBun };
enum Accessory { acRibbon, acBell, acNone, acSnow, acScrunchie, acHeadphones, acGlasses, acKanzashi };
enum Outfit    { ofSailor, ofBlazer, ofSweater, ofHoodie, ofVest, ofKimono };
enum Celestial { ceSun, ceSunset, ceMoon, ceCloudy };
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
};

static const CharacterDef kChars[8] = {
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
};

// instrument -> heroine + her line
struct InstrumentFlavor { int ch; const char* line; };

static const InstrumentFlavor kFlavor[24] = {
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
};

} // namespace retrofm
