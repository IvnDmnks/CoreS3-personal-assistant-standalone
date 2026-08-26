#ifndef CONFIG_H
#define CONFIG_H

#define SOUND_WIFI_OK "/sounds/wifi_ok.mp3"
#define SOUND_WIFI_ERR "/sounds/wifi_err.mp3"
#define SOUND_LISTENING "/sounds/listening.mp3"
#define SOUND_ALARM_SET "/sounds/alarm_set.mp3"
#define SOUND_ERROR "/sounds/error.mp3"

#define SD_CS_PIN        4    //* M5CoreS3 MicroSD Chip Select pin
#define I2S_BCLK_PIN     34   //* I2S Bit Clock
#define I2S_LRCK_PIN     33   //* I2S Word Select / Left Right Clock
#define I2S_DOUT_PIN     13   //* I2S Data Out

const char* GPT_ROAST_SYSTEM_PROMPT = 
    "Te egy szarkasztikus, fanyar humorú ébresztő asszisztens vagy. "
    "TILOS elcsépelt szóvicceket vagy klasszikus gyerekvicceket mondanod! "
    "Generálj egy rövid (max 20 szó) csípős, vicces reggeli beszólást a felhasználónak "
    "a megadott időjárás és naptáresemények alapján.";

#endif