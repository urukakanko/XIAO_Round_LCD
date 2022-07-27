#define LGFX_USE_V1
#include <LovyanGFX.hpp>


class LGFX_RPICO_SPI_GC9A01 : public lgfx::LGFX_Device
{
    lgfx::Panel_GC9A01      _panel_instance;
    lgfx::Bus_SPI       _bus_instance;   // SPIバスのインスタンス
    //lgfx::Light_PWM     _light_instance;
  public:
    LGFX_RPICO_SPI_GC9A01(void)
    {
      { // バス制御の設定を行います。
        auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。

        // SPIバスの設定
        cfg.spi_host = 0 ;//SPI2_HOST;     // 使用するSPIを選択  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
        // ※ ESP-IDFバージョンアップに伴い、VSPI_HOST , HSPI_HOSTの記述は非推奨になるため、エラーが出る場合は代わりにSPI2_HOST , SPI3_HOSTを使用してください。
        cfg.spi_mode = 0;             // SPI通信モードを設定 (0 ~ 3)
        cfg.freq_write = 40000000;    // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
        cfg.freq_read  = 16000000;    // 受信時のSPIクロック
        //     cfg.spi_3wire  = true;        // 受信をMOSIピンで行う場合はtrueを設定
        //      cfg.use_lock   = true;        // トランザクションロックを使用する場合はtrueを設定
        //      cfg.dma_channel = SPI_DMA_CH_AUTO; // 使用するDMAチャンネルを設定 (0=DMA不使用 / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
        // ※ ESP-IDFバージョンアップに伴い、DMAチャンネルはSPI_DMA_CH_AUTO(自動設定)が推奨になりました。1ch,2chの指定は非推奨になります。
        cfg.pin_sclk  = D8;  //IO2            // SPIのSCLKピン番号を設定
        cfg.pin_mosi  = D10; //IO3            // SPIのMOSIピン番号を設定
        cfg.pin_miso  = -1;            // SPIのMISOピン番号を設定 (-1 = disable)
        cfg.pin_dc    = D0;  //IO26           // SPIのD/Cピン番号を設定  (-1 = disable)
        // SDカードと共通のSPIバスを使う場合、MISOは省略せず必ず設定してください。
        _bus_instance.config(cfg);    // 設定値をバスに反映します。
        _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
      }

      { // 表示パネル制御の設定を行います。
        auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。
        cfg.pin_cs           =    D7;//IO1  // CSが接続されているピン番号   (-1 = disable)
        cfg.pin_rst          =    D1;//IO27  // RSTが接続されているピン番号  (-1 = disable)
        cfg.pin_busy         =    -1;  // BUSYが接続されているピン番号 (-1 = disable)
        // ※ 以下の設定値はパネル毎に一般的な初期値が設定されていますので、不明な項目はコメントアウトして試してみてください。
        cfg.memory_width     =   240;  // ドライバICがサポートしている最大の幅
        cfg.memory_height    =   240;  // ドライバICがサポートしている最大の高さ
        cfg.panel_width      =   240;  // 実際に表示可能な幅
        cfg.panel_height     =   240;  // 実際に表示可能な高さ
        cfg.offset_x         =     0;  // パネルのX方向オフセット量
        cfg.offset_y         =     0;  // パネルのY方向オフセット量
        cfg.offset_rotation  =     0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
        cfg.dummy_read_pixel =     8;  // ピクセル読出し前のダミーリードのビット数
        cfg.dummy_read_bits  =     1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
        cfg.readable         =  true;  // データ読出しが可能な場合 trueに設定
        cfg.invert           =  true;  // パネルの明暗が反転してしまう場合 trueに設定
        cfg.rgb_order        = false;  // パネルの赤と青が入れ替わってしまう場合 trueに設定
        cfg.dlen_16bit       = false;  // データ長を16bit単位で送信するパネルの場合 trueに設定
        cfg.bus_shared       =  true;  // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)

        _panel_instance.config(cfg);
      }
      /*
        { // バックライト制御の設定を行います。（必要なければ削除）
          auto cfg = _light_instance.config();    // バックライト設定用の構造体を取得します。

          cfg.pin_bl = 7;              // バックライトが接続されているピン番号
          cfg.invert = false;           // バックライトの輝度を反転させる場合 true
          cfg.freq   = 44100;           // バックライトのPWM周波数
          cfg.pwm_channel = 3;          // 使用するPWMのチャンネル番号

          _light_instance.config(cfg);
          _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
        }
      */
      setPanel(&_panel_instance); // 使用するパネルをセットします。
    }

};

LGFX_RPICO_SPI_GC9A01 lcd;

static LGFX_Sprite canvas(&lcd);       // オフスクリーン描画用バッファ
static LGFX_Sprite base(&canvas);      // 文字盤パーツ
static LGFX_Sprite needle(&canvas);    // 針パーツ

static int32_t width = 239;             // 画像サイズ
static int32_t halfwidth = width >> 1;  // 中心座標
static auto transpalette = 0;           // 透過色パレット番号
static float zoom;                      // 表示倍率

#ifdef min
#undef min
#endif

#define bl D2 //IO28  バックライトピン設定
#define vib D9 //IO4   バイブレータピン設定

void setup(void)
{
  pinMode(bl, OUTPUT);
  digitalWrite(bl, HIGH);
  pinMode(vib, OUTPUT);
  digitalWrite(vib, LOW);
  
  
  Serial.begin(115200);

  lcd.init();
  int lw = std::min(lcd.width(), lcd.height());

  zoom = (float)lw / width; // 表示が画面にフィットするよう倍率を調整

  int px = lcd.width() >> 1;
  int py = lcd.height() >> 1;
  lcd.setPivot(px, py); // 描画時の中心を画面中心に合わせる

  lcd.setColorDepth(24);
  for (int i = 0; i < 180; i+=2) { // 外周を描画する
    lcd.setColor(lcd.color888(i*1.4,i*1.4+2,i*1.4+4));
    lcd.fillArc(px, py, (lw>>1)       , (lw>>1)-zoom*3,  90+i,  92+i);
    lcd.fillArc(px, py, (lw>>1)       , (lw>>1)-zoom*3,  88-i,  90-i);
  }
  for (int i = 0; i < 180; i+=2) { // 外周を描画する
    lcd.setColor(lcd.color888(i*1.4,i*1.4+2,i*1.4+4));
    lcd.fillArc(px, py, (lw>>1)-zoom*4, (lw>>1)-zoom*7, 270+i, 272+i);
    lcd.fillArc(px, py, (lw>>1)-zoom*4, (lw>>1)-zoom*7, 268-i, 270-i);
  }
  lcd.setColorDepth(16);

  canvas.setColorDepth(lgfx::palette_2bit);  // 各部品を２ビットパレットモードで準備する
  base  .setColorDepth(lgfx::palette_2bit);
  needle.setColorDepth(lgfx::palette_2bit);

  canvas.createSprite(width, width); // メモリ確保
  base  .createSprite(width, width);
  needle.createSprite(3, 11);

  base.setFont(&fonts::Orbitron_Light_24);    // フォント種類を変更(盤の文字用)
//base.setFont(&fonts::Roboto_Thin_24);       // フォント種類を変更(盤の文字用)

  base.setTextDatum(lgfx::middle_center);

  base.fillCircle(halfwidth, halfwidth, halfwidth - 8, 1);
  base.fillArc(halfwidth, halfwidth, halfwidth - 10, halfwidth - 11, 135,  45, 3);
  base.fillArc(halfwidth, halfwidth, halfwidth - 20, halfwidth - 23,   2,  43, 2);
  base.fillArc(halfwidth, halfwidth, halfwidth - 20, halfwidth - 23, 317, 358, 2);
  base.setTextColor(3);

  for (int i = -5; i <= 25; ++i) {
    bool flg = 0 == (i % 5);      // ５目盛り毎フラグ
    if (flg) {
      // 大きい目盛り描画
      base.fillArc(halfwidth, halfwidth, halfwidth - 10, halfwidth - 24, 179.8 + i*9, 180.2 + i*9, 3);
      base.fillArc(halfwidth, halfwidth, halfwidth - 10, halfwidth - 20, 179.4 + i*9, 180.6 + i*9, 3);
      base.fillArc(halfwidth, halfwidth, halfwidth - 10, halfwidth - 14, 179   + i*9, 181   + i*9, 3);
      float rad = i * 9 * 0.0174532925;
      float ty = - sin(rad) * (halfwidth * 10 / 15);
      float tx = - cos(rad) * (halfwidth * 10 / 17);
      base.drawFloat((float)i/10, 1, halfwidth + tx, halfwidth + ty); // 数値描画
    } else {
      // 小さい目盛り描画
      base.fillArc(halfwidth, halfwidth, halfwidth - 10, halfwidth - 17, 179.5 + i*9, 180.5 + i*9, 3);
    }
  }

  needle.setPivot(1, 9);  // 針パーツの回転中心座標を設定する
  needle.drawRect(0, 0, 3, 11, 2);

  canvas.setPaletteColor(1, 0, 0, 15);
  canvas.setPaletteColor(2, 255, 31, 31);
  canvas.setPaletteColor(3, 255, 255, 191);

  lcd.startWrite();
}


void draw(float value)
{
  base.pushSprite(0, 0);  // 描画用バッファに盤の画像を上書き

  float angle = 270 + value * 90.0;
  needle.pushRotateZoom( angle, 3.0, 10.0, transpalette); // 針をバッファに描画する
  canvas.fillCircle(halfwidth, halfwidth, 7, 3);
  canvas.pushRotateZoom(0, zoom, zoom, transpalette);    // 完了した盤をLCDに描画する
  if (value >= 1.5)
  lcd.fillCircle(lcd.width()>>1, (lcd.height()>>1) + width * 4/10, 5, 0x007FFFU);
}

void loop(void)
{
  float value = 0;
  do {
    draw(value);
  } while (1.9 > (value += 0.005 + value * 0.05));
  do {
    draw(value);
  } while (-0.5 < (value -= 0.1));
  do {
    draw(value);
  } while (0.0 > (value += 0.05));
}
